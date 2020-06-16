/*******************************************************************************
 *  ======== httpget.c ========
 *
 *  HTTP Client GET application
 ********************************************************************************/

/* BSD support */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ti/display/Display.h>
#include <ti/net/http/httpclient.h>
#include "semaphore.h"

char REQUEST_URI[] = "/lab6/?Action=Save&Student_ID=801075436&Tempr=";
#define HOSTNAME              "10.0.0.31"
#define REQUEST_URI           "/lab6/?Action=Save&Student_ID=801075436&Tempr=temperatureChar[0]"
#define USER_AGENT            "HTTPClient (ARM; TI-RTOS)"
#define HTTP_MIN_RECV         (256)

extern Display_Handle display;
extern sem_t ipEventSyncObj;
extern void printError(char *errString,int code);
extern char* temperatureChar[1];

/*********************************************************************************
 *  Function: HTTP task
 *  Makes a HTTP GET request
 *  param: pvParameters
 *  return none
 *********************************************************************************/
void* httpTask(void* pvParameters)
{
    bool moreDataFlag = false;
    char data[HTTP_MIN_RECV];
    int16_t ret = 0;
    int16_t len = 0;

    Display_printf(display, 0, 0, "Sending a HTTP GET request to '%s'\n",
                   HOSTNAME);

    HTTPClient_Handle httpClientHandle;
    int16_t statusCode;
    httpClientHandle = HTTPClient_create(&statusCode,0);
    if(statusCode < 0)
    {
        printError("httpTask: creation of http client handle failed",
                   statusCode);
    }

    ret =
        HTTPClient_setHeader(httpClientHandle,
                             HTTPClient_HFIELD_REQ_USER_AGENT,
                             USER_AGENT,strlen(USER_AGENT),
                             HTTPClient_HFIELD_PERSISTENT);
    if(ret < 0)
    {
        printError("httpTask: setting request header failed", ret);
    }

    ret = HTTPClient_connect(httpClientHandle,HOSTNAME,0,0);
    if(ret < 0)
    {
        printError("httpTask: connect failed", ret);
    }
    ret =
        HTTPClient_sendRequest(httpClientHandle,HTTP_METHOD_GET,REQUEST_URI,
                               NULL,0,
                               0);
    if(ret < 0)
    {
        printError("httpTask: send failed", ret);
    }

    if(ret != HTTP_SC_OK)
    {
        printError("httpTask: cannot get status", ret);
    }

    Display_printf(display, 0, 0, "HTTP Response Status Code: %d\n", ret);

    len = 0;
    do
    {
        ret = HTTPClient_readResponseBody(httpClientHandle, data, sizeof(data),
                                          &moreDataFlag);
        if(ret < 0)
        {
            printError("httpTask: response body processing failed", ret);
        }
        Display_printf(display, 0, 0, "%.*s \r\n",ret,data);
        len += ret;
    }
    while(moreDataFlag);

    Display_printf(display, 0, 0, "Received %d bytes of payload\n", len);

    ret = HTTPClient_disconnect(httpClientHandle);
    if(ret < 0)
    {
        printError("httpTask: disconnect failed", ret);
    }

    HTTPClient_destroy(httpClientHandle);
    return(0);
}


