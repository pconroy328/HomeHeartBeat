/* 
 * File:   mqtt.c
 * Author: patrick conroy
 *
 * Created on September 17, 2013, 10:03 AM
 * (C) 2013 Patrick Conroy
 * 
 * 12Mar2104    Adding MAC address to MQTT Payload
 */
#define _POSIX_C_SOURCE     200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include "mqtt.h"
#include "logger.h"
#include "mosquitto.h"


static  int                     MQTT_Connected = FALSE;
static  int                     enableMQTTLoggingCallback = FALSE;
static  int                     exitOnTooManyErrors = FALSE;
static  int                     maxReconnectAttempts = 100;
static  int                     mqttQoS = 0;
static  int                     mqttRetainMessages = FALSE;

static  struct  mosquitto       *mosquittoInstance = NULL;

#define MAX_ERRORSTRING_LEN 256
static  char    errorStringBuffer[ MAX_ERRORSTRING_LEN ];



//
// Forward declarations
static  void    subscribedMessageCallback( struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message );
static  void    brokerConnectionCallback( struct mosquitto *mosq, void *userdata, int result );
static  void    mqttLoggingCallback( struct mosquitto *mosq, void *userdata, int level, const char *str );
static  char    *getCurrentDateTime( void );



// ----------------------------------------------------------------------------
//
//  mqtt.c      code to connect to a MQTT broker and send off events
//
//      This code uses Mosquitto v3.1.  If you install the Ubuntu packages you may
//      get the older versioN!  (I did). Follow the instructions on mosquitto.org
//      to install the latest version
//

//
// Don't forget to download and build mosquitto from source!
//      Don't forget you'll need libssl-dev from Synaptic
//      And g++ (sudo apt-get install g++)




// ------------------------------------------------------------------------------
static
void subscribedMessageCallback (struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
    Logger_LogInfo( "MQTT subscribedMessageCallback():: Payload length: %d\n", message->payloadlen );
    if (message->payloadlen > 0) {
        Logger_LogDebug( "MQTT subscribedMessageCallback():: Topic [%s] Data [%s]\n", (char *) message->topic, (char *) message->payload);
        
    } else {
        Logger_LogWarning("MQTT subscribedMessageCallback():: MQTT Topic [%s] with no payload\n", message->topic );
    }
}


//
//      Needs to be of type 
// void mosquitto_connect_callback_set(struct mosquitto *mosq, void (*on_connect)(void *, int));
 
// ------------------------------------------------------------------------------
static
void brokerConnectionCallback (struct mosquitto *mosq, void *userdata, int result)
{
    // Parameters:  
    //      mosq	the mosquitto instance making the callback.
    //      obj	the user data provided in mosquitto_new
    //      rc	the return code of the connection response, one of:
    //          0 - success
    //          1 - connection refused (unacceptable protocol version)
    //          2 - connection refused (identifier rejected)
    //          3 - connection refused (broker unavailable)
    //          4-255 - reserved for future use
    //
    // This guy doesn't get called right away - you need to let Mosquitto run for a bit
    //  Thus don't rely on setting a boolean "Connected" only in here
    //
    Logger_LogInfo( "MQTT brokerConnectionCallback():: MQTT Connection Callback. Result: %d\n", result );

    if (result == 0) {
        /* Subscribe to broker information topics on successful connect. */
        //mosquitto_subscribe( mosq, NULL, "$SYS/#", 2 );
        MQTT_Connected = TRUE;
        
    } else {
        Logger_LogError( "brokerConnectionCallback():: MQTT Connection failed. Result: %d\n", result );
        switch (result) {
            case    1:  Logger_LogError( "MQTT brokerConnectionCallback():: connection refused (unacceptable protocol version)\n" );
            case    2:  Logger_LogError( "MQTT brokerConnectionCallback():: connection refused (identifier rejected)\n" );
            case    3:  Logger_LogError( "MQTT brokerConnectionCallback():: connection refused (broker unavailable)\n" );
        }
              
        MQTT_Connected = FALSE;
    }
}

// ------------------------------------------------------------------------------
static
void mqttLoggingCallback (struct mosquitto *mosq, void *userdata, int level, const char *str)
{
    /* Print all log messages regardless of level. */
    if (enableMQTTLoggingCallback)
        Logger_LogInfo( "MQTT mqttLoggingCallback():: [%s] %d\n", str );    
}



// ---------------------------------------------------------------------------
void    MQTT_SetDefaults (MQTT_Parameters_t *mqttParams)
{
    mqttParams->enableMQTTLoggingCallback = FALSE;
    mqttParams->QoS = 0;
    mqttParams->portNumber = 1883;
    mqttParams->cleanSession = TRUE;
    mqttParams->clientID[0] = '\0';
    mqttParams->keepAliveValue = 60;
    strncpy( &(mqttParams->brokerHostName[0]), "192.168.1.11", MQTT_BROKERHOSTNAME_LEN );
    mqttParams->exitOnTooManyErrors = TRUE;
    mqttParams->maxReconnectAttempts = 100;
    mqttParams->useJSON = TRUE;
}

// ----------------------------------------------------------------------------
int    MQTT_Initialize (MQTT_Parameters_t *mqttParams)
{
    char    id[ 30 ];
    int     major, minor, revision;
    int     errorNo, result;

    MQTT_Connected = FALSE;
          
    //
    // If you get errors complaining about arguments and mismatched prototypes
    //      odds are you're not pulling down the latest version of mosquitto
    //
    Logger_LogInfo( "MQTT_initialize():: Initialization starting.\n" );
    
    //
    // Copy off our parameters so we've got them cached...
    enableMQTTLoggingCallback = mqttParams->enableMQTTLoggingCallback;
    exitOnTooManyErrors = mqttParams->exitOnTooManyErrors;
    maxReconnectAttempts = mqttParams->maxReconnectAttempts;
    mqttQoS = mqttParams->QoS;
    mqttRetainMessages = mqttParams->retainMsgs;
    
    //
    // Now fire up Mosquitto
    mosquitto_lib_version( &major, &minor, &revision );
    Logger_LogInfo( "MQTT Mosquitto Library Version info. Major: %d, Minor: %d, Revision: %d\n", major, minor, revision );
    mosquitto_lib_init();
    
    //
    // Example code shows the first parameter is "id" - but I'm getting EINVAL so I
    // switched it to NULL and it's working. If first param is NULL, CleanSession must be true
    //
    Logger_LogInfo( "MQTT_initialize():: Calling mosquitto_new\n" );
    mosquittoInstance = mosquitto_new( &(mqttParams->clientID[0]), mqttParams->cleanSession, NULL );
    
    if (mosquittoInstance == NULL) {
        errorNo = errno;
        memset( errorStringBuffer, '\0', MAX_ERRORSTRING_LEN );
        strerror_r( errorNo, errorStringBuffer, MAX_ERRORSTRING_LEN );
        
        Logger_LogWarning( "MQTT_initialize()::new -- Unable to instantiate an MQTT instance! ErrNo: %d [%s]\n", errorNo, errorStringBuffer );
        return FALSE;
    }
    
    Logger_LogInfo( "MQTT_initialize():: Setting callbacks.\n" );
    mosquitto_log_callback_set( mosquittoInstance, mqttLoggingCallback );
    mosquitto_connect_callback_set( mosquittoInstance, brokerConnectionCallback );
    mosquitto_message_callback_set( mosquittoInstance, subscribedMessageCallback );
    // mosquitto_subscribe_callback_set( mosquittoInstance, my_subscribe_callback );

    Logger_LogInfo( "MQTT_initialize():: Calling mosquitto_connect(%s, %d, %d )\n", mqttParams->brokerHostName, mqttParams->portNumber, mqttParams->keepAliveValue );
    result = mosquitto_connect(  mosquittoInstance, 
                            mqttParams->brokerHostName,
                            mqttParams->portNumber,
                            mqttParams->keepAliveValue );
    
    if (result != MOSQ_ERR_SUCCESS) {
        errorNo = errno;
        memset( errorStringBuffer, '\0', MAX_ERRORSTRING_LEN );
        strerror_r( errorNo, errorStringBuffer, MAX_ERRORSTRING_LEN );
        
        Logger_LogError( "MQTT_initialize()::connect -- Unable to connect MQTT to broker!. Result: %d. ErrNo; %d [%s]\n", result, errorNo, errorStringBuffer );
        return FALSE;
    }
    
    Logger_LogInfo( "MQTT_initialize():: mosquitto_new() and connect() successful. MQTT Broker Online.\n" );
    MQTT_Connected = TRUE;
    
    return MQTT_Connected;
}

// ----------------------------------------------------------------------------
void    MQTT_Teardown ()
{
    int result = 0;
    
    if (MQTT_Connected) {
        Logger_LogInfo( "MQTT_teardown() -- Calling mosquitto termination routines\n" );
        result = mosquitto_disconnect( mosquittoInstance );
        if (result != MOSQ_ERR_SUCCESS)
            Logger_LogWarning( "MQTT_teardown() -- mosquitto_disconnected returned error: %d. Continuing teardown.\n", result );
        
        mosquitto_destroy( mosquittoInstance );
        (void) mosquitto_lib_cleanup();
        Logger_LogInfo( "MQTT_teardown() -- Broker disconnected, Instance destroyed, Library Cleaned up.\n" );
        
        MQTT_Connected = FALSE;
        // enableMQTTLoggingCallback = FALSE;
    } else {
        Logger_LogWarning( "MQTT_teardown() -- Calling teardown on a DISCONNECTED broker. Nothing happens.\n" );
    }
}

// ----------------------------------------------------------------------------
int MQTT_SendReceive (int *error)
{
    if (!MQTT_Connected) {
        Logger_LogWarning( "MQTT_SendReceive() -- Calling sendReceive() on a DISCONNECTED broker. Nothing happens.\n" );
        return MQTT_NOT_CONNECTED;
    }
    
   
    //
    //
    int result = mosquitto_loop( mosquittoInstance, 1000, 10 );
    if (result != MOSQ_ERR_SUCCESS) {
        
        Logger_LogError( "MQTT SendReceive - error Code : %d\n", result  );
        switch (result) {
            case    MOSQ_ERR_INVAL:     Logger_LogError( "mosquitto_loop() says INVAL\n", 0 ); break;
            case    MOSQ_ERR_NOMEM:     Logger_LogError( "mosquitto_loop() says NOMEM\n", 0 ); break;
            case    MOSQ_ERR_NO_CONN:   Logger_LogError( "mosquitto_loop() says NO CONN\n", 0 ); break;
            case    MOSQ_ERR_CONN_LOST: Logger_LogError( "mosquitto_loop() says CONN LOST\n", 0 ); break;
            case    MOSQ_ERR_PROTOCOL:  Logger_LogError( "mosquitto_loop() says PROTOCOL\n", 0 ); break;
            case    MOSQ_ERR_ERRNO:     Logger_LogError( "mosquitto_loop() says ERRNO\n", 0 ); break;
                
        }        
    }
    
    //
    // Return TRUE if no errors
    *error = result;
    return (result == MOSQ_ERR_SUCCESS);
}

// -----------------------------------------------------------------------------
int MQTT_HandleError (MQTT_Parameters_t *mqttParams, int errorCode) 
{
    int numAttempts = 0;

    MQTT_Connected = FALSE;                             // Set ourselves to offline
    
    while (!MQTT_Connected && (numAttempts < maxReconnectAttempts)) {
        numAttempts += 1;
        
        Logger_LogError( "MQTT_handleError:: ERROR detected in MQTT [%d]\n", errorCode );
        Logger_LogError( "MQTT_handleError:: Disconnecting...\n" );
        MQTT_Teardown();
        Logger_LogError( "MQTT_handleError:: Reconnecting...\n" );
        MQTT_Initialize( mqttParams );

        if (MQTT_Connected) {
            Logger_LogError( "MQTT_handleError:: MQTT has been reinitialized and is back online.\n" );
        } else {
            //
            // Let's not spin at CPU speed - let's give another program a chance at
            // the CPU and we'll see if the Broker comes back
            Logger_LogError( "MQTT_handleError:: MQTT has NOT been reinitialized. Sleeping and then retrying.\n" );
            sleep( 1 );
        }
    }
    
    //
    // Do we just give up?
    if (!MQTT_Connected && exitOnTooManyErrors) {
        Logger_LogFatal( "MQTT_handleError:: MQTT cannot been reinitialized. System is exiting.\n" );
        exit( 1 );
    }
}

// -----------------------------------------------------------------------------
int MQTT_Publish (char *topic, char *payload, int payloadLength)
{
    int     result;
    int     messageID;
    
    if (!MQTT_Connected) {
        Logger_LogWarning( "MQTT_Publish() -- Calling publish() on a DISCONNECTED broker. Nothing happens.\n" );
        return MQTT_NOT_CONNECTED;
    }
    
    result = mosquitto_publish( mosquittoInstance, 
                            &messageID, 
                            topic,
                            payloadLength, 
                            payload, 
                            mqttQoS,
                            mqttRetainMessages );
    
    if (result != 0) {
        Logger_LogError( "MQTT_publish:: - Failure. Result: %\n", result );
    } else {
        Logger_LogInfo( "MQTT_publish:: - Success. Message [%d]:[%s]\n", messageID, payload );
    }
    
    return result;    
}
// -----------------------------------------------------------------------------
int    MQTT_createDeviceAlarm (HomeHeartBeatSystem_t *aSystem, HomeHeartBeatDevice_t *deviceRecPtr)
{
    if (!MQTT_Connected)
        return MQTT_NOT_CONNECTED;

    //
    // We've noticed that the state has changed on one of our devices - send a new message!
    
    char    *alarmFormatString ="%s | %s | %02d | %s | %s | %s | %d | %s |";
    char    *alarmFormatStringJSON ="{ \"alarmTopic\" : \"%s\" , \"datetime\" : \"%s\" , \"deviceType\" : %02d , \"type\" : \"%s\" , \"name\" : \"%s\" , \"state\" : \"%s\" , \"duration\" : %d, \"MACAddress\" : \"%s\" }";
    char    *sensorType = NULL;
    char    *sensorName = NULL;
    char    *state = NULL;
    int     duration = 0;
    char    payload[ 1024 ];
    int     length = 0;
    
    Logger_FunctionStart();
    
    //
    // What I send depends on the device type...
    memset( payload, '\0', sizeof payload );

    if (deviceRecPtr->deviceType == DT_HOME_KEY) {
        sensorType = "HOME KEY";
        state = "NA";
        
    } else if (deviceRecPtr->deviceType == DT_OPEN_CLOSE_SENSOR) {
        sensorType = "OPEN-CLOSE SENSOR";
        state = "???";
        
        if (deviceRecPtr->ocSensor->isOpen && deviceRecPtr->ocSensor->alarmOnOpen)
            state = "OPEN";
        if (!deviceRecPtr->ocSensor->isOpen && deviceRecPtr->ocSensor->alarmOnClose)
            state = "CLOSE";

        //
        // Early exit conditions - transitioned to close but we alarm on open - ignore
        if (!deviceRecPtr->ocSensor->isOpen && deviceRecPtr->ocSensor->alarmOnOpen)
            return 0;
        // transitioned to open but we alarm on close - ignore
        if (deviceRecPtr->ocSensor->isOpen && deviceRecPtr->ocSensor->alarmOnClose)
            return 0;
        
    } else if (deviceRecPtr->deviceType == DT_TILT_SENSOR) {
        sensorType = "TILT SENSOR";
        state = "???";
        
        if (deviceRecPtr->tsSensor->isOpen && deviceRecPtr->tsSensor->alarmOnOpen)
            state = "OPEN";
        if (!deviceRecPtr->tsSensor->isOpen && deviceRecPtr->tsSensor->alarmOnClose)
            state = "CLOSE";

        //
        // Early exit conditions - transitioned to close but we alarm on open - ignore
        if (!deviceRecPtr->tsSensor->isOpen && deviceRecPtr->tsSensor->alarmOnOpen)
            return 0;
        // transitioned to open but we alarm on close - ignore
        if (deviceRecPtr->tsSensor->isOpen && deviceRecPtr->tsSensor->alarmOnClose)
            return 0;
        
    } else if (deviceRecPtr->deviceType == DT_MOTION_SENSOR) {
        sensorType = "MOTION SENSOR";
        state = "???";
        
        if (deviceRecPtr->motSensor->motionDetected && deviceRecPtr->motSensor->alarmOnMotion)
            state = "MOTION";
        if (!deviceRecPtr->motSensor->motionDetected && deviceRecPtr->motSensor->alarmOnNoMotion)
            state = "NO MOTION";
        
        //
        // Early exit conditions - transitioned to NO MOTION but we alarm on MOTION - ignore
        if (!deviceRecPtr->motSensor->motionDetected && deviceRecPtr->motSensor->alarmOnMotion)
            return 0;
        // transitioned to MOTION but we alarm on NO MOTION - ignore
        if (deviceRecPtr->motSensor->motionDetected && deviceRecPtr->motSensor->alarmOnNoMotion)
            return 0;
        

    } else if (deviceRecPtr->deviceType == DT_WATER_LEAK_SENSOR) {
        sensorType = "WATER LEAK SENSOR";
        state = "???";
        
        if (deviceRecPtr->wlSensor->wetnessDetected && deviceRecPtr->wlSensor->alarmOnWet)
            state = "WET";
        if (!deviceRecPtr->wlSensor->wetnessDetected && deviceRecPtr->wlSensor->alarmOnDry)
            state = "DRY";

        //
        // Early exit conditions - transitioned to DRY but we alarm on WET - ignore
        if (!deviceRecPtr->wlSensor->wetnessDetected && deviceRecPtr->wlSensor->alarmOnWet)
            return 0;
        // transitioned to WET but we alarm on DRY - ignore
        if (deviceRecPtr->wlSensor->wetnessDetected && deviceRecPtr->wlSensor->alarmOnDry)
            return 0;

    } else if (deviceRecPtr->deviceType == DT_POWER_SENSOR) {
        sensorType = "POWER SENSOR";
        state = "???";
        
        if (deviceRecPtr->psSensor->isPowerOn && deviceRecPtr->psSensor->alarmOnPowerOn)
            state = "ON";
        if (!deviceRecPtr->psSensor->isPowerOn && deviceRecPtr->psSensor->alarmOnPowerOff)
            state = "DRY";

        //
        // Early exit conditions - transitioned to OFF but we alarm on ON - ignore
        if (!deviceRecPtr->psSensor->isPowerOn && deviceRecPtr->psSensor->alarmOnPowerOn)
            return 0;
        // transitioned to ON but we alarm on OGG - ignore
        if (deviceRecPtr->psSensor->isPowerOn && deviceRecPtr->psSensor->alarmOnPowerOff)
            return 0;

        
    } else {
        sensorType = "UNKNOWN";
        state = "UNKNOWN";
    }

    //
    // Now some hackalicious stuff - are we using JSON?
    if (!aSystem->MQTTParameters.useJSON) {
        length = snprintf( payload, sizeof payload, alarmFormatString,
                    aSystem->MQTTParameters.alarmTopic,
                    getCurrentDateTime(),
                    deviceRecPtr->deviceType,
                    sensorType,
                    deviceRecPtr->deviceName,
                    state,
                    deviceRecPtr->deviceStateTimer,
                    deviceRecPtr->macAddress );
    } else {
        //
        // Yes - we are using JSON! :)
        length = snprintf( payload, sizeof payload, 
                    alarmFormatStringJSON,
                    aSystem->MQTTParameters.alarmTopic,
                    getCurrentDateTime(),
                    deviceRecPtr->deviceType,
                    sensorType,
                    deviceRecPtr->deviceName,
                    state,
                    deviceRecPtr->deviceStateTimer,
                    deviceRecPtr->macAddress );        
    }  
    
    return MQTT_Publish( aSystem->MQTTParameters.alarmTopic, payload, length );
}

// -----------------------------------------------------------------------------
int    MQTT_createDeviceEvent (HomeHeartBeatSystem_t *aSystem, HomeHeartBeatDevice_t *deviceRecPtr)
{
    if (!MQTT_Connected)
        return MQTT_NOT_CONNECTED;
    
    //
    // All we know is that we have a state event
    /*
            int mosquitto_publish(    struct     mosquitto     *mosq,
                                            int         *mid,
                                            const     char     *    topic,
                                            int         payloadlen,
                                            const     void     *    payload,
                                            int         qos,
                                            bool         retain    )
        Publish a message on a given topic.
        Parameters
            mosq    a valid mosquitto instance.
     
            mid     pointer to an int.  
                    If not NULL, the function will set this to the message id of this particular message.  
                    This can be then used with the publish callback to determine when the message has been sent.  
                    Note that although the MQTT protocol doesn’t use message ids for messages with QoS=0, 
                    libmosquitto assigns them message ids so they can be tracked with this parameter.
     
            payloadlen    the size of the payload (bytes).  Valid values are between 0 and 268,435,455.
        
            payload     pointer to the data to send.  If payloadlen > 0 this must be a valid memory location.
     
            qos     integer value 0, 1 or 2 indicating the Quality of Service to be used for the message.
            retain    set to true to make the message retained.
     *                     aSystem->MQTTParameters.statusTopic,
                    getCurrentDateTime(),
                    deviceRecPtr->deviceType,
                    sensorType,
                    deviceRecPtr->deviceName,
                    state,
                    deviceRecPtr->deviceStateTimer,
                    condition1, condition2, condition3, condition4, 
                    offLine, lowBattery, inAlarmState,
                    deviceRecPtr->macAddress );

     */
    
    int     messageID;
    char    *statusFormatString ="%s | %s | %02d | %s | %s | %s | %d | %s | %s | %s | %s | %s | %s | %s | %s |";
    char    *statusFormatStringJSON ="{ \"statusTopic\" : \"%s\" , \"datetime\" : \"%s\" , \"deviceType\" : %02d , \"type\" : \"%s\" , \"name\" : \"%s\" , \"state\" : \"%s\" , \"duration\" : %d , \"setAlarmAction\" : \"%s\" , \"unsetAlarmAction\" : \"%s\" , \"setCallAction\" : \"%s\" , \"unsetCallAction\" : \"%s\" , \"online\" : \"%s\" , \"battery\" : \"%s\" , \"triggered\" : \"%s\" , \"MACAddress\" : \"%s\" }";
    char    *sensorType = NULL;
    char    *sensorName = NULL;
    char    *condition1 = NULL;
    char    *condition2 = NULL;
    char    *condition3 = NULL;
    char    *condition4 = NULL;
    char    *state = NULL;
    char    *offLine = "ONLINE";
    char    *lowBattery = "BATTERY OK";
    char    *inAlarmState = "CLEARED";
    int     duration = 0;
    char    payload[ 1024 ];
    int     length = 0;

    if (deviceRecPtr->deviceOffLine)
        offLine = "OFF-LINE";
    if (deviceRecPtr->deviceLowBattery)
        lowBattery = "LOW BATTERY";
    if (deviceRecPtr->deviceInAlarm)
        inAlarmState = "TRIGGERED";
    
    memset( payload, '\0', sizeof payload );

    if (deviceRecPtr->deviceType == DT_HOME_KEY) {
        sensorType = "HOME KEY";
        
        state = "NA";
        condition1 = "NA";
        condition2 = "NA";
        condition3 = "NA";
        condition4 = "NA";
        
    } else if (deviceRecPtr->deviceType == DT_OPEN_CLOSE_SENSOR) {
        sensorType = "OPEN-CLOSE SENSOR";
        
        state = (deviceRecPtr->ocSensor->isOpen ? "OPEN" : "CLOSED");
        condition1 = (deviceRecPtr->ocSensor->alarmOnOpen ? "ALARM ON OPEN" : "NO ALARM ON OPEN" );
        condition2 = (deviceRecPtr->ocSensor->alarmOnClose ? "ALARM ON CLOSE" : "NO ALARM ON CLOSE" );
        condition3 = (deviceRecPtr->ocSensor->callOnOpen ? "CALL ON OPEN" : "DO NOT CALL ON OPEN" );
        condition4 = (deviceRecPtr->ocSensor->callOnClose ? "CALL ON CLOSE" : "DO NOT CALL ON CLOSE" );

    } else if (deviceRecPtr->deviceType == DT_TILT_SENSOR) {
        sensorType = "TILT SENSOR";
        
        state = (deviceRecPtr->tsSensor->isOpen ? "OPEN" : "CLOSED");
        condition1 = (deviceRecPtr->tsSensor->alarmOnOpen ? "ALARM ON OPEN" : "NO ALARM ON OPEN" );
        condition2 = (deviceRecPtr->tsSensor->alarmOnClose ? "ALARM ON CLOSE" : "NO ALARM ON CLOSE" );
        condition3 = (deviceRecPtr->tsSensor->callOnOpen ? "CALL ON OPEN" : "DO NOT CALL ON OPEN" );
        condition4 = (deviceRecPtr->tsSensor->callOnClose ? "CALL ON CLOSE" : "DO NOT CALL ON CLOSE" );
        
    } else if (deviceRecPtr->deviceType == DT_MOTION_SENSOR) {
        sensorType = "MOTION SENSOR";
        
        state = (deviceRecPtr->motSensor->motionDetected ? "MOTION" : "NO MOTION");
        condition1 = (deviceRecPtr->motSensor->alarmOnMotion ? "ALARM ON MOTION" : "NO ALARM ON MOTION" );
        condition2 = (deviceRecPtr->motSensor->alarmOnNoMotion ? "ALARM ON NO MOTION" : "NO ALARM ON NO MOTION" );
        condition3 = (deviceRecPtr->motSensor->callOnMotion ? "CALL ON MOTION" : "DO NOT CALL ON MOTION" );
        condition4 = (deviceRecPtr->motSensor->callOnNoMotion ? "CALL ON NO MOTION" : "DO NOT CALL ON NO MOTION" );
        
    } else if (deviceRecPtr->deviceType == DT_WATER_LEAK_SENSOR) {
        sensorType = "WATER LEAK SENSOR";
        state = (deviceRecPtr->wlSensor->wetnessDetected ? "WET" : "DRY");
        condition1 = (deviceRecPtr->wlSensor->alarmOnWet ? "ALARM ON WET" : "NO ALARM ON WET" );
        condition2 = (deviceRecPtr->wlSensor->alarmOnDry ? "ALARM ON DRY" : "NO ALARM ON DRY" );
        condition3 = (deviceRecPtr->wlSensor->callOnWet ? "CALL ON WET" : "DO NOT CALL ON WET" );
        condition4 = (deviceRecPtr->wlSensor->callOnDry ? "CALL ON DRY" : "DO NOT CALL ON DRY" );
        
    } else if (deviceRecPtr->deviceType == DT_POWER_SENSOR) {
        sensorType = "POWER SENSOR";
        state = (deviceRecPtr->psSensor->isPowerOn ? "ON" : "OFF");
        condition1 = (deviceRecPtr->psSensor->alarmOnPowerOn ? "ALARM ON ON" : "NO ALARM ON ON" );
        condition2 = (deviceRecPtr->psSensor->alarmOnPowerOff ? "ALARM ON OFF" : "NO ALARM ON OFF" );
        condition3 = (deviceRecPtr->psSensor->callOnPowerOn ? "CALL ON ON" : "DO NOT CALL ON ON" );
        condition4 = (deviceRecPtr->psSensor->callOnPowerOff ? "CALL ON OFF" : "DO NOT CALL ON OFF" );

    } else {
        sensorType = "UNKNOWN";
        state = "UNKNOWN";
        condition1 = "";
        condition2 = "";
        condition3 = "";
        condition4 = "";
    }

    if (!aSystem->MQTTParameters.useJSON) {
        length = snprintf( payload, sizeof payload, statusFormatString,
                    aSystem->MQTTParameters.statusTopic,
                    getCurrentDateTime(),
                    deviceRecPtr->deviceType,
                    sensorType,
                    deviceRecPtr->deviceName,
                    state,
                    deviceRecPtr->deviceStateTimer,
                    condition1, condition2, condition3, condition4, 
                    offLine, lowBattery, inAlarmState,
                    deviceRecPtr->macAddress );
    } else {
        length = snprintf( payload, sizeof payload, 
                    statusFormatStringJSON,
                    aSystem->MQTTParameters.statusTopic,
                    getCurrentDateTime(),
                    deviceRecPtr->deviceType,
                    sensorType,
                    deviceRecPtr->deviceName,
                    state,
                    deviceRecPtr->deviceStateTimer,
                    condition1, condition2, condition3, condition4, 
                    offLine, lowBattery, inAlarmState,
                    deviceRecPtr->macAddress );        
    }        
    
    return MQTT_Publish( aSystem->MQTTParameters.statusTopic, payload, length );
}

// -----------------------------------------------------------------------------
static  char    currentDateTimeBuffer[ 80 ];
static
char    *getCurrentDateTime (void)
{
    //
    // Something quick and dirty... Fix this later - thread safe
    time_t  current_time;
    struct  tm  *tmPtr;
 
    memset( currentDateTimeBuffer, '\0', sizeof currentDateTimeBuffer );
    
    /* Obtain current time as seconds elapsed since the Epoch. */
    current_time = time( NULL );
    if (current_time > 0) {
        /* Convert to local time format. */
        tmPtr = localtime( &current_time );
 
        if (tmPtr != NULL) {
            strftime( currentDateTimeBuffer,
                    sizeof currentDateTimeBuffer,
                    "%FT%T%z",                           // ISO 8601 Format
                    tmPtr );
        }
    }
    
    return &currentDateTimeBuffer[ 0 ];
}

