/*
 * aws_http.h
 *
 *  Created on: May 20, 2025
 *      Author: luuanne
 */

#ifndef AWS_HTTP_H_
#define AWS_HTTP_H_

//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                19    /* Current Date */
#define MONTH               5     /* Month 1-12 */
#define YEAR                2025  /* Current year */
#define HOUR                20    /* Time - hours */
#define MINUTE              30    /* Time - minutes */
#define SECOND              0     /* Time - seconds */


#define APPLICATION_NAME      "SSL"
#define APPLICATION_VERSION   "SQ24"
#define SERVER_NAME           "a3snm4u2mv93o9-ats.iot.us-east-2.amazonaws.com" // CHANGE ME
#define GOOGLE_DST_PORT       8443


#define POSTHEADER "POST /things/kavanneCC3200Board/shadow HTTP/1.1\r\n"             // CHANGE ME
#define HOSTHEADER "Host: a3snm4u2mv93o9-ats.iot.us-east-2.amazonaws.com\r\n"  // CHANGE ME
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"
#define GETHEADER "GET /things/kavanneCC3200Board/shadow HTTP/1.1\r\n"

#define DATA1 "{" \
            "\"state\": {\r\n"                                              \
                "\"desired\" : {\r\n"                                       \
                    "\"var\" :\""                                           \
                        "Hello phone, "                                     \
                        "message from CC3200 via AWS IoT!"                  \
                        "\"\r\n"                                            \
                "}"                                                         \
            "}"                                                             \
        "}\r\n\r\n"

int http_post(int iTLSSockID, const char *message);
int http_get(int iTLSSockID);
int set_time(void);


#endif /* AWS_HTTP_H_ */
