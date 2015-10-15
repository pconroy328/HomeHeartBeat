/* 
 * File:   tiltsensor.c
 * Author: patrick conroy
 *
 * Created on November 4, 2013, 10:03 AM
 * 
 * Routines related to a Eaton Home Heartbeat Open/Close Sensor device
 * (C) 2013 Patrick Conroy
 */


#include <string.h>
#include <stdlib.h>
#include <assert.h>


#include "homeheartbeat.h"
#include "logger.h"
#include "helpers.h"
#include "tiltsensor.h"
#include "utlist.h"


// -----------------------------------------------------------------------------
//
// Forward Declarations
static TiltSensor_t      *TiltSensor_newTSSensorRecord( void );

static  HomeHeartBeatDevice_t   *findThisDevice( HomeHeartBeatDevice_t *deviceListHead, char *macAddress );
static  HomeHeartBeatDevice_t   *TiltSensor_newDeviceRecord( TiltSensor_t *newGDRec );


static  int             TiltSensor_getOpenCloseStateFromInt( int sensorState );
static  int             TiltSensor_getOpenCloseState( char *token );
static  int             TiltSensor_parseDeviceConfiguration( char *token );
static  int             TiltSensor_getAlarmEnabledCondition1( int dcValue );
static  int             TiltSensor_getAlarmEnabledCondition2( int dcValue );
static  int             TiltSensor_getCallMeEnabledCondition1( int dcValue );
static  int             TiltSensor_getCallMeEnabledCondition2( int dcValue );

/* Garage Door Sensor
Garage Left
Garage Right
Attic Hatch
Beverage Cooler
Box
Chest
Container
Hatch
Hope/Cedar Chest
Jewelry Box
Oven Door
Pet Door
Roll Top Desk
Service Hatch
Strongbox
Toy Chest
Tool Box
Tilt Sensor
Tilt Window
Trunk
Laptop PC */
static  char    *deviceNames[] = { 
};



static  char    sensorRecordDumpBuffer[ 8192 ];

// ----------------------------------------------------------------------------
char    *TiltSensor_dumpSensorDeviceRecord (HomeHeartBeatDevice_t *deviceRecPtr)
{
    assert( deviceRecPtr != NULL );
    
    //
    // Always handy to have a way to dump am entire sensor record
    memset( sensorRecordDumpBuffer, '\0', sizeof sensorRecordDumpBuffer );
    snprintf( sensorRecordDumpBuffer,
             sizeof sensorRecordDumpBuffer,
"TiltSensor: Name [%s] MAC [%s] State [%s] Duration: %d secs Stated Changed [%s] \
Alarm On Open [%s]  Alarm On Close [%s]  Call On Open [%s]  Call on Close [%s] \
Alive Update Timer: %d Pending Update Timer: %d Alerts: %d  Alarm Triggered: %d   OffLine: %d   Low Battery: %d Name Index: %d   HHB Name [%s] State rec ID: %d  Zigbee Binding ID: %d\n \
Update Flags: %d, Configuration: %d",
            deviceRecPtr->deviceName, deviceRecPtr->macAddress,
            (deviceRecPtr->tsSensor->isOpen ? "OPEN" : "CLOSED"), deviceRecPtr->deviceStateTimer,
            (deviceRecPtr->stateHasChanged ? "YES" : "NO"),
            (deviceRecPtr->tsSensor->alarmOnOpen ? "YES" : "NO"), (deviceRecPtr->tsSensor->alarmOnClose ? "YES" : "NO"),
            (deviceRecPtr->tsSensor->callOnOpen ? "YES" : "NO"), (deviceRecPtr->tsSensor->callOnClose ? "YES" : "NO"),
            deviceRecPtr->aliveUpdateTimer, deviceRecPtr->pendingUpdateTimer,
            deviceRecPtr->deviceAlerts, deviceRecPtr->deviceInAlarm, deviceRecPtr->deviceOffLine, deviceRecPtr->deviceLowBattery,
            deviceRecPtr->deviceNameIndex, deviceNames[ deviceRecPtr->deviceNameIndex ], deviceRecPtr->stateRecordID, deviceRecPtr->zigbeeBindingID,
            deviceRecPtr->updateFlags, deviceRecPtr->deviceConfiguration
            );
    
    return &sensorRecordDumpBuffer[ 0 ];
}

// -----------------------------------------------------------------------------
static
TiltSensor_t  *TiltSensor_newTSSensorRecord ()           
{
    TiltSensor_t   *recPtr = NULL;
    
    //
    //  We've discovered a new Open Close sensor attached to our system
    //  Create a new record for it
    //
    recPtr = malloc( sizeof ( TiltSensor_t ) );
    if (recPtr == NULL) {
        Logger_LogFatal( "Insufficient memory to allocate space for an Tilt Sensor!\n" );
    }
    
    return recPtr;
}

//------------------------------------------------------------------------------
void    TiltSensor_parseOneStateRecord (HomeHeartBeatDevice_t *deviceRecPtr )
{
    Logger_FunctionStart();
    
    assert( deviceRecPtr != NULL );

    //
    // Do we have an existing sensor attached?
    if (deviceRecPtr->tsSensor == NULL) {
        // Start with a new Sensor record
        deviceRecPtr->tsSensor = TiltSensor_newTSSensorRecord();
        assert( deviceRecPtr->tsSensor != NULL );
        
   } else {
        Logger_LogDebug( "Found this Tilt Door Sensor in the list. Just going to update the values\n", 0 );
    }
    
    //
    // Set Values in the subclass - tsSensor
    assert( deviceRecPtr->tsSensor != NULL );
    
    //
    //  Now take the Device Configuration field and figure out the four settings:
    //  Alarm On Open, Alarm On Close, Call on Open, Call on Close
    //
    deviceRecPtr->tsSensor->alarmOnOpen = TiltSensor_getAlarmEnabledCondition2( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->tsSensor->alarmOnClose = TiltSensor_getAlarmEnabledCondition1( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->tsSensor->callOnOpen =  TiltSensor_getCallMeEnabledCondition2( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->tsSensor->callOnClose =  TiltSensor_getCallMeEnabledCondition1( deviceRecPtr->deviceConfiguration );
    
    
    //
    //  Are we open or closed?
    deviceRecPtr->tsSensor->currentState = TiltSensor_getOpenCloseStateFromInt( deviceRecPtr->deviceState );
    deviceRecPtr->tsSensor->isOpen = (deviceRecPtr->tsSensor->currentState == tsOpen);
    


    
    //
    //  Now I want to check to see if we should alarm.  You may wonder why I don't just check Bit 0 of field 7?
    //  Because the device stays in alarm until the system decides to clear it. And that's not the behavior I want.
    //  I want to alarm once and then not keep alarming!  At least this is what I think I want... :)
    
    //
    // Now - check to see if we've changed state?
    //
    
    //
    //  Let's assume it has NOT changed and reset it...
    deviceRecPtr->stateHasChanged = FALSE;

    //
    // Is the current state different? Then yes - the state has changed
    if (deviceRecPtr->tsSensor->currentState != deviceRecPtr->lastDeviceState) {
        deviceRecPtr->stateHasChanged = TRUE;
        Logger_LogDebug( "Detected state change on the device: %s. current state: %d, last state: %d\n", 
                deviceRecPtr->macAddress,
                deviceRecPtr->tsSensor->currentState, deviceRecPtr->lastDeviceState);
    }
    
    //
    //  Or has the state timer gotten smaller (or stayed the same)?  Then we had a fast state 
    //  change between poll intervals. Eg. Closed -> Opened -> Closed.   Well it turns out we cannot use "<=" because
    //  the resolution of the timer changes to minutes after 60 seconds
    if (deviceRecPtr->deviceStateTimer < deviceRecPtr->lastDeviceStateTimer) {
        deviceRecPtr->stateHasChanged = TRUE;
        Logger_LogDebug( "Detected TIME BASED state change on the device: %s. current Time: %d, last time: %d\n", deviceRecPtr->macAddress,
                deviceRecPtr->deviceStateTimer, deviceRecPtr->lastDeviceStateTimer);
    }

    //Logger_LogDebug( ">>>>>>>>>>>> state stateChaned: %d, current Timer: %d, lastTimer: %d\n", deviceRecPtr->stateHasChanged,
    //                deviceRecPtr->deviceStateTimer, deviceRecPtr->lastDeviceStateTimer);

    //
    //  Ok we've marked that the state has changed. Reset the counters so the next time thru
    //  we can recheck!
    deviceRecPtr->lastDeviceState = deviceRecPtr->tsSensor->currentState;
    deviceRecPtr->lastDeviceStateTimer = deviceRecPtr->deviceStateTimer;
            
    Logger_LogDebug( "After parse. \nTilt Sensor: %s\n\n", TiltSensor_dumpSensorDeviceRecord( deviceRecPtr ) );
}


//------------------------------------------------------------------------------
static
int     TiltSensor_getOpenCloseStateFromInt (int sensorState)
{
    int isClosed    = (sensorState & ISCLOSED_BITMASK);
    int isOpen      = (sensorState & ISOPEN_BITMASK);

    // They'd better not BOTH be on!
    if (isClosed && isOpen) {
        // Logger_LogWarning( "Tilt Sensor reporting both open and closed!\n" );
        ;
        
    } else {

        if (isClosed) {
            // Logger_LogDebug( "Sensor is CLOSED!\n", 0 );
            return tsClosed;
        } else if (isOpen) {
            // Logger_LogDebug( "Sensor is OPEN!\n", 0 );
            return tsOpen;
        } else {
            Logger_LogError( "Tilt Sensor reporting neither Open nor Closed!\n", 0 );
            return tsUnknown;
        }
    }
    
    return tsUnknown;
}

//------------------------------------------------------------------------------
static
int     TiltSensor_getOpenCloseState (char *token)
{
    assert( token != NULL );
    int tmpValue = hexStringToInt( token );
    return tmpValue;
}


//------------------------------------------------------------------------------
static
int TiltSensor_parseDeviceConfiguration (char *token)
{
    assert( token != NULL );

    /*
         Sensor Alarms – Each sensor can be configured to alarm on either of two different conditions:

        Bit 0 (0x0001)  -  Alarm enabled on condition #1
        Bit 1 (0x0002)  -  Alarm enabled on condition #2

        Only one or the other of the bits may be set at a time. When the sensor’s state matches the selected 
        alarm condition, an alarm is generated on the Home Key and bit 0 of field 7 will be set to true.

        Sensor Call me – Each sensor can be configured for “Call me” on either of two conditions:

        Bit 8 (0x0100)  -  Call me enabled on condition #1
        Bit 9 (0x0200)  -  Call me enabled on condition #2    
      */
    int tmpValue = hexStringToInt( token );
    return tmpValue;
}

//------------------------------------------------------------------------------
static
int     TiltSensor_getAlarmEnabledCondition1 (int dcValue)
{
    //int     alarmEnabledCondition2 = (dcValue & 0x0002);
    //int     callMeEnabledCondition1 = (dcValue & 0x0100);
    //int     callMeEnabledCondition2 = (dcValue & 0x0200);

    
    //Logger_LogDebug( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
    //        alarmEnabledCondition1, alarmEnabledCondition2, callMeEnabledCondition1, callMeEnabledCondition2 );

    return (dcValue & 0x0001);
}

//------------------------------------------------------------------------------
static
int     TiltSensor_getAlarmEnabledCondition2 (int dcValue)
{

    //Logger_LogDebug( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
    //        alarmEnabledCondition1, alarmEnabledCondition2, callMeEnabledCondition1, callMeEnabledCondition2 );
    return (dcValue & 0x0002);
}

//------------------------------------------------------------------------------
static
int     TiltSensor_getCallMeEnabledCondition1 (int dcValue)
{
    return (dcValue & 0x0100);
}

//------------------------------------------------------------------------------
static
int     TiltSensor_getCallMeEnabledCondition2 (int dcValue)
{
    return (dcValue & 0x0200);
}

