/*
 *     main.c
 *
 *          Project:  Test of USART for ATmega4808
 *          Author:   Hans-Henrik Fuxelius   
 *          Date:     Uppsala, 2023-05-24          
 */

#define F_CPU 2666666

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>
#include "uart.h"

int main(void) {

    uint16_t c;
    uint8_t j=0;

    char buffer[100];

    // (0) - USART settings; 
    usart_set(&usart0, &PORTA, PORTMUX_USART0_DEFAULT_gc, PIN0_bm, PIN1_bm);

    while (1) {

        // (1) - Init USART
        usart_init(&usart0, (uint16_t)BAUD_RATE(9600));

        // (2) - Enable global interrupts
        sei(); 

        // usart_send_string(&usart0, "U");
        // usart_send_char(&usart0, 'X');
        // usart_send_string_P(&usart0, PSTR("\r\nCounter value is: 0x%02X "), 66);

        // (3) - Send string to USART
        usart_send_string(&usart0, "\r\n\r\nPEACE BRO!\r\n\r\n");

        // (4) - Use sprintf_P, fputs to write to stream
        sprintf_P(buffer, PSTR("Hello world!\r\n"));
        fputs(buffer, &usart0_stream);

        for(size_t i=0; i<5; i++) {
            // (5) - Use formatted fprintf_P to write to stream
            fprintf_P(&usart0_stream, PSTR("\r\nCounter value is: 0x%02X "), j++);
            _delay_ms(500);

            // (6) - Get USART input by polling ringbuffer
            while(!((c = usart_read_char(&usart0)) & USART_NO_DATA)) {

                if (c & USART_PARITY_ERROR) {
                    usart_send_string_P(&usart0, PSTR("USART PARITY ERROR: "));

                }
                if (c & USART_FRAME_ERROR) {
                    usart_send_string_P(&usart0, PSTR("USART FRAME ERROR: "));
                }
                if (c & USART_BUFFER_OVERFLOW) {
                    usart_send_string_P(&usart0, PSTR("USART BUFFER OVERFLOW ERROR: "));
                }

                // (7) - Send single character to USART
                usart_send_char(&usart0, (char)c);
            }
        }

        // (8) - Check that everything is printed before closing USART
        fprintf_P(&usart0_stream, PSTR("\r\n\r\n<-<->->"));

        // (9) - Close USART0
        usart_close(&usart0);    

        // (10) - Clear global interrupts
        cli();

    }
}