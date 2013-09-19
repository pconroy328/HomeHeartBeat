// ----------------------------------------------------------------------------
//
//  mqtt.c      code to connect to a MQTT broker and send off events
//
//      This code uses Mosquitto v3.1.  If you install the Ubuntu packages you may
//      get the older versioN!  (I did). Follow the instructions on mosquitto.org
//      to install the latest version
//
#include <mosquitto.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>


#include "helpers.h"
#include "hhb_structures.h"
#include "mqtt.h"

static  int                     MQTT_Connected;
static  struct  mosquitto       *mosq = NULL;



//
// Forward declarations
static  void    my_message_callback( struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message );
static  void    my_connect_callback( struct mosquitto *mosq, void *userdata, int result );
static  void    my_log_callback( struct mosquitto *mosq, void *userdata, int level, const char *str );
static  char    *getCurrentDateTime( void );


// ------------------------------------------------------------------------------
static
void my_message_callback (struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
    debug_print ( "MQTT Entering. Payload length: %d\n", message->payloadlen );
    if (message->payloadlen > 0) {
        debug_print( "Topic [%s] Data [%s]\n", (char *) message->topic, (char *) message->payload);
        
    } else {
        debug_print(" MQTT Topic [%s] with no payload\n", message->topic );
    }
}


//
//      Needs to be of type 
// void mosquitto_connect_callback_set(struct mosquitto *mosq, void (*on_connect)(void *, int));
 
// ------------------------------------------------------------------------------
static
void my_connect_callback (struct mosquitto *mosq, void *userdata, int result)
{
    debug_print( "MQTT Connection Callback. Result: %d\n", result );

    if(!result){
        /* Subscribe to broker information topics on successful connect. */
        mosquitto_subscribe( mosq, NULL, "$SYS/#", 2 );
        MQTT_Connected = TRUE;
        
    } else {
        fprintf(stderr, " MQTT Connection failed\n" );
        MQTT_Connected = FALSE;
    }
}

// ------------------------------------------------------------------------------
void my_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
    int i;

    debug_print( "MQTT Subscribed (mid: %d): %d", mid, granted_qos[ 0 ] );
    for(i = 1; i < qos_count; i++){
        debug_print( ", %d", granted_qos[ i ] );
    }
    
    debug_print( "\n", 0 );
}

// ------------------------------------------------------------------------------
static
void my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
    /* Print all log messages regardless of level. */
    debug_print("MQTT Logging: [%s]\n", str );
}



// ---------------------------------------------------------------------------
void    MQTT_setDefaults (HomeHeartBeatSystem_t *aSystem, char *brokerHostName)
{
    assert( brokerHostName != NULL );
    strncpy( &(aSystem->MQTTParameters.mqttBrokerHost[ 0 ]), 
             brokerHostName, 
            sizeof aSystem->MQTTParameters.mqttBrokerHost );
    
    aSystem->MQTTParameters.mqttPortNumber = (aSystem->MQTTParameters.mqttPortNumber == 0 ? 1883 : aSystem->MQTTParameters.mqttPortNumber);
    aSystem->MQTTParameters.mqttKeepAliveValue = (aSystem->MQTTParameters.mqttKeepAliveValue == 0 ? 60 : aSystem->MQTTParameters.mqttKeepAliveValue);
    
    aSystem->MQTTParameters.mqttRetainMsgs = false;
    // QoS of zero is ok
}
// ----------------------------------------------------------------------------
void    MQTT_initialize (HomeHeartBeatSystem_t *aSystem)
{
    char    id[30];
    int     i;
    bool    clean_session = true;

    MQTT_Connected = FALSE;
    
    if (!aSystem->logEventsToMQTT)
        return;
    
      
    //
    // If you get errors complaining about aruguments and mismatched prototypes
    //      odds are you're not pulling down the latest version of mosquitto
    //
    mosquitto_lib_init();
    
    //
    // Example code shows the first parameter is "id" - but I'm getting EINVAL so I
    // switched it to NULL and it's working     
    mosq = mosquitto_new( NULL, clean_session, NULL );
    
    if(!mosq) {
        warnAndKeepGoing( "Unable to instantiate an MQTT instance!" );
        fprintf( stderr, "MQTT Broker Host: [%s], Port: %d, KeepAlive: %d\n", 
                            aSystem->MQTTParameters.mqttBrokerHost,
                            aSystem->MQTTParameters.mqttPortNumber,
                            aSystem->MQTTParameters.mqttKeepAliveValue );

        aSystem->logEventsToMQTT = FALSE;
        return;
    }
    
    mosquitto_log_callback_set( mosq, my_log_callback );
    mosquitto_connect_callback_set( mosq, my_connect_callback );
    mosquitto_message_callback_set( mosq, my_message_callback );
    mosquitto_subscribe_callback_set( mosq, my_subscribe_callback );

    if (mosquitto_connect(  mosq, 
                            aSystem->MQTTParameters.mqttBrokerHost,
                            aSystem->MQTTParameters.mqttPortNumber,
                            aSystem->MQTTParameters.mqttKeepAliveValue ) ) {
        warnAndKeepGoing( "Unable to connect MQTT to broker!" );
        fprintf( stderr, "MQTT Broker Host: [%s], Port: %d, KeepAlive: %d\n", 
                            aSystem->MQTTParameters.mqttBrokerHost,
                            aSystem->MQTTParameters.mqttPortNumber,
                            aSystem->MQTTParameters.mqttKeepAliveValue );
        
        aSystem->logEventsToMQTT = FALSE;
        return;
    }
    
    MQTT_Connected = TRUE;
}

// ----------------------------------------------------------------------------
void    MQTT_teardown ()
{
    if (MQTT_Connected) {
        mosquitto_destroy( mosq );
        mosquitto_lib_cleanup();
    }
}

// ----------------------------------------------------------------------------
int MQTT_sendReceive ()
{
    if (!MQTT_Connected)
        return MQTT_NOT_CONNECTED;
    
    int error = mosquitto_loop( mosq, -1, 0 );
    return error;
}

// -----------------------------------------------------------------------------
int    MQTT_createDeviceAlarm (HomeHeartBeatSystem_t *aSystem, HomeHeartBeatDevice_t *deviceRecPtr)
{
    if (!MQTT_Connected)
        return MQTT_NOT_CONNECTED;

    //
    // We've noticed that the state has changed on one of our devices - send a new message!
    
    char    *alarmFormatString ="*%s* | %s | %02d | %s | %s | %s | %d |";
    char    *sensorType = NULL;
    char    *sensorName = NULL;
    char    *state = NULL;
    int     duration = 0;
    char    payload[ 1024 ];
    int     length = 0;
    
    
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

        
    } else {
        sensorType = "UNKNOWN";
        state = "UNKNOWN";
    }

    
    length = snprintf( payload, sizeof payload, alarmFormatString,
                    aSystem->MQTTParameters.mqttAlarmTopic,
                    getCurrentDateTime(),
                    deviceRecPtr->deviceType,
                    sensorType,
                    deviceRecPtr->deviceName,
                    state,
                    deviceRecPtr->deviceStateTimer );
      
    int     messageID;
    int result = mosquitto_publish( mosq, &messageID, 
                                    aSystem->MQTTParameters.mqttAlarmTopic, 
                                    length, 
                                    payload, 
                                    aSystem->MQTTParameters.mqttQoS, 
                                    aSystem->MQTTParameters.mqttRetainMsgs );
    
    if (result != 0) {
        warnAndKeepGoing( "Attempt to publish an alarm to MQTT failed." );
        fprintf( stderr, "Result: %d  Message: [%s]\n", result, payload );
    }
    
    return result;
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
     */
    
    int     messageID;
    char    *statusFormatString ="*%s* | %s | %02d | %s | %s | %s | %d | %s | %s | %s | %s | %s | %s | %s |";
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
        condition1 = (deviceRecPtr->motSensor->alarmOnMotion ? "ALARM ON WET" : "NO ALARM ON WET" );
        condition2 = (deviceRecPtr->motSensor->alarmOnNoMotion ? "ALARM ON DRY" : "NO ALARM ON DRY" );
        condition3 = (deviceRecPtr->motSensor->callOnMotion ? "CALL ON WET" : "DO NOT CALL ON WET" );
        condition4 = (deviceRecPtr->motSensor->callOnNoMotion ? "CALL ON DRY" : "DO NOT CALL ON DRY" );
        
    } else {
        sensorType = "UNKNOWN";
        state = "UNKNOWN";
        condition1 = "";
        condition2 = "";
        condition3 = "";
        condition4 = "";
    }

    
    length = snprintf( payload, sizeof payload, statusFormatString,
                    aSystem->MQTTParameters.mqttStatusTopic,
                    getCurrentDateTime(),
                    deviceRecPtr->deviceType,
                    sensorType,
                    deviceRecPtr->deviceName,
                    state,
                    deviceRecPtr->deviceStateTimer,
                    condition1, condition2, condition3, condition4, 
                    offLine, lowBattery, inAlarmState );
            
   
    //
    //   MQTT defines three levels of Quality of Service (QoS). The QoS defines how hard the broker/client will try 
    //  to ensure that a message is received. Messages may be sent at any QoS level, and clients may attempt to subscribe 
    //  to topics at any QoS level. This means that the client chooses the maximum QoS it will receive. For example, 
    //  if a message is published at QoS 2 and a client is subscribed with QoS 0, the message will be delivered 
    //  to that client with QoS 0. If a second client is also subscribed to the same topic, but with QoS 2, 
    //  then it will receive the same message but with QoS 2. For a second example, if a client is subscribed with QoS 2 
    //  and a message is published on QoS 0, the client will receive it on QoS 0.
    //  
    //  Higher levels of QoS are more reliable, but involve higher latency and have higher bandwidth requirements.
    //      0: The broker/client will deliver the message once, with no confirmation.
    //      1: The broker/client will deliver the message at least once, with confirmation required.
    //      2: The broker/client will deliver the message exactly once by using a four step handshake.
    
    int result = mosquitto_publish( mosq, 
                                    &messageID, 
                                    aSystem->MQTTParameters.mqttStatusTopic,
                                    length, 
                                    payload, 
                                    aSystem->MQTTParameters.mqttQoS,
                                    aSystem->MQTTParameters.mqttRetainMsgs );
    
    if (result != 0) {
        warnAndKeepGoing( "Attempt to publish an alarm to MQTT failed." );
        fprintf( stderr, "Result: %d  Message: [%s]\n", result, payload );
    }
    
    return result;
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
                    "%F %T %z",
                    tmPtr );
            
        }
    }
    
    return &currentDateTimeBuffer[ 0 ];
}

// -----------------------------------------------------------------------------
int MQTT_handleError (HomeHeartBeatSystem_t *aSystem, int errorCode) 
{
    fprintf( stderr, "ERROR detected in MQTT [%d]\n", errorCode );
    fprintf( stderr, "Disconnecting...\n" );
    MQTT_teardown();
    fprintf( stderr, "Reconnecting...\n" );
    MQTT_initialize( aSystem );
}