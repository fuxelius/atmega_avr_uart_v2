/*
 *     uart.c
 *
 *          Description:  UART for megaAVR, tinyAVR & AVR DA DD DB EA
 *          Author:       Hans-Henrik Fuxelius   
 *          Date:         Uppsala, 2023-05-29 
 *          License:      MIT
 *          Version:      RC1          
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "uart.h"

#define USART_RX_ERROR_MASK (USART_BUFOVF_bm | USART_FERR_bm | USART_PERR_bm) // [Datasheet ss. 295]

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// RINGBUFFER FUNCTIONS
void rbuffer_init(volatile ringbuffer_t* rb) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        rb->in = 0;
        rb->out = 0;
        rb->count = 0;
    }
}

uint8_t rbuffer_count(volatile ringbuffer_t* rb) {
    return rb->count;
}

bool rbuffer_full(volatile ringbuffer_t* rb) {
    return (rb->count == (uint8_t)RBUFFER_SIZE);
}

bool rbuffer_empty(volatile ringbuffer_t* rb) {
    return (rb->count == 0);
}

void rbuffer_insert(char data, volatile ringbuffer_t* rb) {   
    *(rb->buffer + rb->in) = data;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        rb->in = (rb->in + 1) & ((uint8_t)RBUFFER_SIZE - 1);
        rb->count++;
    }
}

char rbuffer_remove(volatile ringbuffer_t* rb) {
    char data = *(rb->buffer + rb->out);
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        rb->out = (rb->out + 1) & ((uint8_t)RBUFFER_SIZE - 1);
        rb->count--;
    }
    return data;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// VARIABLES
#ifdef USART0_ENABLE
volatile usart_meta_t usart0 = {.usart = &USART0, .pmuxr = &PORTMUX.USARTROUTEA};
#endif

#ifdef USART1_ENABLE
volatile usart_meta_t usart1 = {.usart = &USART1, .pmuxr = &PORTMUX.USARTROUTEA};
#endif

#ifdef USART2_ENABLE
volatile usart_meta_t usart2 = {.usart = &USART2, .pmuxr = &PORTMUX.USARTROUTEA};
#endif

#ifdef USART3_ENABLE
volatile usart_meta_t usart3 = {.usart = &USART3, .pmuxr = &PORTMUX.USARTROUTEA};
#endif

#ifdef USART4_ENABLE
volatile usart_meta_t usart4 = {.usart = &USART4, .pmuxr = &PORTMUX.USARTROUTEB};
#endif

#ifdef USART5_ENABLE
volatile usart_meta_t usart5 = {.usart = &USART5, .pmuxr = &PORTMUX.USARTROUTEB};
#endif

#ifdef USART6_ENABLE
volatile usart_meta_t usart6 = {.usart = &USART6, .pmuxr = &PORTMUX.USARTROUTEB};
#endif

#ifdef USART7_ENABLE
volatile usart_meta_t usart7 = {.usart = &USART7, .pmuxr = &PORTMUX.USARTROUTEB};
#endif

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// USART FUNCTIONS
void usart_set(volatile usart_meta_t* meta, PORT_t*  port, uint8_t route_gc, uint8_t tx_pin, uint8_t rx_pin) {
    meta->port = port;
    meta->route = route_gc;
    meta->tx_pin = tx_pin;
    meta->rx_pin = rx_pin;
}

void usart_init(volatile usart_meta_t* meta, uint16_t baud_rate) {
    rbuffer_init(&meta->rb_rx);                             // Init Rx buffer
    rbuffer_init(&meta->rb_tx);                             // Init Tx buffer
    *meta->pmuxr |= meta->route;                            // Set Rx, Tx PIN route
    meta->port->DIR &= ~meta->rx_pin;                       // Rx PIN input
    meta->port->DIR |= meta->tx_pin;                        // Tx PIN output
    meta->usart->BAUD = baud_rate;                          // Set BAUD rate
    meta->usart->CTRLB |= (USART_RXEN_bm | USART_TXEN_bm);  // Enable Rx, Tx units
    meta->usart->CTRLA |= USART_RXCIE_bm;                   // Enable Rx interrupt 
}

void usart_send_char(volatile usart_meta_t* meta, char c) {
    while(rbuffer_full(&meta->rb_tx));
    rbuffer_insert(c, &meta->rb_tx);
    meta->usart->CTRLA |= USART_DREIE_bm;                   // Enable Tx interrupt 
}

void usart_send_string(volatile usart_meta_t* meta, const char* str) {
    while (*str) {
        usart_send_char(meta, *str++);
    }
}

void usart_send_string_P(volatile usart_meta_t* meta, const char* chr) {
    char c;
    while ((c = pgm_read_byte(chr++))) {
        usart_send_char(meta, c);
    }
}

uint8_t usart_rx_count(volatile usart_meta_t* meta) {
    return rbuffer_count(&meta->rb_rx);
}

uint16_t usart_read_char(volatile usart_meta_t* meta) {
    if (!rbuffer_empty(&meta->rb_rx)) {
        return (((meta->usart_error & USART_RX_ERROR_MASK) << 8) | (uint16_t)rbuffer_remove(&meta->rb_rx));
    }
    else {
        return (((meta->usart_error & USART_RX_ERROR_MASK) << 8) | USART_NO_DATA);     // Empty ringbuffer
    }
}

void usart_close(volatile usart_meta_t* meta) {
    while(!rbuffer_empty(&meta->rb_tx));                        // Wait for Tx to transmit ALL characters in ringbuffer
    while(!(meta->usart->STATUS & USART_DREIF_bm));             // Wait for Tx unit to transmit the LAST character of ringbuffer

    _delay_ms(200);                                             // Extra safety for Tx to finish!

    meta->usart->CTRLB &= ~(USART_RXEN_bm | USART_TXEN_bm);     // Disable Tx, Rx unit
    meta->usart->CTRLA &= ~(USART_RXCIE_bm | USART_DREIE_bm);   // Disable Tx, Rx interrupt
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// STREAM SETUP (OPTIONAL)
#ifdef USART_STREAM

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

    #ifdef USART4_ENABLE
    int usart4_print_char(char c, FILE *stream) { 
        usart_send_char(&usart4, c);
        return 0; 
    }
    FILE usart4_stream = FDEV_SETUP_STREAM(usart4_print_char, NULL, _FDEV_SETUP_WRITE);
    #endif

    #ifdef USART5_ENABLE
    int usart5_print_char(char c, FILE *stream) { 
        usart_send_char(&usart5, c);
        return 0; 
    }
    FILE usart5_stream = FDEV_SETUP_STREAM(usart5_print_char, NULL, _FDEV_SETUP_WRITE);
    #endif

    #ifdef USART6_ENABLE
    int usart6_print_char(char c, FILE *stream) { 
        usart_send_char(&usart6, c);
        return 0; 
    }
    FILE usart6_stream = FDEV_SETUP_STREAM(usart6_print_char, NULL, _FDEV_SETUP_WRITE);
    #endif

    #ifdef USART7_ENABLE
    int usart7_print_char(char c, FILE *stream) { 
        usart_send_char(&usart7, c);
        return 0; 
    }
    FILE usart7_stream = FDEV_SETUP_STREAM(usart7_print_char, NULL, _FDEV_SETUP_WRITE);
    #endif

#endif

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// ISR HELPER FUNCTIONS
static inline void isr_usart_rxc_vect(volatile usart_meta_t* meta) {
    char data = meta->usart->RXDATAL;
    if(!rbuffer_full(&meta->rb_rx)) {
        rbuffer_insert(data, &meta->rb_rx);
        meta->usart_error = meta->usart->RXDATAH;
    }
    else {
        meta->usart_error = (meta->usart->RXDATAH | USART_BUFFER_OVERFLOW>>8);
    }
}

static inline void isr_usart_dre_vect(volatile usart_meta_t* meta) {
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

#ifdef USART4_ENABLE
ISR(USART4_RXC_vect) {
    isr_usart_rxc_vect(&usart4);
}
ISR(USART4_DRE_vect) {
    isr_usart_dre_vect(&usart4);
}
#endif

#ifdef USART5_ENABLE
ISR(USART5_RXC_vect) {
    isr_usart_rxc_vect(&usart5);
}
ISR(USART5_DRE_vect) {
    isr_usart_dre_vect(&usart5);
}
#endif

#ifdef USART6_ENABLE
ISR(USART6_RXC_vect) {
    isr_usart_rxc_vect(&usart6);
}
ISR(USART6_DRE_vect) {
    isr_usart_dre_vect(&usart6);
}
#endif

#ifdef USART7_ENABLE
ISR(USART7_RXC_vect) {
    isr_usart_rxc_vect(&usart7);
}
ISR(USART7_DRE_vect) {
    isr_usart_dre_vect(&usart7);
}
#endif