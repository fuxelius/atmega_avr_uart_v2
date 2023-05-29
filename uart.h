/*
 *     uart.h
 *
 *          Description:  UART for megaAVR, tinyAVR & AVR DA DD DB EA
 *          Author:       Hans-Henrik Fuxelius   
 *          Date:         Uppsala, 2023-05-24 
 *          License:      MIT
 * 			Version:      RC1         
 */

#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// DEFINE RING BUFFER SIZE; MUST BE 2, 4, 8, 16, 32, 64 or 128  
#define RBUFFER_SIZE 32  

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// UNCOMMENT TO ENABLE USARTn
#define USART0_ENABLE
// #define USART1_ENABLE
// #define USART2_ENABLE
// #define USART3_ENABLE
// #define USART4_ENABLE
// #define USART5_ENABLE
// #define USART6_ENABLE
// #define USART7_ENABLE

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// UNCOMMENT TO ENABLE FILE STREAMS
#define USART_STREAM

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
#define USART_BUFFER_OVERFLOW    0x6400      // ==USART_BUFOVF_bm  
#define USART_FRAME_ERROR        0x0400      // ==USART_FERR_bm             
#define USART_PARITY_ERROR       0x0200      // ==USART_PERR_bm      
#define USART_NO_DATA            0x0100      

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
#define BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// RINGBUFFER STRUCT
typedef struct { 
    volatile char     buffer[RBUFFER_SIZE];     
    volatile uint8_t  in;                           
    volatile uint8_t  out;                          
    volatile uint8_t  count;         
} ringbuffer_t;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// USART META STRUCT
typedef struct { 
	USART_t* usart;					// USART device ptr
    PORT_t*  port;                  // PORT device ptr
    register8_t* pmuxr;             // PORTMUX.USARTROUTE A B ptr
    uint8_t route;                  // PORTMUX PIN route bm
    uint8_t rx_pin;                 // Rx PIN bm
    uint8_t tx_pin;                 // Tx PIN bm
	volatile ringbuffer_t rb_rx;	// Rx ringbuffer
	volatile ringbuffer_t rb_tx;	// Tx ringbuffer
	volatile uint8_t usart_error;	// Holds error from RXDATAH        
} usart_meta_t;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// USART FUNCTIONS
void usart_set(volatile usart_meta_t* meta, PORT_t*  port, uint8_t route, uint8_t tx_pin, uint8_t rx_pin);
void usart_init(volatile usart_meta_t* meta, uint16_t baud_rate);
void usart_send_char(volatile usart_meta_t* meta, char c);
void usart_send_string(volatile usart_meta_t* meta, const char* str);
void usart_send_string_P(volatile usart_meta_t* meta, const char* chr);
uint8_t usart_rx_count(volatile usart_meta_t* meta);
uint16_t usart_read_char(volatile usart_meta_t* meta);
void usart_close(volatile usart_meta_t* meta);

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// usart_meta_t
#ifdef USART_STREAM

    #ifdef USART0_ENABLE
    extern volatile usart_meta_t usart0;
    extern FILE usart0_stream;
    #endif

    #ifdef USART1_ENABLE
    extern volatile usart_meta_t usart1;
    extern FILE usart1_stream;
    #endif

    #ifdef USART2_ENABLE
    extern volatile usart_meta_t usart2;
    extern FILE usart2_stream;
    #endif

    #ifdef USART3_ENABLE
    extern volatile usart_meta_t usart3;
    extern FILE usart3_stream;
    #endif

    #ifdef USART4_ENABLE
    extern volatile usart_meta_t usart4;
    extern FILE usart4_stream;
    #endif

    #ifdef USART5_ENABLE
    extern volatile usart_meta_t usart5;
    extern FILE usart5_stream;
    #endif

    #ifdef USART6_ENABLE
    extern volatile usart_meta_t usart6;
    extern FILE usart6_stream;
    #endif

    #ifdef USART7_ENABLE
    extern volatile usart_meta_t usart7;
    extern FILE usart7_stream;
    #endif

#else

    #ifdef USART0_ENABLE
    extern volatile usart_meta_t usart0;
    #endif

    #ifdef USART1_ENABLE
    extern volatile usart_meta_t usart1;
    #endif

    #ifdef USART2_ENABLE
    extern volatile usart_meta_t usart2;
    #endif

    #ifdef USART3_ENABLE
    extern volatile usart_meta_t usart3;
    #endif

    #ifdef USART4_ENABLE
    extern volatile usart_meta_t usart4;
    #endif

    #ifdef USART5_ENABLE
    extern volatile usart_meta_t usart5;
    #endif

    #ifdef USART6_ENABLE
    extern volatile usart_meta_t usart6;
    #endif

    #ifdef USART7_ENABLE
    extern volatile usart_meta_t usart7;
    #endif

#endif