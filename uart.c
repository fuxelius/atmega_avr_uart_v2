/*
 *     uart.c
 *
 *          Project:  UART for megaAVR, tinyAVR & AVR DA
 *          Author:   Hans-Henrik Fuxelius   
 *          Date:     Uppsala, 2023-05-24           
 */

#include <avr/io.h>
#include <util/atomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include "uart.h"

#define USART_RX_ERROR_MASK (USART_BUFOVF_bm | USART_FERR_bm | USART_PERR_bm) // [Datasheet ss. 295]

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// RINGBUFFER FUNCTIONS
void rbuffer_init(volatile ringbuffer* rb) {
	rb->in = 0;
	rb->out = 0;
	rb->count = 0;
}

uint8_t rbuffer_count(volatile ringbuffer* rb) {
	return rb->count;
}

bool rbuffer_full(volatile ringbuffer* rb) {
	return (rb->count == (uint8_t)RBUFFER_SIZE);
}

bool rbuffer_empty(volatile ringbuffer* rb) {
	return (rb->count == 0);
}

void rbuffer_insert(char data, volatile ringbuffer* rb) {   
	*(rb->buffer + rb->in) = data;
	rb->in = (rb->in + 1) & ((uint8_t)RBUFFER_SIZE - 1);
	rb->count++;
}

char rbuffer_remove(volatile ringbuffer* rb) {
	char data = *(rb->buffer + rb->out);
	rb->out = (rb->out + 1) & ((uint8_t)RBUFFER_SIZE - 1);
	rb->count--;
	return data;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// VARIABLES
#ifdef USART0_ENABLE
volatile usart_meta usart0 = {.usart = &USART0};
#endif

#ifdef USART1_ENABLE
volatile usart_meta usart1 = {.usart = &USART1};
#endif

#ifdef USART2_ENABLE
volatile usart_meta usart2 = {.usart = &USART2};
#endif

#ifdef USART3_ENABLE
volatile usart_meta usart3 = {.usart = &USART3};
#endif

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// USART FUNCTIONS
void usart_set(volatile usart_meta* meta, PORT_t*  port, uint8_t route, uint8_t tx_pin, uint8_t rx_pin) {
	meta->port = port;
	meta->route = route;
	meta->tx_pin = tx_pin;
	meta->rx_pin = rx_pin;
}

void usart_send_char(volatile usart_meta* meta, char c) {
	while(rbuffer_full(&meta->rb_tx));
	rbuffer_insert(c, &meta->rb_tx);
	meta->usart->CTRLA |= USART_DREIE_bm;					// Enable Tx interrupt 
}

void usart_init(volatile usart_meta* meta, uint16_t baud_rate) {
	rbuffer_init(&meta->rb_rx);								// Init Rx buffer
	rbuffer_init(&meta->rb_tx);								// Init Tx buffer
    PORTMUX.USARTROUTEA |= meta->route;   					// Set route
    meta->port->DIR &= ~meta->rx_pin;			    		// Rx PIN input
    meta->port->DIR |= meta->tx_pin;			    		// Tx PIN output
    meta->usart->BAUD = baud_rate; 							// Set BAUD rate
	meta->usart->CTRLB |= USART_RXEN_bm | USART_TXEN_bm; 	// Enable Rx & Enable Tx 
	meta->usart->CTRLA |= USART_RXCIE_bm; 					// Enable Rx interrupt 
}

void usart_send_string(volatile usart_meta* meta, char* str, uint8_t len) {
	for (size_t i=0; i<len; i++) {
		usart_send_char(meta, str[i]);
	}
}

uint16_t usart_read_char(volatile usart_meta* meta) {
	if (!rbuffer_empty(&meta->rb_rx)) {
		return (((meta->usart_error & USART_RX_ERROR_MASK) << 8) | (uint16_t)rbuffer_remove(&meta->rb_rx));
	}
	else {
		return (((meta->usart_error & USART_RX_ERROR_MASK) << 8) | USART_NO_DATA);		// Empty ringbuffer
	}
}

void usart_close(volatile usart_meta* meta) {
	while(!rbuffer_empty(&meta->rb_tx)); 						// Wait for Tx to finish all character in ring buffer
	while(!(meta->usart->STATUS & USART_DREIF_bm)); 			// Wait for Tx unit to finish the last character of ringbuffer

	_delay_ms(200); 											// Extra safety for Tx to finish!

	meta->usart->CTRLB &= ~(USART_RXEN_bm | USART_TXEN_bm); 	// Disable Tx, Rx unit
	meta->usart->CTRLA &= ~(USART_RXCIE_bm | USART_DREIE_bm); 	// Disable Tx, Rx interrupt

	// Disable PORTMUX pins [PORTMUX_USART0_NONE_gc]
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// STREAM SETUP
#ifdef USART0_ENABLE
int usart0_print_char(char c, FILE *stream) { 
    usart_send_char(&usart0, c);							
    return 0; 
}
FILE usart0_stream = FDEV_SETUP_STREAM(usart0_print_char, NULL, _FDEV_SETUP_WRITE);
#endif

#ifdef USART1_ENABLE
int usart1_print_char(char c, FILE *stream) { 
    usart_send_char(&usart1, c);							
    return 0; 
}
FILE usart1_stream = FDEV_SETUP_STREAM(usart1_print_char, NULL, _FDEV_SETUP_WRITE);
#endif

#ifdef USART2_ENABLE
int usart2_print_char(char c, FILE *stream) { 
    usart_send_char(&usart2, c);							
    return 0; 
}
FILE usart2_stream = FDEV_SETUP_STREAM(usart2_print_char, NULL, _FDEV_SETUP_WRITE);
#endif

#ifdef USART3_ENABLE
int usart3_print_char(char c, FILE *stream) { 
    usart_send_char(&usart3, c);							
    return 0; 
}
FILE usart3_stream = FDEV_SETUP_STREAM(usart3_print_char, NULL, _FDEV_SETUP_WRITE);
#endif

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// ISR HELPER FUNCTIONS
void isr_usart_rxc_vect(volatile usart_meta* meta) {
    char data = meta->usart->RXDATAL;
	rbuffer_insert(data, &meta->rb_rx);
	meta->usart_error = meta->usart->RXDATAH;
}

void isr_usart_dre_vect(volatile usart_meta* meta) {
	if(!rbuffer_empty(&meta->rb_tx)) {
		meta->usart->TXDATAL = rbuffer_remove(&meta->rb_tx);     
	}
	else {
		meta->usart->CTRLA &= ~USART_DREIE_bm;
	}
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// ISR FUNCTIONS
#ifdef USART0_ENABLE
ISR(USART0_RXC_vect) {
	isr_usart_rxc_vect(&usart0);
}
ISR(USART0_DRE_vect) {
	isr_usart_dre_vect(&usart0);
}
#endif

#ifdef USART1_ENABLE
ISR(USART1_RXC_vect) {
	isr_usart_rxc_vect(&usart1);
}
ISR(USART1_DRE_vect) {
	isr_usart_dre_vect(&usart1);
}
#endif

#ifdef USART2_ENABLE
ISR(USART2_RXC_vect) {
	isr_usart_rxc_vect(&usart2);
}
ISR(USART2_DRE_vect) {
	isr_usart_dre_vect(&usart2);
}
#endif

#ifdef USART3_ENABLE
ISR(USART3_RXC_vect) {
	isr_usart_rxc_vect(&usart3);
}
ISR(USART3_DRE_vect) {
	isr_usart_dre_vect(&usart3);
}
#endif