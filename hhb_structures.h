/* 
 * File:   hhb_structures.h
 * Author: pconroy
 *
 * Created on September 17, 2013, 10:03 AM
 */

#ifndef HHB_STRUCTURES_H
#define	HHB_STRUCTURES_H

#ifdef	__cplusplus
extern "C" {
#endif


        
// -----------------------------------------------------------------------------
//  Definitions relevant to all devices
//
// What's a good maximum value for the length of a device name?
#define MAX_DEVICE_NAME_LEN     80              // Just taking a conservative guess
#define MAX_MAC_ADDRESS_SIZE    18              // sixteen plus room for nulls
//#define MAX_DEVICES_IN_SYSTEM   32

//
//  When we issus an 's' command 17 tokens of data come back. Most are small but the
//  MAC address and Device Name look pretty good sized
#define NUM_TOKENS_PER_STATE_CMD    17
#define MAX_TOKEN_LENGTH            30

//  A typedef will make the function prototypes easier
typedef     char    TokenArray_t[ NUM_TOKENS_PER_STATE_CMD ][ MAX_TOKEN_LENGTH ];
    
    
    
// ----------------------------------------------------------------------------
//
//  Device Types (Field 4)
//
#define DT_BASE_STATION         0x0001
#define DT_HOME_KEY             0x0002
#define DT_OPEN_CLOSE_SENSOR    0x0003
#define DT_POWER_SENSOR         0x0004
#define DT_WATER_LEAK_SENSOR    0x0005
#define DT_REMINDER_DEVICE      0x0006
#define DT_ATTENTION_DEVICE     0x0007
#define DT_MODEM                0x0010
#define DT_MOTION_SENSOR        0x0017
#define DT_TILT_SENSOR          0x0018

    
    
//
//  Device Alerts (field 7)
//    
#define     ALARMTRIGGERED_BITMASK      (0x01)
#define     DEVICEOFFLINE_BITMASK       (0x02)
#define     LOWBATTERY_BITMASK          (0x04)
#define     BATTERYCHARGING_BITMASK     (0x08)
#define     ONBATTERYBACKUP_BITMASK     (0x20)

    
//
// Device Configuration BitMasks (field 9))
#define     ALARMONOPEN_BITMASK         (0x0001)
#define     ALARMONCLOSE_BITMASK        (0x0002)
#define     CALLONOPEN_BITMASK          (0x0100)
#define     CALLONCLOSE_BITMASK         (0x0200)

    
    
typedef struct  Database_Parameters {
    char    databaseHost[ 256 ];
    char    databaseUserName[ 256 ];
    char    databasePassword[ 256 ];
    char    databaseSchema[ 256 ];
    int     logAlarms;
    int     logStatus;
    int     logHistory;
    int     dropAlarmTable; 
    int     createAlarmTable;
    int     dropStatusTable; 
    int     createStatusTable;
    int     dropHistoryTable; 
    int     createHistoryTable;
    int     maxMinutesOfHistoryStored;
} Database_Parameters_t;    
    

#define     MQTT_BROKERHOSTNAME_LEN 256
#define     MQTT_TOPIC_LEN          256
#define     MQTT_CLIENTID_LEN       30

typedef struct  MQTT_Parameters {
    //
    // MQTT Specific Information
    int     logEventsToMQTT;
    char    brokerHostName[ MQTT_BROKERHOSTNAME_LEN ];
    char    clientID[ MQTT_CLIENTID_LEN ];
    int     portNumber;
    int     keepAliveValue;
    int     cleanSession;
    int     QoS;
    char    statusTopic[ MQTT_TOPIC_LEN ];
    char    alarmTopic[ MQTT_TOPIC_LEN ];
    int     retainMsgs;
    int     enableMQTTLoggingCallback;
    int     exitOnTooManyErrors;
    int     maxReconnectAttempts;
} MQTT_Parameters_t;





// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// 
//  A HomeHeartBeat device is any of the sensors that attach to the system
//

// ============================================================================
//
//  First up - the Motion Sensor!

// ----------------------------------------------------------------------------
//  Device State (Field 5) values specific to an Motion Sensor
//
#define     ISNOMOTION_BITMASK    (0x01)
#define     ISMOTION_BITMASK      (0x02)

typedef enum  motDeviceState { motMotion, motNoMotion, motUnknown } motDeviceState_t;

typedef struct  MotionSensor {
    motDeviceState_t    currentState;
    int             motionDetected;             // derived
    
    int             alarmOnMotion;
    int             alarmOnNoMotion;
    int             callOnMotion;  
    int             callOnNoMotion;
    
    int             motionDelayValueSecs;       // field 13
    
    char            *usersDeviceName[ MAX_DEVICE_NAME_LEN ];
} MotionSensor_t;

// ============================================================================
//
//  Next - the Open Close Sensor!

// ----------------------------------------------------------------------------
//  Device State (Field 5) values specific to an Open/Close Sensor
//
#define     ISOPEN_BITMASK      (0x02)
#define     ISCLOSED_BITMASK    (0x01)
typedef enum  ocDeviceState { ocOpen, ocClosed, ocUnknown } ocDeviceState_t;
    

typedef struct  OpenCloseSensor {
    ocDeviceState_t currentState;               // Field 5
    int             isOpen;                     // derived
    
    int             alarmOnOpen;                // Field 9
    int             alarmOnClose;               // Field 9
    int             callOnOpen;                 // Field 9
    int             callOnClose;                // Field 9
                                                    // ....5....0....5.
    char            usersDeviceName[ MAX_DEVICE_NAME_LEN ];        // Field 17
} OpenCloseSensor_t;


// ============================================================================
//
//  Next - the Water Leak Sensor!
// ----------------------------------------------------------------------------
//
#define     ISWET_BITMASK      (0x01)
#define     ISDRY_BITMASK      (0x02)
typedef enum  wlDeviceState { wlWet, wlDry, wlUnknown } wlDeviceState_t;


typedef struct  WaterLeakSensor {
    wlDeviceState_t currentState;               // Field 5   
    int             wetnessDetected;
    
    int             alarmOnWet;                 // Field 9
    int             alarmOnDry;                 // Field 9
    int             callOnWet;                  // Field 9
    int             callOnDry;                  // Field 9
    
    char            *usersDeviceName[ MAX_DEVICE_NAME_LEN ];        // Field 17
} WaterLeakSensor_t;



// ============================================================================
//
//  Next - the Tilt Sensor (aka Garage Door Sensor)!

// ----------------------------------------------------------------------------
//  Device State (Field 5) values specific to an Tilt Sensor
//
#define     ISOPEN_BITMASK      (0x02)
#define     ISCLOSED_BITMASK    (0x01)
typedef enum  tsDeviceState { tsOpen, tsClosed, tsUnknown } tsDeviceState_t;
    

typedef struct  TiltSensor {
    tsDeviceState_t currentState;               // Field 5
    int             isOpen;                     // derived
    
    int             alarmOnOpen;                // Field 9
    int             alarmOnClose;               // Field 9
    int             callOnOpen;                 // Field 9
    int             callOnClose;                // Field 9
                                                    // ....5....0....5.
    char            usersDeviceName[ MAX_DEVICE_NAME_LEN ];        // Field 17
} TiltSensor_t;


// ============================================================================
//
//  Next - the Power Sensor 

// ----------------------------------------------------------------------------
//
#define     ISON_BITMASK        (0x02)
#define     ISOFF_BITMASK       (0x01)
typedef enum  psDeviceState { psPowerOn, psPowerOff, psUnknown } psDeviceState_t;
    

typedef struct  PowerSensor {
    psDeviceState_t currentState;               // Field 5
    int             isPowerOn;                  // derived
    
    int             alarmOnPowerOn;             // Field 9
    int             alarmOnPowerOff;            // Field 9
    int             callOnPowerOn;              // Field 9
    int             callOnPowerOff;             // Field 9
                                                    // ....5....0....5.
    char            usersDeviceName[ MAX_DEVICE_NAME_LEN ];        // Field 17
} PowerSensor_t;


// -----------------------------------------------------------------------------
//
// Now the definition of any HHB device
typedef  struct  HomeHeartBeatDevice {
    int         deviceType;                             // tell me which union member to use

        
    //
    // every device sends back 17 fields of data - the last two are strings, the first 15 are ints
    int         deviceRawData[ 15 ];
    char        macAddress[ MAX_MAC_ADDRESS_SIZE ]; // field 16
    char        deviceName[ MAX_DEVICE_NAME_LEN ];  // field 17
    
    int         stateRecordID;                      // field 1
    int         zigbeeBindingID;                    // Field 2
    int         deviceCapabilities;                 // Field 3       
    int         deviceState;                        // Field 5
    int         deviceStateTimer;                   // Field 6
    int         deviceAlerts;                       // Field 7
    int         deviceNameIndex;                    // Field 8
    int         deviceConfiguration;                // Field 9
    int         aliveUpdateTimer;                   // Field 10
    int         updateFlags;                        // Field 11
    int         deviceParameter;                    // Field 13 
    int         pendingUpdateTimer;                 // Field 15

    int         deviceInAlarm;                      // bit in Field 7
    int         deviceOffLine;
    int         deviceLowBattery;
    int         deviceBatteryCharging;              // valid only for Home Key
    int         deviceOnBatteryBackup;              // valid only for Base Station

    
    //
    // Extra attributes that aren't coming from the HHB System but we'll find useful
    char        roomName[ MAX_DEVICE_NAME_LEN ];    // room where device is located
    char        alternateDeviceName[ MAX_DEVICE_NAME_LEN ];     // if you don't like the HHB names
    
    
    //
    //  It would help if we can just add a variable that indicates a state change
    //  Open to Closed or v.v., Wet to Dry, No Motion to Motion.  We'll do so by keeping the
    //  last state value.  That's the easy way to check.  But we'll also keep the last time value
    //  too.  If we see the "new state timer" is suddenly less that the last state timer (but the new state
    //  is the same as the old state) then what's probably happened is the device changed state twice
    //  but in between polling intervals.  For example, Closed -> Opened -> Closed quickly.
    int         stateHasChanged;
    int             lastDeviceState;
    int             lastDeviceStateTimer;

    
    union {
        OpenCloseSensor_t   *ocSensor;
        WaterLeakSensor_t   *wlSensor;
        MotionSensor_t      *motSensor;
        TiltSensor_t        *tsSensor;
        PowerSensor_t       *psSensor;                  // redundant, but pSensor would be a ptr
    };
    


    //  I need to manage a collection of devices.  Feel free to make this an array.
    //  But there are a collection of Linked List utilities from a fellow named
    //  Troy D. Hanson (see: http://troydhanson.github.io/uthash/utlist.html).
    //  I think I'll use his implementation of Linked Lists.  It's cool - he's done
    //  it all thru 'C' Macros!  Scoll down to the bottom of this source code file
    //  for a little documentation on his implementation.
    struct      HomeHeartBeatDevice     *next;          // UTLIST.H needs this to be called "next" don't chaneg it
    
} HomeHeartBeatDevice_t;


    
// -----------------------------------------------------------------------------
//
//  A system consists of the Base Station, Modem (in the base station), the Home
//  key and all of the devices
//

typedef struct  HomeHeartBeatSystem {
    int     systemID;                                   // use this as you see fit
    char    name[ 80 ];                                 // give this system a name, maybe you have > 1
    
    char    addressLine1[ 80 ];                         // where your system is located
    char    addressLine2[ 80 ];                         //      sorry for the N.A. centrc
    char    city[ 80 ];                                 //      approach to locations
    char    stateOrProvinceCode[ 80 ];
    char    postalCode[ 80 ];
    
    double  latitude;
    double  longitude;
    int     TZOffsetMins;                               // difference in minutes between UTC and *here*

    int     debugValue;                                 // if non-zero write debug data
    char    debugFileName[ 255 ];                       // to this file
    char    deviceInfoFileName[ 255 ];                  // file name with extra info on our devices
    
    //
    // Now the collection of devices is a linked list, so we need a 'head' pointer
    HomeHeartBeatDevice_t   *deviceListHead;
    // We tried an array before a LinkedList
    // HomeHeartBeatDevice_t   deviceArray[ MAX_DEVICES_IN_SYSTEM ];
    // int                     deviceArrayIndex;
    
    //
    // Port specifics
    char    portName[ 256 ];                            // e.g. /dev/hhb or /dev/ttyUSB0
    int     hhbFD;                                      // file descriptor
    int     portOpen;
    int     pollForEvents;
    int     sleepSecsBetweenEventLoops;
    
    //
    // Database Specific Information
    Database_Parameters_t   DBParameters;
    //
    // MQTT Specific Information
    int     logEventsToMQTT;
    MQTT_Parameters_t   MQTTParameters;
} HomeHeartBeatSystem_t;



#ifdef	__cplusplus
}
#endif

#endif	/* HHB_STRUCTURES_H */

