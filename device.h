/* 
 * File:   device.h
 * Author: pconroy
 *
 * Created on September 9, 2013, 10:28 AM
 * 
 *  From our perspective, Home HeartBeat devices include:
 *      Base Station, Home Key, Open/Closed Sensor, Power Sensor
 *      Water Sensor, Reminder Sensor, Attention Sensor, Base Station Modem
 *      Motion Sensor, Tilt Sensor (aka Garage Door Sensor)
 * 
 *  All of these devices have some thingn in common and that will be captured 
 *  here
 */

#ifndef DEVICE_H
#define	DEVICE_H

#ifdef	__cplusplus
extern "C" {
#endif


    

    
    
// -----------------------------------------------------------------------------
//  Definitions relevant to all devices
//
// What's a good maximum value for the length of a device name?
#define MAX_DEVICE_NAME_LEN     80              // Just taking a conservative guess
#define MAX_MAC_ADDRESS_SIZE    18              // sixteen plus room for nulls
#define MAX_DEVICES_IN_SYSTEM   32

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

    
//
// Device Configuration BitMasks (field 9))
#define     ALARMONOPEN_BITMASK         (0x0001)
#define     ALARMONCLOSE_BITMASK        (0x0002)
#define     CALLONOPEN_BITMASK          (0x0100)
#define     CALLONCLOSE_BITMASK         (0x0200)
typedef enum    deviceAlert { AlarmTriggered, DeviceOffline, LowBattery, daUnknown } deviceAlert_t;




#include "openclose_sensor.h"
#include "waterleak_sensor.h"
#include "motion_sensor.h"


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
    };
    


    //  I need to manage a collection of devices.  Feel free to make this an array.
    //  But there are a collection of Linked List utilities from a fellow named
    //  Troy D. Hanson (see: http://troydhanson.github.io/uthash/utlist.html).
    //  I think I'll use his implementation of Linked Lists.  It's cool - he's done
    //  it all thru 'C' Macros!  Scoll down to the bottom of this source code file
    //  for a little documentation on his implementation.
    struct      HomeHeartBeatDevice     *next;          // UTLIST.H needs this to be called "next" don't chaneg it
    
} HomeHeartBeatDevice_t;


//
//
extern  char    *Device_dumpDeviceRecord (HomeHeartBeatDevice_t *deviceRecPtr);

extern  int     Device_parseStateRecordID (char *token);
extern  int     Device_parseZigbeeBindingID (char *token);
extern  int     Device_parseDeviceState( char *token );
extern  int     Device_parseDeviceCapabilties (char *token);
extern  int     Device_parseDeviceType (char *token);
//extern  int     Device_parseDeviceState( char *token, int deviceType );
extern  int     Device_parseDeviceStateTimer (char *token);
extern  int     Device_parseDeviceAlerts (char *token);
extern  int     Device_parseDeviceNameIndex( char * );
extern  int     Device_deviceAliveUpdateTimer( char * );
extern  char    *Device_parseMacAddress( char *token );
extern  char    *Device_parseDeviceName( char *token );

extern  HomeHeartBeatDevice_t   *Device_newDeviceRecord (char *macAddress);
extern  HomeHeartBeatDevice_t   *Device_findThisDevice (HomeHeartBeatDevice_t *deviceListHead, char *macAddress);


#ifdef	__cplusplus
}
#endif

#endif	/* DEVICE_H */

