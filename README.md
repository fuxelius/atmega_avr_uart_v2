# Interrupt driven and buffered UART for tiny- and megaAVR
**``By Hans-Henrik Fuxelius, 2023-05-12``

This UART library is loosely based on a *Technical Brief* [[TB3216](https://ww1.microchip.com/downloads/en/Appnotes/TB3216-Getting-Started-with-USART-DS90003216.pdf)] from **Microchip** 
that I have tried to adhere to in function and naming conventions. The library supports up to 6 cuncurrent UART and they can be enabled in any order and number as long as it is supported by the microcontroller. Each UART has its own circular buffer and code, so they work fully independent of each other.

The size of the **main.c** project compiled, is **~900 bytes** with one UART and without usage of the `fprintf` function. It is **~2700 bytes** with the library for `fprintf` linked in. The code is more or less self-explanatory. At  places I have tried to comment it sparingly.

**Note 1**: In the text I have used [UART](https://en.wikipedia.org/wiki/Universal_asynchronous_receiver-transmitter) for the library implementation and [USART](https://en.wikipedia.org/wiki/Universal_synchronous_and_asynchronous_receiver-transmitter) for the actual device of the microcontroller which is capable of both.

**Note 2**: The compilation step (separate compilation and linking) is done on a UNIX machine (**Apple Mac**) but should be easy to compile in **Microchip Studio** as well.

## Intended devices

**megaAVR 0-series**: ATmega808, ATmega809, ATmega1608, ATmega1609, ATmega3208, ATmega3209, ATmega4808 and ATmega4809.

**tinyAVR 0-series**: ATtiny202, ATtiny402, ATtiny204, ATtiny404, ATtiny804, ATtiny1604, ATtiny406, ATtiny806, ATtiny1606, ATtiny807 and ATtiny1607

**tinyAVR 1-series**: ATtiny212, ATtiny412, ATtiny214, ATtiny414, ATtiny814, ATtiny1614, ATtiny416, ATtiny816, ATtiny1616, ATtiny3216, ATtiny417, ATtiny817, ATtiny1617 and ATtiny3217 

**AVR DA devices**: AVR32DA28, AVR64DA28, AVR128DA28, AVR32DA32, AVR64DA32, AVR128DA32, AVR32DA48, AVR64DA48, AVR128DA48, AVR64DA64, AVR128DA64

This library was initially developed for a [bare metal atmega4808](https://github.com/fuxelius/atmega4808_bare_metal) project. If you use the ATmega4808 and USART0 it will compile out of the box, otherwise some settings must be done before it works. The setup process of the library is direct and will be outlined below.

<img src="doc/pic/DevBoard.png"  width="400">

> A development board designed for low power battery operation

**P.S.** As a convenience for rapid test and evaluation, I have also [applied the uart library ](https://github.com/fuxelius/atmega_avr_uart_nano_every) to the [Arduino Nano Every](https://docs.arduino.cc/hardware/nano-every) with an ATmega4809 microcontroller. The setup for how to compile is detailed in [C Programming for 'Arduino Nano Every' Board (ATmega4809) with a Mac and VS Code](https://github.com/fuxelius/nano_every_bare_metal#c-programming-for-arduino-nano-every-board-atmega4809-with-a-mac-and-vs-code)

<img src="doc/pic/closeup.png"  width="400">

> The standard Arduino Nano Every development board

## Setting up parameters
All setting for the library is done in `uart_settings.h` and `uart_settings.c` 

### RBUFFER_SIZE
	// DEFINE RING BUFFER SIZE; must be 2, 4, 8, 16, 32, 64 or 128  
	#define RBUFFER_SIZE 32
	
> The default value is 32
	
`RBUFFER_SIZE` defines the size of the ringbuffers for Rx and Tx and even out the data flow through these units over time. It also mediates the interrupt driven design. The buffer size is symmetric and equal for both transmit (Tx) and receive (Rx). It has a typical size of 32 or 64, but can be set to any size in its range from 2, 4, 8, 16, 32, 64 or 128. 

### Enabling USARTn

	// ENABLE USART UNITS
	#define USART0_ENABLE
	// #define USART1_ENABLE
	// #define USART2_ENABLE
	// #define USART3_ENABLE
	// #define USART4_ENABLE
	// #define USART5_ENABLE
	
> Enable USARTn by uncommenting it, here USART0 is enabled

Depending on microcontroller, the library supports up to 6 concurrent USART units. As previously mentioned, they can be enabled in any order and number as long as it is supported by the microcontroller. Each USART has its own circular buffer and code, so they work fully independent of each other.
	

### Assign PORTMUX & Rx, Tx Pinout

The Port Multiplexer (PORTMUX) can either enable or disable the functionality of the pins, or change between default and alternative pin positions. Available options are described in detail in the PORTMUX register map and depend on the actual pin and its properties. Select which ever is appropriate for your selection of USARTn and pin selection. [ATmega 4809 Datasheet ss. 139]

	#ifdef USART0_ENABLE
	void usart0_port_init(void) {
	    asm("NOP");                         // PORTMUX
	    PORTA.DIR &= ~PIN1_bm;			    // Rx
	    PORTA.DIR |= PIN0_bm;			    // Tx
	}
	#endif

> Above the default pins are used, no need to change PORTMUX. Only to enable Rx for input and Tx for output.

	#ifdef USART3_ENABLE
	void usart3_port_init(void) {
	    PORTMUX.USARTROUTEA = 0b01111111;   // Set PB04, PB05
	    PORTB.DIR &= ~PIN5_bm;              // Rx
	    PORTB.DIR |= PIN4_bm;               // Tx
	}
	#endif

> Above example is port multiplexing for pin PB04 and PB05 for USART3 as given in the USART library given for Arduino Nano Every. [ATmega 4809 Datasheet ss. 143]


## UART functions

The number of functions is comprehensive and easy to use.

	extern FILE USARTn_stream;
	
	void usartN_init(uint16_t baud_rate);
	void usartN_send_char(char c);
	void usartN_send_string(char* str, uint8_t len);
	uint16_t usartN_read_char(void);
	void usartN_close(void);

> N and n above denotes the USART in use (0 to 5)

### File Stream
The file stream `FILE USARTn_stream;` is used for printing formatted strings with `fprintf` to each USART in use

### init
Each unit must be initialized before it can operate correctly.

### send_char
Sends a single character to an USART

### send_string
Sends a complete string to USART

### read_char
Polling with read_char is used for reading input from an USART

### close
To be able to close a unit in a proper way is essential for proper operation. This makes it possible to initialize and close units as they are needed.

## How to use the library
Here is a short overview of how to use the library. The **order of calling** `init()`, `sei()` and `usart0_close()`, `cli()` is crucial for correct operation. A **correct session** looks like below!

    // (1) - Init USART
    usart0_init((uint16_t)BAUD_RATE(9600));

    // (2) - Enable global interrupts
    sei(); 

    // (3) - Send string to USART
    usart0_send_string("\r\n\r\nPEACE BRO!\r\n\r\n", 18);

    // (4) - Use fprintf to write to stream
    fprintf(&USART0_stream, "Hello world!\r\n");

    for(size_t i=0; i<5; i++) {
        // (5) - Use formatted fprintf to write to stream
        fprintf(&USART0_stream, "\r\nCounter value is: 0x%02X ", j++);
        _delay_ms(500);

        // (6) - Get USART input by polling ringbuffer
        while(!((c = usart0_read_char()) & USART_NO_DATA)) {

            if (c & USART_PARITY_ERROR) {
                fprintf(&USART0_stream, "USART PARITY ERROR: ");
            }
            if (c & USART_FRAME_ERROR) {
                fprintf(&USART0_stream, "USART FRAME ERROR: ");
            }
            if (c & USART_BUFFER_OVERFLOW) {
                fprintf(&USART0_stream, "USART BUFFER OVERFLOW ERROR: ");
            }

            // (7) - Send single character to USART
            usart0_send_char((char)c);
        }
    }

    // (8) - Check that everything is printed before closing USART
    fprintf(&USART0_stream, "\r\n\r\n<-<->->");

    // (9) - Close USART0
    usart0_close();    

    // (10) - Clear global interrupts
    cli();


### (1) - Init UART
The library must be initialized **before** enabling global interrupts in step 2.

### (2) - Enable global interrupts
Once global interrupts are enabled the library starts to work, **not** before!

### (3) - Send string to UART
`usart0_send_string(str, len)` is the plain function for printing to USART, it has no formatting but also has a smaller library footprint than using `fprintf()`

### (4) - Use fprintf to write to UART
The created filestream `&USART0_stream` lets you write directly with `fprintf(FILE *stream, const char          *format)`

### (5) - Use formatted fprintf to write to UART
An example of *formatted printing* with `fprintf()`

### (6) - Get UART input by polling ringbuffer
All USART input and output is mediated by ringbuffers, and input from the units is done by active **polling** of the input buffer with `usart0_read_char()`. What you type on keyboard are printed to USART0!

### (7) - Send single character to UART
`usart0_send_char()` is the plain function for sending a single character to USART.

### (8) - Check that everything is printed before closing UART
This string is just a test to see that it is completely and correctly written to USART before unit is closed.

### (9) - Close UART0
Is is important to properly being able to open and close USART devices without loosing any information. Here it loops over and over again for testing!

### (10) - Clear global interrupts
`cli()` **must** be called after `usart0_close()`