#include "custom_feature_def.h"
#include "ql_type.h"
#include "ql_trace.h"
#include "ril_util.h"
#include "ril_sms.h"
#include "ril_telephony.h"
#include "ril_system.h"
#include "ql_stdlib.h"
#include "ql_uart.h"
#include "ql_adc.h"
#include "ql_system.h"
#include "ql_memory.h"
#include "ql_timer.h"
#include "ril.h"
#include <stdint.h>
#include <stddef.h>

char DBG_BUFFER[256];
static u32 ADC_CustomParam = 1;
int pressure;
#define DBG_PORT UART_PORT1
#define DBG(FORMAT, ...)                                                  \
    {                                                                     \
        Ql_memset(DBG_BUFFER, 0, sizeof(DBG_BUFFER));                     \
        Ql_sprintf(DBG_BUFFER, FORMAT, ##__VA_ARGS__);                    \
        Ql_UART_Write(DBG_PORT, (u8 *)DBG_BUFFER, Ql_strlen(DBG_BUFFER)); \
    }
#define CON_SMS_BUF_MAX_CNT (1)
#define CON_SMS_SEG_MAX_CHAR (160)
#define CON_SMS_SEG_MAX_BYTE (4 * CON_SMS_SEG_MAX_CHAR)
#define CON_SMS_MAX_SEG (7)
typedef struct
{
    u8 aData[CON_SMS_SEG_MAX_BYTE];
    u16 uLen;
} ConSMSSegStruct;

typedef struct
{
    u16 uMsgRef;
    u8 uMsgTot;

    ConSMSSegStruct asSeg[CON_SMS_MAX_SEG];
    bool abSegValid[CON_SMS_MAX_SEG];
} ConSMSStruct;
static bool ConSMSBuf_IsIntact(ConSMSStruct *pCSBuf, u8 uCSMaxCnt, u8 uIdx, ST_RIL_SMS_Con *pCon);
static bool ConSMSBuf_AddSeg(ConSMSStruct *pCSBuf, u8 uCSMaxCnt, u8 uIdx, ST_RIL_SMS_Con *pCon, u8 *pData, u16 uLen);
static s8 ConSMSBuf_GetIndex(ConSMSStruct *pCSBuf, u8 uCSMaxCnt, ST_RIL_SMS_Con *pCon);
static bool ConSMSBuf_ResetCtx(ConSMSStruct *pCSBuf, u8 uCSMaxCnt, u8 uIdx);
ConSMSStruct g_asConSMSBuf[CON_SMS_BUF_MAX_CNT];

void uart_callback(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void *customizedPara) {}
static void Callback_OnADCSampling(Enum_ADCPin adcPin, u32 adcValue, void *customParam)
{
    APP_DEBUG("<-- Callback_OnADCSampling: FSR voltage(mV)=%d  times=%d -->\r\n", adcValue, *((s32 *)customParam));
    if(adcValue < 500){
        SMS_TextMode_Send();
    }
}

static void ADC_Program(void)
{
    Enum_PinName adcPin = PIN_ADC0;

    // Register callback foR ADC
    APP_DEBUG("<-- Register callback for ADC -->\r\n");
    Ql_ADC_Register(adcPin, Callback_OnADCSampling, (void *)&ADC_CustomParam);

    // Initialize ADC (sampling count, sampling interval)
    APP_DEBUG("<-- Initialize ADC (sampling count=5, sampling interval=200ms) -->\r\n");
    Ql_ADC_Init(adcPin, 5, 200);

    // Start ADC sampling
    APP_DEBUG("<-- Start ADC sampling -->\r\n");
    Ql_ADC_Sampling(adcPin, TRUE);

    // Stop  sampling ADC
    //Ql_ADC_Sampling(adcPin, FALSE);
}
void proc_main_task(s32 taskId)
{
    s32 ret;
    ST_MSG msg;
    Ql_UART_Register(DBG_PORT, uart_callback, NULL);
    Ql_UART_Open(DBG_PORT, 115200, FC_NONE);
    ADC_Program();
    DBG("BEGIN\n");
    while (1)
    {
        Ql_OS_GetMessage(&msg);
        switch (msg.message)
        {
        case MSG_ID_RIL_READY:
            Ql_RIL_Initialize();
            DBG("Ril Ready\n");
            break;
        }
    }
}

void SMS_TextMode_Send(void)
{
    s32 iResult;
    u32 nMsgRef;
    char strPhNum[] = "+918668184922\0";
    char strTextMsg[] = "Your car has low pressure value in tyres\0";
    ST_RIL_SMS_SendExt sExt;

    //Initialize
    Ql_memset(&sExt, 0x00, sizeof(sExt));

    APP_DEBUG("< Send Normal Text SMS begin... >\r\n");

    iResult = RIL_SMS_SendSMS_Text(strPhNum, Ql_strlen(strPhNum), LIB_SMS_CHARSET_GSM, strTextMsg, Ql_strlen(strTextMsg), &nMsgRef);
    if (iResult != RIL_AT_SUCCESS)
    {
        APP_DEBUG("< Fail to send Text SMS, iResult=%d, cause:%d >\r\n", iResult, Ql_RIL_AT_GetErrCode());
        return;
    }
    APP_DEBUG("< Send Text SMS successfully, MsgRef:%u >\r\n", nMsgRef);

}
