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
static HomeHeartBeatDevice_t  *OpenClose_updateOCDeviceRecord( HomeHeartBeatDevice_t *recPtr, int stateRecordID, int zigbeeBindingID);
static OpenCloseSensor_t      *OpenClose_newOCSensorRecord( int stateRecordID,int zigbeeBindingID );

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
OpenCloseSensor_t  *OpenClose_newOCSensorRecord (int stateRecordID, int zigbeeBindingID)           
{
    OpenCloseSensor_t   *recPtr = NULL;
    
    //
    //  We've discovered a new Open Close sensor attached to our system
    //  Create a new record for it
    //
    recPtr = malloc( sizeof ( OpenCloseSensor_t ) );
    if (recPtr != NULL) {
        
        recPtr->stateRecordID = stateRecordID;
        recPtr->zigbeeBindingID = zigbeeBindingID;
    }
    
    return recPtr;
}

// -----------------------------------------------------------------------------
static
HomeHeartBeatDevice_t   *OpenClose_newDeviceRecord (OpenCloseSensor_t *newOCRec)
{
    HomeHeartBeatDevice_t   *recPtr = NULL;
    
    //
    //  We've discovered a new Open/Close Sensor attached to our system. After we allocate
    //  a record for the O/C Sensor, we call this function which allocates space for
    //  what is essentially the Device Superclass
    //
    recPtr = malloc( sizeof ( HomeHeartBeatDevice_t ) );
    if (recPtr != NULL) {  
        recPtr->deviceType = DT_OPEN_CLOSE_SENSOR;
        recPtr->ocSensor = newOCRec;
        recPtr->next = NULL;
    }
    
    return recPtr;
}

//------------------------------------------------------------------------------
void    OpenClose_parseOneStateRecord (HomeHeartBeatSystem_t *aSystem, char *token[] )
{
    debug_print( "Entering\n", 0 );
    
    assert( aSystem != NULL );
    assert( token[ 3 ] != NULL );                   // 4th token is the Device Type
    
    
    //
    // Now let's parse the tokens we've got 
    int stateRecordID = Device_parseStateRecordID( token[ 0 ] );
    int zigbeeBindingID = Device_parseZigbeeBindingID( token[ 1 ] );
    int deviceCapabilties = Device_parseDeviceCapabilties( token[ 2 ] );
    //
    // 4th token (token[3]) is the device type - we already know we're an Open/Close sensor
    //
    int deviceState = OpenClose_getOpenCloseState( token[ 4 ] );  
    long deviceStateTimer = Device_parseDeviceStateTimer( token[ 5 ] );
    int deviceAlerts = Device_parseDeviceAlerts( token[ 6 ] );
    int deviceNameIndex = Device_parseDeviceNameIndex( token[ 7 ] );

    //
    // Four bit values stashed in this one field
    int deviceConfiguration = OpenClose_parseDeviceConfiguration( token[ 8 ] );
    
    long aliveUpdateTimer = Device_parseAliveUpdateTimer( token[ 9 ] );
    int updateFlags = Device_parseUpdateFlags( token[ 10 ] );
    
    // No one has figured what the 12th token does yet
    
    int deviceParameter = Device_parseDeviceParameter( token[ 12 ] );
    
    // No one has figured what the 14th token does yet
    
    long pendingUpdateTimer = Device_parsePendingUpdateTimer( token[ 14 ] );
        
    char *macAddress = Device_parseMacAddress( token[ 15 ] );
    char *deviceName = Device_parseDeviceName( token[ 16 ] );
    
    debug_print( "Device Name: [%s], MAC: [%s]\n", deviceName, macAddress );
    
    
    //
    // Now we look to see if we can find this device (by it's MAC address) already in our list of devices
    HomeHeartBeatDevice_t   *deviceRecPtr = Device_findThisDevice( aSystem->deviceListHead, macAddress );

    int     firstTimeDeviceSeen = FALSE;
    if (deviceRecPtr == NULL) {
        //
        // Not found! -- easy peasy - this is the first device. Time to add it
        //
        firstTimeDeviceSeen = TRUE;
        
        // Start with a new OC Sensor record
        OpenCloseSensor_t       *newSensorRecPtr = OpenClose_newOCSensorRecord( stateRecordID, zigbeeBindingID );
        assert( newSensorRecPtr != NULL );
        
        //
        // Now make a new device record and link in the OC Sensor
        deviceRecPtr = OpenClose_newDeviceRecord( newSensorRecPtr );
        assert( deviceRecPtr != NULL );
        
        //
        // Add it to the linked list
        LL_APPEND( aSystem->deviceListHead, deviceRecPtr );

    } else {
        debug_print( "Found this device in the list. Just going to update the values\n", 0 );
        // OpenClose_updateOCSensorRecord( deviceRecPtr, stateRecordID, zigbeeBindingID       );
    }
    
    
    //
    // Whether we created a new device or found the old one - time to update it!
    //
    // Set values in the Supertype Device
    assert( deviceRecPtr != NULL );

    deviceRecPtr->deviceName = deviceName;
    deviceRecPtr->macAddress = macAddress;
    deviceRecPtr->deviceRawData[ 0 ] = stateRecordID;                  // Field 1
    deviceRecPtr->deviceRawData[ 1 ] = zigbeeBindingID;                // Field 2
    deviceRecPtr->deviceRawData[ 2 ] = deviceCapabilties;              // Field 3       
    deviceRecPtr->deviceRawData[ 3 ] = DT_OPEN_CLOSE_SENSOR;           // Field 4
    deviceRecPtr->deviceRawData[ 4 ] = deviceState;                    // Field 5
    deviceRecPtr->deviceRawData[ 5 ] = deviceStateTimer;               // Field 6  -- oops I made this a long!
    deviceRecPtr->deviceRawData[ 6 ] = deviceAlerts;                   // Field 7
    deviceRecPtr->deviceRawData[ 7 ] = deviceNameIndex;                // Field 8
    deviceRecPtr->deviceRawData[ 8 ] = deviceConfiguration;            // Field 9
    deviceRecPtr->deviceRawData[ 9 ] = aliveUpdateTimer;               // Field 10 -- another long/int mismatch
    deviceRecPtr->deviceRawData[ 10 ] = updateFlags;                   // Field 11
    deviceRecPtr->deviceRawData[ 11 ] = 0;                             // Field 12 - unknown function so far
    deviceRecPtr->deviceRawData[ 12 ] = deviceParameter;               // Field 13 
    deviceRecPtr->deviceRawData[ 13 ] = 0;                             // Field 14 - unknown function so far
    deviceRecPtr->deviceRawData[ 14 ] = pendingUpdateTimer;            // Field 15 -- third long/int mismatch
    
    deviceRecPtr->stateRecordID = stateRecordID;
    deviceRecPtr->zigbeeBindingID = zigbeeBindingID;
    deviceRecPtr->deviceCapabilities = deviceCapabilties;
    deviceRecPtr->deviceType = DT_OPEN_CLOSE_SENSOR;
    deviceRecPtr->deviceState = deviceState;
    deviceRecPtr->deviceStateTimer = deviceStateTimer;
    deviceRecPtr->deviceAlerts = deviceAlerts;
    deviceRecPtr->deviceNameIndex = deviceNameIndex;
    deviceRecPtr->deviceConfiguration = deviceConfiguration;
    deviceRecPtr->aliveUpdateTimer = aliveUpdateTimer;
    deviceRecPtr->updateFlags = updateFlags;
    deviceRecPtr->deviceParameter = deviceParameter;
    deviceRecPtr->pendingUpdateTimer = pendingUpdateTimer;

    //
    // Set Values in the subclass - ocSensor
    // These should be values that are specific to an Open / Close Sensor
    assert( deviceRecPtr->ocSensor != NULL );
    
    deviceRecPtr->ocSensor->alarmOnOpen = OpenClose_getAlarmEnabledCondition2( deviceConfiguration );
    deviceRecPtr->ocSensor->alarmOnClose = OpenClose_getAlarmEnabledCondition1( deviceConfiguration );
    deviceRecPtr->ocSensor->callOnOpen =  OpenClose_getCallMeEnabledCondition2( deviceConfiguration );
    deviceRecPtr->ocSensor->callOnClose =  OpenClose_getCallMeEnabledCondition1( deviceConfiguration );
    //
    //  Are we open of closed?
    deviceRecPtr->ocSensor->currentState = OpenClose_getOpenCloseStateFromInt( deviceState );
    deviceRecPtr->ocSensor->isOpen = (deviceRecPtr->ocSensor->currentState == ocOpen);
    
    //
    // Now - check to see if we've changed state?
    if (firstTimeDeviceSeen) {
        deviceRecPtr->stateHasChanged = FALSE;
        deviceRecPtr->lastDeviceState = deviceRecPtr->ocSensor->currentState;
        deviceRecPtr->lastDeviceStateTimer = deviceRecPtr->deviceStateTimer;
    } else {
        //
        //  Let's assume it's just the same as the last time we looked and reset it...
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
        
    }
    
    
    if (aSystem->logEventsToDatabase) {
        Database_insertDeviceStateLogRecord( deviceRecPtr );
        Database_updateDeviceStateCurrentRecord( deviceRecPtr );
    }
    
    if (aSystem->logEventsToMQTT) {
        MQTT_CreateDeviceEvent( deviceRecPtr );
    }
        
    debug_print( "After parse. \nOC Sensor: %s\n\n", dumpOCSensorDeviceRecord( deviceRecPtr ) );
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
            debug_print( "Sensor is CLOSED!\n", 0 );
            return ocClosed;
        } else if (isOpen) {
            debug_print( "Sensor is OPEN!\n", 0 );
            return ocOpen;
        } else {
            debug_print( "Sensor is ???\n", 0 );
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
    // debug_print( "----------- [%s] %d\n", token, tmpValue );

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

    return (dcValue & 0x0001);
    //int     alarmEnabledCondition2 = (dcValue & 0x0002);
    //int     callMeEnabledCondition1 = (dcValue & 0x0100);
    //int     callMeEnabledCondition2 = (dcValue & 0x0200);

    
    //debug_print( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
    //        alarmEnabledCondition1, alarmEnabledCondition2, callMeEnabledCondition1, callMeEnabledCondition2 );
}

//------------------------------------------------------------------------------
static
int     OpenClose_getAlarmEnabledCondition2 (int dcValue)
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
int     OpenClose_getCallMeEnabledCondition1 (int dcValue)
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
int     OpenClose_getCallMeEnabledCondition2 (int dcValue)
{

    // return (dcValue & 0x0001);
    //int     alarmEnabledCondition2 = (dcValue & 0x0002);
    //int     callMeEnabledCondition1 = (dcValue & 0x0100);
    return (dcValue & 0x0200);

    
    //debug_print( "Device Configuration alarm1: %d, alarm2: %d, callMe1: %d, callMe2: %d\n", 
    //        alarmEnabledCondition1, alarmEnabledCondition2, callMeEnabledCondition1, callMeEnabledCondition2 );
}
