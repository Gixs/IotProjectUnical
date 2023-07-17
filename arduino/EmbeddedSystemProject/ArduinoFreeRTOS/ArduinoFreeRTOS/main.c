    /*-----------------------------------------------------------------------------------------*/
   //																					      //
  //             Luigi Rachiele - Low Level and Embedded system programming project		     //
 //				       																	    //
/*-----------------------------------------------------------------------------------------*/

/*------------------------------------- DEFINES ------------------------------------------*/

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
#define ASCII_OFFSET 0x30 //OFFSET TO CONVERT CHAR FROM SERIAL TO ASCII
#define ASCII_CONVERT(x) (x+ASCII_OFFSET) //FUNCTION TO CONVERT FROM SERIAL TO ASCII
#define SIZE_ARRAY 50
#define THRESHOLD_TEMPERATURE 57.00
#define THRESHOLD_AIR 200.00

/* ----------------------------------- INCLUDES -------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"
#include <avr/io.h>
#include <util/delay.h>

#include <avr/interrupt.h>
#include "semphr.h"
#include "timers.h"
#include <stdlib.h>


/* ----------------------------------- ENUMS -------------------------------------*/

enum enumState
{
	OFF = 0,
	ON = 1
};

enum enumActions
{
	ALARM_ON,
	ALARM_OFF,
	BUZZER_ON,
	BUZZER_OFF,
	FAN_ON,
	FAN_OFF
};

enum enumActionsChar
{
	c_ALARM_ON = '0',
	c_ALARM_OFF = '1',
	c_BUZZER_ON = '2',
	c_BUZZER_OFF = '3',
	c_FAN_ON = '4',
	c_FAN_OFF = '5'
};

/* ----------------------------------- GLOBAL DECLARATIONS -------------------------------------*/
float temperature;
float air;

//Handlers of the task
TaskHandle_t serialFromISRTaskHandle; 
TaskHandle_t temperatureReadTaskHandle;
TaskHandle_t temperatureSendTaskHandle;
TaskHandle_t AirReadTaskHandle;
TaskHandle_t AirSendTaskHandle;

//QueuesHandler
QueueHandle_t xCharQueue;
QueueHandle_t xQueueTemperatureSendings;
QueueHandle_t xQueueAirSendings;

//Semaphore handler
SemaphoreHandle_t xSemaphoreADC;
SemaphoreHandle_t xSemaphoreUART;
SemaphoreHandle_t xSemaphoreLCD;

//Timer Handler
TimerHandle_t xTimerReadings;

//State Variable
uint8_t alarmState;
uint8_t stateBuzzer;
uint8_t stateFan;

/* ----------------------------------- FUNCTION DECLARATIONS -------------------------------------*/
static void alarmON(); 
static void alarmOFF();
static void buzzerToggle(); 
static void fanToggle(); 
static void UART_init();
void UART_sendString(const char* data);
static void UART_sendChar(char data);
static void LCD_putValue(unsigned char val);
static void LCD_sendCommand( unsigned char cmnd);
static void LCD_sendData( unsigned char data);
static void LCD_clear();
/* ----------------------------------- NOT USED -------------------------------------*/
/*
static void LCD_showCursor();
static void LCD_hideCursor();
*/
static void LCD_init();
static void LCD_goToXY(unsigned char x, unsigned char y);
static void LCD_print(char* str);
void LCD_sendTemp (unsigned char d1, unsigned char d2, unsigned char d3, unsigned char d4);
static void portInit (void);
static void ADC_setupToReadTemp(void);
static void ADC_setupToReadAir(void);
static float ADC_read();
char* generateArrayTemperature(unsigned char d1, unsigned char d2, unsigned char d3, unsigned char d4); //to use it in LCD writing
unsigned char* convertTemperature(float value); //convert Analog value in digital
static char *generateArrayAir(unsigned char d1, unsigned char d2, unsigned char d3); //to use it in LCD writing
unsigned char* convertAir(float value); //convert Analog value in digital

/* ----------------------------------- FREE RTOS TASK DECLARATION -------------------------------------*/

void serialFromISRTask (void *parameters); 
void temperatureReadTask(void* parameter);
void temperatureSendTask (void* parameter);
void airReadTask(void* parameter);
void airSendTask (void* parameter);

/* ----------------------------------- FREE RTOS CALLBACK DECLARATION -------------------------------------*/

void vTimerCallback(TimerHandle_t xTimerReadings); //called when timer elapse

/* ----------------------------------- ISR -------------------------------------*/

//INTERRUPT SERVICE ROUTINE
ISR (USART_RX_vect) {
	
	signed char cChar;
	
	cChar = UDR0;
	//We need it for the task YIELD
	BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	
	//The queue is useful exchange message with the other task
	xQueueSendFromISR( xCharQueue, &cChar, &xHigherPriorityTaskWoken );
	
	//TaskYIELD in Atmega328p.
	if( xHigherPriorityTaskWoken != pdFALSE ) {
		taskYIELD();
	}
	
}

/* ----------------------------------- FUNCTION DEFINITIONS -------------------------------------*/

static void alarmON() {
				
		alarmState = ON;
		stateBuzzer = ON;
		stateFan = ON;
		PORTD |= 1<<PD2; //toggle on a led on the board
		buzzerToggle();
		fanToggle();
		if( xSemaphoreTake( xSemaphoreUART, ( TickType_t ) 100 ) == pdTRUE )
		{
			UART_sendChar(c_ALARM_ON);
			UART_sendChar('\n');
			xSemaphoreGive( xSemaphoreUART );
 		}	
			
}

static void alarmOFF() {
		
		alarmState = OFF;
		PORTD &= (~(1<< PD2)); //toggle off the led on the board
		
		stateBuzzer = OFF;
		stateFan = OFF;
		fanToggle();
		buzzerToggle();
		if( xSemaphoreTake( xSemaphoreUART, ( TickType_t ) 100 ) == pdTRUE )
		{
			UART_sendChar(c_ALARM_OFF);
			UART_sendChar('\n');
			xSemaphoreGive( xSemaphoreUART );
		}
 	
}

static void buzzerToggle() {		
		//Se il buzzer non è impostato dall'esterno o il sistema non è in stato di allarme, il buzzer deve essere spento.
		if (stateBuzzer == OFF){
			//stop the timer
			TCCR2A &= (~(1 << 1 | 1 << 7 | 1 << 8)); 
			TCCR2B &= (~(1 << 1 | 1 << 2));
		}
		else { //Start the PWM
		//Setup the timer
		//COM2A0 & COM2B1 set OC2b on compare match; WGM20 select PWM, Phase correct.
		//DDRD |= (1<<PD3);
		TCCR2A |= (1 << COM2A0 ) | (1 << COM2B1 ) | (1 << WGM20); //COM2A0 & 
		
		//Select clck/32 (from prescaler); WGM22 is clear, it updates OC2B on 0xFF and not on OCRB.
		TCCR2B |= (1 << CS21) | (1 << CS20); 
		OCR2B = 0xFF - 120;
		
		if( xSemaphoreTake( xSemaphoreUART, ( TickType_t ) 100 ) == pdTRUE )
		{
			if (stateBuzzer == OFF){
				//UART_sendString("{\"actuator\":\"buzzer\",\"value\":\"off\"}\n");
				UART_sendChar(c_BUZZER_OFF);
				UART_sendChar('\n');
			}
			else{
				//UART_sendString("{\"actuator\":\"buzzer\",\"value\":\"on\"}\n");
				UART_sendChar(c_BUZZER_ON);
				UART_sendChar('\n');
				}
			xSemaphoreGive( xSemaphoreUART );
		}
	}	
		
}

static void fanToggle() {
		//Se il buzzer non è impostato dall'esterno o il sistema non è in stato di allarme, il buzzer deve essere spento.
		if (stateFan == OFF){
			PORTB &= (~(1 << PB3)); //stop the fan
		}
		else {
			PORTB |= (1 << PB3); //ENABLE FAN
			PORTB |= (1 << PB2); //5v
			PORTB &= (~(1<< PB4)); //0v
		}
		if( xSemaphoreTake( xSemaphoreUART, ( TickType_t ) 100 ) == pdTRUE )
		{
			if (stateFan == OFF)
			{
				UART_sendChar(c_FAN_OFF);
				UART_sendChar('\n');
					
			}else {
				UART_sendChar(c_FAN_ON);
				UART_sendChar('\n');
			}
			
			xSemaphoreGive( xSemaphoreUART );
		}
}

static void UART_init()
{
	// Setup transmission speed
	UBRR0H = (BAUD_PRESCALER >> 8);
	UBRR0L = BAUD_PRESCALER;
	
	// Enable transmission and receiving (FULL DUPLEX)
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
	
	// Setup data format: 8 bit data, 1 stop bit
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_sendString(const char* data)
{
	//Send character until null terminator
	while (*data != '\0')
	{
		UART_sendChar(*data);
		data++;
	}
}

static void UART_sendChar(char data)
{
	//Wait for the transmission buffer is empty
	while (!(UCSR0A & (1 << UDRE0)));
	
	//Load the transmission buffer
	UDR0 = data;
}

static void LCD_putValue(unsigned char val)
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

static void LCD_sendCommand( unsigned char cmnd )
{ 
  LCD_CPRT &= ~ (1<<LCD_RS);    //RS = 0 for command  
  LCD_putValue(cmnd);
}

static void LCD_sendData( unsigned char data )
{
  LCD_CPRT |= (1<<LCD_RS);    //RS = 1 for data   
  LCD_putValue(data);  
}

static void LCD_clear()
{
  LCD_sendCommand(0x01);
  _delay_us(1700);
}

/* ----------------------------------- NOT USED -------------------------------------*/
/*
static void LCD_showCursor()
{
  LCD_sendCommand(0x0E);
  _delay_us(50);
}

static void LCD_hideCursor()
{
  LCD_sendCommand(0x0C);
  _delay_us(50);
}
*/

static void LCD_init()
{
  LCD_DDDR |= 0xF0;
  LCD_CDDR |= (1<<LCD_RS)|(1<<LCD_EN);
 
  LCD_CPRT &=~(1<<LCD_EN);  //LCD_EN = 0
  _delay_us(2000);      //wait for init.

  LCD_sendCommand(0x33); //send $33 for init.
  LCD_sendCommand(0x32); //send $32 for init
  LCD_sendCommand(0x28); //init. LCD 2 line,5*7 matrix
  LCD_sendCommand(0x0C); //display on, cursor on
  LCD_sendCommand(0x06); //shift cursor right

  LCD_clear();    
}

static void LCD_goToXY(unsigned char x, unsigned char y)
{  
  unsigned char firstCharAdr[]={0x80,0xC0,0x94,0xD4};
  LCD_sendCommand(firstCharAdr[y-1] + x - 1);
  _delay_us(100); 
}

static void LCD_print( char * str )
{
  unsigned char i = 0;

  while(str[i] != 0) //while it is not end of string
  {
    LCD_sendData(str[i]);
    i++ ;
  }
}

static void portInit (void){
	
	DDRC = 0; //make Porct C an input for ADC input
	DDRD = 0xFF; //make port D output
	//DDRD |= (1<<PD3);
	DDRB |= (1 << 3) | (1 << 2) | (1 << 4); //make port b output for PB3, PB2, PB4
	
}

static void ADC_setupToReadTemp(void){
	DDRC&=~(1<<0);
	ADCSRA = 0x87; //make ADC enagle and select ck/128
	ADMUX = 0b11000000; //adc0
	
}

static void ADC_setupToReadAir(void){
	DDRC&=~(1<<1);
	ADCSRA = 0x87; //make ADC enagle and select ck/128
	ADMUX = 0b01000001; //adc1
	
}

static float ADC_read(){

	ADCSRA |= (1<<ADSC); //Start conversion
	while ((ADCSRA&(1<<ADIF)) ==0); //wait for end of conversion
	ADCSRA |= (1<<ADIF); //clear the ADIF flag

	return (ADCL+(ADCH<<8));
	//return ADC;
}

char* generateArrayTemperature(unsigned char d1, unsigned char d2, unsigned char d3, unsigned char d4){
	//Handwriting the array of sending temperature
	const char jsonDataTemperatureTemp [] = "{\"sensor\":\"temperature\",\"value\"";
	int len1 = sizeof(jsonDataTemperatureTemp)/sizeof(jsonDataTemperatureTemp[0]);
	static char jsonDataTemperatureTempT2 [SIZE_ARRAY];
	
	for (int i = 0; i<len1; i++){
		jsonDataTemperatureTempT2[i] = jsonDataTemperatureTemp[i];
	}
	jsonDataTemperatureTempT2 [len1-1] = ':';
	jsonDataTemperatureTempT2 [len1] = ASCII_CONVERT(d1);
	jsonDataTemperatureTempT2 [len1+1] = ASCII_CONVERT(d2);
	jsonDataTemperatureTempT2 [len1+2] = '.';
	jsonDataTemperatureTempT2 [len1+3] = ASCII_CONVERT(d3);
	jsonDataTemperatureTempT2 [len1+4] = ASCII_CONVERT(d4);
	jsonDataTemperatureTempT2 [len1+5] = '}';
	jsonDataTemperatureTempT2 [len1+6] = '\n';
	jsonDataTemperatureTempT2 [len1+7] = '\0';
	return jsonDataTemperatureTempT2;
	
}

unsigned char* convertTemperature(float value){ //it must create  "{\"sensor\":\"temperature\",\"value\":25}\n"
	unsigned short x,y,z,w;
	//unsigned char d1,d2,d3,d4;
	
	static unsigned char result[4];
		
	/*float convert = value*10/93-50;*/
	float convert = value;
	//it must take single digit to print in LCD
	z = convert*100;
	w = z/10;
	x=w/10;
	y=x/10;
	
	//module to take single unit digit
	result[0]=y%10;
	result[1]=x%10;
	result[2]=w%10;
	result[3]=z%10;

// 	LCD_sendData(d1+0x30);
// 	LCD_sendData(d2+0x30);
// 	LCD_sendData('.');
// 	LCD_sendData(d3+0x30);
// 	LCD_sendData(d4+0x30);
// 	LCD_sendData(223);

	//return generateArrayTemperature(d1, d2, d3, d4);
	return result;
}

 void LCD_sendTemp (unsigned char d1, unsigned char d2, unsigned char d3, unsigned char d4){
 	LCD_sendData(d1+0x30);
 	LCD_sendData(d2+0x30);
 	LCD_sendData('.');
 	LCD_sendData(d3+0x30);
 	LCD_sendData(d4+0x30);
 	LCD_sendData(223);	
}

static char* generateArrayAir(unsigned char d1, unsigned char d2, unsigned char d3){
	char jsonDataAir [] = "{\"sensor\":\"air\",\"value\"";
	int len = sizeof(jsonDataAir)/sizeof(jsonDataAir[0]);
	//char *jsonDataAirT2 = malloc(len+8);
	static char jsonDataAirT2 [SIZE_ARRAY];
		
	for (int i = 0; i<len; i++){
		jsonDataAirT2[i] = jsonDataAir[i];
	}

		
	jsonDataAirT2 [len-1] = ':';

	if ((d1+ASCII_OFFSET) == '0'){ //excluding 0 when the value is < 100
		jsonDataAirT2 [len] = d2+ASCII_OFFSET;
		len = len -1;
		} else {
		jsonDataAirT2 [len] = d1+ASCII_OFFSET;
		jsonDataAirT2 [len+1] = d2+ASCII_OFFSET;
	}
		
	jsonDataAirT2 [len+2] = d3+ASCII_OFFSET;
		
	jsonDataAirT2 [len+3] = '.';
	jsonDataAirT2 [len+4] = '0';
	jsonDataAirT2 [len+5] = '0';
		
	jsonDataAirT2 [len+6] = '}';
	
	jsonDataAirT2 [len+7] = '\n';
	jsonDataAirT2 [len+8] = '\0';

	return jsonDataAirT2;
	
}

unsigned char* convertAir(float value){
	//unsigned char d1,d2,d3;
	//3 digit: 000 -> 999 
	unsigned int x,y,w;
	
	static unsigned char result[3];
	
	w = value;
	x=w/10;
	y=x/10;
	
	result[0]=y%10;
	result[1]=x%10;
	result[2]=w%10;
	
// 	LCD_sendData(d1+ASCII_OFFSET);
// 	LCD_sendData(d2+ASCII_OFFSET);
// 	LCD_sendData(d3+ASCII_OFFSET);

	//return generateArrayAir(d1, d2, d3);
	return result;
}

 void LCD_sendAir (unsigned char d1, unsigned char d2, unsigned char d3){
	LCD_sendData(d1+ASCII_OFFSET);
	LCD_sendData(d2+ASCII_OFFSET);
	LCD_sendData(d3+ASCII_OFFSET);
 }

void serialFromISRTask (void *parameters) {
	
	char *pcString; //where the queue receive the msg.
	uint8_t commandReceived;
	
	for( ;; ) {
		/* Block on the queue to wait for data to arrive. */
		xQueueReceive(xCharQueue, &pcString, portMAX_DELAY);
		
		commandReceived = pcString - ASCII_OFFSET; //Gives the number from exadecimal to Ascii
		
		switch (commandReceived) {
			case ALARM_ON:
				alarmON();
				break;
			case ALARM_OFF:
				alarmOFF();
				break;
			case BUZZER_ON:
				stateBuzzer = ON;
				buzzerToggle();
				break;
			case BUZZER_OFF:
				stateBuzzer = OFF;
				buzzerToggle();
				break;
			case FAN_ON:
				stateFan = ON;
				fanToggle();
				break;
			case FAN_OFF:
				stateFan = OFF;
				fanToggle();
				break;
			default:
				break;
 		}
		
	}
}

void temperatureReadTask(void* parameter) {
	
	//float threshold = THRESHOLD_TEMPERATURE;
	float temperature;
	
	for (;;){
		
		vTaskSuspend(temperatureReadTaskHandle);
		
		if( xSemaphoreADC != NULL )
		{
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
			if( xSemaphoreTake( xSemaphoreADC, ( TickType_t ) 100 ) == pdTRUE )
			{
				/* We were able to obtain the semaphore and can now access the
				shared resource. */
				ADC_setupToReadTemp();
				vTaskDelay(10);
				float tempTemperature = ADC_read();
				temperature = (tempTemperature*10/93)-50;
				
				/* We have finished accessing the shared resource.  Release the
				semaphore. */
				xSemaphoreGive( xSemaphoreADC );
				if (temperature > THRESHOLD_TEMPERATURE) {
					alarmON();
				}
				else {
					if (alarmState == ON){
					alarmOFF();
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
	}
}
	
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
                         &temperature,
                         100 ) == pdPASS )
						 
			{
				
				unsigned char *values = convertTemperature(temperature);
				char *jsonDataTemperature = generateArrayTemperature(values[0], values[1], values[2], values[3]);
				vTaskDelay(pdMS_TO_TICKS(1500));
				
				
				if( xSemaphoreTake( xSemaphoreLCD, ( TickType_t ) 100 ) == pdTRUE )
				{
					LCD_goToXY(6,1);
					LCD_sendTemp(values[0], values[1], values[2], values[3]);
					xSemaphoreGive( xSemaphoreLCD );
				}	
				
				if( xSemaphoreTake( xSemaphoreUART, ( TickType_t ) 100 ) == pdTRUE )
				{
					/* We were able to obtain the semaphore and can now access the
					shared resource. */
					
					UART_sendString(jsonDataTemperature);
					
					/* We have finished accessing the shared resource.  Release the
					semaphore. */
					xSemaphoreGive( xSemaphoreUART );
				}	
			}	
		}
	}
}

void airReadTask(void* parameter) {
	
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

				ADC_setupToReadAir();
				vTaskDelay(10);
				air = ADC_read();
			

				/* We have finished accessing the shared resource.  Release the
				semaphore. */
				xSemaphoreGive( xSemaphoreADC );
				
				if (air > THRESHOLD_AIR) {
					alarmON();
				}
				else {
					if (alarmState == ON){
						alarmOFF();
					}
				}
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
	
	float air;
	for (;;)
 	{	
 		if( xQueueAirSendings != NULL )
 		{
 			if( xQueueReceive( xQueueAirSendings,
                          &( air ),
                          ( TickType_t ) 100 ) == pdPASS )		 
 			{
				unsigned char *values = convertAir(air);
				char *jsonDataAir = generateArrayAir(values[0], values[1], values[2]);
				vTaskDelay(pdMS_TO_TICKS(1500));
				
				if( xSemaphoreTake( xSemaphoreLCD, ( TickType_t ) 100 ) == pdTRUE )
				{
					LCD_goToXY(6,2);
					LCD_sendAir(values[0], values[1], values[2]);
					xSemaphoreGive( xSemaphoreLCD );
				}
				
				if( xSemaphoreTake( xSemaphoreUART, ( TickType_t ) 100 ) == pdTRUE )
				{
					/* We were able to obtain the semaphore and can now access the
					shared resource. */
					UART_sendString(jsonDataAir);
					
					/* We have finished accessing the shared resource.  Release the
					semaphore. */
					xSemaphoreGive( xSemaphoreUART );
				}				
 			}	
 		}
 	}//for
}//airSendTask

void vTimerCallback(TimerHandle_t xTimerReadings)
 {
	//LCD_print("test");
	vTaskResume(temperatureReadTaskHandle);
	
	vTaskResume (AirReadTaskHandle);  
 }
 
// MAIN PROGRAM
int main(void)
{
	alarmState = OFF;
	stateBuzzer = OFF;
	stateFan = OFF;
	LCD_init();
	
	LCD_print("Temp:");
	LCD_goToXY(1,2);
	LCD_print("Air:");

	UART_init();
	
	portInit();
	
	xTimerReadings = xTimerCreate ("Timer", 5000, pdTRUE, ( void * ) 0, vTimerCallback);

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
	
    // Attempt to create a semaphores.
    xSemaphoreADC = xSemaphoreCreateBinary();
	xSemaphoreUART = xSemaphoreCreateBinary();
	xSemaphoreLCD = xSemaphoreCreateBinary();
	
	if( xSemaphoreADC != NULL )
   {
       xSemaphoreGive(xSemaphoreADC);
   }
   
   if( xSemaphoreUART != NULL )
   {
       xSemaphoreGive(xSemaphoreUART);
   }
   
   	if( xSemaphoreLCD != NULL )
   	{
	   	xSemaphoreGive(xSemaphoreLCD);
   	}
	
	xCharQueue = xQueueCreate( 2, sizeof( char * ) );
	
	xQueueAirSendings = xQueueCreate( 2, sizeof (float) );
	xQueueTemperatureSendings = xQueueCreate( 2, sizeof (float) );
			
	
	
 	xTaskCreate(temperatureReadTask, "temperatureReadTask", 100, NULL, 2, &temperatureReadTaskHandle);
 	
 	xTaskCreate(temperatureSendTask, "temperatureSendTask", 250, NULL, 2, &temperatureSendTaskHandle);
 	
 	xTaskCreate(airReadTask, "airReadTask", 100, NULL, 2, &AirReadTaskHandle);
 	
 	xTaskCreate(airSendTask, "airSendTask", 200, NULL, 2, &AirSendTaskHandle);
 	
 	xTaskCreate(serialFromISRTask, "serialFromISRTask", 100, NULL, 3, &serialFromISRTaskHandle);
	
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