
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
#include "stdlib.h"
#include "stm32f30x.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "stm32f3xx_nucleo.h"
#include "queue.h"
#include "semphr.h"

/*Macros*/
#define TRUE 1
#define FALSE 0

/*Task functions prototypes*/
void toggleGPIOBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
static void prvSetupHardware(void);
static void prvSetupUART(void);
static void prvSetupGPIO(void);
void printmsg(char* msg);
void rtos_delay(uint32_t delay_in_ms);
void vEmployeeWork(unsigned int workID);

/*Tasks prototypes*/
void vTask1_manager(void* params);
void vTask2_employee(void* params);

/*Global variable section*/
char usr_msg[37];
/*Task handles*/
TaskHandle_t xManagerTaskHandle = NULL;
TaskHandle_t xEmployeeTaskHandle = NULL;
/*Queue handle*/
QueueHandle_t xWorkQueue=NULL;
/*Semaphore handle*/
SemaphoreHandle_t xWorkSemaphore=NULL;

int main(void)
{
	DWT->CTRL |= (1<<0);/*Enable CYCCNT in DWT_CTRL*/

	/*1. Resets the RCC clock configuration to the default reset state.*/
	/*HSI ON, PLL OFF, HSE OFF, System clock = 8 MHz, CPU clock = 8 MHZ*/
	RCC_DeInit();
	/*2. Update SystemCoreClock variable according to Clock Register Values.*/
	SystemCoreClockUpdate();
	prvSetupHardware();

	sprintf(usr_msg,"\r\nSemaphore Demo\r\n");
	printmsg(usr_msg);

	/*Start Recording*/
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	/*Let's create a queue*/
	xWorkQueue=xQueueCreate(1,sizeof(unsigned int));

	/*Let's create a semaphore*/
	vSemaphoreCreateBinary(xWorkSemaphore);

	if((xWorkQueue!=NULL)&&(xWorkSemaphore!=NULL))
	{
		/*3. Let's create 2 tasks, task-1 and task-2*/
		xTaskCreate(vTask1_manager,"TASK1_MANAGER",200,NULL,3,&xManagerTaskHandle);
		xTaskCreate(vTask2_employee,"TASK2_EMPLOYEE",200,NULL,1,&xEmployeeTaskHandle);
		/*4. Start the scheduler*/
		vTaskStartScheduler();
	}
	else
	{
		sprintf(usr_msg,"Queue or semaphore creation failed\r\n");
		printmsg(usr_msg);
	}

	for(;;);
}

void vTask1_manager(void* params)
{
	unsigned int workTicketID;
	BaseType_t xStatus;
	/*The semaphore is created in an empty state, so it must be given to initialiaze
	 * so that it can be used*/
	//xSemaphoreGive(xWorkSemaphore);
	while(1)
	{
		workTicketID=rand()&0xFF;
		xStatus=xQueueSend(xWorkQueue,&workTicketID,portMAX_DELAY);
		if(xStatus!=pdPASS)
		{
			sprintf(usr_msg,"Could not send to the queue\r\n");
			printmsg(usr_msg);
		}
		else
		{
			xSemaphoreGive(xWorkSemaphore);
			traceISR_EXIT_TO_SCHEDULER();
			taskYIELD();
		}
	}
}

void vTask2_employee(void* params)
{
	unsigned int workTicketID;
	BaseType_t xStatus;
	while(1)
	{
		xSemaphoreTake(xWorkSemaphore,portMAX_DELAY);
		xStatus=xQueueReceive(xWorkQueue,&workTicketID,0);
		if(xStatus!=pdPASS)
		{
			sprintf(usr_msg,"There is nothing to do\r\n");
			printmsg(usr_msg);
		}
		else
		{
			vEmployeeWork(workTicketID);
		}
	}
}

void vEmployeeWork(unsigned int workID)
{
	sprintf(usr_msg,"Working on the task ID: %d\r\n",workID);
	printmsg(usr_msg);
	vTaskDelay(workID);
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
	uart2_init.USART_BaudRate=19200;
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

void vApplicationMallocFailedHook(void)
{
	sprintf(usr_msg,"Memory allocation failed\r\n");
	printmsg(usr_msg);
}
