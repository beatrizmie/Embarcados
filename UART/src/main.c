/*
* Example RTOS Atmel Studio
*/

#include <asf.h>
#include <string.h>
#include "conf_board.h"

/* Pinos do LED1 */

//LED 1 OLED
#define LED1_OLED_PIO			PIOA
#define LED1_OLED_PIO_ID		ID_PIOA
#define LED1_OLED_PIO_IDX			0
#define LED1_OLED_IDX_MASK	(1 << LED1_OLED_PIO_IDX)

// LED 2 OLED
#define LED2_OLED_PIO      PIOC
#define LED2_OLED_PIO_ID   ID_PIOC
#define LED2_OLED_IDX      30
#define LED2_OLED_IDX_MASK (1 << LED2_OLED_IDX)

// LED 3 OLED
#define LED3_OLED_PIO      PIOB
#define LED3_OLED_PIO_ID   ID_PIOB
#define LED3_OLED_IDX      2
#define LED3_OLED_IDX_MASK (1 << LED3_OLED_IDX)

// BUT 1 OLED
#define BUT1_OLED_PIO           PIOD
#define BUT1_OLED_PIO_ID        ID_PIOD
#define BUT1_OLED_PIO_IDX       28
#define BUT1_OLED_PIO_IDX_MASK  (1u << BUT1_OLED_PIO_IDX)

// BUT 2 OLED
#define BUT2_OLED_PIO           PIOC
#define BUT2_OLED_PIO_ID        ID_PIOC
#define BUT2_OLED_PIO_IDX       31
#define BUT2_OLED_PIO_IDX_MASK  (1u << BUT2_OLED_PIO_IDX)

// BUT 3 OLED
#define BUT3_OLED_PIO		   PIOA
#define BUT3_OLED_PIO_ID		   ID_PIOA
#define BUT3_OLED_PIO_IDX	   19
#define BUT3_OLED_PIO_IDX_MASK  (1u << BUT3_OLED_PIO_IDX)

#define TASK_LED1_STACK_SIZE  (1024/sizeof(portSTACK_TYPE))
#define TASK_LED1_STACK_PRIORITY (tskIDLE_PRIORITY)

#define TASK_LED2_STACK_SIZE  (1024/sizeof(portSTACK_TYPE))
#define TASK_LED2_STACK_PRIORITY (tskIDLE_PRIORITY)

#define TASK_LED3_STACK_SIZE  (1024/sizeof(portSTACK_TYPE))
#define TASK_LED3_STACK_PRIORITY (tskIDLE_PRIORITY)

#define TASK_MONITOR_STACK_SIZE            (2048/sizeof(portSTACK_TYPE))
#define TASK_MONITOR_STACK_PRIORITY        (tskIDLE_PRIORITY)

#define TASK_UART_STACK_SIZE  (4096/sizeof(portSTACK_TYPE))
#define TASK_UART_STACK_PRIORITY (tskIDLE_PRIORITY)

#define TASK_EXECUTE_STACK_SIZE (4096 / sizeof(portSTACK_TYPE))
#define TASK_EXECUTE_STACK_PRIORITY (tskIDLE_PRIORITY)


/** Semaforo a ser usado pela task led 
    tem que ser var global! */
SemaphoreHandle_t xSemaphore1;
SemaphoreHandle_t xSemaphore2;
SemaphoreHandle_t xSemaphore3;


//cria uma queue
QueueHandle_t xQueueChar;
QueueHandle_t xQueueCommand;


extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,	signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);


void but1_callback() {
	// xSemaphoreGiveFromISR(xSemaphore1, 4);
	xQueueSendFromISR(xQueueCommand, "toggle led 1", 0);
}
void but2_callback() {
	xQueueSendFromISR(xQueueCommand, "toggle led 2", 0);
}
void but3_callback() {
	xQueueSendFromISR(xQueueCommand, "toggle led 3", 0);
}

void pin_toggle(Pio *pio, uint32_t mask){
	if(pio_get_output_data_status(pio, mask)) {
		pio_clear(pio, mask);	
	}
	else {
		pio_set(pio,mask);		
	}
}

/**
* \brief Called if stack overflow during execution
*/
extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
signed char *pcTaskName)
{
  printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
  /* If the parameters have been corrupted then inspect pxCurrentTCB to
  * identify which task has overflowed its stack.
  */
  for (;;) {
  }
}

/**
* \brief This function is called by FreeRTOS idle task
*/
extern void vApplicationIdleHook(void)
{
	pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
}

/**
* \brief This function is called by FreeRTOS each tick
*/
extern void vApplicationTickHook(void)
{
}

extern void vApplicationMallocFailedHook(void)
{
  /* Called if a call to pvPortMalloc() fails because there is insufficient
  free memory available in the FreeRTOS heap.  pvPortMalloc() is called
  internally by FreeRTOS API functions that create tasks, queues, software
  timers, and semaphores.  The size of the FreeRTOS heap is set by the
  configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

  /* Force an assert. */
  configASSERT( ( volatile void * ) NULL );
}



/**
* \brief This task, when activated, send every ten seconds on debug UART
* the whole report of free heap and total tasks status
*/
static void task_monitor(void *pvParameters)
{
  static portCHAR szList[256];
  UNUSED(pvParameters);

  for (;;) {
    printf("--- task ## %u", (unsigned int)uxTaskGetNumberOfTasks());
    vTaskList((signed portCHAR *)szList);
    printf(szList);
    vTaskDelay(1000);
  }
}

static void task_led1(void *pvParameters){
	pmc_enable_periph_clk(LED1_OLED_PIO_ID);
	pio_configure(LED1_OLED_PIO, PIO_OUTPUT_0, LED1_OLED_IDX_MASK, PIO_DEFAULT);

	const TickType_t xDelay = 3000 / portTICK_PERIOD_MS;
	const TickType_t xDelayLed = 100 / portTICK_PERIOD_MS;
  
    xSemaphore1 = xSemaphoreCreateBinary();

	/* devemos iniciar a interrupcao no pino somente apos termos alocado
	os recursos (no caso semaforo), nessa funcao inicializamos 
	o botao e seu callback*/
	/* init botão */
	pmc_enable_periph_clk(BUT1_OLED_PIO_ID);
	pio_configure(BUT1_OLED_PIO, PIO_INPUT, BUT1_OLED_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_handler_set(BUT1_OLED_PIO, BUT1_OLED_PIO_ID, BUT1_OLED_PIO_IDX_MASK, PIO_IT_FALL_EDGE, but1_callback);
	pio_enable_interrupt(BUT1_OLED_PIO, BUT1_OLED_PIO_IDX_MASK);
	NVIC_EnableIRQ(BUT1_OLED_PIO_ID);
	NVIC_SetPriority(BUT1_OLED_PIO_ID, 4); // Prioridade 4
	
	if (xSemaphore1 == NULL)
	  printf("falha em criar o semaforo \n");

  for (;;) {
	 if( xSemaphoreTake(xSemaphore1, ( TickType_t ) 500) == pdTRUE ){
		pin_toggle(LED1_OLED_PIO, LED1_OLED_IDX_MASK);
	}
  }
}

static void task_led2(void *pvParameters){
	pmc_enable_periph_clk(LED2_OLED_PIO_ID);
	pio_configure(LED2_OLED_PIO, PIO_OUTPUT_0, LED2_OLED_IDX_MASK, PIO_DEFAULT);

	const TickType_t xDelay = 3000 / portTICK_PERIOD_MS;
	const TickType_t xDelayLed = 100 / portTICK_PERIOD_MS;
  
    xSemaphore2 = xSemaphoreCreateBinary();

	/* devemos iniciar a interrupcao no pino somente apos termos alocado
	os recursos (no caso semaforo), nessa funcao inicializamos 
	o botao e seu callback*/
	/* init botão */
	pmc_enable_periph_clk(BUT2_OLED_PIO_ID);
	pio_configure(BUT2_OLED_PIO, PIO_INPUT, BUT2_OLED_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_handler_set(BUT2_OLED_PIO, BUT2_OLED_PIO_ID, BUT2_OLED_PIO_IDX_MASK, PIO_IT_FALL_EDGE, but2_callback);
	pio_enable_interrupt(BUT2_OLED_PIO, BUT2_OLED_PIO_IDX_MASK);
	NVIC_EnableIRQ(BUT2_OLED_PIO_ID);
	NVIC_SetPriority(BUT2_OLED_PIO_ID, 4); // Prioridade 4
	
	if (xSemaphore2 == NULL)
	  printf("falha em criar o semaforo \n");

  for (;;) {
	 if( xSemaphoreTake(xSemaphore2, ( TickType_t ) 500) == pdTRUE ){
		pin_toggle(LED2_OLED_PIO, LED2_OLED_IDX_MASK);
	}
  }
}

static void task_led3(void *pvParameters){
	pmc_enable_periph_clk(LED3_OLED_PIO_ID);
	pio_configure(LED3_OLED_PIO, PIO_OUTPUT_0, LED3_OLED_IDX_MASK, PIO_DEFAULT);

	const TickType_t xDelay = 3000 / portTICK_PERIOD_MS;
	const TickType_t xDelayLed = 100 / portTICK_PERIOD_MS;
  
    xSemaphore3 = xSemaphoreCreateBinary();

	/* devemos iniciar a interrupcao no pino somente apos termos alocado
	os recursos (no caso semaforo), nessa funcao inicializamos 
	o botao e seu callback*/
	/* init botão */
	pmc_enable_periph_clk(BUT3_OLED_PIO_ID);
	pio_configure(BUT3_OLED_PIO, PIO_INPUT, BUT3_OLED_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_handler_set(BUT3_OLED_PIO, BUT3_OLED_PIO_ID, BUT3_OLED_PIO_IDX_MASK, PIO_IT_FALL_EDGE, but3_callback);
	pio_enable_interrupt(BUT3_OLED_PIO, BUT3_OLED_PIO_IDX_MASK);
	NVIC_EnableIRQ(BUT3_OLED_PIO_ID);
	NVIC_SetPriority(BUT3_OLED_PIO_ID, 4); // Prioridade 4
	
	if (xSemaphore3 == NULL)
	  printf("falha em criar o semaforo \n");

  for (;;) {
	 if( xSemaphoreTake(xSemaphore3, ( TickType_t ) 500) == pdTRUE ){
		pin_toggle(LED3_OLED_PIO, LED3_OLED_IDX_MASK);
	}
  }
}
/**
* \brief Configure the console UART.
*/
static void configure_console(void)
{
  const usart_serial_options_t uart_serial_options = {
    .baudrate = CONF_UART_BAUDRATE,
    #if (defined CONF_UART_CHAR_LENGTH)
    .charlength = CONF_UART_CHAR_LENGTH,
    #endif
    .paritytype = CONF_UART_PARITY,
    #if (defined CONF_UART_STOP_BITS)
    .stopbits = CONF_UART_STOP_BITS,
    #endif
  };

  /* Configure console UART. */
  stdio_serial_init(CONF_UART, &uart_serial_options);

  /* Specify that stdout should not be buffered. */
  #if defined(__GNUC__)
  setbuf(stdout, NULL);
  #else
  /* Already the case in IAR's Normal DLIB default configuration: printf()
  * emits one character at a time.
  */
  #endif
}


static void USART1_init(void){
	/* Configura USART1 Pinos */
	sysclk_enable_peripheral_clock(ID_PIOB);
	sysclk_enable_peripheral_clock(ID_PIOA);
	pio_set_peripheral(PIOB, PIO_PERIPH_D, PIO_PB4); // RX
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA21); // TX
	MATRIX->CCFG_SYSIO |= CCFG_SYSIO_SYSIO4;

	/* Configura opcoes USART */
	const sam_usart_opt_t usart_settings = {
		.baudrate       = 115200,
		.char_length    = US_MR_CHRL_8_BIT,
		.parity_type    = US_MR_PAR_NO,
		.stop_bits    = US_MR_NBSTOP_1_BIT    ,
		.channel_mode   = US_MR_CHMODE_NORMAL
	};

	/* Ativa Clock periferico USART0 */
	sysclk_enable_peripheral_clock(ID_USART1);

	stdio_serial_init(CONF_UART, &usart_settings);

	/* Enable the receiver and transmitter. */
	usart_enable_tx(USART1);
	usart_enable_rx(USART1);

	/* map printf to usart */
	ptr_put = (int (*)(void volatile*,char))&usart_serial_putchar;
	ptr_get = (void (*)(void volatile*,char*))&usart_serial_getchar;

	/* ativando interrupcao */
	usart_enable_interrupt(USART1, US_IER_RXRDY);
	NVIC_SetPriority(ID_USART1, 4);
	NVIC_EnableIRQ(ID_USART1);
}

void USART1_Handler(void){
	uint32_t ret = usart_get_status(USART1);

	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	char c;

	// Verifica por qual motivo entrou na interrupçcao?
	// RXRDY ou TXRDY

	//  Dados disponível para leitura
	if(ret & US_IER_RXRDY){
		usart_serial_getchar(USART1, &c);
	    xQueueSendFromISR( xQueueChar, &c, 0);           /* send mesage to queue */

		// -  Transmissoa finalizada
		} else if(ret & US_IER_TXRDY){

	}
}

uint32_t usart1_puts(uint8_t *pstring){
	uint32_t i ;

	while(*(pstring + i))
	if(uart_is_tx_empty(USART1))
	usart_serial_putchar(USART1, *(pstring+i++));
}

void task_uartRX(void){
	
	xQueueChar = xQueueCreate( 20, sizeof(char) );
	
	int cont = 0;
	char c;
	char command[20];
	
	while (true) {
		if (xQueueReceive( xQueueChar, &(c), ( TickType_t )  500 / portTICK_PERIOD_MS)) {
			if(c == '\n') {
				command[cont] = NULL;
				cont = 0;
				printf('\n');
				xQueueSendFromISR(xQueueCommand, &command, 0);					/* send message to queue */
			} else {
				command[cont] = c;
				printf("%c", c);
				cont++;
			}
		}
	}
}

void task_execute(void) {
	
	xQueueCommand = xQueueCreate( 20, sizeof(char[20]) );
	
	char command[20];
	
	while(true) {
		if(xQueueReceive( xQueueCommand, &(command), ( TickType_t )  500 / portTICK_PERIOD_MS)) {		
							
			if(strcmp(command, "toggle led 1") == 0) {
				xSemaphoreGive(xSemaphore1);
			}
			else if(strcmp(command, "toggle led 3") == 0) {
				xSemaphoreGive(xSemaphore3);
			}
		}
	}
}

/**
*  \brief FreeRTOS Real Time Kernel example entry point.
*
*  \return Unused (ANSI-C compatibility).
*/
int main(void)
{
  /* Initialize the SAM system */
  sysclk_init();
  board_init();

  /* Initialize the console uart */
  USART1_init();

  /* Output demo information. */
  printf("-- Freertos Example --\n\r");
  printf("-- %s\n\r", BOARD_NAME);
  printf("-- Compiled: %s %s --\n\r", __DATE__, __TIME__);


  /* Create task to monitor processor activity */
  if (xTaskCreate(task_monitor, "Monitor", TASK_MONITOR_STACK_SIZE, NULL,
  TASK_MONITOR_STACK_PRIORITY, NULL) != pdPASS) {
    printf("Failed to create Monitor task\r\n");
  }


 /* Create task to make led blink */
 if (xTaskCreate(task_led1, "Led1", TASK_LED1_STACK_SIZE, NULL,
 TASK_LED1_STACK_PRIORITY, NULL) != pdPASS) {
   printf("Failed to create test led task\r\n");
 }
 
  /* Create task to make led blink */
  if (xTaskCreate(task_led2, "Led2", TASK_LED2_STACK_SIZE, NULL,
  TASK_LED2_STACK_PRIORITY, NULL) != pdPASS) {
	  printf("Failed to create test led task\r\n");
  }
  
/* Create task to make led blink */
if (xTaskCreate(task_led3, "Led3", TASK_LED3_STACK_SIZE, NULL,
TASK_LED3_STACK_PRIORITY, NULL) != pdPASS) {
	printf("Failed to create test led task\r\n");
}

/* Create task UART */
if (xTaskCreate(task_uartRX, "UART", TASK_UART_STACK_SIZE, NULL,
TASK_UART_STACK_PRIORITY, NULL) != pdPASS) {
	printf("Failed to create test uart task\r\n");
}

if (xTaskCreate(task_execute, "Execute", TASK_EXECUTE_STACK_SIZE, NULL, TASK_EXECUTE_STACK_PRIORITY, NULL) != pdPASS)
{
	printf("Failed to create execute task\r\n");
}
 
  /* Start the scheduler. */
  vTaskStartScheduler();

  /* Will only get here if there was insufficient memory to create the idle task. */
  return 0;
}
