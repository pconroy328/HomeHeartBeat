/* 
 * File:   tiltsensor.c
 * Author: patrick conroy
 *
 * Created on November 4, 2013, 10:03 AM
 * 
 * Routines related to a Eaton Home Heartbeat PowerOn/PowerOff Sensor device
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
static PowerSensor_t      *PowerSensor_newPSensorRecord( void );

static  HomeHeartBeatDevice_t   *findThisDevice( HomeHeartBeatDevice_t *deviceListHead, char *macAddress );
static  HomeHeartBeatDevice_t   *PowerSensor_newDeviceRecord( PowerSensor_t *newGDRec );


static  int             PowerSensor_getPowerOnPowerOffStateFromInt( int sensorState );
static  int             PowerSensor_getPowerOnPowerOffState( char *token );
static  int             PowerSensor_parseDeviceConfiguration( char *token );
static  int             PowerSensor_getAlarmEnabledCondition1( int dcValue );
static  int             PowerSensor_getAlarmEnabledCondition2( int dcValue );
static  int             PowerSensor_getCallMeEnabledCondition1( int dcValue );
static  int             PowerSensor_getCallMeEnabledCondition2( int dcValue );

static  char    *deviceNames[] = { 
};



static  char    sensorRecordDumpBuffer[ 8192 ];

// ----------------------------------------------------------------------------
char    *PowerSensor_dumpSensorDeviceRecord (HomeHeartBeatDevice_t *deviceRecPtr)
{
    assert( deviceRecPtr != NULL );
    
    //
    // Always handy to have a way to dump am entire sensor record
    memset( sensorRecordDumpBuffer, '\0', sizeof sensorRecordDumpBuffer );
    snprintf( sensorRecordDumpBuffer,
             sizeof sensorRecordDumpBuffer,
"PowerSensor: Name [%s] MAC [%s] State [%s] Duration: %d secs Stated Changed [%s] \
Alarm On PowerOn [%s]  Alarm On PowerOff [%s]  Call On PowerOn [%s]  Call on PowerOff [%s] \
Alive Update Timer: %d Pending Update Timer: %d Alerts: %d  Alarm Triggered: %d   OffLine: %d   Low Battery: %d Name Index: %d   HHB Name [%s] State rec ID: %d  Zigbee Binding ID: %d\n \
Update Flags: %d, Configuration: %d",
            deviceRecPtr->deviceName, deviceRecPtr->macAddress,
            (deviceRecPtr->psSensor->isPowerOn ? "ON" : "OFF"), deviceRecPtr->deviceStateTimer,
            (deviceRecPtr->stateHasChanged ? "YES" : "NO"),
            (deviceRecPtr->psSensor->alarmOnPowerOn ? "YES" : "NO"), (deviceRecPtr->psSensor->alarmOnPowerOff ? "YES" : "NO"),
            (deviceRecPtr->psSensor->callOnPowerOn ? "YES" : "NO"), (deviceRecPtr->psSensor->callOnPowerOff ? "YES" : "NO"),
            deviceRecPtr->aliveUpdateTimer, deviceRecPtr->pendingUpdateTimer,
            deviceRecPtr->deviceAlerts, deviceRecPtr->deviceInAlarm, deviceRecPtr->deviceOffLine, deviceRecPtr->deviceLowBattery,
            deviceRecPtr->deviceNameIndex, deviceNames[ deviceRecPtr->deviceNameIndex ], deviceRecPtr->stateRecordID, deviceRecPtr->zigbeeBindingID,
            deviceRecPtr->updateFlags, deviceRecPtr->deviceConfiguration
            );
    
    return &sensorRecordDumpBuffer[ 0 ];
}

// -----------------------------------------------------------------------------
static
PowerSensor_t  *PowerSensor_newTSSensorRecord ()           
{
    PowerSensor_t   *recPtr = NULL;
    
    //
    //  We've discovered a new PowerOn PowerOff sensor attached to our system
    //  Create a new record for it
    //
    recPtr = malloc( sizeof ( PowerSensor_t ) );
    if (recPtr == NULL) {
        Logger_LogFatal( "Insufficient memory to allocate space for an Power Sensor!\n" );
    }
    
    return recPtr;
}

//------------------------------------------------------------------------------
void    PowerSensor_parseOneStateRecord (HomeHeartBeatDevice_t *deviceRecPtr )
{
    Logger_FunctionStart();
    
    assert( deviceRecPtr != NULL );

    //
    // Do we have an existing sensor attached?
    if (deviceRecPtr->psSensor == NULL) {
        // Start with a new Sensor record
        deviceRecPtr->psSensor = PowerSensor_newTSSensorRecord();
        assert( deviceRecPtr->psSensor != NULL );
        
   } else {
        Logger_LogDebug( "Found this Power Door Sensor in the list. Just going to update the values\n", 0 );
    }
    
    //
    // Set Values in the subclass - psSensor
    assert( deviceRecPtr->psSensor != NULL );
    
    //
    //  Now take the Device Configuration field and figure out the four settings:
    //  Alarm On PowerOn, Alarm On PowerOff, Call on PowerOn, Call on PowerOff
    //
    deviceRecPtr->psSensor->alarmOnPowerOn = PowerSensor_getAlarmEnabledCondition2( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->psSensor->alarmOnPowerOff = PowerSensor_getAlarmEnabledCondition1( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->psSensor->callOnPowerOn =  PowerSensor_getCallMeEnabledCondition2( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->psSensor->callOnPowerOff =  PowerSensor_getCallMeEnabledCondition1( deviceRecPtr->deviceConfiguration );
    
    
    //
    //  Are we open or closed?
    deviceRecPtr->psSensor->currentState = PowerSensor_getPowerOnPowerOffStateFromInt( deviceRecPtr->deviceState );
    deviceRecPtr->psSensor->isPowerOn = (deviceRecPtr->psSensor->currentState == psPowerOn);
    


    
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
    if (deviceRecPtr->psSensor->currentState != deviceRecPtr->lastDeviceState) {
        deviceRecPtr->stateHasChanged = TRUE;
        Logger_LogDebug( "Detected state change on the device: %s. current state: %d, last state: %d\n", 
                deviceRecPtr->macAddress,
                deviceRecPtr->psSensor->currentState, deviceRecPtr->lastDeviceState);
    }
    
    //
    //  Or has the state timer gotten smaller (or stayed the same)?  Then we had a fast state 
    //  change between poll intervals. Eg. PowerOffd -> PowerOned -> PowerOffd.   Well it turns out we cannot use "<=" because
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
    deviceRecPtr->lastDeviceState = deviceRecPtr->psSensor->currentState;
    deviceRecPtr->lastDeviceStateTimer = deviceRecPtr->deviceStateTimer;
            
    Logger_LogDebug( "After parse. \nPower Sensor: %s\n\n", PowerSensor_dumpSensorDeviceRecord( deviceRecPtr ) );
}


//------------------------------------------------------------------------------
static
int     PowerSensor_getPowerOnPowerOffStateFromInt (int sensorState)
{
    int isPowerOff     = (sensorState & ISOFF_BITMASK);
    int isPowerOn      = (sensorState & ISON_BITMASK);

    //  
    // They'd better not BOTH be on! - but if the device is offline, they could be
    //
    if (isPowerOff && isPowerOn) {
        // Logger_LogWarning( "Power Sensor reporting both On and Off!\n" );
        ;
        
    } else {

        if (isPowerOff) {
            // Logger_LogDebug( "Sensor is CLOSED!\n", 0 );
            return psPowerOff;
        } else if (isPowerOn) {
            // Logger_LogDebug( "Sensor is OPEN!\n", 0 );
            return psPowerOn;
        } else {
            Logger_LogError( "Power Sensor reporting neither PowerOn nor PowerOff!\n", 0 );
            return psUnknown;
        }
    }
    
    return tsUnknown;
}

//------------------------------------------------------------------------------
static
int     PowerSensor_getPowerOnPowerOffState (char *token)
{
    assert( token != NULL );
    int tmpValue = hexStringToInt( token );
    return tmpValue;
}


//------------------------------------------------------------------------------
static
int PowerSensor_parseDeviceConfiguration (char *token)
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
int     PowerSensor_getAlarmEnabledCondition1 (int dcValue)
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
int     PowerSensor_getAlarmEnabledCondition2 (int dcValue)
{

    //Logger_LogDebug( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
    //        alarmEnabledCondition1, alarmEnabledCondition2, callMeEnabledCondition1, callMeEnabledCondition2 );
    return (dcValue & 0x0002);
}

//------------------------------------------------------------------------------
static
int     PowerSensor_getCallMeEnabledCondition1 (int dcValue)
{
    return (dcValue & 0x0100);
}

//------------------------------------------------------------------------------
static
int     PowerSensor_getCallMeEnabledCondition2 (int dcValue)
{
    return (dcValue & 0x0200);
}


