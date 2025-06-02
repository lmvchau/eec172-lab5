//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - LAB4 TASK 3
// Luuanne Chau and Kavya Khare
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup uart_demo
//! @{
//
//*****************************************************************************

// Standard includes
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>  // for va_list, va_start, etc.


// Driverlib includes
#include "rom.h"
#include "rom_map.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_ints.h"
#include "hw_nvic.h"
#include "hw_apps_rcm.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "uart.h"
#include "interrupt.h"
#include "pin_mux_config.h"
#include "utils.h"
#include "prcm.h"
#include "gpio.h"
#include "systick.h"
#include "utils.h"
#include "i2c.h"
#include "spi.h"
#include "timer.h"

#include "simplelink.h"

// Common interface include
#include "common.h"
#include "uart_if.h"
#include "aws_http.h"
#include "aws_sync.h"
#include "gpio_if.h"
#include "timer_if.h"
#include "i2c_if.h"

// Includes Adafruit
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "glcdfont.h"

//Include test
#include "oled_test.h"

// Custom includes
#include "utils/network_utils.h"


//*****************************************************************************
//                          MACROS                                  
//*****************************************************************************

// OLED defines
#define SPI_IF_BIT_RATE  100000
#define TR_BUFF_SIZE     100

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define YELLOW  0xFFE0

// the cc3200's fixed clock frequency of 80 MHz
// note the use of ULL to indicate an unsigned long long constant
#define SYSCLKFREQ 80000000ULL

// macro to convert ticks to microseconds
#define TICKS_TO_US(ticks) \
  ((((uint64_t)(ticks) / SYSCLKFREQ) * 1000000ULL) + \
   ((((uint64_t)(ticks) % SYSCLKFREQ) * 1000000ULL) / SYSCLKFREQ))

// macro to convert microseconds to ticks
#define US_TO_TICKS(us) ((SYSCLKFREQ / 1000000ULL) * (us))

// systick reload value set to ~14ms period
// (PERIOD_SEC) * (SYSCLKFREQ) = PERIOD_TICKS
#define SYSTICK_RELOAD_VAL 1120000UL

#define IR_GPIO_PORT    GPIOA0_BASE
#define IR_GPIO_PIN     0x40 //pin 61

#define UartGetChar()        MAP_UARTCharGet(CONSOLE)
#define UartPutChar(c)       MAP_UARTCharPut(CONSOLE,c)
#define MAX_STRING_LENGTH    80

#define RX_BUFF_SIZE     128

#define MAX_TAPS        5

#define UART1_LINE_MAX 128

#define MAX_MSG_LEN 128

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
volatile int g_iCounter = 0;

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

volatile char uart1_rx_buffer[RX_BUFF_SIZE];
volatile int uart1_rx_head = 0;
volatile int uart1_rx_tail = 0;

// track systick count to know if counter wrapped
volatile int systick_count = 0;

volatile int lowercaseNext = 0;


uint64_t incrementer = 0;

//Stopwatch Variables
volatile uint32_t delta = 0;      // tick differences
volatile uint32_t delta_us = 0;   // microseconds difference
volatile uint32_t last_frame_ms = 0; // last timer microsecond value stored
volatile uint32_t last_frame_delay = 0; // microseconds difference of last frame
volatile uint32_t current_time_ms = 0; // current microsecond count

//IR Variables
volatile unsigned char ir_intflag;
volatile unsigned int ir_signal = 0;
volatile unsigned int prev_signal = 0;
volatile uint16_t bit_idx = 0;
volatile uint8_t is_repeat = 0;

volatile unsigned int flag_aws_sync = 0;
int g_aws_sock = -1;

typedef enum {
    IR_WAIT_START,
    IR_READING_BITS
} IrState;
volatile IrState irState = IR_WAIT_START;

volatile int repeatcounter = 0;


char button_chars[11][MAX_TAPS] = {
    {' ', 0, 0, 0, 0},         // for 0
    {'.', ',', '?', '!', 0},  // for 1
    {'A', 'B', 'C', 0, 0},    // for 2
    {'D', 'E', 'F', 0, 0},    // for 3
    {'G', 'H', 'I', 0, 0},    // for 4
    {'J', 'K', 'L', 0, 0},    // for 5
    {'M', 'N', 'O', 0, 0},    // for 6
    {'P', 'Q', 'R', 'S', 0},  // for 7
    {'T', 'U', 'V', 0, 0},    // for 8
    {'W', 'X', 'Y', 'Z', 0},   // for 9
    {':', ')', 0, 0, 0}       // for CH+
};

int max_taps[11] = {1, 4, 3, 3, 3, 3, 3, 4, 3, 4, 2};

char uart1_line[UART1_LINE_MAX];
int uart1_line_len = 0;

//cursor states
int curX  = 0;
int curY  = 64;
int prevX = 0, prevY = 0;
const int CHAR_W = 6, CHAR_H = 8;

const char *ascii_art[] = {
    " _._     _,-'\"\"`-._",
    "(,-.`._,'(       |\\`-/|",
    "    `-.-' \\ )-`( , o o)",
    "          `-    \\`_`\"'-"
};




//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************



//*****************************************************************************
//                      LOCAL DEFINITION                                   
//*****************************************************************************

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}


/**
 * Draws Ascii Art that looks like a cat
 */
void drawAsciiArt(int x, int y) {
    int i;
    for (i = 0; i < sizeof(ascii_art) / sizeof(ascii_art[0]); i++) {
        const char *line = ascii_art[i];
        int col = 0;
        while (*line) {
            drawChar(x + col * 6, y + i * 8, *line++, WHITE, BLACK, 1);
            col++;
        }
    }
}

static void
DisplayBanner(void)
{

    Message("\n\n\n\r");
    Message("\t\t *************************************************\n\r");
    Message("\t\t        LAB 4, TASK 3       \n\r");
    Message("\t\t *************************************************\n\r");
    Message("\n\n\n\r");
}

/**
 * Reset SysTick Counter
 */
static inline void SysTickReset(void) {
    // any write to the ST_CURRENT register clears it
    // after clearing it automatically gets reset without
    // triggering exception logic
    // see reference manual section 3.2.1
    HWREG(NVIC_ST_CURRENT) = 1;

    // clear the global count variable
    systick_count = 0;
}

/**
 * SysTick Interrupt Handler
 *
 * Keep track of whether the systick counter wrapped
 */
static void SysTickHandler(void) {
    // increment every time the systick handler fires
    systick_count++;

}

static void GPIOirHandler(void) {
    unsigned long ulStatus = MAP_GPIOIntStatus(IR_GPIO_PORT, true);
    MAP_GPIOIntClear(IR_GPIO_PORT, ulStatus);

      // If SysTick wrapped, abort this capture and exit handler:
      if (systick_count) {
           // clear the wrap flag and restart timing
          systick_count = 0;
          SysTickReset();
          return;
       }

    // read current down-counter and compute delta in Âµs
    uint32_t now   = SysTickValueGet();
    delta    = SYSTICK_RELOAD_VAL - now;
    delta_us = TICKS_TO_US(delta);

    // Finite State Machine
    switch (irState) {
        // waiting for starting pulse
        case IR_WAIT_START:
            // look for starting pulse ~13ms)
            if (delta_us > 13000) {
                // set bit_idx to 0 and clear the ir_signal
                // we can move to the next state
                bit_idx     = 0;
                ir_signal   = 0;
                irState     = IR_READING_BITS;
            }
            break;

        case IR_READING_BITS:
            if (bit_idx < 32) {
                // keep reading incoming bits
                if (delta_us > 1000 && delta_us < 2000) {
                    // logical â€œ0â€ if pulse width is ~1.12 ms
                    // bit shift left to insert 0
                    ir_signal <<= 1;
                    bit_idx++;
                }
                else if (delta_us > 2000 && delta_us < 3000) {
                    // logical â€œ1â€ if pulse width is ~2.2 ms
                    // bit shift left and "OR" 1 to set LSB to 1
                    ir_signal = (ir_signal << 1) | 1;
                    bit_idx++;
                }
                else {
                    // invalid bit timing -> abort frame
                    irState     = IR_WAIT_START;
                    bit_idx     = 0;
                }
            }
            if (bit_idx == 32) {
                // ir signal ready, get time from A0 timer
                uint32_t now_time = current_time_ms;
                // calculate number of ms passed
                last_frame_delay = now_time - last_frame_ms;
                last_frame_ms = now_time;

                // repeat if within threshold of 1 second = 1000ms
                if (ir_signal == prev_signal && last_frame_delay < 1000) {
                    is_repeat = 1;
                }

                else {
                    is_repeat = 0;
                    // reset prev_signal
                    prev_signal = ir_signal;
                }

                // full frame received -> flag main() to decode
                ir_intflag = 1;
                // reset state back to idle
                irState    = IR_WAIT_START;
            }
            break;
    }

    // reset timer for next edge measurement
    SysTickReset();
}


/**
 * Initializes SysTick Module
 */
static void SysTickInit(void) {

    // configure the reset value for the systick countdown register
    MAP_SysTickPeriodSet(SYSTICK_RELOAD_VAL);

    // register interrupts on the systick module
    MAP_SysTickIntRegister(SysTickHandler);

    // enable interrupts on systick
    // (trigger SysTickHandler when countdown reaches 0)
    MAP_SysTickIntEnable();

    // enable the systick module itself
    MAP_SysTickEnable();
}

// UART1 receive interrupt handler
static void UART1_IntHandler(void)
{
    // 1) Read & clear all pending UART1 interrupt sources
    unsigned long ulStatus = MAP_UARTIntStatus(UARTA1_BASE, true);
    MAP_UARTIntClear(UARTA1_BASE, ulStatus);

    // 2) Drain the RX FIFO, echo each character on UART0
    while (MAP_UARTCharsAvail(UARTA1_BASE))
    {
        char c = MAP_UARTCharGetNonBlocking(UARTA1_BASE);
        int next_head = (uart1_rx_head + 1) % RX_BUFF_SIZE;

        if (next_head != uart1_rx_tail) { // Avoid buffer overrun
            uart1_rx_buffer[uart1_rx_head] = c;
            uart1_rx_head = next_head;
        }
    }
}

static void UART1_Init(void){
    // Baud, 8N1 on UARTA1
    MAP_UARTConfigSetExpClk(UARTA1_BASE,
        MAP_PRCMPeripheralClockGet(PRCM_UARTA1),
        115200,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // Enable FIFOs & pick trigger levels
    MAP_UARTFIFOEnable(UARTA1_BASE);
    MAP_UARTFIFOLevelSet(UARTA1_BASE, UART_FIFO_TX4_8, UART_FIFO_RX4_8);

    // Hook up the interrupt handler
    MAP_UARTIntRegister(UARTA1_BASE, UART1_IntHandler);
    MAP_UARTIntEnable(UARTA1_BASE, UART_INT_RX | UART_INT_RT);
    MAP_IntEnable(INT_UARTA1);

    // Finally turn on UART1
    MAP_UARTEnable(UARTA1_BASE);
}


uint8_t getButton(void){
    switch (ir_signal) {
        // Compare expected signals in hex to received ir_signal
        case 0x02FD00FF: return 0;
        case 0x02FD807F: return 1;
        case 0x02FD40BF: return 2;
        case 0x02FDC03F: return 3;
        case 0x02FD20DF: return 4;
        case 0x02FDA05F: return 5;
        case 0x02FD609F: return 6;
        case 0x02FDE01F: return 7;
        case 0x02FD10EF: return 8;
        case 0x02FD906F: return 9;
        case 0x02FD02FD: return 10;    // LAST
        case 0x02FD08F7: return 11;    // MUTE
        case 0x02FDD827: return 13;    // CHANNEL+
        case 0x02FDF807: return 14;    // CHANNEL-
        default: return 12;  // unknown
    }
}

/**
 * Initializes Timer A0 for millisecond tracking
 */

void
TimerBaseIntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    Timer_IF_InterruptClear(TIMERA0_BASE);

    current_time_ms++;
}

static void TimerA0Init(void) {

    //
    // Configuring the timers
    //
    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC, TIMER_A, 0);

    //
    // Setup the interrupts for the timer timeouts.
    //
    Timer_IF_IntSetup(TIMERA0_BASE, TIMER_A, TimerBaseIntHandler);

    //
    // Turn on the timers feeding values in mSec
    //
    Timer_IF_Start(TIMERA0_BASE, TIMER_A, 1);
}

/**
 * Initializes Timer A0 for millisecond tracking
 */


static void IR_Interrupt_Init(void) {

    unsigned long ulStatus;
    // Register the interrupt handlers
    MAP_GPIOIntRegister(IR_GPIO_PORT, GPIOirHandler); //Register Interrupt handler

    //Configure  GPIO_FALLING_EDGE () interrupts on IR Pin
    MAP_GPIOIntTypeSet(IR_GPIO_PORT, IR_GPIO_PIN, GPIO_FALLING_EDGE);

    ulStatus = MAP_GPIOIntStatus(IR_GPIO_PORT, false);
    MAP_GPIOIntClear(IR_GPIO_PORT, ulStatus);           // clear interrupts on ir

    // clear global variables
    ir_intflag = 0;

    //Enable IR interrupts
    MAP_GPIOIntEnable(IR_GPIO_PORT, IR_GPIO_PIN);
}

//*****************************************************************************
//
//! Main function
//!
//! \param  None
//!
//! \return None
//! 
//*****************************************************************************
void main()
{


    long lRetVal = -1;
    //
    // Initailizing the board
    //
    BoardInit();
    //
    // Muxing for Enabling UART_TX and UART_RX.
    //
    PinMuxConfig();
    //
    // Initialising the Terminal.
    //
    InitTerm();
    //
    // Clearing the Terminal.
    //
    ClearTerm();
    DisplayBanner();

    // Reset the peripheral
    MAP_PRCMPeripheralReset(PRCM_GSPI);

    // Reset SPI
    MAP_SPIReset(GSPI_BASE);

    // Configure SPI interface
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                                 SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                                 (SPI_SW_CTRL_CS |
                                 SPI_4PIN_MODE |
                                 SPI_TURBO_OFF |
                                 SPI_CS_ACTIVEHIGH |
                                 SPI_WL_8));


    //Enable SPI for communication
    MAP_SPIEnable(GSPI_BASE);

//    MAP_PRCMPeripheralClkEnable(PRCM_UARTA1, PRCM_RUN_MODE_CLK);

    //Initialization of the OLED
    Adafruit_Init();
    fillScreen(BLACK);

    //Enable Systick:
    SysTickInit();

    // Initialize UART1 for board-to-board communication
    UART1_Init();

    // Initialize GPIO interrupt for IR signal
    IR_Interrupt_Init();

    // Initialize A0 Timer and interrupt
    TimerA0Init();

    //Reset Timer:
    SysTickReset();

    // initialize global default app configuration
    g_app_config.host = SERVER_NAME;
    g_app_config.port = GOOGLE_DST_PORT;

    //Connect the CC3200 to the local access point
    lRetVal = connectToAccessPoint();
    //Set time so that encryption can be used
    lRetVal = set_time();
    if(lRetVal < 0) {
        UART_PRINT("Unable to set time in the device");
        LOOP_FOREVER();
    }
    Report("Passed connectToAccessPoint() and set_time() :D\n\r");

    // TLS connect
    g_aws_sock = tls_connect();
    if (g_aws_sock < 0){
        ERR_PRINT(g_aws_sock);
        LOOP_FOREVER();
    }
    Report("tls_connect() at the beginning successful! :D\n\r");


    // Init TimerA1 for AWS IoT Shadow Synchronization
    TimerA1_Init();
    Report("INit TImerA1 Successful!:D) \n\r");

    int i;
    static char tx_buffer[MAX_MSG_LEN];
    static int  msg_len = 0;
    static int tap_count = 0;

    while (1) {
            if (ir_intflag) {
                ir_intflag = 0;

                if (bit_idx == 32) {
                    Report("Decoded IR Signal successful!! :D\n\r");
                    // if signal ready, get button and
                    uint32_t button_digit = getButton();

                    if (button_digit <= 9 || button_digit == 13) {
                        // update tap_count if digit button has been pressed repeatedly
                        if (is_repeat) {
                            tap_count++;
                        } else {
                            tap_count = 0;  // reset tap count if new key has been pressed
                        }

                        char c;
                        // pick the character
                        if (button_digit <= 9) {
                            c = button_chars[button_digit]
                                             [ tap_count % max_taps[button_digit] ];
                        } else {
                            c = button_chars[10][ tap_count % max_taps[10] ];  // special case for button_digit = 13
                        }

                        if (lowercaseNext && c >= 'A' && c <= 'Z'){
                            c = c - 'A' + 'a';
                        }

                        Report("Typed: %c\n\r", c);  // for debugging

                        // draw it on the OLED
                        if (is_repeat) {
                            // use previous location
                            drawChar(prevX, prevY, c, BLACK, BLACK, 1);
                            drawChar(prevX, prevY, c, WHITE, BLACK, 1);
                        } else {
                            // use updated location
                            drawChar(curX, curY, c, WHITE, BLACK, 1);
                            prevX = curX;  prevY = curY;
                            curX += CHAR_W;
                            if (curX > 128 - CHAR_W) {
                                curX = 0;  curY += CHAR_H;
                            }
                            if (curY > 127) {
                                fillRect(0, 40, 128, 88, BLACK);
                                curX = 0;  curY = 40;
                            }
                        }
                        // draw vertical line in green for cursor
                        fillRect(curX, curY, 1, CHAR_H, GREEN);


                        // append to tx_buffer
                        if (msg_len < MAX_MSG_LEN - 1) {
                            if (is_repeat && msg_len > 0){
                                msg_len--;

                            }
                            tx_buffer[msg_len++] = c;
                            tx_buffer[msg_len]   = '\0';
                        }
                    }
                    else if (button_digit == 10){
                        // LAST / DELETE
                        if (msg_len > 0){
                            Report("Delete\n\r");

                            // fill green cursor with black to delete
                            fillRect(curX, curY, 1, CHAR_H, BLACK);

                            // go back 1 character's worth (a CHAR_W x CHAR_H block) , and update current position
                            if (curX == 0) {
                                curY -= CHAR_H;
                                curX = 128 - CHAR_W;
                            }
                            else {
                                curX -= CHAR_W;
                            }
                            // fill next CHAR_W x CHAR_H block in BLACK to "delete"
                            fillRect(curX, curY, CHAR_W, CHAR_H, BLACK);

                            // fill new location with green vertical line for updated cursor
                            fillRect(curX, curY, 1, CHAR_H, GREEN);

                            // remove last appended character for tx buffer by replacing with '\0'
                            msg_len--;
                            tx_buffer[msg_len] = '\0';

                            // update previous position
                            prevX = curX;
                            prevY = curY;
                        }

                    }

                    else if (button_digit == 11) {
                        Report("\nSending message: \"%s\"\n\r", tx_buffer);
                        // MUTE pressed -> send the composed string over UART1
                        for (i = 0; i < msg_len; i++) {
                            MAP_UARTCharPut(UARTA1_BASE, tx_buffer[i]);
                        }
                        // optional line ending
//                        MAP_UARTCharPut(UARTA1_BASE, '\r');
                        MAP_UARTCharPut(UARTA1_BASE, '\n');

                        Report("Message sent.\n\r\n\r");

//                        int sock = tls_connect();
//                        if (sock >= 0){
//                            http_post(sock, tx_buffer);
//                            http_get(sock);
//                            sl_Close(sock);
//                        } else {
//                            ERR_PRINT(sock);
//                        }

                        int success = UpdateAWS_Shadow(tx_buffer);
                        if (success < 0){
                            Report("UpdateAWS_SHadow not successful\n\r");
                        }

                        unsigned int clear_height = curY - 64 + 8;
                        if (clear_height > 0){
                            fillRect(0, 64, 128, clear_height, BLACK);
                        }
                        curX = 0;
                        curY = 64;

                        // clear for next message
                        msg_len = 0;
                        tx_buffer[0] = '\0';


                    }
                    // CH- button pressed
                    else if (button_digit == 14){
                        // toggle globally declared flag
                        if (lowercaseNext == 0){
                            lowercaseNext = 1;
                        } else if (lowercaseNext == 1) {
                            lowercaseNext = 0;
                        }
                        // reset ir_intflag and bit_idx to get ready for next button press
                        ir_intflag = 0;
                        bit_idx = 0;
                    }

                    // reset for next IR frame
                    bit_idx = 0;
                }
            }

            // 2. Handle UART1 receive
            while (uart1_rx_tail != uart1_rx_head) {
                char c = uart1_rx_buffer[uart1_rx_tail];
                uart1_rx_tail = (uart1_rx_tail + 1) % RX_BUFF_SIZE;

                // If newline received or line is full, print the line
                    if (c == '\n'|| c == '\r' || uart1_line_len >= UART1_LINE_MAX - 1) {

                        while (uart1_line_len > 0 &&
                                   (uart1_line[uart1_line_len - 1] == '\r' ||
                                    uart1_line[uart1_line_len - 1] == '\n')) {
                                uart1_line_len--;
                            }

                        uart1_line[uart1_line_len] = '\0';  // null-terminate

                        // Print to UART0 console
                        Report("Received: %s\r\n", uart1_line);

                        fillRect(0, 0, 128, 64, BLACK);

                        if (strstr(uart1_line, ":)")){
                            Report("YOU'VE DISCOVERED AN ELUSIVE, RARE CAT!\r\n");
//                            Report("_._     _,-'""`-._\r\n");
//                            Report("(,-.\\._,'(       /\\`-/|\r\n");
//                            Report("    `-.-' \\ )-`( , o o)\r\n");
//                            Report("          `-    \\`_`'''-\r\n");
                            drawAsciiArt(0, 0);
                        }

                        else {
                            char receivedMsg[140];
                            snprintf(receivedMsg, sizeof(receivedMsg), "Received: %s", uart1_line);


                            // Print to OLED (you can choose y position, here y=0 for top)
                            int rxX = 0, rxY = 8;
                            char *ptr = receivedMsg;
                            while (*ptr) {
                                drawChar(rxX, rxY, *ptr++, WHITE, BLACK, 1);
                                rxX += 6;
                                if (rxX > 128 - 6) {
                                    rxX = 0;
                                    rxY += 8;
                                }
                            }
                        }


                        uart1_line_len = 0;
                    }
                    else {
                        uart1_line[uart1_line_len++] = c;
                    }
            }


            if (flag_aws_sync){
                // Clear flag to 0
                flag_aws_sync = 0;

                const char *test_message = "I eat cake";

                if (UpdateAWS_Shadow(test_message) < 0 ){
                    Report("Periodic sync failed: %d\n\r", g_aws_sock);
                    sl_Close(g_aws_sock);
//                    g_aws_sock = tls_connect();
//                    if (g_aws_sock >= 0) {
//                        UpdateAWS_Shadow(test_message);
                } else {
                    UART_PRINT("Periodic sync success!");
                }


            }
        }







}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

    

