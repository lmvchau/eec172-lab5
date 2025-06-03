/*
 * aws_http.c
 *
 *  Created on: May 20, 2025
 *      Author: luuanne
 */

#include <stdio.h>

#include "aws_http.h"
#include "pet_state.h"

// Simplelink includes
#include "simplelink.h"

//Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"

//Common interface includes
#include "pin_mux_config.h"
#include "gpio_if.h"
#include "common.h"
#include "uart_if.h"


// Custom includes
#include "utils/network_utils.h"


//*****************************************************************************
//
//! This function updates the date and time of CC3200.
//!
//! \param None
//!
//! \return
//!     0 for success, negative otherwise
//!
//*****************************************************************************

int set_time(void) {
    long retVal;

    g_time.tm_day = DATE;
    g_time.tm_mon = MONTH;
    g_time.tm_year = YEAR;
    g_time.tm_sec = HOUR;
    g_time.tm_hour = MINUTE;
    g_time.tm_min = SECOND;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                          SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                          sizeof(SlDateTime),(unsigned char *)(&g_time));

    ASSERT_ON_ERROR(retVal);
    return SUCCESS;
}

//*****************************************************************************
//
//! This function creates HTTP POST request with request line, headers, and message body, and then sends to server.
//!
//! \param const char *message
//!
//! \return
//!     0 for success, negative otherwise
//!
//*****************************************************************************

//int http_post(int iTLSSockID, const char *message){
//    char acSendBuff[512];
//    char acRecvbuff[1460];
//    char cCLLength[200];
//    char* pcBufHeaders;
//    int lRetVal = 0;
//
//    char json_body[512];
//    snprintf(json_body, sizeof(json_body),
//        "{"
//            "\"state\": {"
//                "\"desired\" : {"
//                     "\"var\" : \"%s\","
//                     "\"status\": %d,"
//                     "\"name\": \"%s\","
//                     "\"hunger\": %d,"
//                     "\"heart\": %d,"
//                     "\"sleep\": %d"
//                "}"
//            "}"
//        "}\r\n\r\n", message,
//        1,
//        "Boots",
//        80,
//        50,
//        10
//        );
//
//
//    pcBufHeaders = acSendBuff;
//    strcpy(pcBufHeaders, POSTHEADER);
//    pcBufHeaders += strlen(POSTHEADER);
//    strcpy(pcBufHeaders, HOSTHEADER);
//    pcBufHeaders += strlen(HOSTHEADER);
//    strcpy(pcBufHeaders, CHEADER);
//    pcBufHeaders += strlen(CHEADER);
//    strcpy(pcBufHeaders, "\r\n\r\n");
//
//    int dataLength = strlen(json_body);
//
//    strcpy(pcBufHeaders, CTHEADER);
//    pcBufHeaders += strlen(CTHEADER);
//    strcpy(pcBufHeaders, CLHEADER1);
//
//    pcBufHeaders += strlen(CLHEADER1);
//    sprintf(cCLLength, "%d", dataLength);
//
//    strcpy(pcBufHeaders, cCLLength);
//    pcBufHeaders += strlen(cCLLength);
//    strcpy(pcBufHeaders, CLHEADER2);
//    pcBufHeaders += strlen(CLHEADER2);
//
//    strcpy(pcBufHeaders, json_body);
//    pcBufHeaders += strlen(json_body);
//
//    int testDataLength = strlen(pcBufHeaders);
//
//    UART_PRINT(acSendBuff);
//
//
//    //
//    // Send the packet to the server */
//    //
//    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
//    if(lRetVal < 0) {
//        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
//        sl_Close(iTLSSockID);
//        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
//        return lRetVal;
//    }
//    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
//    if(lRetVal < 0) {
//        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
//        //sl_Close(iSSLSockID);
//        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
//           return lRetVal;
//    }
//    else {
//        acRecvbuff[lRetVal+1] = '\0';
//        UART_PRINT(acRecvbuff);
//        UART_PRINT("\n\r\n\r");
//    }
//
//    return 0;
//}



int http_post(int iTLSSockID, const PetState *pet){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    int lRetVal = 0;

    char json_body[512];


    int status_code;
    switch(pet->status){
        case HUNGRY:
            status_code = 1;
            break;
        case NORMAL:
            status_code = 0;
            break;
        case SLEEPY:
            status_code = 2;
            break;
        case LONELY:
            status_code = 3;
            break;
        case SICK:
            status_code = 4;
            break;
        case DEAD:
            status_code = 5;
            break;
        default:
            status_code = 0;
            break;
    }

//    switch(pet->mode){
//        case PET_FEED:
//            status_code = 1;
//            break;
//        case PET_IDLE:
//            status_code = 0;
//            break;
//        case PET_SLEEP:
//            status_code = 2;
//            break;
//        case PET_PLAY:
//            status_code = 3;
//            break;
//        case PET_SICK:
//            status_code = 4;
//            break;
//        case PET_DEAD:
//            status_code = 5;
//            break;
//        default:
//            status_code = 0;
//            break;
//    }



    snprintf(json_body, sizeof(json_body),
        "{"
            "\"state\": {"
                "\"desired\" : {"
                    "\"name\": \"%s\","
                    "\"status\": %d,"
                    "\"hunger\": %d,"
                    "\"heart\": %d,"
                    "\"sleep\": %d"
                "}"
            "}"
        "}\r\n\r\n",
        pet->name,
        status_code,
        pet->hunger,
        pet->happiness,
        pet->sleepiness
    );


    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    int dataLength = strlen(json_body);

    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    sprintf(cCLLength, "%d", dataLength);

    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);

    strcpy(pcBufHeaders, json_body);
    pcBufHeaders += strlen(json_body);

    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT(acSendBuff);


    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    return 0;
}


//*****************************************************************************
//
//! This function creates HTTP GET request with request line, headers, and then sends to server.
//!
//! \param const char *message
//!
//! \return
//!     0 for success, negative otherwise
//!
//*****************************************************************************
int http_get(int iTLSSockID) {
    char acSendBuff[512];
    char acRecvbuff[1460];
    char* pcBufHeaders = acSendBuff;
    int lRetVal = 0;

    strcpy(pcBufHeaders, GETHEADER); // Adjust thing name
    pcBufHeaders += strlen(GETHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    // Print the full GET request
    UART_PRINT("----- HTTP GET REQUEST -----\n\r");
    UART_PRINT(acSendBuff);
    UART_PRINT("----------------------------\n\r");

    // Send GET request
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if (lRetVal < 0) {
        UART_PRINT("GET failed. Error Number: %i\n\r", lRetVal);
        return lRetVal;
    }

    // Receive response
    lRetVal = sl_Recv(iTLSSockID, acRecvbuff, sizeof(acRecvbuff), 0);
    if (lRetVal < 0) {
        UART_PRINT("Receive failed. Error Number: %i\n\r", lRetVal);
        return lRetVal;
    }

    acRecvbuff[lRetVal] = '\0';
    UART_PRINT(acRecvbuff);
    UART_PRINT("\n\r\n\r");

    return 0;
}





