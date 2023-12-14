#include <stdio.h>
#include <string.h>
#include "delay.h"
#include "timer.h"
#include "radio.h"
#include "tremo_uart.h"
#include "tremo_gpio.h"
#include "tremo_rcc.h"
#include "tremo_pwr.h"
#include "tremo_delay.h"
#include "rtc-board.h"
#include "tremo_lpuart.h"
#include "tremo_regs.h"

// uint8_t rx_data[11] = { 0 };
// uint8_t rx_index    = 0;

uint16_t counter = 0;

static enum eDeviceState
{
    DEVICE_STATE_INIT,
    DEVICE_STATE_JOIN,
    DEVICE_STATE_SEND,
    DEVICE_STATE_CYCLE,
    DEVICE_STATE_SLEEP
}DeviceState;

extern int app_start(void);

// void lpuart_txrx()
// {
//     lpuart_init_t lpuart_init_cofig;

//     gpio_set_iomux(GPIOC, GPIO_PIN_15, 2); // TX:GP47
//     gpio_set_iomux(GPIOD, GPIO_PIN_12, 2); // RX:GP60

//     lpuart_init_cofig.baudrate         = 9600;
//     lpuart_init_cofig.data_width       = LPUART_DATA_8BIT;
//     lpuart_init_cofig.parity           = LPUART_PARITY_NONE;
//     lpuart_init_cofig.stop_bits        = LPUART_STOP_1BIT;
//     lpuart_init_cofig.low_level_wakeup = true;
//     lpuart_init_cofig.start_wakeup     = false;
//     lpuart_init_cofig.rx_done_wakeup   = false;
//     lpuart_init(LPUART, &lpuart_init_cofig);

//     lpuart_config_interrupt(LPUART, LPUART_CR1_RX_NOT_EMPTY_INT, ENABLE);

//     lpuart_config_tx(LPUART, true);
//     lpuart_config_rx(LPUART, true);

//     /* NVIC config */
//     NVIC_SetPriority(LPUART_IRQn, 2);
//     NVIC_EnableIRQ(LPUART_IRQn);

//     // for (int i = 0; i < 11; i++) {
//     //     lpuart_send_data(LPUART, tx_data[i]);
//     //     while (!lpuart_get_tx_done_status(LPUART)) ;
//     //     lpuart_clear_tx_done_status(LPUART);
//     // }
// }

// void lpuart_IRQHandler()
// {
//     if (lpuart_get_rx_not_empty_status(LPUART)) {
//         rx_data[rx_index++] = lpuart_receive_data(LPUART);
//     }
// }

void uart_log_init(void)
{
    // uart0
    gpio_set_iomux(GPIOB, GPIO_PIN_0, 1);
    gpio_set_iomux(GPIOB, GPIO_PIN_1, 1);

    /* uart config struct init */
    uart_config_t uart_config;
    uart_config_init(&uart_config);

    uart_config.baudrate = UART_BAUDRATE_115200;
    uart_init(CONFIG_DEBUG_UART, &uart_config);
    uart_cmd(CONFIG_DEBUG_UART, ENABLE);
}

static TimerEvent_t TxNextPacketTimer;
static uint32_t TxDutyCycleTime = 3000;

static void OnTxNextPacketTimerEvent( void *context )
{
    printf("Timer Fired\n");
    
    TimerStop( &TxNextPacketTimer );
   
    DeviceState = DEVICE_STATE_SEND;        
}

void board_init()
{
    rcc_enable_oscillator(RCC_OSC_XO32K, true);

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_UART0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_PWR, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_RTC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SAC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LORA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LPUART, true);

    uart_log_init();

    RtcInit();
}

int main(void)
{
    // Target board initialization
    board_init();

    // lpuart_txrx();

    delay_ms(5000);
    // app_start();

    /* Infinite loop */
    DeviceState = DEVICE_STATE_INIT;

    printf("ClassC app start\n");
    while( 1 )
    {
        switch( DeviceState )
        {
            case DEVICE_STATE_INIT:
            {
                TimerInit( &TxNextPacketTimer, OnTxNextPacketTimerEvent );
                
                DeviceState = DEVICE_STATE_SEND;
                break;
            }
        
            case DEVICE_STATE_SEND:
            {
                // do something...
                counter += 1;
                printf("counter: %d\n", counter);
                
                // Schedule next packet transmission
                DeviceState = DEVICE_STATE_CYCLE;
                break;
            }
            case DEVICE_STATE_CYCLE:
            {
                DeviceState = DEVICE_STATE_SLEEP;

                // Schedule next packet transmission
                TimerSetValue( &TxNextPacketTimer, TxDutyCycleTime );
                TimerStart( &TxNextPacketTimer );
                printf("starting timer: %ld ms\n", TxDutyCycleTime);
                break;
            }
            case DEVICE_STATE_SLEEP:
            {
                // Process Radio IRQ
                Radio.IrqProcess( );
                break;
            }
            default:
            {
                DeviceState = DEVICE_STATE_INIT;
                break;
            }
        }
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(void* file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1) { }
}
#endif
