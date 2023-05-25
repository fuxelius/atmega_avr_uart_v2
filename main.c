/*
 *     main.c
 *
 *          Project:  Test of USART for ATmega4808
 *          Author:   Hans-Henrik Fuxelius   
 *          Date:     Uppsala, 2023-05-24          
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "uart.h"

int main(void) {

    uint16_t c;
    uint8_t j=0;

    // (0) - USART settings; 
    usart_set(&usart0, &PORTA, PORTMUX_USART0_DEFAULT_gc, PIN0_bm, PIN1_bm);

    while (1) {

        // (1) - Init USART
        usart_init(&usart0, (uint16_t)BAUD_RATE(9600));

        // (2) - Enable global interrupts
        sei(); 

        // (3) - Send string to USART
        usart_send_string(&usart0, "\r\n\r\nPEACE BRO!\r\n\r\n", 18);

        // (4) - Use fprintf to write to stream
        fprintf(&usart0_stream, "Hello world!\r\n");

        for(size_t i=0; i<5; i++) {
            // (5) - Use formatted fprintf to write to stream
            fprintf(&usart0_stream, "\r\nCounter value is: 0x%02X ", j++);
            _delay_ms(500);

            // (6) - Get USART input by polling ringbuffer
            while(!((c = usart_read_char(&usart0)) & USART_NO_DATA)) {

                if (c & USART_PARITY_ERROR) {
                    fprintf(&usart0_stream, "USART PARITY ERROR: ");
                }
                if (c & USART_FRAME_ERROR) {
                    fprintf(&usart0_stream, "USART FRAME ERROR: ");
                }
                if (c & USART_BUFFER_OVERFLOW) {
                    fprintf(&usart0_stream, "USART BUFFER OVERFLOW ERROR: ");
                }

                // (7) - Send single character to USART
                usart_send_char(&usart0, (char)c);
            }
        }

        // (8) - Check that everything is printed before closing USART
        fprintf(&usart0_stream, "\r\n\r\n<-<->->");

        // (9) - Close USART0
        usart_close(&usart0);    

        // (10) - Clear global interrupts
        cli();

    }
}