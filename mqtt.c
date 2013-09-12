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


#include "helpers.h"
#include "homeheartbeat.h"

static     struct mosquitto *mosq = NULL;


void my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
    debug_print ( "Entering. Payload length: %d\n", message->payloadlen );
    if (message->payloadlen > 0) {
        debug_print( "Topic [%s] Data [%s]\n", (char *) message->topic, (char *) message->payload);
        
    } else {
        debug_print("Topic [%s] with no payload\n", message->topic );
    }
}


//
//      Needs to be of type 
// void mosquitto_connect_callback_set(struct mosquitto *mosq, void (*on_connect)(void *, int));
 
void my_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
    debug_print( "MQTT Connection Callback. Result: %d\n", result );

    if(!result){
        /* Subscribe to broker information topics on successful connect. */
        mosquitto_subscribe( mosq, NULL, "$SYS/#", 2 );
        
    } else {
        fprintf(stderr, "Connect failed\n");
    }
}

void my_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
    int i;

    debug_print("Subscribed (mid: %d): %d", mid, granted_qos[0]);
    for(i=1; i<qos_count; i++){
        debug_print(", %d", granted_qos[i]);
    }
    debug_print("\n", 0 );
}

void my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
    /* Print all log messages regardless of level. */
    debug_print("MQTT Logging: [%s]\n", str );
}

// ---------------------------------------------------------------------------
void    MQTT_setDefaults (HomeHeartBeatSystem_t *aSystem, char *brokerHostName)
{
    assert( brokerHostName != NULL );
    strncpy( aSystem->mqttBrokerHost, brokerHostName, sizeof aSystem->mqttBrokerHost );
    if (aSystem->mqttPortNumber == 0)
        aSystem->mqttPortNumber = 1883;
    if (aSystem->mqttKeepAliveValue == 0)
        aSystem->mqttKeepAliveValue = 60;
}
// ----------------------------------------------------------------------------
void    MQTT_initialize (HomeHeartBeatSystem_t *aSystem)
{
    char id[30];
    int i;
    //char *host = "192.168.0.11";
    //int port = 1883;
    //int keepalive = 60;
    bool clean_session = true;

    //
    // If values aren't set - use the defaults
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
        perror( "Unable to instantiate an MQTT instance!" );
        debug_print( "MQTT Broker Host: [%s], Port: %d, KeepAlive: %d\n", aSystem->mqttBrokerHost, aSystem->mqttPortNumber,aSystem->mqttKeepAliveValue );
        return;
    }
    
    mosquitto_log_callback_set( mosq, my_log_callback );

    mosquitto_connect_callback_set( mosq, my_connect_callback );
    mosquitto_message_callback_set( mosq, my_message_callback );
    mosquitto_subscribe_callback_set( mosq, my_subscribe_callback );

    if (mosquitto_connect(mosq, aSystem->mqttBrokerHost, aSystem->mqttPortNumber,aSystem->mqttKeepAliveValue ) ) {
        perror( "Unable to connect MQTT to broker!" );
        debug_print( "MQTT Broker Host: [%s], Port: %d, KeepAlive: %d\n", aSystem->mqttBrokerHost, aSystem->mqttPortNumber,aSystem->mqttKeepAliveValue );
        return;
    }

}

// ----------------------------------------------------------------------------
void    MQTT_teardown()
{
    mosquitto_destroy( mosq );
       mosquitto_lib_cleanup();
}

// ----------------------------------------------------------------------------
int MQTT_SendReceive ()
{
    int error = mosquitto_loop( mosq, -1, 0 );
    return error;
}

// -----------------------------------------------------------------------------
void    MQTT_CreateDeviceAlarm( HomeHeartBeatDevice_t *deviceRecPtr )
{
    //
    // We've noticed that the state has changed on one of our devices - send a new message!
    
    char    payload[ 1024 ];
    memset( payload, '\0', sizeof payload );
    int     length = 0;
    char    *topic = "HHB";
    
    //
    // What I send depends on the device type...
    if (deviceRecPtr->deviceType == DT_OPEN_CLOSE_SENSOR) {
        
        //
        // Are we open and alarm on open?
        if (deviceRecPtr->ocSensor->isOpen && deviceRecPtr->ocSensor->alarmOnOpen)
            length = snprintf( payload, sizeof payload, "*HHB ALARM* OPEN-CLOSE SENSOR, Name %s, State: OPEN, Duration: %d",
                    deviceRecPtr->deviceName, deviceRecPtr->deviceStateTimer );
                    
        //
        // Are we Closed and alarm on closed?
        if (!deviceRecPtr->ocSensor->isOpen && deviceRecPtr->ocSensor->alarmOnClose)
            length = snprintf( payload, sizeof payload, "*HHB ALARM* OPEN-CLOSE SENSOR, Name %s, State: CLOSED, Duration: %d",
                    deviceRecPtr->deviceName, deviceRecPtr->deviceStateTimer );

        if (length > 0)
            topic = "HHB/ALARM/OCSENSOR";

    } if (deviceRecPtr->deviceType == DT_MOTION_SENSOR) {
        
        //
        // Are we moving and alarm on motion?
        if (deviceRecPtr->motSensor->motionDetected && deviceRecPtr->motSensor->alarmOnMotion)
            length = snprintf( payload, sizeof payload, "*HHB ALARM* MOTION SENSOR, Name %s, State: MOTION, Duration: %d",
                    deviceRecPtr->deviceName, deviceRecPtr->deviceStateTimer );
                    
        //
        // Are we not moving and alarm on no motion?
        if (!deviceRecPtr->motSensor->motionDetected && deviceRecPtr->motSensor->alarmOnNoMotion)
            length = snprintf( payload, sizeof payload, "*HHB ALARM* MOTION SENSOR, Name %s, State: NO MOTION, Duration: %d",
                    deviceRecPtr->deviceName, deviceRecPtr->deviceStateTimer );

        if (length > 0)
            topic = "HHB/ALARM/MOTIONSENSOR";
    }
    
    if (payload[ 0 ] == '\0')
        return;
      
    int     messageID;
    int     QoS = 0;

    int result = mosquitto_publish( mosq, &messageID, topic, length, payload, QoS, false );
}


// -----------------------------------------------------------------------------
void    MQTT_CreateDeviceEvent( HomeHeartBeatDevice_t *deviceRecPtr )
{
    
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
    
    int length = 0;
    char    payload[ 1024 ];
    int     messageID;
    char    *topic = "HHB/STATUS";

    memset( payload, '\0', sizeof payload );


    if (deviceRecPtr->deviceType == DT_OPEN_CLOSE_SENSOR) {
        length = snprintf( payload, sizeof payload, "*HHB STATUS* OPEN-CLOSE SENSOR, Name %s, State: %s, Duration: %d. Alive: %d, Alarm On Open: %s, Alerts: %d",
                    deviceRecPtr->deviceName, 
                    (deviceRecPtr->ocSensor->isOpen ? "OPEN" : "CLOSED" ),
                    deviceRecPtr->deviceStateTimer,
                    deviceRecPtr->aliveUpdateTimer,
                    (deviceRecPtr->ocSensor->alarmOnOpen ? "YES" : "NO" ),
                    deviceRecPtr->deviceAlerts );
                   

    } else if (deviceRecPtr->deviceType == DT_WATER_LEAK_SENSOR) {
        length = snprintf( payload, sizeof payload, "*HHB STATUS* WATER LEAK SENSOR, Name %s, State: %s, Duration: %d. Alive: %d, Alarm On Wet: %s, Alerts: %d",
                    deviceRecPtr->deviceName, 
                    (deviceRecPtr->deviceState == 1 ? "DRY" : "WET" ),
                    deviceRecPtr->deviceStateTimer,
                    deviceRecPtr->aliveUpdateTimer,
                    "YES",
                    deviceRecPtr->deviceAlerts );
        
    } else if (deviceRecPtr->deviceType == DT_MOTION_SENSOR) {
        length = snprintf( payload, sizeof payload, "*HHB STATUS* MOTION SENSOR, Name %s, State: %s, Duration: %d. Alive: %d, Alarm On Motion: %s, Alerts: %d",
                    deviceRecPtr->deviceName, 
                    (deviceRecPtr->motSensor->motionDetected ? "MOTION" : "NO MOTION"),
                    deviceRecPtr->deviceStateTimer,
                    deviceRecPtr->aliveUpdateTimer,
                    (deviceRecPtr->motSensor->alarmOnMotion ? "YES" : "NO" ),
                    deviceRecPtr->deviceAlerts );
    } else {
        debug_print( "Unknown device type status: type: %d, name: [%s]\n", deviceRecPtr->deviceType, deviceRecPtr->deviceName );
    }
    
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
    
    int     QoS = 0;
    int     result = 0;
    if (length > 0)
        result = mosquitto_publish( mosq, &messageID, topic, length, payload, QoS, false );
    
    if (result != 0)
        warnAndKeepGoing( "MQTT Publish. Non zero result\n" );
}
