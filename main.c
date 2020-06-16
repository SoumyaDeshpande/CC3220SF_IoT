/*********************************************************************
* LAB 6
*
* Aim: Internet-Of-Things(IOT) and Wi-Fi Version 2.0
*
* Components: CC3220SF
*
* Soumya Deshpande  ID:801075436  email: sdeshp11@uncc.edu
*
* Citation:-
* http://dev.ti.com/tirex/#/?link=Software%2FSimpleLink%20CC32xx%20SDK
* From above link we have used HTTP get application and I2C code to
* read the temperature from temperature sensors.
*********************************************************************/

/********************************************************************
 *                                README.txt
 *
 *    Requirements:
 *    - We shall be able to connect to “Embedded_Lab_Ext” wireless by
 *        using CC3220SF.
 *    - We shall make an I2C transaction to communicate with the
 *        temperature sensor that is connected to the I2C bus.
 *    - We shall be able to communicate to HTTP server after
 *        successfully connecting to the wireless.
 *
 *    Specifications:
 *    -  Connecting to the “Embedded_Lab_Ext” wireless  network using
 *       HTTP GET/POST application.
 *    - To pass the parameter to the server using the HTTP GET
 *        application.
 *    - To read the temperature from the CC3220SF board temperature
 *        sensors.
 *    - After reading the temperature, communicate it with the I2C
 *        bus.
 *    - Send the temperature read from the I2C bus to the web server
 *        using HTTP Get.
 *
 *     Steps followed for HTTP Get application:
 *    - Modified the Network parameters in platform.c as below:
 *         Wi-fi SSID_NAME               "Embedded_Lab_EXT"
 *         Encrypytion SECURITY_TYPE     "SL_WLAN_SEC_TYPE_WPA_WPA2"
 *         Password SECURITY_KEY         "embedded"
 *    - Modified the HTTP parameters in httpget.c as below:
 *         #define HOSTNAME      "10.0.0.31"
 *         #define REQUEST_URI   "/lab6/?Action=Save&Student_ID=801075436&Tempr==temperatureChar[0]"
 *         #define USER_AGENT    "HTTPClient (ARM; TI-RTOS)"
*
*    The lab server is at 10.0.0.31 IP address. Using a HTTP client to
*    we are connected to this IP address and the Get method is used to
*    push the data to the above mentioned URL.
*
*    To read the temperature value from the temperature sensor:
*    - Downloaded the FreeRTOS v10.0.0 and set a new variable
*        “FREERTOS_INSTALL_DIR” in the project properties with its
*         type as Directory.
*    - pthreat_create() function creates a thread named mainThread
*        which will be scheduled by the scheduler.
*    - I2C is initialized in the mainThread.
*    - I2C address is set to communicate with the temperature sensor
*        and transfer of data is started.
*    - Value of temperature is read from the rxBuffer and is
*        converted to degree Celsius.
*********************************************************************/

/*
 *  ======== main_freertos.c code ========
 *
 */
#include <stdint.h>

/* Include POSIX Header files */
#include <pthread.h>

/* Include RTOS header files */
#include "FreeRTOS.h"
#include "task.h"

/* Include TI-RTOS Header files */
#include <ti/drivers/GPIO.h>

/* Include Board Header files */
#include "Board.h"

extern void * mainThread(void *arg0);

/* Defined Stack size in bytes */
#define THREADSTACKSIZE   4096

/*
 *  Function : main
 *  Passed argument: void(none)
 *  pthreat_create() function creates a thread named mainThread
 *  which will be scheduled by the scheduler.
 */
int main(void)
{
    pthread_t thread;
    pthread_attr_t pAttrs;
    struct sched_param priParam;
    int retc;
    int detachState;

    /* Board init function is called */
    Board_initGeneral();

    /* priority and stack size attributes are set */
    pthread_attr_init(&pAttrs);
    priParam.sched_priority = 1;

    detachState = PTHREAD_CREATE_DETACHED;
    retc = pthread_attr_setdetachstate(&pAttrs, detachState);
    if(retc != 0)
    {
        while(1)
        {
            ;
        }
    }

    pthread_attr_setschedparam(&pAttrs, &priParam);

    retc |= pthread_attr_setstacksize(&pAttrs, THREADSTACKSIZE);
    if(retc != 0)
    {
        while(1)
        {
            ;
        }
    }

    retc = pthread_create(&thread, &pAttrs, mainThread, NULL);
    if(retc != 0)
    {
        /* pthread_create() failed */
        while(1)
        {
            ;
        }
    }

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    return (0);
}

/*****************************************************************************
* Function: Application defined malloc failed hook
* vApplicationMallocFailedHook()
* passed parameter: none
* return parameter: none
*****************************************************************************/
void vApplicationMallocFailedHook()
{
    /* Used to handle Memory Allocation Errors */
    while(1)
    {
    }
}

/*****************************************************************************
*
* Function: Application defined stack overflow hook
* vApplicationStackOverflowHook()
* param  pxTask,*pcTaskName
* return none
*****************************************************************************/
void vApplicationStackOverflowHook(TaskHandle_t pxTask,
                                   char *pcTaskName)
{
    //Handle FreeRTOS Stack Overflow
    while(1)
    {
    }
}

/*****************************************************************************
*
* Function: Application defined Tick hook
* vApplicationTickHook()
* param  none
* return none
* This function will be called by each tick interrupt by setting the value of
* configUSE_TICK_HOOK to 1.
*****************************************************************************/

void vApplicationTickHook(void)
{
    /*
     * This function will be called by each tick interrupt if
     * configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.
     */
}

/*****************************************************************************
* Function: vPreSleepProcessing
* return none
* passed parameter : ulExpectedIdleTime
*****************************************************************************/
void vPreSleepProcessing(uint32_t ulExpectedIdleTime)
{
}

/*****************************************************************************
* Function: Application defined idle task hook
* vApplicationIdleHook()
* param  none
* return none
* It is used to handle Idle Hook.
*****************************************************************************/
void vApplicationIdleHook(void)
{
    /* used to Handle Idle Hook for Profiling, Power Management etc */
}

/*****************************************************************************
*
* Function:  Used to Overwrite the GCC _sbrk function which check the heap limit
*            related to the stack pointer.
* param: delta
* return none
*
*****************************************************************************/
#if defined (__GNUC__)
void * _sbrk(uint32_t delta)
{
    extern char _end;     /* It is Defined by the linker */
    extern char __HeapLimit;
    static char *heap_end;
    static char *heap_limit;
    char *prev_heap_end;

    if(heap_end == 0)
    {
        heap_end = &_end;
        heap_limit = &__HeapLimit;
    }

    prev_heap_end = heap_end;
    if(prev_heap_end + delta > heap_limit)
    {
        return((void *) -1L);
    }
    heap_end += delta;
    return((void *) prev_heap_end);
}

#endif