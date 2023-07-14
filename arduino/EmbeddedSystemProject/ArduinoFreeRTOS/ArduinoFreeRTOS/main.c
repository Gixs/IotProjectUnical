#define F_CPU 16000000UL
#define BAUDRATE 9600
#define BAUD_PRESCALER (((F_CPU / (BAUDRATE * 16UL))) - 1)
#define LCD_DPRT  PORTD   //LCD DATA PORT
#define LCD_DDDR  DDRD    //LCD DATA DDR
#define LCD_DPIN  PIND    //LCD DATA PIN
#define LCD_CPRT  PORTB   //LCD COMMANDS PORT
#define LCD_CDDR  DDRB    //LCD COMMANDS DDR
#define LCD_CPIN  PINB    //LCD COMMANDS PIN
#define LCD_RS  0       //LCD RS
#define LCD_EN  1       //LCD EN

// LIBRARIES
#include "FreeRTOS.h"
#include "task.h"
#include <avr/io.h>

#include <util/delay.h>
#include <math.h>

#include <avr/interrupt.h>

#include "semphr.h"
#include "timers.h"


//Handlers of the task
TaskHandle_t serialInterruptTaskHandle = NULL; 
TaskHandle_t temperatureReadTaskHandle = NULL;
TaskHandle_t temperatureSendTaskHandle = NULL;
TaskHandle_t AirReadTaskHandle = NULL;
TaskHandle_t AirSendTaskHandle = NULL;

//QueuesHandler
QueueHandle_t xCharQueue;
QueueHandle_t xQueueTemperatureSendings;
QueueHandle_t xQueueAirSendings;

//Semaphore handler
SemaphoreHandle_t xSemaphoreADC;

//Timer Handler
TimerHandle_t xTimerReadings;

 

enum enumState
{
	ON,
	OFF,	
};

enum enumActions
{
	ALARM_ON,
	ALARM_OFF,
	BUZZER_ON,
	BUZZER_OFF,
	FAN_ON,
	FAN_OFF,	
};

uint8_t alarmState; = enumState.OFF; 
uint8_t stateBuzzer; = enumState.OFF;
uint8_t stateFan; = enumState.OFF;

float temperature;
float air;


void alarmON() {
				
		UART_sendString("{\"actuator\":\"alarm\",\"value\":\"on\"}\n");
		alarmState = 1;
		stateBuzzer = 1;
		stateFan = 1;
		PORTD |= 1<<PD2;
		buzzerTask();
		fanTask();
}


void alarmOFF() {
		
		UART_sendString("{\"actuator\":\"alarm\",\"value\":\"off\"}\n");
		alarmState = 0;
		PORTD &= (~(1<< PD2));
		
		stateBuzzer = 0;
		stateFan = 0;
		fanTask();
		buzzerTask();
	
}

void buzzerTask() {
	for (;;){
		
		//Se il buzzer non è impostato dall'esterno o il sistema non è in stato di allarme, il buzzer deve essere spento.
		if (stateBuzzer == 0){
			
			TCCR0A &= (~(1 << 1 | 1 << 7 | 1 << 8));
			TCCR0B &= (~(1 << 1 | 1 << 2));
		}
		else { //Esegui la PWM
		
		OCR0A = 127;
		TCCR0A = 0b11000001;
		TCCR0B = 0b00000011;
		
		}
		
	}
}



void fanTask() {
		//Se il buzzer non è impostato dall'esterno o il sistema non è in stato di allarme, il buzzer deve essere spento.
		if (stateFan == 0){
			PORTB &= (~(1 << PB3));
		}
		else {
			
			PORTB |= (1 << PB3); //ENABLE FAN
			
			PORTB |= (1 << PB2);
			PORTB &= (~(1<< PB4));
			}
		}
		



ISR (USART_RX_vect) {
	
	signed char cChar;
	
	cChar = UDR0;
	
		
	//Due righe che servono sempre per lo YIELD delle task di FREERTOS
	BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	
	//La coda serve per scambiare messaggi tra le task. 
	//Penso che per altre task si possa utilizzare anche il suspend ed il resume. 
	xQueueSendFromISR( xCharQueue, &cChar, &xHigherPriorityTaskWoken );
		
	//Serve sempre per lo yield. Sulla documentazione è scritto come un'altra funzione, ma per atmega328p è questa. 
	if( xHigherPriorityTaskWoken != pdFALSE ) {
		taskYIELD();
	}
	
		
}



void serialInterruptTask (void *parameters) {
		
	char *pcString; //where the queue receive the msg.
	uint8_t commandReceived;
	
	for( ;; ) {
		/* Block on the queue to wait for data to arrive. */
		xQueueReceive( xCharQueue, &pcString, portMAX_DELAY );
		
		commandReceived  = pcString - 0x30; //Gives the number from exadecimal to Ascii
		
		switch (commandReceived) {
			case enumActions.ALARM_ON:
				alarmON();
				break;//causa l'uscita immediata dallo switch
			case enumActions.ALARM_OFF:
				alarmOFF();
				break;
			case enumActions.BUZZER_ON:
				UART_sendString("{\"actuator\":\"buzzer\",\"value\":\"on\"}\n");
				stateBuzzer = 1;
				buzzerTask();
				break;
			case enumActions.BUZZER_OFF:
				UART_sendString("{\"actuator\":\"buzzer\",\"value\":\"off\"}\n");
				stateBuzzer = 0;
				buzzerTask();
				break;
			case enumActions.FAN_ON:
				UART_sendString("{\"actuator\":\"fan\",\"value\":\"on\"}\n");
				stateFan = 1;
				fanTask();
				break;
			case enumActions.FAN_OFF:
				UART_sendString("{\"actuator\":\"fan\",\"value\":\"off\"}\n");
				stateFan = 0;
				fanTask();
				break;
			default:
			break;
		}
			
	}
}

void UART_init()
{
	// Imposta la velocità di trasmissione
	UBRR0H = (BAUD_PRESCALER >> 8);
	UBRR0L = BAUD_PRESCALER;
	
	// Abilita la trasmissione e la ricezione
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
	
	// Imposta il formato dei dati: 8 bit di dati, 1 bit di stop
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_sendString(const char* data)
{
	// Invia carattere per carattere fino a quando non si raggiunge il terminatore null
	while (*data != '\0')
	{
		UART_sendChar(*data);
		data++;
	}
}

void UART_sendChar(char data)
{
	// Attendi che il buffer di trasmissione sia vuoto
	while (!(UCSR0A & (1 << UDRE0)));
	
	// Carica il dato nel buffer di trasmissione
	UDR0 = data;
}

void lcd_putValue(unsigned char val)
{
  LCD_DPRT &= 0x0F;
  LCD_DPRT |= (val&0xF0);     //send cmnd to data port
  LCD_CPRT |= (1<<LCD_EN);    //EN = 1 for H-to-L pulse
  _delay_us(1);       //wait to make enable wide
  LCD_CPRT &= ~ (1<<LCD_EN);  //EN = 0 for H-to-L pulse
  _delay_us(100);     //wait to make enable wide

  LCD_DPRT &= 0x0F;
  LCD_DPRT |= val<<4;     //send cmnd to data port
  LCD_CPRT |= (1<<LCD_EN);    //EN = 1 for H-to-L pulse
  _delay_us(1);       //wait to make enable wide
  LCD_CPRT &= ~ (1<<LCD_EN);  //EN = 0 for H-to-L pulse
  _delay_us(100);     //wait to make enable wide    
}

//*******************************************************
void lcdCommand( unsigned char cmnd )
{ 
  LCD_CPRT &= ~ (1<<LCD_RS);    //RS = 0 for command  
  lcd_putValue(cmnd);
}

//*******************************************************
void lcdData( unsigned char data )
{
  LCD_CPRT |= (1<<LCD_RS);    //RS = 1 for data   
  lcd_putValue(data);  
}

void lcd_clear()
{
  lcdCommand(0x01);
  _delay_us(1700);
}

void lcd_showCursor()
{
  lcdCommand(0x0E);
  _delay_us(50);
}

void lcd_hideCursor()
{
  lcdCommand(0x0C);
  _delay_us(50);
}

//*******************************************************

void lcd_init()
{
  LCD_DDDR |= 0xF0;
  LCD_CDDR |= (1<<LCD_RS)|(1<<LCD_EN);
 
  LCD_CPRT &=~(1<<LCD_EN);  //LCD_EN = 0
  _delay_us(2000);      //wait for init.

  lcdCommand(0x33); //send $33 for init.
  lcdCommand(0x32); //send $32 for init
  lcdCommand(0x28); //init. LCD 2 line,5*7 matrix
  lcdCommand(0x0C); //display on, cursor on
  lcdCommand(0x06); //shift cursor right

  lcd_clear();    
}

//*******************************************************
void lcd_gotoxy(unsigned char x, unsigned char y)
{  
  unsigned char firstCharAdr[]={0x80,0xC0,0x94,0xD4};//Table 12-4  
  lcdCommand(firstCharAdr[y-1] + x - 1);
  _delay_us(100); 
}

//*******************************************************
void lcd_print( char * str )
{
  unsigned char i = 0;

  while(str[i] != 0) //while it is not end of string
  {
    lcdData(str[i]);
    i++ ;
  }
}

//*******************************************************

void port_setup (void){
	
	DDRC = 0; //make Porct C an input for ADC input
	DDRD = 0xFF;
	DDRD |= (1<<PD3);
	DDRB |= (1 << 3) | (1 << 2) | (1 << 4);
	
}

void adc_setup_temp(void){
	DDRC&=~(1<<0);
	ADCSRA = 0x87; //make ADC enagle and select ck/128
	ADMUX = 0b11000000; //adc0
	
}

void adc_setup_air(void){
	DDRC&=~(1<<1);
	ADCSRA = 0x87; //make ADC enagle and select ck/128
	ADMUX = 0b01000001; //adc1
	
}



float read_adc (){

	ADCSRA |= (1<<ADSC); //Start conversion
	while ((ADCSRA&(1<<ADIF)) ==0); //wait for end of conversion
	ADCSRA |= (1<<ADIF); //clear the ADIF flag

	return (ADCL+(ADCH<<8));
}

char *generate_array_temp(unsigned char d1, unsigned char d2, unsigned char d3, unsigned char d4){
	char jsonDataTemperatureTemp [] = "{\"sensor\":\"temperature\",\"value\"";
	int len1 = sizeof(jsonDataTemperatureTemp);
	char *jsonDataTemperatureTempT2 = malloc(len1+7);
	
	for (int i = 0; i<len1; i++){
		jsonDataTemperatureTempT2[i] = jsonDataTemperatureTemp[i];
	}
	jsonDataTemperatureTempT2 [len1-1] = ':';
	jsonDataTemperatureTempT2 [len1] = d1+0x30;
	jsonDataTemperatureTempT2 [len1+1] = d2+0x30;
	jsonDataTemperatureTempT2 [len1+2] = '.';
	jsonDataTemperatureTempT2 [len1+3] = d3+0x30;
	jsonDataTemperatureTempT2 [len1+4] = d4+0x30;
	jsonDataTemperatureTempT2 [len1+5] = '}';
	jsonDataTemperatureTempT2 [len1+6] = '\n';
	jsonDataTemperatureTempT2 [len1+7] = '\0';
	
	return jsonDataTemperatureTempT2;
	
}

char *convert_and_write_temp(float value){ //deve creare questo  "{\"sensor\":\"temperature\",\"value\":25}\n"
	unsigned short x,y,z,w;
	unsigned char d1,d2,d3,d4;
	//sono 3 cifre

	//float convert = value *10/93-50;

	z = value*100;
	w = z/10;
	x=w/10;
	y=x/10;

	d1=y%10;
	d2=x%10;
	d3=w%10;
	d4=z%10;

	lcdData(d1+0x30);
	lcdData(d2+0x30);
	lcdData('.');
	lcdData(d3+0x30);
	lcdData(d4+0x30);
	lcdData(223);

	return generate_array_temp(d1, d2, d3, d4);

}

char *generate_array_air(unsigned char d1, unsigned char d2, unsigned char d3){
	char jsonDataAir [] = "{\"sensor\":\"air\",\"value\"";
		int len = sizeof(jsonDataAir);
		char *jsonDataAirT2 = malloc(len+8);
		
		for (int i = 0; i<len; i++){
			jsonDataAirT2[i] = jsonDataAir[i];
		}

		
		jsonDataAirT2 [len-1] = ':';

		if ((d1+0x30) == '0'){ //Non digerisco gli interi che iniziano per 0.
			jsonDataAirT2 [len] = d2+0x30;
			len = len -1;
			} else {
			jsonDataAirT2 [len] = d1+0x30;
			jsonDataAirT2 [len+1] = d2+0x30;
		}
		
		
		jsonDataAirT2 [len+2] = d3+0x30;
		
		jsonDataAirT2 [len+3] = '.';
		jsonDataAirT2 [len+4] = '0';
		jsonDataAirT2 [len+5] = '0';
		
	jsonDataAirT2 [len+6] = '}';
	
	jsonDataAirT2 [len+7] = '\n';
	jsonDataAirT2 [len+8] = '\0';

	return jsonDataAirT2;
	
}

char *convert_and_write_air(float value){
	unsigned char d1,d2,d3,d4,d5,d6;
	//sono 3 cifre
	unsigned int x,y,z,w;
	
	w = value;
	x=w/10;
	y=x/10;
	
	
	d1=y%10;
	d2=x%10;
	d3=w%10;
	

	
	lcdData(d1+0x30);
	lcdData(d2+0x30);
	lcdData(d3+0x30);

	return generate_array_air(d1, d2, d3);
}

void temperatureReadTask(void* parameter) {
	
	float threshold = 30.00;
	float temperature;
	
	for (;;){
		
		vTaskSuspend(NULL);
		
		if( xSemaphoreADC != NULL )
		{
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
		
			if( xSemaphoreTake( xSemaphoreADC, ( TickType_t ) 100 ) == pdTRUE )
			{
				/* We were able to obtain the semaphore and can now access the
				shared resource. */
				adc_setup_temp();
				temperature = read_adc()*10/93-50;
			
				/* We have finished accessing the shared resource.  Release the
				semaphore. */
				xSemaphoreGive( xSemaphoreADC );
			
				if (temperature >= threshold) {
					alarmON();
				}
				else {
					if (alarmState = 1){
					alarmState = 0;
					}
				}
			}
			else
			{
				/* We could not obtain the semaphore and can therefore not access
				the shared resource safely. */
			}
		}
				
		if( xQueueTemperatureSendings != 0 )
		{
        /* Send an unsigned long.  Wait for 10 ticks for space to become
        available if necessary. */
			if( xQueueSend( xQueueTemperatureSendings,
						   ( void * ) &temperature,
						   ( TickType_t ) 100 ) != pdPASS )
			{
				/* Failed to post the message, even after 100 ticks. */
			}
		}
		
		
		
		
		
		}//FOR
		
	}//TEMPERATURE SESNING TASK
	

void temperatureSendTask (void* parameter){
	
	float temperature;
	for (;;)
	{
		if( xQueueTemperatureSendings != NULL )
		{
		 /* Receive a message from the created queue to hold pointers.  Block for 10
		ticks if a message is not immediately available.  The value is read into a
		 pointer variable, and as the value received is the address of the xMessage
		variable, after this call pxRxedPointer will point to xMessage. */
			if( xQueueReceive( xQueueTemperatureSendings,
                         &( temperature ),
                         ( TickType_t ) 10 ) == pdPASS )
						 
			{
				lcd_gotoxy(6,1);
				char *jsonDataTemperature = convert_and_write_temp(temperature);
				vTaskDelay(pdMS_TO_TICKS(10));
				UART_sendString(jsonDataTemperature);
				vTaskDelay(pdMS_TO_TICKS(10));
				free(jsonDataTemperature);
				vTaskDelay(pdMS_TO_TICKS(10));
				
			}	
		
		}
	
	}
}

void airReadTask(void* parameter) {
	
	float threshold = 200.00;
	float air;
	
	for (;;)
	{
		vTaskSuspend(NULL);
		
		if( xSemaphoreADC != NULL )
		{
			/* See if we can obtain the semaphore.  If the semaphore is not
			available wait 10 ticks to see if it becomes free. */
			if( xSemaphoreTake( xSemaphoreADC, ( TickType_t ) 100 ) == pdTRUE )
			{
				/* We were able to obtain the semaphore and can now access the
				shared resource. */

				adc_setup_air();
				air = read_adc();
			

				/* We have finished accessing the shared resource.  Release the
				semaphore. */
				xSemaphoreGive( xSemaphoreADC );
			}
			else
			{
				/* We could not obtain the semaphore and can therefore not access
				the shared resource safely. */
			}
		}
		
		
		
		
		if( xQueueAirSendings != 0 )
		{
			/* Send an unsigned long.  Wait for 10 ticks for space to become
			available if necessary. */
			if( xQueueSend( xQueueAirSendings,
						   ( void * ) &air,
						   ( TickType_t ) 100 ) != pdPASS )
			{
				/* Failed to post the message, even after 10 ticks. */
			}
		}
		
		
		
	}//FOR
		
}//READING AIR


void airSendTask (void* parameter){
	
	float temperature;
	for (;;)
	{	
		if( xQueueTemperatureSendings != NULL )
		{
		 /* Receive a message from the created queue to hold pointers.  Block for 10
		ticks if a message is not immediately available.  The value is read into a
		 pointer variable, and as the value received is the address of the xMessage
		variable, after this call pxRxedPointer will point to xMessage. */
			if( xQueueReceive( xQueueTemperatureSendings,
                         &( temperature ),
                         ( TickType_t ) 10 ) == pdPASS )
						 
			{
				lcd_gotoxy(6,2);
				char *jsonDataAir = convert_and_write_air(air);
				vTaskDelay(pdMS_TO_TICKS(10));
				UART_sendString(jsonDataAir);
				vTaskDelay(pdMS_TO_TICKS(10));
				free(jsonDataAir);
				vTaskDelay(pdMS_TO_TICKS(10));
				
				
			}	
		
		}
	
	}//for
}//airSendTask

 
 
 


//************************************


void vTimerCallback(xTimerReadings )
 {
	vTaskResume (temperatureReadTaskHandle);
	vTaskResume (AirReadTaskHandle);  
 }
 
// MAIN PROGRAM
int main(void)
{
	alarmState = enumState.OFF;
	stateBuzzer = enumState.OFF;
	stateFan = enumState.OFF;
	
	lcd_init();
	
	lcd_print("Temp:");
	lcd_gotoxy(1,2);
	lcd_print("Air:");

	UART_init();
	
	port_setup();
	
	xTimerReadings = xTimerCreate ("Timer", pdMS_TO_TICKS( 1000 ), pdTRUE, ( void * ) 0, vTimerCallback);

         if( xTimerReadings == NULL )
         {
             /* The timer was not created. */
         }
         else
         {
             /* Start the timer.  No block time is specified, and
             even if one was it would be ignored because the RTOS
             scheduler has not yet been started. */
             if( xTimerStart( xTimerReadings, 0) != pdPASS )
             {
                 /* The timer could not be set into the Active
                 state. */
             }
         }
	
    // Attempt to create a semaphore.
    xSemaphoreADC = xSemaphoreCreateBinary();

	xSemaphoreGive(xSemaphoreADC);

	
	xCharQueue = xQueueCreate( 2, sizeof( char * ) );
	
	xQueueAirSendings = xQueueCreate( 2, sizeof (float) );
	xQueueTemperatureSendings = xQueueCreate( 2, sizeof (float) );
			
	xTaskCreate(serialInterruptTask, "serialInterruptTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &serialInterruptTaskHandle);
	
	xTaskCreate(temperatureReadTask, "temperatureReadTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &temperatureReadTaskHandle);
	
	xTaskCreate(temperatureSendTask, "temperatureSendTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &temperatureSendTaskHandle);
	
	xTaskCreate(airReadTask, "airReadTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &temperatureReadTaskHandle);
	
	xTaskCreate(airSendTask, "airSendTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &AirSendTaskHandle);
	
	sei();
	
	// START SCHELUDER
	vTaskStartScheduler();
		
	while (1)
	{
		
	}

}

// IDLE TASK
void vApplicationIdleHook(void){
	// THIS RUNS WHILE NO OTHER TASK RUNS
}
