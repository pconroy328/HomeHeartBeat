//------------------------------------------------------------------------------
//
// The "System" is a collection of sensors
//
// -----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>


#include "homeheartbeat.h"
#include "mqtt.h"
#include "helpers.h"
#include "serialport.h"
#include "openclose_sensor.h"
#include "waterleak_sensor.h"
#include "motion_sensor.h"
#include "utlist.h"


//
// Forward declarations
static  int     getOneStateRecord( char *, int );
static  HomeHeartBeatDevice_t *parseOneStateRecord (char *receiveBuf, int numRead);
static  HomeHeartBeatDevice_t *parseOneStateRecordA (char *receiveBuf, int numRead);
static  HomeHeartBeatDevice_t *parseOneStateRecordL (char *receiveBuf, int numRead);
static  int     tokenizeStateData (char *receiveBuf, int numRead, char *token[]);


//
// Every HHB installation has a System
static  HomeHeartBeatSystem_t   *aSystem;


// -----------------------------------------------------------------------------
void    HomeHeartBeatSystem_Initialize ()
{
    debug_print( "entering\n", 0 );
    
    //
    //  We're bringing up a whole new system - initialize everything
    aSystem = malloc( sizeof( HomeHeartBeatSystem_t ) );            // grab some space
    if (aSystem == NULL)
        haltAndCatchFire( "Out of memory trying to initialize the system" );
    
    //
    // Initialize some necessary fields.
    aSystem->deviceListHead = NULL;                 // utlist says always set this to NULL
    aSystem->deviceArrayIndex = 0;
    
    aSystem->systemID = 0;
    strncpy( aSystem->name, "UNNAMED SYSTEM", sizeof aSystem->name );
    
    strncpy( aSystem->addressLine1,"No Address Set", sizeof aSystem->addressLine1 );
    strncpy( aSystem->addressLine2, "", sizeof aSystem->addressLine2 );
    strncpy( aSystem->city, "", sizeof aSystem->city );
    strncpy( aSystem->stateOrProvinceCode, "", sizeof aSystem->stateOrProvinceCode );
    strncpy( aSystem->postalCode, "", sizeof aSystem->postalCode );
    
    aSystem->latitude = 0.0;
    aSystem->longitude = 0.0;
    
    aSystem->sleepSecsBetweenEventLoops = 1;           // seconds
    // 
    // Database Logging if enabled
    aSystem->logEventsToDatabase = FALSE;
    
    //
    // MQTT Stuff
    aSystem->logEventsToMQTT = FALSE;

    //
    //  Read in data from an IniFile if it exists
    // readIniFileValues( aSystem );
    //
    IniFile_readIniFile( aSystem );    
    
    if (aSystem->logEventsToDatabase) {
        Database_setDatabaseHost( aSystem->databaseHost );
        Database_setDatabaseUserName( aSystem->databaseUserName );
        Database_setDatabasePassword( aSystem->databasePassword );
        Database_setDatabaseSchema( aSystem->databaseSchema );
        Database_setFailOnDatabaseErrors( 1 );
        Database_openDatabase();
        Database_dropDeviceStateLogTable(); Database_createDeviceStateLogTable();
        Database_dropDeviceStateCurrentTable(); Database_createDeviceStateCurrentTable();
    }
    
    if (aSystem->logEventsToMQTT) {
        MQTT_setDefaults( aSystem, aSystem->mqttBrokerHost );
        MQTT_initialize( aSystem );
    }
}

// -----------------------------------------------------------------------------
void    HomeHeartBeatSystem_Shutdown ()
{
    debug_print( "entering\n", 0 );
    SerialPort_Close( aSystem->hhbFD );
    aSystem->portOpen = FALSE;

    if (aSystem->logEventsToMQTT) {
        MQTT_teardown();
    }
    
    if (aSystem->logEventsToDatabase) {
        ;
    }
    free( aSystem );
}

// -----------------------------------------------------------------------------
void    HomeHeartBeatSystem_SetPortName (char *portName)
{
    debug_print( "entering. portName: %s\n", portName );
    strncpy( aSystem->portName, portName, sizeof aSystem->portName );
}

// -----------------------------------------------------------------------------
void    HomeHeartBeatSystem_OpenPort (char *portName)
{
    debug_print( "entering. portName: %s\n", portName );
    
    if (portName != NULL)
        HomeHeartBeatSystem_SetPortName( portName );
    
    //
    // Open up the serial port!
    if ( (aSystem->hhbFD = SerialPort_Open( aSystem->portName )) > 0)
        aSystem->portOpen = TRUE;
}

// -----------------------------------------------------------------------------
void    HomeHeartBeatSystem_EventLoop ()
{
    int     numBytesRead = 0;
    char    rawStateRecord[ 256 ];                  // should be 4x bigger than what we need for one state record
    long    numLoops = 0;
    HomeHeartBeatDevice_t   *deviceRecPtr = NULL;
    
    
    debug_print( "entering\n", 0 );

    if (!aSystem->portOpen) {
        fprintf( stderr, "HHB Port is not open - cannot start event loop" );
        return;
    }
    
    aSystem->pollForEvents = TRUE;
    while (aSystem->pollForEvents) {
        numBytesRead = getOneStateRecord( rawStateRecord, sizeof rawStateRecord );
        deviceRecPtr = parseOneStateRecordL( rawStateRecord, numBytesRead );
        
        //
        // debugging for valgrind
        numLoops += 1L;
        if (numLoops > 6000L) {
            return;
        }

        if (deviceRecPtr != NULL) {
    
            if (aSystem->logEventsToDatabase) {
                Database_insertDeviceStateLogRecord( deviceRecPtr );
                Database_updateDeviceStateCurrentRecord( deviceRecPtr );
            }
    
            if (aSystem->logEventsToMQTT) {
                //
                // Have we had a state change???
                if (deviceRecPtr->stateHasChanged)
                    MQTT_CreateDeviceAlarm( deviceRecPtr );
                
                MQTT_CreateDeviceEvent( deviceRecPtr );
                MQTT_SendReceive();
            }
        }
        
        sleep( aSystem->sleepSecsBetweenEventLoops );
    }
}

// -----------------------------------------------------------------------------
void    HomeHeartBeat_ReadIniFile ()
{
    
}

// ----------------------------------------------------------------------------
static int  getOneStateRecord (char *rawStateRecord, int bufSize)
{
    char    commandBuf[ 2 ];
    int     numRead = 0;

    assert( rawStateRecord != NULL );
    assert( bufSize > 0 );
    // debug_print( "entering\n", 0 );
    
    //
    // Be good - zero things out
    memset( rawStateRecord, '\0', sizeof bufSize );         // zero out raw data buffer
    memset( commandBuf, '\0', sizeof commandBuf );          // zero out command buffer

    SerialPort_FlushBuffers( aSystem->hhbFD );
    
    //
    // Send a lowercase 's' to the Home HeartBeat device
    commandBuf[ 0 ] = 's'; 
    SerialPort_SendData( aSystem->hhbFD, commandBuf, 1 );
    
    
    //
    // Get the data back from the HHB
    numRead = SerialPort_ReadLine( aSystem->hhbFD, rawStateRecord, bufSize );
    // debug_print( "read returned %d bytes. Data [%s]\n", numRead, rawStateRecord );
    
    return numRead;
}   // getOneStateRecord


// ----------------------------------------------------------------------------
static  
HomeHeartBeatDevice_t *parseOneStateRecord2 (char *receiveBuf, int numRead)
{
    char    *token[ 17 ];
    HomeHeartBeatDevice_t   *deviceRecPtr = NULL;
    
    
    assert( receiveBuf != NULL );

    int numTokensParsed = tokenizeStateData( receiveBuf, numRead, token );
    if (numTokensParsed != 17)
        return NULL;
    int j;
    for (j = 0; j < 17; j +=1)
        printf( ">>>>>>>[%d] [%s]\n", j+1, token[j] );
    
    int     deviceType = Device_parseDeviceType( token[ 3 ] );
    char    *macAddress = Device_parseMacAddress( token[ 15 ] );
 
    //
    //  We are going to use a device's MAC address to uniquely identify it.  That's fine
    //  except the Base Station (Type 1, Record ID = 0) and the Modem (Type 16, Record ID= 2)
    //  don't have MAC Addresses!  So - I could add alot of checking for device type of 1 or 16
    //  or I could jus6t create a MAC address for them. :)
    if (deviceType == 1 || deviceType == 16) {
        debug_print( "Received a state record for the Base Station or Modem. Ignoring.\n", 0 );
        return NULL;
    }
   
    //
    // Now we look to see if we can find this device (by it's MAC address) already in our list of devices
    for (j = 0; j < 17; j +=1)
        printf( ">>555555 >>>>>[%d] [%s]\n", j+1, token[j] );
    // deviceRecPtr = Device_findThisDevice( aSystem->deviceListHead, macAddress );

    
    //
    // Have we found this device already or is it the first time we've seen it?
    int     firstTimeDeviceSeen = FALSE;
    if (deviceRecPtr == NULL) {
        firstTimeDeviceSeen = TRUE;
        deviceRecPtr = Device_newDeviceRecord( macAddress );
        assert( deviceRecPtr != NULL );
    for (j = 0; j < 17; j +=1)
        printf( ">>>>>66666666666>[%d] [%s]\n", j+1, token[j] );

        
        printf( "FIRST TIME DEVICE [%s]\n", macAddress );
        printf( "               1. Head <%p>, RecPtr: <%p>\n", aSystem->deviceListHead, deviceRecPtr );
        
        //
        // Debugging dump the list
        puts( "\n====================== BEFORE =========================\n" );
        HomeHeartBeatDevice_t    *tmpPtr = NULL;
        int     i = 1;
        LL_FOREACH( aSystem->deviceListHead, tmpPtr ) {
            printf( "[[%2d]] %s\n------------------------------------\n", i, Device_dumpDeviceRecord( tmpPtr ));
            i += 1;
        }
        
    for (j = 0; j < 17; j +=1)
        printf( ">>>>>777777777777>[%d] [%s]\n", j+1, token[j] );
        printf( "               2. Head <%p>, RecPtr: <%p>\n", aSystem->deviceListHead, deviceRecPtr );
        
        //
        // Add it to the linked list
        deviceRecPtr->next = NULL;
        LL_APPEND( aSystem->deviceListHead, deviceRecPtr );
        printf( "               3. Head <%p>, RecPtr: <%p>\n", aSystem->deviceListHead, deviceRecPtr );
    for (j = 0; j < 17; j +=1)
        printf( ">>>>>88888888888>[%d] [%s]\n", j+1, token[j] );

        
        
        
        ////////////////////
        /////////// TOKENS OK!!!!!!!!!!
        puts( "\n====================== AFTER =========================\n" );
        //tmpPtr = NULL;
        //i = 1;
        //LL_FOREACH( aSystem->deviceListHead, tmpPtr ) {
        //    printf( "[[%2d]] %s\n------------------------------------\n", i, Device_dumpDeviceRecord( tmpPtr ));
        //    i += 1;
       // }
        printf( "               4. Head <%p>, RecPtr: <%p>\n", aSystem->deviceListHead, deviceRecPtr );

        ////////////// TOKENS WHACKED!!!
        
    for (j = 0; j < 17; j +=1)
        printf( ">>>>>999999999999>[%d] [%s]\n", j+1, token[j] );
        
        int numDevices = 0;
        HomeHeartBeatDevice_t   *elementPtr;
        LL_COUNT( aSystem->deviceListHead, elementPtr, numDevices );             // numDevices is not passed by reference here
        printf( "After addition there are %d devices in the device list.\n", numDevices );
        printf( "               5. Head <%p>, RecPtr: <%p>\n", aSystem->deviceListHead, deviceRecPtr );

    for (j = 0; j < 17; j +=1)
        printf( ">>>>>AAAAAAAAAAAAAAAAA>[%d] [%s]\n", j+1, token[j] );

        //
        // Something we need to do just once and that's initialize the State Changed fields
        deviceRecPtr->stateHasChanged = FALSE;
        deviceRecPtr->lastDeviceState = deviceRecPtr->deviceState;
        deviceRecPtr->lastDeviceStateTimer = deviceRecPtr->deviceStateTimer;
            for (j = 0; j < 17; j +=1)
        printf( ">>>>>BBBBBBBBBBBBB>[%d] [%s]\n", j+1, token[j] );

    }

    printf( "               6. Head <%p>, RecPtr: <%p>\n", aSystem->deviceListHead, deviceRecPtr );

    //
    // TOKENS ARE SCREW UP AT THIS POINT!!!
    
    for (j = 0; j < 17; j +=1)
        printf( ">>22222222222222>>>>>[%d] [%s]\n", j+1, token[j] );
    
    deviceType = Device_parseTokens( deviceRecPtr, token );
    printf( "After calling find - this is what we found: %s", Device_dumpDeviceRecord( deviceRecPtr ) );

    //
    // What we do next depends on what device is reporting in.
    switch (deviceType) {
        case DT_BASE_STATION:   
            break;
        case DT_HOME_KEY:
            break; 
        case DT_OPEN_CLOSE_SENSOR:  OpenClose_parseOneStateRecord( deviceRecPtr );
                                    break;
        case DT_POWER_SENSOR:     
            break;
        case DT_WATER_LEAK_SENSOR:  WaterLeak_parseOneStateRecord( deviceRecPtr );
                                    break;
        case DT_REMINDER_DEVICE:
        case DT_ATTENTION_DEVICE:
        case DT_MODEM:
        case DT_MOTION_SENSOR:      Motion_parseOneStateRecord( deviceRecPtr );
                                    break;
        case DT_TILT_SENSOR:
            break;

        default:
            warnAndKeepGoing( "Unrecognized device type just came through" );
            break;
    }

    printf( "exiting \n\n", 0 );
    return deviceRecPtr;
}


// -----------------------------------------------------------------------------
void    HomeHeartBeat_ReleaseMemory()
{
    HomeHeartBeatDevice_t   *elementPtr1;
    HomeHeartBeatDevice_t   *elementPtr2;
    
    LL_FOREACH_SAFE( aSystem->deviceListHead, elementPtr1, elementPtr2 ) {
        free( elementPtr1->ocSensor );
        LL_DELETE( aSystem->deviceListHead, elementPtr1 );
    }
} 

// -----------------------------------------------------------------------------
static
int    tokenizeStateData (char *receiveBuf, int numRead, char *token[])
{
   debug_print( "entering. recieveBuf[%s], numRead: %d\n", receiveBuf, numRead );
    
    /*
     *  A retrieved state record has the following format:
     *      STATE="F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12,F13,F14,F15,F16,F17"
     *  Where F1 through F17 are the returned data fields. Field 17 (F17) contains the 
     *  ASCII text name of the device. All of the remaining fields are numerical and 
     *  are expressed in hexadecimal notation (base 16).
     */
    const char  delimiters[] = ",";
    char        dataCopy[ 1024 ];
    char        *cPtr;
    
    //
    // Since this is my code, I'm going to use "strsep()" instead of strtok_r
    memcpy( dataCopy, receiveBuf, numRead );
    cPtr = &dataCopy[ 0 ];
    //
    // These functions modify the buffer, that's why we made a copy.
    // I'm OK with putting a hard 17 in here. The HHB is dead and unlikely to change :)
    int     i;
    for (i = 0; i < 17; i += 1) {
        token[ i ] = strsep( &cPtr, delimiters );
        if (token[ i ] == NULL) {
            warnAndKeepGoing( "ERROR:homeheartbeat:parseOneStateRecord(): Hit null token on loop\n" );
            break;
        }
    }
    int numTokensFound = i;
    //debug_print( "Total number of tokens in data: %d\n", numTokensFound );
        
                
    if (numTokensFound != 17) {
       warnAndKeepGoing( "Less than 17 tokens found in the data stream. Considering it corrupt and discarding it all\n" );
       return numTokensFound;
    } 
    
    return 17;
}


// -----------------------------------------------------------------------------
static
HomeHeartBeatDevice_t   *addNewDevice (HomeHeartBeatDevice_t *deviceRecPtr)
{
    //
    //
    if (aSystem->deviceArrayIndex >= 0 && aSystem->deviceArrayIndex < MAX_DEVICES_IN_SYSTEM) {
        aSystem->deviceArray[ aSystem->deviceArrayIndex ] = *deviceRecPtr;
        aSystem->deviceArrayIndex += 1;
        return deviceRecPtr;
    }
    
    return NULL;
}

// -----------------------------------------------------------------------------
static
int     updateExistingDevice (int indexPosition, char *token[])
{
    //
    //
    debug_print( "Updating device at position [%d]\n", indexPosition );
    return Device_parseTokens( &(aSystem->deviceArray[ indexPosition ]), token );     // fill in the fields!
}

// -----------------------------------------------------------------------------
static
HomeHeartBeatDevice_t   *findDevice (char *macAddress, int *indexPosition)
{
    //
    //
    *indexPosition = -1;
    int     i = 0;
    for (i = 0; i < aSystem->deviceArrayIndex; i += 1) {
        
        if (strncmp( macAddress, aSystem->deviceArray[ i ].macAddress, MAX_MAC_ADDRESS_SIZE ) == 0) {
            *indexPosition = i;
            debug_print( "Found matching device at position [%d]\n", *indexPosition );
            return &(aSystem->deviceArray[ i ]);
        }
        
    }
    
    debug_print( "Not found!\n", 0 );
    return NULL;            
}

// -----------------------------------------------------------------------------
static
HomeHeartBeatDevice_t   *findDeviceInList (char *macAddress)
{
    debug_print( "Looking for a device with a MAC address of [%s]\n", macAddress );
    //
    //
    int                     i = 0;
    HomeHeartBeatDevice_t   *elementPtr;
    
    
    elementPtr = NULL;
    LL_FOREACH( aSystem->deviceListHead, elementPtr ) {
        char    *listMac = &(elementPtr->macAddress[ 0 ]);
        
        debug_print( "   [%d] List has: [%s], looking for: [%s]", i, listMac, macAddress );
        
        if (strncmp( listMac, macAddress, MAX_MAC_ADDRESS_SIZE ) == 0) {
            debug_print( " == FOUND!\n", 0 );
            return elementPtr;
        } else {
            debug_print( "\n", 0 );
        }
        
        i += 1;
    }
    
    debug_print( "Not found! Must be a new device!\n", 0 );
    return NULL;            
}

// -----------------------------------------------------------------------------
static
HomeHeartBeatDevice_t   *addNewDeviceToList (HomeHeartBeatDevice_t *deviceRecPtr)
{
    //
    //
    debug_print( "Adding a new device to the list. Mac :[%s]\n", deviceRecPtr->macAddress );
    
    LL_APPEND( aSystem->deviceListHead, deviceRecPtr );
    return NULL;
}

// -----------------------------------------------------------------------------
static
int     updateExistingDeviceInList (HomeHeartBeatDevice_t *deviceRecPtr, char *token[])
{
    //
    //
    debug_print( "Updating device in the list\n", 0 );
    return Device_parseTokens( deviceRecPtr, token );     // fill in the fields!
}


// ----------------------------------------------------------------------------
static  
HomeHeartBeatDevice_t *parseOneStateRecordA (char *receiveBuf, int numRead)
{
    char    *token[ 17 ];
    HomeHeartBeatDevice_t   *deviceRecPtr = NULL;
    

    //
    //  Convert serial port stream into tokens! We know the device sends 17!
    assert( receiveBuf != NULL );
    int numTokensParsed = tokenizeStateData( receiveBuf, numRead, token );
    if (numTokensParsed != 17)
        return NULL;
    
 
    //
    //  We are going to use a device's MAC address to uniquely identify it.  That's fine
    //  except the Base Station (Type 1, Record ID = 0) and the Modem (Type 16, Record ID= 2)
    //  don't have MAC Addresses!  So - I could add alot of checking for device type of 1 or 16
    //  or I could jus6t create a MAC address for them. :)
    int     deviceType = Device_parseDeviceType( token[ 3 ] );
    char    *macAddress = Device_parseMacAddress( token[ 15 ] );
    
    if (deviceType == 1 || deviceType == 16) {
        debug_print( "Received a state record for the Base Station or Modem. Ignoring.\n", 0 );
        return NULL;
    }
   
    int     firstTimeDeviceSeen = FALSE;
    int     arrayIndex = -1;
    //
    // Ok - have we seen this device before?
    //    deviceRecPtr = Device_findThisDevice( aSystem->deviceListHead, macAddress );
    deviceRecPtr = findDevice( macAddress, &arrayIndex );
    if (arrayIndex < 0 || deviceRecPtr == NULL) {
        
        //
        //  Ok - this is the first time we've seen this device!
        firstTimeDeviceSeen = TRUE;
        deviceRecPtr = Device_newDeviceRecord( macAddress );        // create a new record
        
        deviceType = Device_parseTokens( deviceRecPtr, token );     // fill in the fields!
        
        //
        // Something we need to do just once and that's initialize the State Changed fields
        deviceRecPtr->stateHasChanged = FALSE;
        deviceRecPtr->lastDeviceState = deviceRecPtr->deviceState;
        deviceRecPtr->lastDeviceStateTimer = deviceRecPtr->deviceStateTimer;
        
        addNewDevice( deviceRecPtr );                               // add it!
    } else {
        //
        // This is not the first time we've seen this device - just update the data
        updateExistingDevice( arrayIndex, token );
    }

    //
    // What we do next depends on what device is reporting in.
    switch (deviceType) {
        case DT_BASE_STATION:   
            break;
        case DT_HOME_KEY:
            break; 
        case DT_OPEN_CLOSE_SENSOR:  OpenClose_parseOneStateRecord( deviceRecPtr );
                                    break;
        case DT_POWER_SENSOR:     
            break;
        case DT_WATER_LEAK_SENSOR:  WaterLeak_parseOneStateRecord( deviceRecPtr );
                                    break;
        case DT_REMINDER_DEVICE:
        case DT_ATTENTION_DEVICE:
        case DT_MODEM:
        case DT_MOTION_SENSOR:      Motion_parseOneStateRecord( deviceRecPtr );
                                    break;
        case DT_TILT_SENSOR:
            break;

        default:
            warnAndKeepGoing( "Unrecognized device type just came through" );
            break;
    }

    printf( "exiting \n\n", 0 );
    return deviceRecPtr;
}


// ----------------------------------------------------------------------------
static  
HomeHeartBeatDevice_t *parseOneStateRecordL (char *receiveBuf, int numRead)
{
    char    *token[ 17 ];
    HomeHeartBeatDevice_t   *deviceRecPtr = NULL;
    

    //
    //  Convert serial port stream into tokens! We know the device sends 17!
    assert( receiveBuf != NULL );
    int numTokensParsed = tokenizeStateData( receiveBuf, numRead, token );
    if (numTokensParsed != 17)
        return NULL;
    
 
    //
    //  We are going to use a device's MAC address to uniquely identify it.  That's fine
    //  except the Base Station (Type 1, Record ID = 0) and the Modem (Type 16, Record ID= 2)
    //  don't have MAC Addresses!  So - I could add alot of checking for device type of 1 or 16
    //  or I could jus6t create a MAC address for them. :)
    int     deviceType = Device_parseDeviceType( token[ 3 ] );
    char    *macAddress = Device_parseMacAddress( token[ 15 ] );
    
    if (deviceType == 1 || deviceType == 16) {
        debug_print( "Received a state record for the Base Station or Modem. Ignoring.\n", 0 );
        return NULL;
    }
   
    int     firstTimeDeviceSeen = FALSE;
    //
    // Ok - have we seen this device before?
    //    deviceRecPtr = Device_findThisDevice( aSystem->deviceListHead, macAddress );
    deviceRecPtr = findDeviceInList( macAddress );
    if (deviceRecPtr == NULL) {
        
        //
        //  Ok - this is the first time we've seen this device!
        firstTimeDeviceSeen = TRUE;
        deviceRecPtr = Device_newDeviceRecord( macAddress );        // create a new record
        
        deviceType = Device_parseTokens( deviceRecPtr, token );     // fill in the fields!
        
        //
        // Something we need to do just once and that's initialize the State Changed fields
        deviceRecPtr->stateHasChanged = FALSE;
        deviceRecPtr->lastDeviceState = deviceRecPtr->deviceState;
        deviceRecPtr->lastDeviceStateTimer = deviceRecPtr->deviceStateTimer;
        
        addNewDeviceToList( deviceRecPtr );                               // add it!
    } else {
        //
        // This is not the first time we've seen this device - just update the data
        updateExistingDeviceInList( deviceRecPtr, token );
    }

    //
    // What we do next depends on what device is reporting in.
    switch (deviceType) {
        case DT_BASE_STATION:   
            break;
        case DT_HOME_KEY:
            break; 
        case DT_OPEN_CLOSE_SENSOR:  OpenClose_parseOneStateRecord( deviceRecPtr );
                                    break;
        case DT_POWER_SENSOR:     
            break;
        case DT_WATER_LEAK_SENSOR:  WaterLeak_parseOneStateRecord( deviceRecPtr );
                                    break;
        case DT_REMINDER_DEVICE:
        case DT_ATTENTION_DEVICE:
        case DT_MODEM:
        case DT_MOTION_SENSOR:      Motion_parseOneStateRecord( deviceRecPtr );
                                    break;
        case DT_TILT_SENSOR:
            break;

        default:
            warnAndKeepGoing( "Unrecognized device type just came through" );
            break;
    }

    printf( "exiting \n\n", 0 );
    return deviceRecPtr;
}


// -----------------------------------------------------------------------------
//
//  A Little Documentation on Troy Hanson's Linked List stuff
//  (see: http://troydhanson.github.io/uthash/utlist.html)
//
/*
 utlist

Types of lists
Three types of linked lists are supported:

singly-linked lists,

doubly-linked lists, and

circular, doubly-linked lists

Efficiency

For all types of lists, prepending elements and deleting elements are constant-time operations. 
 * Appending to a singly-linked list is an O(n) operation but appending to a doubly-linked list 
 * is constant time using these macros. (This is because, in the utlist implementation of the doubly-linked 
 * list, the head element’s prev member points back to the list tail, even when the list is non-circular). 
 * Sorting is an O(n log(n)) operation. Iteration, counting and searching are O(n) for all list types.

List elements
You can use any structure with these macros, as long as the structure contains a next pointer. If 
 * you want to make a doubly-linked list, the element also needs to have a prev pointer.

typedef struct element {
    char *name;
    struct element *prev; //* needed for a doubly-linked list only 
    struct element *next; // needed for singly- or doubly-linked lists 
} element;
 * 
You can name your structure anything. In the example above it is called element. Within a particular 
 * list, all elements must be of the same type.

Flexible prev/next naming

You can name your prev and next pointers something else. If you do, there is a family of macros 
 * that work identically but take these names as extra arguments.

List head
The list head is simply a pointer to your element structure. You can name it anything. It must be initialized to NULL.

element *head = NULL;
List operations
The lists support inserting or deleting elements, sorting the elements and iterating over them.

Singly-linked	 Doubly-linked	 Circular, doubly-linked
LL_PREPEND(head,add);

DL_PREPEND(head,add);

CDL_PREPEND(head,add;

LL_PREPEND_ELEM(head,elt,add)

DL_PREPEND_ELEM(head,elt,add)

CDL_PREPEND_ELEM(head,elt,add)

LL_REPLACE_ELEM(head,elt,add)

DL_REPLACE_ELEM(head,elt,add)

CDL_REPLACE_ELEM(head,elt,add)

LL_APPEND(head,add);

DL_APPEND(head,add);


LL_CONCAT(head1,head2);

DL_CONCAT(head1,head2);


LL_DELETE(head,del);

DL_DELETE(head,del);

CDL_DELETE(head,del);

LL_SORT(head,cmp);

DL_SORT(head,cmp);

CDL_SORT(head,cmp);

LL_FOREACH(head,elt) {…}

DL_FOREACH(head,elt) {…}

CDL_FOREACH(head,elt) {…}

LL_FOREACH_SAFE(head,elt,tmp) {…}

DL_FOREACH_SAFE(head,elt,tmp) {…}

CDL_FOREACH_SAFE(head,elt,tmp1,tmp2) {…}

LL_SEARCH_SCALAR(head,elt,mbr,val);

DL_SEARCH_SCALAR(head,elt,mbr,val);

CDL_SEARCH_SCALAR(head,elt,mbr,val);

LL_SEARCH(head,elt,like,cmp);

DL_SEARCH(head,elt,like,cmp);

CDL_SEARCH(head,elt,like,cmp);

LL_COUNT(head,elt,count);

DL_COUNT(head,elt,count);

CDL_COUNT(head,elt,count);

Prepend means to insert an element in front of the existing list head (if any), changing the list 
 * head to the new element. Append means to add an element at the end of the list, so it becomes 
 * the new tail element. Concatenate takes two properly constructed lists and appends the second 
 * list to the first. (Visual Studio 2008 does not support LL_CONCAT and DL_CONCAT, but VS2010 is ok.) 
 * To prepend before an arbitrary element instead of the list head, use the _PREPEND_ELEM macro family. 
 * To replace an arbitary list element with another element use the _REPLACE_ELEM family of macros.

The sort operation never moves the elements in memory; rather it only adjusts the list order by altering 
 * the prev and next pointers in each element. Also the sort operation can change the list head 
 * to point to a new element.

The foreach operation is for easy iteration over the list from the head to the tail. A usage 
 * example is shown below. You can of course just use the prev and next pointers directly instead of 
 * using the foreach macros. The foreach_safe operation should be used if you plan to delete any of 
 * the list elements while iterating.

The search operation is a shortcut for iteration in search of a particular element. It is not 
 * any faster than manually iterating and testing each element. There are two forms: the "scalar" 
 * version searches for an element using a simple equality test on a given structure member, while 
 * the general version takes an element to which all others in the list will be compared using a cmp function.

The count operation iterates over the list and increments a supplied counter.

The parameters shown in the table above are explained here:

head
The list head (a pointer to your list element structure).

add
A pointer to the list element structure you are adding to the list.

del
A pointer to the list element structure you are deleting from the list.

elt
A pointer that will be assigned to each list element in succession (see example) in the case of iteration macros; or, the output pointer from the search macros; or the element to be prepended to or replaced.

like
An element pointer, having the same type as elt, for which the search macro seeks a match (if found, the match is stored in elt). A match is determined by the given cmp function.

cmp
pointer to comparison function which accepts two arguments-- these are pointers to two element structures to be compared. The comparison function must return an int that is negative, zero, or positive, which specifies whether the first item should sort before, equal to, or after the second item, respectively. (In other words, the same convention that is used by strcmp). Note that under Visual Studio 2008 you may need to declare the two arguments as void * and then cast them back to their actual types.

tmp
A pointer of the same type as elt. Used internally. Need not be initialized.

mbr
In the scalar search macro, the name of a member within the elt structure which will be tested (using ==) for equality with the value val.

val
In the scalar search macro, specifies the value of (of structure member field) of the element being sought.

count
integer which will be set to the length of the list
 
 */