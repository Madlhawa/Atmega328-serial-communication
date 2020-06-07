#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

/* UART SERIAL DEFINES */
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define FOSC 16000000 // Clock frequency

void USART_Init( unsigned int ubrr);
void USART_Transmit( unsigned char data );
unsigned char USART_Receive( void );
void SendString(char *StringPtr);
void RecieveString(void);
void clearbuff(char arr[]);
void init_lcd();
void moveto(unsigned char row, unsigned char col);
void stringout(char *str);
void writecommand(unsigned char cmd);
void writedata(unsigned char dat);
void writenibble(unsigned char lcdbits);
void initKeypad(void);
void keyPress();
void recieveIndication();
void deliverIndication();

char input[16];
char output[15];
char msg[16];
int counter;
int k=0;
int outputPointer = 0;
int flag=0;
  
  
void USART_Init( unsigned int ubrr){/* SETUP UART */
	UBRR0 = MYUBRR; /*Set baud rate */
	UCSR0B |= (1 << RXCIE0);//rx interrupt
	UCSR0B |= (1 << TXEN0 | 1 << RXEN0); /*Enable receiver and transmitter */ 
	UCSR0C = (3 << UCSZ00); /* Set frame format */
}
/* Simple methods to make UART read and transmit more readble - Extremely unnecessary*/
void USART_Transmit( unsigned char data ){
	// Wait for transmitter data register empty
	while ((UCSR0A & (1<<UDRE0)) == 0) {}
	UDR0 = data;
}
unsigned char USART_Receive( void ){
	// Wait for receive complete flag to go high
	while ( !(UCSR0A & (1 << RXC0)) ) {}
	return UDR0;
}
void SendString(char *StringPtr){
	while(*StringPtr != 0x00)
	{  
		USART_Transmit(*StringPtr);   
		StringPtr++;
	}        
}
void RecieveString(void){
	int i;
	for(i=0;i<16;i++){
		input[i]=USART_Receive();
	}
}
void clearbuff(char arr[]){
	int i;
	for(i=0;i<16;i++){
			msg[i]='\0';
	}
}


void init_lcd(){
    // Set the DDR register bits for ports B and D
	DDRC|=0x0f;
	DDRD|=0x0c;
    _delay_ms(15);// Delay at least 15ms

	 // Use writenibble to send 0011
	writecommand(0x03);
    _delay_ms(5);// Delay at least 4msec
	
	// Use writenibble to send 0011
	writecommand(0x03);
    _delay_us(105);// Delay at least 100usec

     // Use writenibble to send 0011, no delay needed
	writecommand(0x03);
	
	// Use writenibble to send 0010
	writecommand(0x02);
    _delay_ms(2); // Delay at least 2ms
    
    writecommand(0x28);         // Function Set: 4-bit interface, 2 lines
	_delay_ms(2);
    
	writecommand(0x0f);         // Display and cursor on
	_delay_ms(25); 

	writecommand(0x01); 
}
/*moveto - Move the cursor to the row (0 or 1) and column (0 to 15) specified*/
void moveto(unsigned char row, unsigned char col){
	if(row==0){
		writecommand(0x80+col);
	}
	if(row==1){
		writecommand(0xc0+col);
	}
}
/*stringout - Write the string pointed to by "str" at the current position*/
void stringout(char *str){
	do{
		writedata(*str);
		str++;
	}while(*str!= '\0');
}
/*writecommand - Send the 8-bit byte "cmd" to the LCD command register*/
void writecommand(unsigned char cmd){
	unsigned char temp;

	PORTD&=~(0x04);
	temp=cmd&0xF0;
	temp=temp>>4;
	writenibble(temp);
	temp=cmd&0x0F;
	writenibble(temp);
	_delay_ms(3);
}
/*writedata - Send the 8-bit byte "dat" to the LCD data register*/
void writedata(unsigned char dat){
	unsigned char temp;

	PORTD|=0x04;
	temp=dat&0xF0;
	temp=temp>>4;
	writenibble(temp);
	temp=dat&0x0F;
	writenibble(temp);
	_delay_ms(3);
}
/*writenibble - Send four bits of the byte "lcdbits" to the LCD*/
void writenibble(unsigned char lcdbits){
	PORTC = lcdbits;
	PORTD &= ~(0x08);
	PORTD |= 0x08;
	PORTD &= ~(0x08);
}

void initKeypad(void){
	DDRD |= 0xe0;
	DDRB &= ~(0x1f);
	PORTB |= 0x1f;
	PORTD |= 0x40;
}
void keyPress(){
	if((PINB & 0x1f)!=0x1f){
		_delay_ms(5);
		moveto(1,0);
		if(!(PINB & (1<<PB1))){
			_delay_ms(5);
			while(!(PINB & (1<<PB1))){}
			output[outputPointer]='a';
		}
		else if (!(PINB & (1<<PB2))){
			_delay_ms(5);
			while(!(PINB & (1<<PB2))){}
			output[outputPointer]='b';
		}
		else if (!(PINB & (1<<PB3))){
			_delay_ms(5);
			while(!(PINB & (1<<PB3))){}
			output[outputPointer]='c';
		}
		else if (!(PINB & (1<<PB0))){
			_delay_ms(5);
			while(!(PINB & (1<<PB0))){}
			outputPointer--;
			output[outputPointer]='\0';
			stringout("                ");
			moveto(1,0);
			if(outputPointer>=0)
				outputPointer--;
		}
		else if (!(PINB & (1<<PB4))){
			_delay_ms(5);
			while(!(PINB & (1<<PB4))){}
			output[outputPointer]='.';
			SendString(output);
			clearbuff(output);
			stringout("                ");
			moveto(1,0);
			_delay_ms(10);
		}
		outputPointer++;
		stringout(output);
	}
}

void recieveIndication(){
	PORTD |= 0x20;//small buzzer indication
	_delay_ms(70);
	PORTD &= ~(0x20);
	_delay_ms(70);
	PORTD |= 0x20;//small buzzer indication
	_delay_ms(70);
	PORTD &= ~(0x20);
}

void deliverIndication(){
	PORTD |= 0x80;
	_delay_ms(70);
	PORTD &= ~(0x80);
	_delay_ms(70);
	PORTD |= 0x80;
	_delay_ms(70);
	PORTD &= ~(0x80);
}

int main(void){
    USART_Init(MYUBRR);
	init_lcd();
	_delay_ms(15);
	writecommand(0x01);	
	initKeypad();
	sei();
	
    while(1){
		keyPress();
		
		if(flag==1){
			recieveIndication();
			msg[k-1]='\0';
			flag=0;
			k=0;
			writecommand(0x01);
			stringout(msg);
			clearbuff(msg);
			SendString("?");
		}
		else if(flag==2){//react for ack
			flag=0;
			k=0;
			deliverIndication();
		}
	}
}

ISR(USART_RX_vect){
	
	msg[k++]=USART_Receive();
	if(msg[k-1]=='?')//check for ack
		flag=2;
	else if((msg[k-1]=='.')||(k==15))
		flag=1;
	else
		flag=0;
}



