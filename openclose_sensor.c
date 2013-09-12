#include <string.h>
#include <stdlib.h>
#include <assert.h>


#include "homeheartbeat.h"
#include "helpers.h"
#include "openclose_sensor.h"
#include "utlist.h"


// -----------------------------------------------------------------------------
//
// Forward Declarations
//static HomeHeartBeatDevice_t  *OpenClose_updateOCDeviceRecord( HomeHeartBeatDevice_t *recPtr, int stateRecordID, int zigbeeBindingID);
static OpenCloseSensor_t      *OpenClose_newOCSensorRecord( void );

static  HomeHeartBeatDevice_t   *findThisDevice( HomeHeartBeatDevice_t *deviceListHead, char *macAddress );
static  HomeHeartBeatDevice_t   *OpenClose_newDeviceRecord( OpenCloseSensor_t *newOCRec );


static  int             OpenClose_getOpenCloseStateFromInt (int sensorState);
static  int             OpenClose_getOpenCloseState (char *token);
static  int             OpenClose_parseDeviceConfiguration (char *token);
static  int             OpenClose_getAlarmEnabledCondition1 (int dcValue);
static  int             OpenClose_getAlarmEnabledCondition2 (int dcValue);
static  int             OpenClose_getCallMeEnabledCondition1 (int dcValue);
static  int             OpenClose_getCallMeEnabledCondition2 (int dcValue);



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
char    *dumpOCSensorDeviceRecord (HomeHeartBeatDevice_t *deviceRecPtr)
{
    assert( deviceRecPtr != NULL );
    
    //
    // Always handy to have a way to dump am entire sensor record
    memset( sensorRecordDumpBuffer, '\0', sizeof sensorRecordDumpBuffer );
    snprintf( sensorRecordDumpBuffer,
             sizeof sensorRecordDumpBuffer,
                "OpenCloseSensor: Name [%s] Mac [%s]\n \
                    State [%s]  Duration: %d seconds\n \
                    Stated Changed [%s]\n \
                    Alarm On Open [%s]  Alarm On Close [%s]\n \
                    Call On Open [%s]  Call on Close [%s]\n \
                    Alive Update Timer: %d seconds   Pending Update Timer: %d seconds \
                ",
            deviceRecPtr->deviceName, deviceRecPtr->macAddress,
                (deviceRecPtr->ocSensor->isOpen ? "OPEN" : "CLOSED"), deviceRecPtr->deviceStateTimer,
                (deviceRecPtr->stateHasChanged ? "YES" : "NO"),
                (deviceRecPtr->ocSensor->alarmOnOpen ? "YES" : "NO"), (deviceRecPtr->ocSensor->alarmOnClose ? "YES" : "NO"),
                (deviceRecPtr->ocSensor->callOnOpen ? "YES" : "NO"), (deviceRecPtr->ocSensor->callOnClose ? "YES" : "NO"),
                deviceRecPtr->aliveUpdateTimer, deviceRecPtr->pendingUpdateTimer 
            );
    
    return &sensorRecordDumpBuffer[ 0 ];
}

// -----------------------------------------------------------------------------
static
OpenCloseSensor_t  *OpenClose_newOCSensorRecord ()           
{
    OpenCloseSensor_t   *recPtr = NULL;
    
    //
    //  We've discovered a new Open Close sensor attached to our system
    //  Create a new record for it
    //
    recPtr = malloc( sizeof ( OpenCloseSensor_t ) );
    if (recPtr == NULL) {
        haltAndCatchFire( "Insufficient memory to allocate space for an Open/Close Sensor!" );
    }
    
    return recPtr;
}

//------------------------------------------------------------------------------
void    OpenClose_parseOneStateRecord (HomeHeartBeatDevice_t *deviceRecPtr )
{
    debug_print( "Entering\n", 0 );
    
    assert( deviceRecPtr != NULL );

    //
    // Do we have an existing OC Sensor attached?
    if (deviceRecPtr->ocSensor == NULL) {
        // Start with a new OC Sensor record
        deviceRecPtr->ocSensor = OpenClose_newOCSensorRecord();
        assert( deviceRecPtr->ocSensor != NULL );
        
   } else {
        debug_print( "Found this Open Close Sensor in the list. Just going to update the values\n", 0 );
    }
    
    //
    // Set Values in the subclass - ocSensor
    // These should be values that are specific to an Open / Close Sensor
    assert( deviceRecPtr->ocSensor != NULL );
    
    //
    //  Now take the Device Configuration field and figure out the four settings:
    //  Alarm On Open, Alarm On Close, Call on Open, Call on Close
    deviceRecPtr->ocSensor->alarmOnOpen = OpenClose_getAlarmEnabledCondition2( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->ocSensor->alarmOnClose = OpenClose_getAlarmEnabledCondition1( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->ocSensor->callOnOpen =  OpenClose_getCallMeEnabledCondition2( deviceRecPtr->deviceConfiguration );
    deviceRecPtr->ocSensor->callOnClose =  OpenClose_getCallMeEnabledCondition1( deviceRecPtr->deviceConfiguration );
    
    
    //
    //  Are we open or closed?
    deviceRecPtr->ocSensor->currentState = OpenClose_getOpenCloseStateFromInt( deviceRecPtr->deviceState );
    deviceRecPtr->ocSensor->isOpen = (deviceRecPtr->ocSensor->currentState == ocOpen);
    
    
    
    //
    // Now - check to see if we've changed state?
    //
    //
    //  Let's assume it has NOT changed and reset it...
    deviceRecPtr->stateHasChanged = FALSE;

    //
    // Is the current state different? Then yes - the state has changed
    if (deviceRecPtr->ocSensor->currentState != deviceRecPtr->lastDeviceState) {
        deviceRecPtr->stateHasChanged = TRUE;
        debug_print( "Detected state change on the device: %s. current state: %d, last state: %d\n", 
                deviceRecPtr->macAddress,
                deviceRecPtr->ocSensor->currentState, deviceRecPtr->lastDeviceState);
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
    deviceRecPtr->lastDeviceState = deviceRecPtr->ocSensor->currentState;
    deviceRecPtr->lastDeviceStateTimer = deviceRecPtr->deviceStateTimer;
            
    // debug_print( "After parse. \nOC Sensor: %s\n\n", dumpOCSensorDeviceRecord( deviceRecPtr ) );
}


//------------------------------------------------------------------------------
static
int     OpenClose_getOpenCloseStateFromInt (int sensorState)
{
    int isClosed    = (sensorState & ISCLOSED_BITMASK);
    int isOpen      = (sensorState & ISOPEN_BITMASK);

    // They'd better not BOTH be on!
    if (isClosed && isOpen)
        warnAndKeepGoing( "Open/Close sensor reporting both open and closed!" );
    else {

        if (isClosed) {
            // debug_print( "Sensor is CLOSED!\n", 0 );
            return ocClosed;
        } else if (isOpen) {
            // debug_print( "Sensor is OPEN!\n", 0 );
            return ocOpen;
        } else {
            // debug_print( "Sensor is ???\n", 0 );
            return ocUnknown;
        }
    }
    
    return ocUnknown;
}

//------------------------------------------------------------------------------
static
int     OpenClose_getOpenCloseState (char *token)
{
    assert( token != NULL );
    int tmpValue = hexStringToInt( token );
    return tmpValue;
}


//------------------------------------------------------------------------------
static
int OpenClose_parseDeviceConfiguration (char *token)
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
int     OpenClose_getAlarmEnabledCondition1 (int dcValue)
{
    //int     alarmEnabledCondition2 = (dcValue & 0x0002);
    //int     callMeEnabledCondition1 = (dcValue & 0x0100);
    //int     callMeEnabledCondition2 = (dcValue & 0x0200);

    
    //debug_print( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
    //        alarmEnabledCondition1, alarmEnabledCondition2, callMeEnabledCondition1, callMeEnabledCondition2 );

    return (dcValue & 0x0001);
}

//------------------------------------------------------------------------------
static
int     OpenClose_getAlarmEnabledCondition2 (int dcValue)
{

    //debug_print( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
    //        alarmEnabledCondition1, alarmEnabledCondition2, callMeEnabledCondition1, callMeEnabledCondition2 );
    return (dcValue & 0x0002);
}

//------------------------------------------------------------------------------
static
int     OpenClose_getCallMeEnabledCondition1 (int dcValue)
{
    return (dcValue & 0x0100);
}

//------------------------------------------------------------------------------
static
int     OpenClose_getCallMeEnabledCondition2 (int dcValue)
{
    return (dcValue & 0x0200);
}
