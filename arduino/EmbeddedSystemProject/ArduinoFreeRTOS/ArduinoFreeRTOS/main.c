// LIBRARIES
#include "FreeRTOS.h"
#include "task.h"
#include <avr/io.h>

#include <util/delay.h>
#include <math.h>

#include <avr/interrupt.h>

#include "semphr.h"


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


//TaskHandle_t blinkLedHandle = NULL;
//TaskHandle_t stopBlinkLedHandle = NULL; //Crea il task handler che va passato nel xTaskCreate come ultimo parametro tramite &. Ad es: &myTask1Handle
//TaskHandle_t myTask2Handle = NULL; 

TaskHandle_t serialInterruptTaskHandle = NULL; 

TaskHandle_t alarmONTaskHandle = NULL;
TaskHandle_t alarmOFFTaskHandle = NULL;

TaskHandle_t buzzerTaskHandle = NULL;
TaskHandle_t fanTaskHandle = NULL;


QueueHandle_t xIntegerQueue;

int alarmState = 0; //0 if not alarm, 1 if alarm

int stateBuzzer = 0;
int stateFan = 0;

void alarmON(void* parameter) {
	for (;;){
		//la task viene creata ma va subito sospesa in attesa di chiamata. non servono semafori perché non ci sono risorse condivise.
		vTaskSuspend(NULL); //Quando impostato a Null sospende se stessa.	
		alarmState = 1;
		PORTD |= 1<<PD2;
		vTaskResume(buzzerTaskHandle);
		vTaskResume(fanTaskHandle);
	}
}

void alarmOFF(void* parameter) {
	for (;;){
		//la task viene creata ma va subito sospesa in attesa di chiamata. non servono semafori perché non ci sono risorse condivise.
		vTaskSuspend(NULL); //Quando impostato a Null sospende se stessa.
		alarmState = 0;
		PORTD &= (~(1<< PD2));
		vTaskSuspend(buzzerTaskHandle);
		vTaskSuspend(fanTaskHandle);
	}
}

void buzzerTask(void* parameter) {
	for (;;){
		
		//Se il buzzer non è impostato dall'esterno o il sistema non è in stato di allarme, il buzzer deve essere spento.
		if (stateBuzzer == 0){
			
			TCCR0A &= (~(1 << 1 | 1 << 7 | 1 << 8));
			TCCR0B &= (~(1 << 1 | 1 << 2));
			vTaskSuspend(NULL); 
		}
		else { //Esegui la PWM
		
		/*PORTD |= (1 << PD3);
		vTaskDelay(1); //da fare con i timer;
		PORTD &= (~(1 << PD3));
		vTaskDelay(1);
		*/
		
		OCR0A = 127;
		TCCR0A = 0b11000001;
		TCCR0B = 0b00000011;
		
		}
		
	}
}

void fanTask(void* parameter) {
	for (;;){
		
		//Se il buzzer non è impostato dall'esterno o il sistema non è in stato di allarme, il buzzer deve essere spento.
		if (stateFan == 0){
			PORTB &= (~(1 << PB3));
			vTaskSuspend(NULL);
		}
		else { //Esegui la PWM
			
			PORTB |= (1 << PB3); //ENABLE FAN
			
			PORTB |= (1 << PB2);
			PORTB &= (~(1<< PB4));
			}
			
			//PORTB &= (~(1<<5));
			//vTaskDelay (pdMS_TO_TICKS(30000));
			
			/*
			digitalWrite(DIRA,HIGH); //one way
			digitalWrite(DIRB,LOW);
			delay(750);
			digitalWrite(DIRA,LOW);
			digitalWrite(DIRB,HIGH);
			delay(750);
			*/
		}
		
	}




/*
// EXAMPLE BLINKER TASK
void blinkLED(void* parameter) {	
	for (;;){	
	 //la task viene creata ma va subito sospesa in attesa di chiamata. non servono semafori perché non ci sono risorse condivise.
		vTaskSuspend(NULL); //Quando impostato a Null sospende se stessa.
		PORTD |= (1 << 3);
		_delay_ms(75);
		PORTD &= (~(1<<3));
		_delay_ms(25);
		
		
		
		
		
		//PORTB |= (1 << PB5);						
	}
}


void stopBlinkLED(void* parameter) {
	for (;;) {	
		vTaskSuspend(NULL); //SUSPEND ITSELF
		PORTD &= ~(1 << 3);	
		
		
		
		 &= ~(1 << PB5);	
	}
}
*/

 

/*
void myTask2(void* parameter){

	for (;;){
		vTaskDelay(pdMS_TO_TICKS( 5000 )); 
		vTaskSuspend (myTask1Handle); //COME SOSPENDERE UNA TASK
		vTaskDelay(pdMS_TO_TICKS( 5000 ));
		vTaskResume (myTask1Handle); //COME FARLA AVVIARE DI NUOVO
		
	}
}
*/

ISR (USART_RX_vect) {
	
	signed char cChar;
	
	cChar = UDR0;
	
	//UART_sendString("Interrupt");
		
	//Due righe che servono sempre per lo YIELD delle task di FREERTOS
	BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	
	//La coda serve per scambiare messaggi tra le task. 
	//Penso che per altre task si possa utilizzare anche il suspend ed il resume. 
	xQueueSendFromISR( xIntegerQueue, &cChar, &xHigherPriorityTaskWoken );
	
	//xSemaphoreGiveFromISR( xBinarySemaphore, &xHigherPriorityTaskWoken );
	
	
	//Serve sempre per lo yield. Sulla documentazione è scritto come un'altra funzione, ma per atmega328p è questa. 
	if( xHigherPriorityTaskWoken != pdFALSE ) {
		taskYIELD();
	}
	
		
}



void serialInterruptTask (void *parameters) {
		
	char *pcString; //where the queue receive the msg.
	
	for( ;; ) {
		/* Block on the queue to wait for data to arrive. */
		xQueueReceive( xIntegerQueue, &pcString, portMAX_DELAY );
		
		uint8_t commandReceived  = pcString - 0x30; //Gives the number from exadecimal to Ascii
		
		switch (commandReceived) {
			case 0:
			UART_sendString("{\"actuator\":\"alarm\",\"value\":\"on\"}\n");
			stateBuzzer = 1;
			stateFan = 1;
			PORTD |= 1<<PD2;
			vTaskResume(buzzerTaskHandle);
			vTaskResume(fanTaskHandle);
			break;//causa l'uscita immediata dallo switch
			case 1:
			UART_sendString("{\"actuator\":\"alarm\",\"value\":\"off\"}\n");
			PORTD &= (~(1<< PD2));
			stateBuzzer = 0;
			stateFan = 0;
			break;
			case 2:
			UART_sendString("{\"actuator\":\"buzzer\",\"value\":\"on\"}\n");
			stateBuzzer = 1;
			vTaskResume(buzzerTaskHandle);
			break;
			case 3:
			UART_sendString("{\"actuator\":\"buzzer\",\"value\":\"off\"}\n");
			stateBuzzer = 0;
			//vTaskSuspend(buzzerTaskHandle);
			break;
			case 4:
			UART_sendString("{\"actuator\":\"fan\",\"value\":\"on\"}\n");
			stateFan = 1;
			vTaskResume(fanTaskHandle);
			break;
			case 5:
			UART_sendString("{\"actuator\":\"fan\",\"value\":\"off\"}\n");
			stateFan = 0;
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


// MAIN PROGRAM
int main(void)
{
	
	UART_init();
	
	DDRD = 0xFF;
	
	DDRD |= (1<<PD3);
	
	DDRB |= (1 << 3) | (1 << 2) | (1 << 4);
	
	//DDRB |= (1 << PB5);
	
	xIntegerQueue = xQueueCreate( 10, sizeof( char * ) );
	
	
		
	// CREATE BLINKER TASK
	//xTaskCreate(blinkLED, "blink", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &blinkLedHandle);
	//xTaskCreate(stopBlinkLED, "stopBlink", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &stopBlinkLedHandle);
	
	xTaskCreate(alarmON, "alarmON", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &alarmONTaskHandle); 
	xTaskCreate(alarmOFF, "alarmOFF", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &alarmOFFTaskHandle); 
	
	xTaskCreate(buzzerTask, "buzzerTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &buzzerTaskHandle); 
	xTaskCreate(fanTask, "fanTask", 200, NULL, tskIDLE_PRIORITY, &fanTaskHandle);	
	
	
	//xTaskCreate(myTask2, "myTask2", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &myTask2Handle);
		
	xTaskCreate(serialInterruptTask, "serialInterruptTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &serialInterruptTaskHandle);

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







/*


float temperature;
float air;

float _rzero;

//TODO La resistenza montata sul Flying-Fish è di 20kmOhm
#define MQ135_PULLDOWNRES 20000

//TODO Spiegare questi parametri
#define PARA 116.6020682
#define PARB 2.769034857

//GLOBAL CO2 JUNE 2023
#define ATMOCO2 420.13 


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
  // DDRB = 0xFF; //make port b an output
  
  DDRC = 0; //make Porct C an input for ADC input
  
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

  float convert = value *10/93-50;
  
  z = convert*100;
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

//TODO Spiegare questa funzione https://github.com/Phoenix1747/MQ135/blob/master/MQ135.cpp
float get_resistance_air(float value) {
  return ((1023/value) - 1)*MQ135_PULLDOWNRES;
}

float get_r_zero(float value) {
  return get_resistance_air(value) * pow((ATMOCO2/PARA), (1/PARB));
}

float get_PPM(float value) {
  return PARA * pow((get_resistance_air(value)/_rzero), -PARB);
}

void setup_MQ135(){
  adc_setup_air();
  air = read_adc();
  _rzero = get_r_zero(air);

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

//************************************

void UART_init()
{
    // Imposta la velocità di trasmissione
    UBRR0H = (BAUD_PRESCALER >> 8);
    UBRR0L = BAUD_PRESCALER;
    
    // Abilita la trasmissione e la ricezione
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    
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

char UART_receiveChar()
{
    // Attendi il completamento della ricezione
    while (!(UCSR0A & (1 << RXC0)));
    
    // Restituisci il dato ricevuto dal buffer di ricezione
    return UDR0;
}

//********************************************************

int main (void)
{

  lcd_init();
  //usart_init();       //initialize the USART
  lcd_print("Temp:");
  lcd_gotoxy(1,2);
  lcd_print("Air:");

  setup_MQ135(); //need to get _rzero first

  UART_init();
  
  while(1){         //do forever
    adc_setup_temp();  
    temperature = read_adc();    
    lcd_gotoxy(6,1);
    char *jsonDataTemperature = convert_and_write_temp(temperature);
    _delay_ms(1500);
    UART_sendString(jsonDataTemperature);
    _delay_ms(1500);
    free(jsonDataTemperature);
    _delay_ms(500);

    adc_setup_air();
    air = read_adc();
    lcd_gotoxy(6,2);
    
    char *jsonDataAir = convert_and_write_air(air);
    UART_sendString(jsonDataAir);
    free(jsonDataAir);
    _delay_ms(500);

    

    
  }
  return 0;
}
*/