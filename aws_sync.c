/*
 * aws_sync.c
 *
 *  Created on: May 28, 2025
 *      Author: luuanne
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// Simplelink includes
#include "simplelink.h"
#include "hw_types.h"

#include "pet_state.h"
#include "aws_http.h"
#include "aws_sync.h"
#include "prcm.h"
#include "timer.h"
#include "timer_if.h"
#include "hw_memmap.h"
#include "uart.h"

// Custom includes
#include "utils/network_utils.h"
#include "common.h"

extern int tls_connect(void);
extern volatile unsigned int flag_aws_sync;
extern int g_aws_sock;
extern volatile uint32_t current_time_ms;
extern PetState myPet;

static volatile uint8_t thirty_second_ticks = 0;


//int UpdateAWS_Shadow(const char *message)
//{
//
////    // Open a fresh TLS socket
////    int sock = tls_connect();
////    if (sock < 0){
////        ERR_PRINT(sock);
////        return -1
////    }
////
////    if (http_post(sock, message) < 0 ){
////        ERR_PRINT(ret);
////        sl_Close(sock);
////        return -1
////    }
//
//    if (g_aws_sock < 0){
//        ERR_PRINT(g_aws_sock);
//        Report("boo hoo there is an error with g_aws_sock!!!! :C");
//        return g_aws_sock;
//    }
//
//
//    if (http_post(g_aws_sock, message) < 0){
////        sl_Close(g_aws_sock);
//        return -1;
//    }
//
////    sl_Close(g_aws_sock);
//    return 0;
//}


int UpdateAWS_Shadow(const PetState *pet)
{

//    // Open a fresh TLS socket
//    int sock = tls_connect();
//    if (sock < 0){
//        ERR_PRINT(sock);
//        return -1
//    }
//
//    if (http_post(sock, message) < 0 ){
//        ERR_PRINT(ret);
//        sl_Close(sock);
//        return -1
//    }

    if (g_aws_sock < 0){
        ERR_PRINT(g_aws_sock);
        Report("boo hoo there is an error with g_aws_sock!!!! :C");
        return g_aws_sock;
    }


    if (http_post(g_aws_sock, pet) < 0){
//        sl_Close(g_aws_sock);
        return -1;
    }

//    sl_Close(g_aws_sock);
    return 0;
}



/**
 * Initializes Timer A0 for millisecond tracking
 */

void
SyncAWSIntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    Timer_IF_InterruptClear(TIMERA1_BASE);

    thirty_second_ticks++;
    if (thirty_second_ticks >= 2) {
        thirty_second_ticks = 0;
        flag_aws_sync = 1;
    }

}

void TimerA1_Init(void) {

    //
    // Configuring the timers
    //
    Timer_IF_Init(PRCM_TIMERA1, TIMERA1_BASE, TIMER_CFG_PERIODIC, TIMER_A, 0);

    //
    // Setup the interrupts for the timer timeouts.
    //
    Timer_IF_IntSetup(TIMERA1_BASE, TIMER_A, SyncAWSIntHandler);

    //
    // Turn on the timers feeding values in 120000 mSec = 2 min
    //
    Timer_IF_Start(TIMERA1_BASE, TIMER_A, 30000);
}



