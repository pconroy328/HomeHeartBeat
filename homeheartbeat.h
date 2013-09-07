/* 
 * File:   homeheartbeat.h
 * Author: pconroy
 *
 * Created on August 29, 2013, 9:00 AM
 */

#ifndef HOMEHEARTBEAT_H
#define	HOMEHEARTBEAT_H


#ifdef	__cplusplus
extern "C" {
#endif


    
/*    
#ifndef     FALSE
# define    FALSE   (0)
# define    TRUE    (!FALSE)    
#endif
  */
    
    
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
// Common to all sensors, I hope
#define MAX_DEVICE_NAME_LEN     80              // Just taking a conservative guess

    
    
// -----------------------------------------------------------------------------
//  Definitions relevant to all devices
//
//  Device Alerts (field 7)
//    
#define     ALARMTRIGGERED_BITMASK  (0x01)
#define     DEVICEOFFLINE_BITMASK   (0x02)
#define     LOWBATTERY_BITMASK      (0x04)

//
// Device Configuration BitMasks (field 9))
#define     ALARMONOPEN_BITMASK         (0x0001)
#define     ALARMONCLOSE_BITMASK        (0x0002)
#define     CALLONOPEN_BITMASK          (0x0100)
#define     CALLONCLOSE_BITMASK         (0x0200)
typedef enum    deviceAlert { AlarmTriggered, DeviceOffline, LowBattery, daUnknown } deviceAlert_t;


#include "openclose_sensor.h"
#include "waterleak_sensor.h"



// -----------------------------------------------------------------------------
//
//  There are times when you really wish you could do some OO things in 'C' easier.
//  This is the Device Superclaass.  All Eaton Home Heartbeat devices are of this
//  type.
//
typedef  struct  HomeHeartBeatDevice {
    int         deviceType;                             // tell me which union member to use

        
    //
    // every device sends back 17 fields of data - the last two are strings, the first 15 are ints
    int         deviceRawData[ 15 ];
    
    char        *macAddress;                        // field 16
    char        *deviceName;                        // field 17
    
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
    };
    
    struct      HomeHeartBeatDevice     *next;          // UTLIST.H needs this to be called "next" don't chaneg it
    
} HomeHeartBeatDevice_t;


// -----------------------------------------------------------------------------
//
//  A system consists of the Base Station, Modem (in the base station), the Home
//  key and all of the devices
//
//  I need to manage a collection of devices.  Feel free to make this an array.
//  But there are a collection of Linked List utilities from a fellow named
//  Troy D. Hanson (see: http://troydhanson.github.io/uthash/utlist.html).
//  I think I'll use his implementation of Linked Lists.  It's cool - he's done
//  it all thru 'C' Macros!  Scoll down to the bottom of this source code file
//  for a little documentation on his implementation.

#include "utlist.h"

typedef struct  HomeHeartBeatSystem {
    int     systemID;                                   // use this as you see fit
    char    *name;                                      // give this system a name, maybe you have > 1
    
    char    *addressLine1;                              // where your system is located
    char    *addressLin2;                               //      sorry for the N.A. centrc
    char    *city;                                      //      approach to locations
    char    *stateOrProvinceCode;
    char    *postalCode;
    
    float   latitude;
    float   longitude;
    long    timeZoneSecs;                               // difference in seconds between UTC and *here*
    
    //
    // Now the collection of devices is a linked list, so we need a 'head' pointer
    HomeHeartBeatDevice_t   *deviceListHead;
    
    //
    // Port specifics
    char    *portName;                                  // e.g. /dev/hhb or /dev/ttyUSB0
    int     hhbFD;                                      // file descriptor
    int     portOpen;
    int     pollForEvents;
    int     sleepBetweenEventLoops;
    
    //
    // Database Specific Information
    int     logEventsToDatabase;
    char    *databaseHost;
    char    *databaseUserName;
    char    *databasePassword;
    char    *databaseSchema;
    
    //
    // MQTT Specific Information
    int     logEventsToMQTT;
    char    *mqttBrokerHost;
    int     mqttPortNumber;
    int     mqttKeepAliveValue;
} HomeHeartBeatSystem_t;



//
// Home Heart Beat System Function Declarations 
extern  void        homeHeartBeatSystem_Initialize( void );
extern  void        homeHeartBeatSystem_SetPortName( char * );
extern  void        homeHeartBeatSystem_OpenPort( char * );
extern  void        homeHeartBeatSystem_eventLoop( void );
extern  void        homeHeartBeatSystem_Shutdown( void );



#ifdef	__cplusplus
}
#endif

#endif	/* HOMEHEARTBEAT_H */

