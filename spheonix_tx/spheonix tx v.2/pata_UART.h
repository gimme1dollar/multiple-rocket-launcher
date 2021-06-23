/*
 * pata_UART.h
 *
 * Created: 2018-08-30 오전 11:56:35
 *  Author: pata
 *
 *
 *
 *						Atmega8 pin map
 *
 *				 RESET  = PC6   PC5 =
 *					RX	= PD0   PC4 =
 *					TX	= PD1   PC3 =
 *						= PD2   PC2 =
 *						= PD3   PC1 =
 *						= PD4   PC0 =
 *				 VCC	= VCC   GND =
 *				 GND	= GND   AREF=
 *						= PB6   AVCC=
 *            			= PB7   PB5 =
 *			  			= PD5   PB4 =
 *						= PD6   PB3 =
 *						= PD7   PB2 =
 *						= PB0   PB1 =
 *
 *
 */ 


#ifndef PATA_UART_H_
#define PATA_UART_H_
#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#define TX		0x01
#define RX		0x02
#define TXRX	0x03
#define RXI		0x04
#define TXRXI	0x05

#define BAUD_9600	1
#define BAUD_14k	2
#define BAUD_19k	3
#define BAUD_38k	4


#include <avr/interrupt.h>
#include <stdint.h>
#include <string.h>



// UART initialize
void UART_init(uint8_t baud, uint8_t mode);

// Standard TX/RX
void UART_tx(uint8_t data);
void UART_tx_m(uint8_t *data, uint8_t length);
uint8_t UART_rx();
void UART_rx_m(uint8_t *data, uint8_t length);
void UART_NWL();


volatile uint8_t data = 0;
volatile uint8_t data_m[10] = {0};
uint8_t ubrr = 0;


ISR (USART_RXC_vect)
{
	data = UDR;
	
}

void UART_init(uint8_t baud, uint8_t mode)
{
	switch (baud)
	{
		case BAUD_9600:
			ubrr = 51;
			break;
		case BAUD_14k:
			ubrr = 34;
			break;
		case BAUD_19k:
			ubrr = 25;
			break;
		case BAUD_38k:
			ubrr = 12;
			break;
	}
	UBRRH = (ubrr >> 8);
	UBRRL = ubrr;
	
	if (mode & TX)
		UCSRB = (1 << TXEN);
	if ((mode & RX) || (mode & RXI))
		UCSRB |= (1 << RXEN);
	if (mode & RXI)
		UCSRB |= (1 << RXCIE);
	
	DDRD &= ~(1<<0);
	DDRD |= (1<<1);
}

void UART_tx(uint8_t data)
{
	while(!(UCSRA & (1<<UDRE)));
	UDR = data;
}

void UART_tx_m(uint8_t *data, uint8_t length)
{
	for (uint8_t i = 0; i < length; i++)
	{
		while(!(UCSRA & (1<<UDRE)));
		UDR = data[i];
	}
}

void UART_LF()
{
	while(!(UCSRA & (1<<UDRE)));
	UDR = 0x0A;
}

uint8_t UART_rx()
{
	while (!(UCSRA & (1<<RXC)));
	data = UDR;
	
	return data;
}

void UART_rx_m(uint8_t *data, uint8_t length)
{
	for (uint8_t i = 0; i < length; i++)
	{
		while(!(UCSRA & (1<<RXC)));
		UDR = data[i];
	}
}

void UART_NWL()
{
	while(!(UCSRA & (1<<UDRE)));
	UDR = 0x0A;
}


#endif /* PATA_UART_H_ */