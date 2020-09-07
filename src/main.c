#include "custom_feature_def.h"
#include "ql_type.h"
#include "ql_trace.h"
#include "ql_stdlib.h"
#include "ql_uart.h"
#include "ql_adc.h"
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

void uart_callback(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void *customizedPara) {}
static void Callback_OnADCSampling(Enum_ADCPin adcPin, u32 adcValue, void *customParam)
{
    APP_DEBUG("<-- Callback_OnADCSampling: FSR voltage(mV)=%d  times=%d -->\r\n", adcValue, *((s32*)customParam));
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