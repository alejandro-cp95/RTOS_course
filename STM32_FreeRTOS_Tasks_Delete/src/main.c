
/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/

#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "stm32f30x.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "stm32f3xx_nucleo.h"

/*Task functions prototypes*/
void toggleGPIOBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
static void prvSetupHardware(void);
static void prvSetupUART(void);
static void prvSetupGPIO(void);
void printmsg(char* msg);
void rtos_delay(uint32_t delay_in_ms);
void vTask1_handler(void* params);
void vTask2_handler(void* params);

/*Global variable section*/
char usr_msg[50];

int main(void)
{

	DWT->CTRL |= (1<<0);/*Enable CYCCNT in DWT_CTRL*/

	/*1. Resets the RCC clock configuration to the default reset state.*/
	/*HSI ON, PLL OFF, HSE OFF, System clock = 8 MHz, CPU clock = 8 MHZ*/
	RCC_DeInit();
	/*2. Update SystemCoreClock variable according to Clock Register Values.*/
	SystemCoreClockUpdate();
	prvSetupHardware();

	sprintf(usr_msg,"Demo \r\n");
	printmsg(usr_msg);

	/*Start Recording*/
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	/*3. Let's create 2 tasks, task-1 and task-2*/
	xTaskCreate(vTask1_handler,"TASK_1",150,NULL,1,NULL);
	xTaskCreate(vTask2_handler,"TASK_2",150,NULL,2,NULL);
	/*4. Start the scheduler*/
	vTaskStartScheduler();

	for(;;);
}

void vTask1_handler(void* params)
{
	sprintf(usr_msg,"Task_1 is running\r\n");
	printmsg(usr_msg);
	while(1)
	{
		//rtos_delay(200);
		vTaskDelay(200);
		toggleGPIOBits(GPIOA,GPIO_Pin_5);
	}
}

void vTask2_handler(void* params)
{
	sprintf(usr_msg,"Task_2 is running\r\n");
	printmsg(usr_msg);
	while(1)
	{
		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13))
		{
			//If button is not pressed
			//Let's toggle the LED every 1 sec
			rtos_delay(1000);
			toggleGPIOBits(GPIOA,GPIO_Pin_5);
		}
		else
		{
			//If button is pressed
			//Task_2 deletes itself
			sprintf(usr_msg,"Task_2 is being deleted\r\n");
			printmsg(usr_msg);
			vTaskDelete(NULL);
		}
	}
}

static void prvSetupUART(void)
{
	GPIO_InitTypeDef gpio_uart_pins;
	USART_InitTypeDef uart2_init;
	/*1. Enable the UART2 and GPIOA peripheral clock*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	/*PA2 is UART2_TX, PA3 is UART2_RX*/
	/*2. Alternate function configuration of MCU pins to behave as UART2 TX and RX*/
	gpio_uart_pins.GPIO_Pin=GPIO_Pin_2|GPIO_Pin_3;
	gpio_uart_pins.GPIO_Mode=GPIO_Mode_AF;
	gpio_uart_pins.GPIO_PuPd=GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &gpio_uart_pins);
	/*3. AF mode settings for the pins*/
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_7);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_7);
	/*4. UART parameter initialization*/
	uart2_init.USART_BaudRate=115200;
	uart2_init.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
	uart2_init.USART_Mode=USART_Mode_Tx|USART_Mode_Rx;
	uart2_init.USART_Parity=USART_Parity_No;
	uart2_init.USART_StopBits=USART_StopBits_1;
	uart2_init.USART_WordLength=USART_WordLength_8b;
	USART_Init(USART2, &uart2_init);
	/*5. Enable the UART peripheral*/
	USART_Cmd(USART2, ENABLE);
}

static void prvSetupHardware(void)
{
	/*Setup UART2*/
	prvSetupUART();
	/*Setup LED and button*/
	prvSetupGPIO();
}

void printmsg(char* msg)
{
	for(uint32_t i=0;i<strlen(msg);i++)
	{
		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE)!=SET);
		USART_SendData(USART2, msg[i]);
	}
}

static void prvSetupGPIO(void)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA,ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC,ENABLE);

	GPIO_InitTypeDef gpio_led_pin, gpio_button_pin;
	gpio_led_pin.GPIO_Pin=GPIO_Pin_5;
	gpio_led_pin.GPIO_Mode=GPIO_Mode_OUT;
	gpio_led_pin.GPIO_OType=GPIO_OType_PP;
	gpio_led_pin.GPIO_PuPd=GPIO_PuPd_NOPULL;
	gpio_led_pin.GPIO_Speed=GPIO_Speed_Level_2;
	GPIO_Init(GPIOA,&gpio_led_pin);

	gpio_button_pin.GPIO_Pin=GPIO_Pin_13;
	gpio_button_pin.GPIO_Mode=GPIO_Mode_IN;
	gpio_button_pin.GPIO_OType=GPIO_OType_PP;
	gpio_button_pin.GPIO_PuPd=GPIO_PuPd_NOPULL;
	gpio_button_pin.GPIO_Speed=GPIO_Speed_Level_2;
	GPIO_Init(GPIOC,&gpio_button_pin);
}

void rtos_delay(uint32_t delay_in_ms)
{
	uint32_t current_tick_count = xTaskGetTickCount();
	uint32_t delay_in_ticks=(delay_in_ms*configTICK_RATE_HZ)/1000;
	while(xTaskGetTickCount()<(current_tick_count+delay_in_ticks));
}

void toggleGPIOBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	if(GPIO_ReadOutputDataBit(GPIOx, GPIO_Pin)==1)
	{
		GPIO_ResetBits(GPIOx, GPIO_Pin);
	}
	else
	{
		GPIO_SetBits(GPIOx, GPIO_Pin);
	}
}
