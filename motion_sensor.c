#include <string.h>
#include <stdlib.h>
#include <assert.h>


#include "homeheartbeat.h"
#include "helpers.h"
#include "motion_sensor.h"
#include "logger.h"


// -----------------------------------------------------------------------------
//
// Forward Declarations
//static HomeHeartBeatDevice_t  *Motion_updateMotionDeviceRecord( HomeHeartBeatDevice_t *recPtr, int stateRecordID, int zigbeeBindingID);
static MotionSensor_t      *Motion_newMotionSensorRecord( void );


static  int         Motion_getMotionStateFromInt( int sensorState );
static  int         Motion_getMotionState( char *token );
static  int         Motion_parseDeviceConfiguration( char *token );
static  int         Motion_getAlarmEnabledCondition1( int dcValue );
static  int         Motion_getAlarmEnabledCondition2( int dcValue );
static  int         Motion_getCallMeEnabledCondition1( int dcValue );
static  int         Motion_getCallMeEnabledCondition2( int dcValue );
static  int         Motion_parseDeviceParameter( int dpValue );



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
char    *MotionSensor_dumpSensorDeviceRecord (HomeHeartBeatDevice_t *deviceRecPtr)
{
    assert( deviceRecPtr != NULL );
    
    //
    // Always handy to have a way to dump am entire sensor record
    memset( sensorRecordDumpBuffer, '\0', sizeof sensorRecordDumpBuffer );
    snprintf( sensorRecordDumpBuffer,
             sizeof sensorRecordDumpBuffer,
"MotionSensor: Name [%s] MAC [%s] State [%s] Duration: %d secs Stated Changed [%s] \
Alarm On Motion [%s]  Alarm On No Motion [%s] Call On Motion [%s] Call on No Motion [%s] \
Alive Update Timer: %d secs   Pending Update Timer: %d secs",
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
        Logger_LogFatal( "Insufficient memory to allocate space for an Motion Sensor!\n" );
    }
    
    return recPtr;
}
//------------------------------------------------------------------------------
void    Motion_parseOneStateRecord (HomeHeartBeatDevice_t *deviceRecPtr )
{
    Logger_FunctionStart();
    
    assert( deviceRecPtr != NULL );

    //
    // Do we have an existing Motion Sensor attached?
    if (deviceRecPtr->motSensor == NULL) {
        // Start with a new Motion Sensor record
        deviceRecPtr->motSensor = Motion_newMotionSensorRecord();
        assert( deviceRecPtr->motSensor != NULL );
        
   } else {
        Logger_LogDebug( "Found this Motion Sensor in the list. Just going to update the values\n", 0 );
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
    // The motion sensor uses the device parameter to set the delay value
    deviceRecPtr->motSensor->motionDelayValueSecs = Motion_parseDeviceParameter( deviceRecPtr->deviceParameter );
    
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
        Logger_LogDebug( "Detected state change on the device: %s. current state: %d, last state: %d\n", 
                deviceRecPtr->macAddress,
                deviceRecPtr->motSensor->currentState, deviceRecPtr->lastDeviceState);
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
    deviceRecPtr->lastDeviceState = deviceRecPtr->motSensor->currentState;
    deviceRecPtr->lastDeviceStateTimer = deviceRecPtr->deviceStateTimer;
            
    // Logger_LogDebug( "After parse. \nMotion Sensor: %s\n\n", MotionSensor_dumpSensorDeviceRecord( deviceRecPtr ) );
}


//------------------------------------------------------------------------------
static
int     Motion_getMotionStateFromInt (int sensorState)
{
    int isNoMotion    = (sensorState & ISNOMOTION_BITMASK);
    int isMotion      = (sensorState & ISMOTION_BITMASK);

    // They'd better not BOTH be on!
    if (isNoMotion && isMotion)
        Logger_LogWarning( "Motion sensor reporting both motion and no motion!\n" );
    else {

        if (isNoMotion) {
            //Logger_LogDebug( "Sensor sees MOTION!\n", 0 );
            return motNoMotion;
        } else if (isMotion) {
            //Logger_LogDebug( "Sensor sees NO MOTION!\n", 0 );
            return motMotion;
        } else {
            Logger_LogError( "Motion Sensor reporting neither Motion nor No-Motion!\n", 0 );
            return motUnknown;
        }
    }
    
    return motUnknown;
}

//------------------------------------------------------------------------------
static
int     Motion_getMotionState (char *token)
{
    assert( token != NULL );
    int tmpValue = hexStringToInt( token );
    // Logger_LogDebug( "----------- [%s] %d\n", token, tmpValue );

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

    
    //Logger_LogDebug( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
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

    
    //Logger_LogDebug( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
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

    
    //Logger_LogDebug( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
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

    
    //Logger_LogDebug( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
    //        alarmEnabledCondition1, alarmEnabledCondition2, callMeEnabledCondition1, callMeEnabledCondition2 );
}

//------------------------------------------------------------------------------
static
int     Motion_parseDeviceParameter (int dpValue)
{
    //  
    //  For motion sensors, field 13 has meaning - the delay value
    //      0 (0x00)  -  10 sec Delay
    //      1 (0x01)  -  1 min Delay
    //      2 (0x02)  -  5 min Delay
    //      3 (0x03)  -  10 min Delay
    //      4 (0x04)  -  30 min Delay
    //      5 (0x05)  -  1 hour Delay
    //      6 (0x06)  -  2 hour Delay
    
    int     motionDelayValueSecs = 10;              
    
    switch (dpValue) {
        case    0:      motionDelayValueSecs = 10;          break;
        case    1:      motionDelayValueSecs = 60;          break;
        case    2:      motionDelayValueSecs = (5 * 60);    break;
        case    3:      motionDelayValueSecs = (10 * 60);   break;
        case    4:      motionDelayValueSecs = (30 * 60);   break;
        case    5:      motionDelayValueSecs = (60 * 60);   break;
        case    6:      motionDelayValueSecs = (120 * 60);  break;
    }
    
    return motionDelayValueSecs;
}