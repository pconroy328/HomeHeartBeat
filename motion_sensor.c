#include <string.h>
#include <stdlib.h>
#include <assert.h>


#include "homeheartbeat.h"
#include "helpers.h"
#include "motion_sensor.h"
#include "utlist.h"


// -----------------------------------------------------------------------------
//
// Forward Declarations
//static HomeHeartBeatDevice_t  *Motion_updateMotionDeviceRecord( HomeHeartBeatDevice_t *recPtr, int stateRecordID, int zigbeeBindingID);
static MotionSensor_t      *Motion_newMotionSensorRecord( void );


static  int             Motion_getMotionStateFromInt (int sensorState);
static  int             Motion_getMotionState (char *token);
static  int             Motion_parseDeviceConfiguration (char *token);
static  int             Motion_getAlarmEnabledCondition1 (int dcValue);
static  int             Motion_getAlarmEnabledCondition2 (int dcValue);
static  int             Motion_getCallMeEnabledCondition1 (int dcValue);
static  int             Motion_getCallMeEnabledCondition2 (int dcValue);



static  char    *deviceNames[] = { 
    "Open/Closed",      "Door",             "Window",           "Back Door",            "Basement Door",
    "Bathroom Door",    "Bedroom Door",     "Deck Door",        "Dining Room Door",     "Front Door",
    "Garage Door",      "Kitchen Door",     "Pet Door",         "Side Door",            "Back Window",
    "Basement Window",  "Bathroom Window",  "Bedroom Window",   "Dining Room Window",   "Front Window",
    "Kitchen Window",   "Living Room Window","Side Window",     "Closet",               "Cabinet",
    "Kitchen Cabinet",  "Drawer",           "Utensil Drawer",   "Freezer",              "Gun Cabinet",
    "Jewelry Box",      "Mail Box",         "Pantry",           "Refrigerator",         "Safe",
    "Storage Area",     "Supply Room",      "Trunk",            "TV/Stereo Cabinet",    "???"
};



static  char    sensorRecordDumpBuffer[ 8192 ];

// ----------------------------------------------------------------------------
char    *dumpMotionSensorDeviceRecord (HomeHeartBeatDevice_t *deviceRecPtr)
{
    assert( deviceRecPtr != NULL );
    
    //
    // Always handy to have a way to dump am entire sensor record
    memset( sensorRecordDumpBuffer, '\0', sizeof sensorRecordDumpBuffer );
    snprintf( sensorRecordDumpBuffer,
             sizeof sensorRecordDumpBuffer,
                "MotionSensor: Name [%s] Mac [%s]\n \
                    State [%s]  Duration: %d seconds\n \
                    Stated Changed [%s]\n \
                    Alarm On Motion [%s]  Alarm On No Motion [%s]\n \
                    Call On Motion [%s]  Call on No Motion [%s]\n \
                    Alive Update Timer: %d seconds   Pending Update Timer: %d seconds \
                ",
            deviceRecPtr->deviceName, deviceRecPtr->macAddress,
                (deviceRecPtr->motSensor->motionDetected ? "MOTION" : "NO MOTION"), deviceRecPtr->deviceStateTimer,
                (deviceRecPtr->stateHasChanged ? "YES" : "NO"),
                (deviceRecPtr->motSensor->alarmOnMotion ? "YES" : "NO"), (deviceRecPtr->motSensor->alarmOnNoMotion ? "YES" : "NO"),
                (deviceRecPtr->motSensor->callOnMotion ? "YES" : "NO"), (deviceRecPtr->motSensor->callOnNoMotion ? "YES" : "NO"),
                deviceRecPtr->aliveUpdateTimer, deviceRecPtr->pendingUpdateTimer 
            );
    
    return &sensorRecordDumpBuffer[ 0 ];
}

// -----------------------------------------------------------------------------
static
MotionSensor_t  *Motion_newMotionSensorRecord ()           
{
    MotionSensor_t   *recPtr = NULL;
    
    //
    //  We've discovered a new Motion sensor attached to our system
    //  Create a new record for it
    //
    recPtr = malloc( sizeof ( MotionSensor_t ) );
    if (recPtr == NULL) {
        haltAndCatchFire( "Insufficient memory to allocate space for an Motion Sensor!" );
    }
    
    return recPtr;
}
//------------------------------------------------------------------------------
void    Motion_parseOneStateRecord (HomeHeartBeatDevice_t *deviceRecPtr )
{
    debug_print( "Entering\n", 0 );
    
    assert( deviceRecPtr != NULL );

    //
    // Do we have an existing Motion Sensor attached?
    if (deviceRecPtr->motSensor == NULL) {
        // Start with a new Motion Sensor record
        deviceRecPtr->motSensor = Motion_newMotionSensorRecord();
        assert( deviceRecPtr->motSensor != NULL );
        
   } else {
        debug_print( "Found this Motion Sensor in the list. Just going to update the values\n", 0 );
    }
    
    //
    // Set Values in the subclass - motSensor
    assert( deviceRecPtr->motSensor != NULL );
    
    deviceRecPtr->motSensor->alarmOnMotion = Motion_getAlarmEnabledCondition2( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->motSensor->alarmOnNoMotion = Motion_getAlarmEnabledCondition1( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->motSensor->callOnMotion =  Motion_getCallMeEnabledCondition2( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->motSensor->callOnNoMotion =  Motion_getCallMeEnabledCondition1( deviceRecPtr->deviceConfiguration );
    //
    //  Are we open of closed?
    deviceRecPtr->motSensor->currentState = Motion_getMotionStateFromInt( deviceRecPtr->deviceState );
    deviceRecPtr->motSensor->motionDetected = (deviceRecPtr->motSensor->currentState == motMotion);
    
    //
    // Now - check to see if we've changed state?
    //
    //
    //  Let's assume it has NOT changed and reset it...
    deviceRecPtr->stateHasChanged = FALSE;

    //
    // Is the current state different? Then yes - the state has changed
    if (deviceRecPtr->motSensor->currentState != deviceRecPtr->lastDeviceState) {
        deviceRecPtr->stateHasChanged = TRUE;
        debug_print( "Detected state change on the device: %s. current state: %d, last state: %d\n", 
                deviceRecPtr->macAddress,
                deviceRecPtr->motSensor->currentState, deviceRecPtr->lastDeviceState);
    }
    //
    //  Or has the state timer gotten smaller (or stayed the same)?  Then we had a fast state 
    //  change between poll intervals. Eg. Closed -> Opened -> Closed.   Well it turns out we cannot use "<=" because
    //  the resolution of the timer changes to minutes after 60 seconds
    if (deviceRecPtr->deviceStateTimer < deviceRecPtr->lastDeviceStateTimer) {
        deviceRecPtr->stateHasChanged = TRUE;
        debug_print( "Detected TIME BASED state change on the device: %s. current Time: %d, last time: %d\n", deviceRecPtr->macAddress,
                deviceRecPtr->deviceStateTimer, deviceRecPtr->lastDeviceStateTimer);
    }

    //debug_print( ">>>>>>>>>>>> state stateChaned: %d, current Timer: %d, lastTimer: %d\n", deviceRecPtr->stateHasChanged,
    //                deviceRecPtr->deviceStateTimer, deviceRecPtr->lastDeviceStateTimer);

    //
    //  Ok we've marked that the state has changed. Reset the counters so the next time thru
    //  we can recheck!
    deviceRecPtr->lastDeviceState = deviceRecPtr->motSensor->currentState;
    deviceRecPtr->lastDeviceStateTimer = deviceRecPtr->deviceStateTimer;
            
    debug_print( "After parse. \nMotion Sensor: %s\n\n", dumpMotionSensorDeviceRecord( deviceRecPtr ) );
}


//------------------------------------------------------------------------------
static
int     Motion_getMotionStateFromInt (int sensorState)
{
    int isNoMotion    = (sensorState & ISNOMOTION_BITMASK);
    int isMotion      = (sensorState & ISMOTION_BITMASK);

    // They'd better not BOTH be on!
    if (isNoMotion && isMotion)
        warnAndKeepGoing( "Motion sensor reporting both motion and no motion!" );
    else {

        if (isNoMotion) {
            debug_print( "Sensor sees MOTION!\n", 0 );
            return motNoMotion;
        } else if (isMotion) {
            debug_print( "Sensor sees NO MOTION!\n", 0 );
            return motMotion;
        } else {
            debug_print( "Sensor sees ???\n", 0 );
            return motUnknown;
        }
    }
    
    return ocUnknown;
}

//------------------------------------------------------------------------------
static
int     Motion_getMotionState (char *token)
{
    assert( token != NULL );
    int tmpValue = hexStringToInt( token );
    // debug_print( "----------- [%s] %d\n", token, tmpValue );

    return tmpValue;
}


//------------------------------------------------------------------------------
static
int Motion_parseDeviceConfiguration (char *token)
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
int     Motion_getAlarmEnabledCondition1 (int dcValue)
{

    return (dcValue & 0x0001);
    //int     alarmEnabledCondition2 = (dcValue & 0x0002);
    //int     callMeEnabledCondition1 = (dcValue & 0x0100);
    //int     callMeEnabledCondition2 = (dcValue & 0x0200);

    
    //debug_print( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
    //        alarmEnabledCondition1, alarmEnabledCondition2, callMeEnabledCondition1, callMeEnabledCondition2 );
}

//------------------------------------------------------------------------------
static
int     Motion_getAlarmEnabledCondition2 (int dcValue)
{

    // return (dcValue & 0x0001);
    return (dcValue & 0x0002);
    //int     callMeEnabledCondition1 = (dcValue & 0x0100);
    //int     callMeEnabledCondition2 = (dcValue & 0x0200);

    
    //debug_print( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
    //        alarmEnabledCondition1, alarmEnabledCondition2, callMeEnabledCondition1, callMeEnabledCondition2 );
}

//------------------------------------------------------------------------------
static
int     Motion_getCallMeEnabledCondition1 (int dcValue)
{

    // return (dcValue & 0x0001);
    //int     alarmEnabledCondition2 = (dcValue & 0x0002);
    return (dcValue & 0x0100);
    //int     callMeEnabledCondition2 = (dcValue & 0x0200);

    
    //debug_print( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
    //        alarmEnabledCondition1, alarmEnabledCondition2, callMeEnabledCondition1, callMeEnabledCondition2 );
}

//------------------------------------------------------------------------------
static
int     Motion_getCallMeEnabledCondition2 (int dcValue)
{

    // return (dcValue & 0x0001);
    //int     alarmEnabledCondition2 = (dcValue & 0x0002);
    //int     callMeEnabledCondition1 = (dcValue & 0x0100);
    return (dcValue & 0x0200);

    
    //debug_print( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
    //        alarmEnabledCondition1, alarmEnabledCondition2, callMeEnabledCondition1, callMeEnabledCondition2 );
}

