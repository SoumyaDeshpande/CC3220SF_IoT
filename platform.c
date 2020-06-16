/*****************************************************************************
 *  ======== platform.c ========
 *
 *  Basic initilizations are done
/*****************************************************************************/

// Include standard libraries
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

/* Include Driver Header files */
#include <ti/drivers/I2C.h>
#include <ti/display/Display.h>

/* Include Board Header files */
#include "Board.h"

#define TMP006_ADDR         0x41
#define TMP006_DIE_TEMP     0x0001  /* Die Temp Result Register */

#ifndef Board_TMP_ADDR
#define Board_TMP_ADDR       TMP006_ADDR
#endif

static Display_Handle display;

//Include TI libraries
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/slnetifwifi.h>
#include <ti/display/Display.h>
#include <ti/drivers/SPI.h>

#include "Board.h"
#include "pthread.h"
#include "semaphore.h"

#define APPLICATION_NAME                      "HTTP GET"
#define DEVICE_ERROR                          ("Device error, please refer \"DEVICE ERRORS CODES\" section in errors.h")
#define WLAN_ERROR                            ("WLAN error, please refer \"WLAN ERRORS CODES\" section in errors.h")
#define SL_STOP_TIMEOUT                       (200)
#define SPAWN_TASK_PRIORITY                   (9)
#define SPAWN_STACK_SIZE                      (4096)
#define TASK_STACK_SIZE                       (2048)
#define SLNET_IF_WIFI_PRIO                    (5)
#define SLNET_IF_WIFI_NAME                    "CC32xx"
/* Wi-Fi SSID */
#define SSID_NAME                             "Embedded_Lab_EXT"

/* Security type could be SL_WLAN_SEC_TYPE_WPA_WPA2 */
#define SECURITY_TYPE                         SL_WLAN_SEC_TYPE_WPA_WPA2

/* Password of the secured AP */
#define SECURITY_KEY                          "embedded"

pthread_t httpThread = (pthread_t)NULL;
pthread_t spawn_thread = (pthread_t)NULL;

int32_t mode;
Display_Handle display;

extern void* httpTask(void* pvParameters);
extern char REQUEST_URI[];

/*****************************************************************************
* Function: printError
* param: code, *errString
* return none
/*****************************************************************************
void printError(char *errString,
                int code)
{
    Display_printf(display, 0, 0, "Error! code = %d, Description = %s\n", code,
                   errString);
    while(1)
    {
        ;
    }
}

/*****************************************************************************
*    Function: SimpleLinkNetAppEventHandler
*
*    This handler is called whenever a Netapp event is reported
*    by the host driver / NWP. This handler is used by 'network_terminal'
*    application to show case the following scenarios:
*
*    1. Handling IPv4 / IPv6 IP address acquisition.
*    2. Handling IPv4 / IPv6 IP address Dropping.
*
*    param          pNetAppEvent     -   pointer to Netapp event data.
*    return         void
******************************************************************************/
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    int32_t             status = 0;
    pthread_attr_t      pAttrs;
    struct sched_param  priParam;

    if(pNetAppEvent == NULL)
    {
        return;
    }

    switch(pNetAppEvent->Id)
    {
    case SL_NETAPP_EVENT_IPV4_ACQUIRED:
    case SL_NETAPP_EVENT_IPV6_ACQUIRED:
        /* Initialize SlNetSock layer with CC3x20 interface                   */
        SlNetIf_init(0);
        SlNetIf_add(SLNETIF_ID_1, SLNET_IF_WIFI_NAME,
                   (const SlNetIf_Config_t *)&SlNetIfConfigWifi,
                    SLNET_IF_WIFI_PRIO);

        SlNetSock_init(0);
        SlNetUtil_init(0);
        if(mode != ROLE_AP)
        {
            Display_printf(display, 0, 0,"[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
                        "Gateway=%d.%d.%d.%d\n\r",
                        SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Ip,3),
                        SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Ip,2),
                        SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Ip,1),
                        SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Ip,0),
                        SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Gateway,3),
                        SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Gateway,2),
                        SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Gateway,1),
                        SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Gateway,0));

            pthread_attr_init(&pAttrs);
            priParam.sched_priority = 1;
            status = pthread_attr_setschedparam(&pAttrs, &priParam);
            status |= pthread_attr_setstacksize(&pAttrs, TASK_STACK_SIZE);

            status = pthread_create(&httpThread, &pAttrs, httpTask, NULL);
            if(status)
            {
                printError("Task create failed", status);
            }
        }
        break;
    default:
        break;
    }
}

/******************************************************************************
*  Function: SimpleLinkFatalErrorEventHandler
*
*    This function handler gets called whenever a socket event is reported
*    by the NWP / Host driver. After this routine is called, the user's
*    application must restart the device in order to recover.
*
*    param          slFatalErrorEvent    -   pointer to fatal error event.
*    return         void
******************************************************************************/
void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *slFatalErrorEvent)
{
    /* Unused in this application */
}

/******************************************************************************
*    Function        SimpleLinkNetAppRequestMemFreeEventHandler
*
*   This handler gets called whenever the NWP is done handling with
*   the buffer used in a NetApp request. This allows the use of
*   dynamic memory with these requests.
*
*    \param         buffer
*    \return        void
******************************************************************************/
void SimpleLinkNetAppRequestMemFreeEventHandler(uint8_t *buffer)
{

}

/******************************************************************************
*    Function:         SimpleLinkNetAppRequestEventHandler
*
*    This functiuon handler gets called whenever a NetApp event is reported
*    by the NWP / Host driver.
*
*    \param         pNetAppRequest     -   Pointer to NetApp request structure.
*                   pNetAppResponse    -   Pointer to NetApp request Response.
*    \return        void
*
******************************************************************************/
void SimpleLinkNetAppRequestEventHandler(SlNetAppRequest_t *pNetAppRequest,
                                         SlNetAppResponse_t *pNetAppResponse)
{

}

/******************************************************************************
*    Function:    SimpleLinkHttpServerEventHandler
*
*    This function handler gets called whenever a HTTP event is reported
*    by the NWP internal HTTP server.
*
*    \param          pHttpEvent       -   pointer to http event data.
*                    pHttpEvent       -   pointer to http response.
*    \return         void
******************************************************************************/
void SimpleLinkHttpServerEventHandler(
    SlNetAppHttpServerEvent_t *pHttpEvent,
    SlNetAppHttpServerResponse_t *
    pHttpResponse)
{

}

/******************************************************************************
*    Function:          SimpleLinkWlanEventHandler
*
*    This handler gets called whenever a WLAN event is reported
*    by the host driver / NWP. This handler is used by 'network_terminal'
*    application to show the scenarios as below:
*
*    1. Handling connection / Disconnection.
*    2. Handling Addition of station / removal.
*    3. RX filter match handler.
*    4. P2P connection establishment.
*    \param          pWlanEvent       -   pointer to Wlan event data.
*    \return         void
*    \sa             cmdWlanConnectCallback, cmdEnableFilterCallback,
*                    cmdWlanDisconnectCallback,cmdP2PModecallback.
******************************************************************************/
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{

}

/*******************************************************************************
*   Function:         SimpleLinkGeneralEventHandler
*   This function handler gets called whenever a general error is reported
*   by the NWP / Host driver. Since these errors are not fatal,
*   application can handle them.
*
*    \param          pDevEvent    -   pointer to device error event.
*    \return         void
*******************************************************************************//
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{

}

/*******************************************************************************/
*   Function:          SimpleLinkSockEventHandler
*
*    This function handler gets called whenever a socket event is reported
*    by the NWP / Host driver.
*
*    \param          SlSockEvent_t    -   pointer to socket event data.
*    \return         void
******************************************************************************/*/
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    /* Unused in this application */
}

/*******************************************************************************
* Function : Connect
* param none
* return none
******************************************************************************/

void Connect(void)
{
    SlWlanSecParams_t secParams = {0};
    int16_t ret = 0;
    secParams.Key = (signed char*)SECURITY_KEY;
    secParams.KeyLen = strlen(SECURITY_KEY);
    secParams.Type = SECURITY_TYPE;
    Display_printf(display, 0, 0, "Connecting to : %s.\r\n",SSID_NAME);
    ret = sl_WlanConnect((signed char*)SSID_NAME, strlen(
                             SSID_NAME), 0, &secParams, 0);
    if(ret)
    {
        printError("Connection failed", ret);
    }
}

/*******************************************************************************
* Function         Display application banner
*    This routine shows application startup display on UART.
*    \param         appName   points to a string representing application name.
******************************************************************************/
static void DisplayBanner(char * AppName)
{
    Display_printf(display, 0, 0, "\n\n\n\r");
    Display_printf(display, 0, 0,
                   "\t\t *************************"
                   "************************\n\r");
    Display_printf(display, 0, 0, "\t\t            %s Application       \n\r",
                   AppName);
    Display_printf(display, 0, 0,
                   "\t\t **************************"
                   "***********************\n\r");
    Display_printf(display, 0, 0, "\n\n\n\r");
}


/*******************************************************************************
* Function         mainThread
* \param           pvParameters parameter
******************************************************************************/

void mainThread(void *pvParameters)
{
        unsigned int    i;
        int         temperature;
        uint8_t         txBuffer[1];
        uint8_t         rxBuffer[2];
        I2C_Handle      i2c;
        I2C_Params      i2cParams;
        I2C_Transaction i2cTransaction;
        char* temperatureChar[1];

        /* Call driver init functions */
        Display_init();
        I2C_init();

        /* Open the HOST display for output */
        display = Display_open(Display_Type_UART, NULL);
        if (display == NULL) {
            while (1);
        }


        /* Create I2C for usage */
        I2C_Params_init(&i2cParams);
        i2cParams.bitRate = I2C_400kHz; //I2C communication speed
        i2c = I2C_open(Board_I2C_TMP, &i2cParams);
        if (i2c == NULL) {
            Display_printf(display, 0, 0, "Error Initializing I2C\n");
            while (1);
        }
        else {
            Display_printf(display, 0, 0, "I2C Initialized!\n");
        }

        /* Point to the T ambient register and read its 2 bytes */
        txBuffer[0] = TMP006_DIE_TEMP;
        i2cTransaction.slaveAddress = Board_TMP_ADDR;
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 1;
        i2cTransaction.readBuf = rxBuffer;
        i2cTransaction.readCount = 2;

       if (I2C_transfer(i2c, &i2cTransaction)) {
                /* Extract degrees C from the received data; see TMP102 datasheet */
                temperature = (rxBuffer[0] << 6) | (rxBuffer[1] >> 2);

                /*
                 * If the MSB is set '1', then we have a 2's complement
                 * negative value which needs to be sign extended
                 */
                if (rxBuffer[0] & 0x80) {
                    temperature |= 0xF000;
                }
               /*
                * For simplicity, divide the temperature value by 32 to get rid of
                * the decimal precision; see TI's TMP006 datasheet
                */
                temperature /= 32;

                Display_printf(display, 0, 0, " %d (C)\n",temperature);
            }
            else {
                Display_printf(display, 0, 0, "I2C Bus fault\n");
            }

            /* Sleep for 1 second */
            sleep(1);

        /* Deinitialized I2C */
        I2C_close(i2c);
        Display_printf(display, 0, 0, "I2C closed!\n");

        sprintf(temperatureChar,"%d",temperature);
        strcat(REQUEST_URI,temperatureChar);

    int32_t status = 0;
    pthread_attr_t pAttrs_spawn;
    struct sched_param priParam;

    Display_printf(display, 0, 0, "platform init\n");

   //Init call for SPI
    SPI_init();
    Display_init();
    display = Display_open(Display_Type_UART, NULL);


    /* Print Application name */
    DisplayBanner(APPLICATION_NAME);

    /* Start the SimpleLink Host */
    pthread_attr_init(&pAttrs_spawn);
    priParam.sched_priority = SPAWN_TASK_PRIORITY;
    status = pthread_attr_setschedparam(&pAttrs_spawn, &priParam);
    status |= pthread_attr_setstacksize(&pAttrs_spawn, SPAWN_STACK_SIZE);

    status = pthread_create(&spawn_thread, &pAttrs_spawn, sl_Task, NULL);
    if(status)
    {
        printError("Task create failed", status);
    }

    /* Turn NWP on - initialize the device*/
    mode = sl_Start(0, 0, 0);
    if (mode < 0)
    {
        Display_printf(display, 0, 0,"\n\r[line:%d, error code:%d] %s\n\r", __LINE__, mode, DEVICE_ERROR);
    }

    if(mode != ROLE_STA)
    {
        /* Set NWP role as STA */
        mode = sl_WlanSetMode(ROLE_STA);
        if (mode < 0)
        {
            Display_printf(display, 0, 0,"\n\r[line:%d, error code:%d] %s\n\r", __LINE__, mode, WLAN_ERROR);
        }

        /* For changes to take affect, we restart the NWP */
        status = sl_Stop(SL_STOP_TIMEOUT);
        if (status < 0)
        {
            Display_printf(display, 0, 0,"\n\r[line:%d, error code:%d] %s\n\r", __LINE__, status, DEVICE_ERROR);
        }

        mode = sl_Start(0, 0, 0);
        if (mode < 0)
        {
            Display_printf(display, 0, 0,"\n\r[line:%d, error code:%d] %s\n\r", __LINE__, mode, DEVICE_ERROR);
        }
    }

    if(mode != ROLE_STA)
    {
        printError("Failed to configure device to it's default state", mode);
    }
    Connect();
}
