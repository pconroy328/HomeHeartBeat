/* 
 * File:   motion_sensor.c
 * Author: patrick conroy
 *
 * Created on September 17, 2013, 10:03 AM
 * 
 * Routines related to a Eaton Home Heartbeat Water Leak Sensor device
 * (C) 2013 Patrick Conroy
 */


#include <string.h>
#include <stdlib.h>
#include <assert.h>


#include "homeheartbeat.h"
#include "helpers.h"
#include "motion_sensor.h"
#include "logger.h"


static  char    *deviceNames[] = { 
  "Water Sensor",   "Basement Floor",   "Bathroom Floor",   "Shower",   "Sump Pump",
    "Toilet",       "Washing Machine",  "Water Heater",     "Attic",    "Bedroom",
    "Crawl Space",  "Dining Room",      "Garage",           "Laundry Room", "Living Room",
    "Loft",         "Mud Room",         "Storage Area",     "Bathroom Sink",    "Kitchen Sink",
    "Under Sink",   "Utility Sink",     "Cat Bowl",         "Dog Bowl",         "Fish Tank",
    "Fountain",     "Garden",           "Pool",             "Water Bowl",       "???"  }; 



// -----------------------------------------------------------------------------
//
// Forward Declarations
//static HomeHeartBeatDevice_t  *WaterLeak_updateWaterLeakDeviceRecord( HomeHeartBeatDevice_t *recPtr, int stateRecordID, int zigbeeBindingID);
static WaterLeakSensor_t      *WaterLeak_newWaterLeakSensorRecord( void );


static  int         WaterLeak_getWaterLeakStateFromInt( int sensorState );
static  int         WaterLeak_getWaterLeakState( char *token );
static  int         WaterLeak_parseDeviceConfiguration( char *token );
static  int         WaterLeak_getAlarmEnabledCondition1( int dcValue );
static  int         WaterLeak_getAlarmEnabledCondition2( int dcValue );
static  int         WaterLeak_getCallMeEnabledCondition1( int dcValue );
static  int         WaterLeak_getCallMeEnabledCondition2( int dcValue );
static  int         WaterLeak_parseDeviceParameter( int dpValue );




static  char    sensorRecordDumpBuffer[ 8192 ];

// ----------------------------------------------------------------------------
char    *Waterleak_dumpSensorDeviceRecord (HomeHeartBeatDevice_t *deviceRecPtr)
{
    assert( deviceRecPtr != NULL );
    
    //
    // Always handy to have a way to dump am entire sensor record
    memset( sensorRecordDumpBuffer, '\0', sizeof sensorRecordDumpBuffer );
    snprintf( sensorRecordDumpBuffer,
             sizeof sensorRecordDumpBuffer,
"WaterLeak Sensor: Name [%s] Mac [%s] State [%s]  Duration: %d secs Stated Changed [%s] \
Alarm On Wet [%s]  Alarm On Dry [%s] Call On Wet [%s]  Call on  [%s] \
Alive Update Timer: %d secs   Pending Update Timer: %d secs",
            deviceRecPtr->deviceName, deviceRecPtr->macAddress,
                (deviceRecPtr->wlSensor->wetnessDetected ? "WET" : "DRY"), deviceRecPtr->deviceStateTimer,
                (deviceRecPtr->stateHasChanged ? "YES" : "NO"),
                (deviceRecPtr->wlSensor->alarmOnWet ? "YES" : "NO"), (deviceRecPtr->wlSensor->alarmOnDry ? "YES" : "NO"),
                (deviceRecPtr->wlSensor->callOnWet ? "YES" : "NO"), (deviceRecPtr->wlSensor->callOnDry ? "YES" : "NO"),
                deviceRecPtr->aliveUpdateTimer, deviceRecPtr->pendingUpdateTimer 
            );
    
    return &sensorRecordDumpBuffer[ 0 ];
}

// -----------------------------------------------------------------------------
static
WaterLeakSensor_t  *WaterLeak_newWaterLeakSensorRecord ()           
{
    WaterLeakSensor_t   *recPtr = NULL;
    
    //
    //  We've discovered a new WaterLeak sensor attached to our system
    //  Create a new record for it
    //
    recPtr = malloc( sizeof ( WaterLeakSensor_t ) );
    if (recPtr == NULL) {
        Logger_LogFatal( "Insufficient memory to allocate space for an WaterLeak Sensor!\n" );
    }
    
    return recPtr;
}
//------------------------------------------------------------------------------
void    WaterLeak_parseOneStateRecord (HomeHeartBeatDevice_t *deviceRecPtr )
{
    Logger_LogDebug( "Entering\n", 0 );
    
    assert( deviceRecPtr != NULL );

    //
    // Do we have an existing WaterLeak Sensor attached?
    if (deviceRecPtr->wlSensor == NULL) {
        // Start with a new WaterLeak Sensor record
        deviceRecPtr->wlSensor = WaterLeak_newWaterLeakSensorRecord();
        assert( deviceRecPtr->wlSensor != NULL );
        
   } else {
        Logger_LogDebug( "Found this WaterLeak Sensor in the list. Just going to update the values\n", 0 );
    }
    
    //
    // Set Values in the subclass - wlSensor
    assert( deviceRecPtr->wlSensor != NULL );
    
    deviceRecPtr->wlSensor->alarmOnWet = WaterLeak_getAlarmEnabledCondition1( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->wlSensor->alarmOnDry = WaterLeak_getAlarmEnabledCondition2( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->wlSensor->callOnWet =  WaterLeak_getCallMeEnabledCondition1( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->wlSensor->callOnDry =  WaterLeak_getCallMeEnabledCondition2( deviceRecPtr->deviceConfiguration );
    //
    //  Are we wet or dry?
    deviceRecPtr->wlSensor->currentState = WaterLeak_getWaterLeakStateFromInt( deviceRecPtr->deviceState );
    deviceRecPtr->wlSensor->wetnessDetected = (deviceRecPtr->wlSensor->currentState == wlWet);
    
    //
    
    
    //
    // Now - check to see if we've changed state?
    //
    //
    //  Let's assume it has NOT changed and reset it...
    deviceRecPtr->stateHasChanged = FALSE;

    //
    // Is the current state different? Then yes - the state has changed
    if (deviceRecPtr->wlSensor->currentState != deviceRecPtr->lastDeviceState) {
        deviceRecPtr->stateHasChanged = TRUE;
        Logger_LogDebug( "Detected state change on the device: %s. current state: %d, last state: %d\n", 
                deviceRecPtr->macAddress,
                deviceRecPtr->wlSensor->currentState, deviceRecPtr->lastDeviceState);
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
    deviceRecPtr->lastDeviceState = deviceRecPtr->wlSensor->currentState;
    deviceRecPtr->lastDeviceStateTimer = deviceRecPtr->deviceStateTimer;
            
    // Logger_LogDebug( "After parse. \nWaterLeak Sensor: %s\n\n", WaterLeak_dumpSensorDeviceRecord( deviceRecPtr ) );
}


//------------------------------------------------------------------------------
static
int     WaterLeak_getWaterLeakStateFromInt (int sensorState)
{
    int isWet    = (sensorState & ISWET_BITMASK);
    int isDry      = (sensorState & ISDRY_BITMASK);

    // They'd better not BOTH be on!
    if (isWet && isDry)
        Logger_LogWarning( "WaterLeak sensor reporting both wet and dry!\n" );
    else {

        if (isWet) {
            //Logger_LogDebug( " -----------------------[ %d ] ---> Sensor is wet!\n", sensorState );
            return wlWet;
        } else if (isDry) {
            //Logger_LogDebug( "------------------------[ %d ] ---> Sensor is dry!\n", sensorState );
            return wlDry;
        } else {
            Logger_LogError( "WaterLeak Sensor reporting neither wet nor dry!\n", 0 );
            return wlUnknown;
        }
    }
    
    return wlUnknown;
}

//------------------------------------------------------------------------------
static
int     WaterLeak_getWaterLeakState (char *token)
{
    assert( token != NULL );
    int tmpValue = hexStringToInt( token );
    return tmpValue;
}


//------------------------------------------------------------------------------
static
int WaterLeak_parseDeviceConfiguration (char *token)
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
int     WaterLeak_getAlarmEnabledCondition1 (int dcValue)
{
    return (dcValue & 0x0001);
}

//------------------------------------------------------------------------------
static
int     WaterLeak_getAlarmEnabledCondition2 (int dcValue)
{
    return (dcValue & 0x0002);
}

//------------------------------------------------------------------------------
static
int     WaterLeak_getCallMeEnabledCondition1 (int dcValue)
{
    return (dcValue & 0x0100);
}

//------------------------------------------------------------------------------
static
int     WaterLeak_getCallMeEnabledCondition2 (int dcValue)
{
    return (dcValue & 0x0200);
}

