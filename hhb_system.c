/* 
 * File:   hhb_structures.c
 * Author: patrick conroy
 *
 * Created on September 17, 2013, 10:03 AM
 * (C) 2013 Patrick Conroy
 */


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
#include <unistd.h>


#include "homeheartbeat.h"
#include "hhb_structures.h"
#include "database.h"
#include "device.h"
#include "mqtt.h"
#include "logger.h"
#include "serialport.h"


#include "openclose_sensor.h"
#include "waterleak_sensor.h"
#include "motion_sensor.h"
#include "tiltsensor.h"
#include "powersensor.h"
#include "utlist.h"

//
//  External definitions that aren't in header files
extern      void IniFile_readIniFile( HomeHeartBeatSystem_t *aSystem );    

//
// Forward declarations
static  int                     getOneStateRecord( char *, int );
static  HomeHeartBeatDevice_t   *parseOneStateRecord( char *receiveBuf, int numRead );
//static  HomeHeartBeatDevice_t   *parseOneStateRecord( char *receiveBuf, int numRead );
static  int                     tokenizeStateData( char *receiveBuf, int numRead, char token[NUM_TOKENS_PER_STATE_CMD][MAX_TOKEN_LENGTH] );
static  void                    releaseMemory( void );
static  void                    turnOffModem( void );
static  void                    turnOffDebug( void );


//
// Every HHB installation has a System
static  HomeHeartBeatSystem_t   *aSystem;


// -----------------------------------------------------------------------------
void    HomeHeartBeatSystem_initialize ()
{
    // log4c_category_debug( log, "entering\n", 0 );
    
    //
    //  We're bringing up a whole new system - initialize everything
    aSystem = malloc( sizeof( HomeHeartBeatSystem_t ) );            // grab some space
    if (aSystem == NULL) {
        Logger_LogFatal( "Out of memory trying to initialize the system. We must exit.\n" );
    }
    
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
    // MQTT Stuff
    aSystem->logEventsToMQTT = FALSE;
    
    Database_setDefaults( aSystem );

    
    //
    //  Read in data from an IniFile if it exists
    // readIniFileValues( aSystem );
    //
    IniFile_readIniFile( aSystem );   
    
    //
    // Let's initialize the logging system
    // printf( "Checking debugging value [%d]\n", aSystem->debugValue );
    // printf( "Checking debugging filename [%s]\n", aSystem->debugFileName );
    // fflush( stdout );

    if (aSystem->debugValue > 0) {
        Logger_Initialize( aSystem->debugFileName, aSystem->debugValue );
    }
    
    //
    // Now Database Stuff
    Database_initialize( aSystem->DBParameters );
    
    
    if (aSystem->logEventsToMQTT) {
        MQTT_setDefaults( aSystem, aSystem->MQTTParameters.mqttBrokerHost );
        MQTT_initialize( aSystem );
    }    
    
    //
    // Log4C initialization
    Logger_LogInfo( "HomeHeartBeat System initialized. Starting up.\n" );
}

// -----------------------------------------------------------------------------
void    HomeHeartBeatSystem_shutdown ()
{
    SerialPort_Close( aSystem->hhbFD );
    aSystem->portOpen = FALSE;

    if (aSystem->logEventsToMQTT) {
        MQTT_teardown();
    }
    
    Database_closeDatabase();
    releaseMemory();
    Logger_LogInfo( "HomeHeartBeat System shut down complete.\n" );
    Logger_Terminate();
}

// -----------------------------------------------------------------------------
void    HomeHeartBeatSystem_setPortName (char *portName)
{
    Logger_FunctionStart( portName );
    Logger_LogDebug( "portName ; [%s]\n", portName );
    strncpy( aSystem->portName, portName, sizeof aSystem->portName );
}

// -----------------------------------------------------------------------------
void    HomeHeartBeatSystem_openPort (char *portName)
{
    Logger_FunctionStart( portName );
    Logger_LogDebug( "portName ; [%s]\n", portName );
    
    if (portName != NULL)
        HomeHeartBeatSystem_setPortName( portName );
    
    //
    // Open up the serial port!
    if ( (aSystem->hhbFD = SerialPort_Open( aSystem->portName )) > 0)
        aSystem->portOpen = TRUE;
  
    if (aSystem->portOpen) {
        //
        //  Kick Base Unit into NO MODEM and Debug Off
        //  No Modem - send 'M' until we get "MODEM=0
        //  Debug Off - send 'a' until we get DBG=0
        turnOffModem();
        turnOffDebug();
    }
}

// -----------------------------------------------------------------------------
void    HomeHeartBeatSystem_eventLoop ()
{
    int     numBytesRead = 0;
    long    numLoops = 0;
    HomeHeartBeatDevice_t   *deviceRecPtr = NULL;
    int     mqttResult = 0;
    
    //
    //  Heads-up - this is allocated on the stack.  Be sure that as we parse out the
    //  tokens, the data - we don't keep references to a pointer in a stack frame 
    //  that will go away!   DAMHIKT.  :)
    //
    char    rawStateRecord[ 256 ];                  // should be 4x bigger than what we need for one state record
    
    Logger_FunctionStart();

    if (!aSystem->portOpen) {
        Logger_LogError( "HHB Port is not open - cannot start event loop\n" );
        return;
    }
    
    aSystem->pollForEvents = TRUE;
    while (aSystem->pollForEvents) {
        numBytesRead = getOneStateRecord( rawStateRecord, sizeof rawStateRecord );
        
        //
        // Sometimes we pick up the STATE=NEW message from the Base Station - we can ignore that
        if (strstr( rawStateRecord, "NEW" ) != NULL) {
            Logger_LogDebug( "Detected STATE=NEW message from device.  Ignoring!\n", 0 );
            continue;
        }
        
        deviceRecPtr = parseOneStateRecord( rawStateRecord, numBytesRead );
        
        //
        // debugging for valgrind
        numLoops += 1L;
        //if (numLoops > 18000L) {
         //   return;
        //}

        if (deviceRecPtr != NULL) {
            //
            //  Update any tables if we have things enabled...
            Database_updateDeviceTables( deviceRecPtr );
    
            if (aSystem->logEventsToMQTT) {
                
                //
                // Have we had a state change???   Hmmmmmmmmmm - why didn't I check deviceRecPtr->deviceInAlarm
                //  I should change this method to deviceStateChange
                if (deviceRecPtr->stateHasChanged)
                    mqttResult = MQTT_createDeviceAlarm( aSystem, deviceRecPtr );
                
                mqttResult = MQTT_createDeviceEvent( aSystem, deviceRecPtr );
                mqttResult = MQTT_sendReceive();
                
                //
                // Need to check for MQTT broker errors and reestablish connections
                if (mqttResult != 0)
                    MQTT_handleError( aSystem, mqttResult );
            }
        }
        
        sleep( aSystem->sleepSecsBetweenEventLoops );
        
        //
        // Reload the INI file each time thru to pick up new values!
        IniFile_readIniFile( aSystem );
        // TO-DO - now that the new values are in, we need to reinitialize things!
    }
}

// -----------------------------------------------------------------------------
static
void    turnOffModem ()
{
    Logger_LogInfo( "Attempting to turn off the HHB System modem.\n", 0 );
    
    //
    //  No Modem - send 'M' until we get "MODEM=0
    //
    int     numAttempts = 0;
    int     modemOn = TRUE;
    char    commandBuf[ 1 ];
    int     numRead;
    char    resultBuf[ 40 ];
    
    while (modemOn && numAttempts < 10) {
        SerialPort_FlushBuffers( aSystem->hhbFD );
    
        commandBuf[ 0 ] = 'M'; 
        SerialPort_SendData( aSystem->hhbFD, commandBuf, 1 );
    
        //
        // Get the data back from the HHB
        memset( resultBuf, '\0', sizeof resultBuf );
        numRead = SerialPort_ReadLine( aSystem->hhbFD, resultBuf, sizeof resultBuf );
        // log4c_category_debug( log, "read returned %d bytes. Data [%s]\n", numRead, rawStateRecord );
        
        if (numRead > 0 && strstr( resultBuf, "MODEM=0" ) != NULL)
            modemOn = FALSE;
    
        numAttempts += 1;
    }
    
    if (modemOn)
        Logger_LogWarning( "WARNING - Unable to turn off HHB System Modem!\n" );
}

// -----------------------------------------------------------------------------
static
void    turnOffDebug ()
{
    Logger_LogInfo( "Attempting to turn off the HHB System debug mode.\n", 0 );
    
    //
    //  Debug Off - send 'a' until we get DBG=0
    //
    int     numAttempts = 0;
    int     debugOn = TRUE;
    char    commandBuf[ 1 ];
    int     numRead;
    char    resultBuf[ 40 ];
    
    while (debugOn && numAttempts < 10) {
        SerialPort_FlushBuffers( aSystem->hhbFD );
    
        commandBuf[ 0 ] = 'a'; 
        SerialPort_SendData( aSystem->hhbFD, commandBuf, 1 );
    
        //
        // Get the data back from the HHB
        memset( resultBuf, '\0', sizeof resultBuf );
        numRead = SerialPort_ReadLine( aSystem->hhbFD, resultBuf, sizeof resultBuf );
        // log4c_category_debug( log, "read returned %d bytes. Data [%s]\n", numRead, rawStateRecord );
        
        if (numRead > 0 && strstr( resultBuf, "DBG=0" ) != NULL)
            debugOn = FALSE;
    
        numAttempts += 1;
    }
    
    if (debugOn)
        Logger_LogWarning( "WARNING - Unable to turn off HHB System Debug Mode!\n" );
}

// ----------------------------------------------------------------------------
static int  getOneStateRecord (char *rawStateRecord, int bufSize)
{
    char    commandBuf[ 2 ];
    int     numRead = 0;

    assert( rawStateRecord != NULL );
    assert( bufSize > 0 );
    
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
    Logger_LogDebug( "SerialPort_ReadLine returned %d bytes. Data [%s]\n", numRead, rawStateRecord );
    
    return numRead;
}   // getOneStateRecord


// -----------------------------------------------------------------------------
static
void    releaseMemory()
{
    HomeHeartBeatDevice_t   *elementPtr1;
    HomeHeartBeatDevice_t   *elementPtr2;
    
    LL_FOREACH_SAFE( aSystem->deviceListHead, elementPtr1, elementPtr2 ) {
        
        int deviceType = elementPtr1->deviceType;
        Logger_LogDebug( "Freeing memory for device [%s] [%s]\n", elementPtr1->deviceName, elementPtr1->macAddress );
        
        switch (deviceType) {
            case DT_BASE_STATION:       /* nothing needed */ ;  break;
            case DT_MODEM:              /* nothing needed */ ;  break;
            
            case DT_HOME_KEY:           /* free( elementPtr1->hkDevice ) */ ;  break;
            case DT_OPEN_CLOSE_SENSOR:  free( elementPtr1->ocSensor );      break;
            case DT_POWER_SENSOR:       free( elementPtr1->psSensor );  break;
            case DT_WATER_LEAK_SENSOR:  free( elementPtr1->wlSensor );      break;
            case DT_REMINDER_DEVICE:    /* free( elementPtr1->remSensor ) */ ;  break;
            case DT_ATTENTION_DEVICE:   /* free( elementPtr1->atSensor ) */ ;  break;
            case DT_MOTION_SENSOR:      free( elementPtr1->motSensor );      break;
            case DT_TILT_SENSOR:        free( elementPtr1->tsSensor );      break;
        
        }
        
        LL_DELETE( aSystem->deviceListHead, elementPtr1 );
    }
    
    Logger_LogDebug( "Freeing memory for HHB System\n", 0 );
    free( aSystem );
} 

// -----------------------------------------------------------------------------
static
HomeHeartBeatDevice_t   *findDeviceInList (char *macAddress)
{
   
    Logger_LogInfo( "Looking for a device in out list with a MAC address of [%s]\n", macAddress );
    //
    //
    int                     i = 0;
    HomeHeartBeatDevice_t   *elementPtr;
    
    
    elementPtr = NULL;
    LL_FOREACH( aSystem->deviceListHead, elementPtr ) {
        char    *listMac = &(elementPtr->macAddress[ 0 ]);
        
        Logger_LogInfo( "   [%d] Element in List has: [%s], looking for: [%s]\n", i, listMac, macAddress );
        
        if (strncmp( listMac, macAddress, MAX_MAC_ADDRESS_SIZE ) == 0) {
            Logger_LogInfo( "       FOUND!\n" );
            return elementPtr;
        } else {
            ;
        }
        
        i += 1;
    }
    
    Logger_LogInfo( "Not found! Must be a new device!\n", 0 );
    return NULL;            
}

// -----------------------------------------------------------------------------
static
HomeHeartBeatDevice_t   *addNewDeviceToList (HomeHeartBeatDevice_t *deviceRecPtr)
{
    //
    //
    Logger_LogInfo( "New HHB Device Detected. Name : [%s], MAC Address : [%s], Type : %d\n", 
            deviceRecPtr->deviceName, 
            deviceRecPtr->macAddress,
            deviceRecPtr->deviceType );
    
    LL_APPEND( aSystem->deviceListHead, deviceRecPtr );
    return NULL;
}

// -----------------------------------------------------------------------------
static
int     updateExistingDeviceInList (HomeHeartBeatDevice_t *deviceRecPtr, char token[NUM_TOKENS_PER_STATE_CMD][MAX_TOKEN_LENGTH])
{
    //
    //
    Logger_LogInfo( "Updating existing HHB Device. Name : [%s], MAC Address : [%s], Type : %d\n", 
            deviceRecPtr->deviceName, 
            deviceRecPtr->macAddress,
            deviceRecPtr->deviceType );
    return Device_parseTokens( deviceRecPtr, token );     // fill in the fields!
}


// ----------------------------------------------------------------------------
static  
HomeHeartBeatDevice_t *parseOneStateRecord (char *receiveBuf, int numRead)
{
    // array of NUM_TOKENS_PER_STATE_CMD strings MAX_TOKEN_LENGTH characters long...
    char                    token[ NUM_TOKENS_PER_STATE_CMD ][ MAX_TOKEN_LENGTH ];           
    HomeHeartBeatDevice_t   *deviceRecPtr = NULL;
    
    //
    //  Convert serial port stream into tokens! We know the device sends NUM_TOKENS_PER_STATE_CMD!
    assert( receiveBuf != NULL );
    
    //
    // receiveBuf coming in is on the stack - don't hold onto pointers to it!
    memset( token, '\0', sizeof token );
    
    int numTokensParsed = tokenizeStateData( receiveBuf, numRead, token );
    if (numTokensParsed != NUM_TOKENS_PER_STATE_CMD) {
        Logger_LogWarning( "Data error. Did not parse correct number of tokens from HHB System.\n" );
        Logger_LogWarning( "Num Tokens Parsed: [%d], Num Chars Read: [%d], Rcv Buf[%s]\n", numTokensParsed, numRead, receiveBuf );
        return NULL;
    }
    
 
    //
    //  We are going to use a device's MAC address to uniquely identify it.  That's fine
    //  except the Base Station (Type 1, Record ID = 0) and the Modem (Type 16, Record ID= 2)
    //  don't have MAC Addresses!  So - I could add alot of checking for device type of 1 or 16
    //  or I could jus6t create a MAC address for them. :)
    int     deviceType = Device_parseDeviceType( token[ 3 ] );
    char    *macAddress = Device_parseMacAddress( token[ 15 ] );
    
    if (deviceType == 1 || deviceType == 16) {
        Logger_LogInfo( "Received a state record for the Base Station or Modem. Ignoring.\n" );
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
        case DT_POWER_SENSOR:       PowerSensor_parseOneStateRecord( deviceRecPtr );
                                    break;
        case DT_WATER_LEAK_SENSOR:  
                                    // Temp debug - look at raw device state timer data
                                    // Logger_LogError( "Water Leak token[5]=[%s] dStateTimer: [%d]\n", token[5], deviceRecPtr->deviceStateTimer );
                                    WaterLeak_parseOneStateRecord( deviceRecPtr );
                                    break;
        case DT_REMINDER_DEVICE:    break;
        case DT_ATTENTION_DEVICE:   break;
        case DT_MODEM:              break;
        case DT_MOTION_SENSOR:      Motion_parseOneStateRecord( deviceRecPtr );
                                    break;
        case DT_TILT_SENSOR:        TiltSensor_parseOneStateRecord( deviceRecPtr );
                                    break;

        default:
            Logger_LogWarning( "Unrecognized device type just came through. Type: [%d]\n", deviceRecPtr->deviceType );
            break;
    }

    return deviceRecPtr;
}

// -----------------------------------------------------------------------------
static
int    tokenizeStateData (char *receiveBuf, int numRead, char token[NUM_TOKENS_PER_STATE_CMD][MAX_TOKEN_LENGTH])
{
    Logger_LogDebug( "Tokenizing receiveBuf : [%s], numRead : %d\n", receiveBuf, numRead );
    
    /*
     *  A retrieved state record has the following format:
     *      STATE="F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12,F13,F14,F15,F16,F17"
     *  Where F1 through F17 are the returned data fields. Field 17 (F17) contains the 
     *  ASCII text name of the device. All of the remaining fields are numerical and 
     *  are expressed in hexadecimal notation (base 16).
     */
   
    //
    // pluck 'em out one by one
    char    *startPtr, *endPtr;
    size_t  numBytes = 0;
   
    //
    // first token is special, advance to the '"' sign and then skip over it
    startPtr = strchr( receiveBuf, '"' );
    if (startPtr == NULL)
        return 0;
    
    startPtr++;
    endPtr = strchr( startPtr, ',' );                     // stop at the comma
    if (endPtr == NULL)
        return 0;
    
    numBytes = (endPtr - startPtr);
    memcpy( token[ 0 ], startPtr, numBytes );
   
    //
    // Now the next 15 are all the same...
    for (int i = 0; i < 15; i +=1) {
        endPtr++;                                       // skip over the last comma
        startPtr = endPtr;                              // start there
        endPtr = strchr( startPtr, ',' );               // stop at the next comma
        if (endPtr == NULL)
            return i;
        
        numBytes = (endPtr - startPtr);
        memcpy( token[ i + 1 ], startPtr, numBytes );
    }
   
    //
    //  Last one we stop at the eol...
    endPtr++;                                           // skip over last comma
    startPtr = endPtr;
    endPtr = strchr( startPtr, '\r' );                  // stop at the CRLF
    if (endPtr == NULL)
        return 16;

    numBytes = (endPtr - startPtr) - 1;                 // -1 to not include trailing '"'
    memcpy( token[ 16 ], startPtr, numBytes );
    
    //for (int j = 0; j < NUM_TOKENS_PER_STATE_CMD; j +=1)
    //    printf( ">>>>>>>>> [%s]\n", token[ j ] );
    
    //
    //  Well - that's a new one - I dropped a character in the middle of the stream.
    //  A four byte field had only three.  I guess I should verify that the tokens are the
    //  appropriate length...
    
    //  Looking at a state record:
    //  Token #     Length
    //      0       2
    //      1       2
    //      2       4
    //      3       4
    //      4       2
    //      5       2
    //      6       2
    //      7       2
    //      8       4
    //      9       2
    //     10       4 
    //     11       2
    //     12       2
    //     13       8
    //     14       2
    //     15       Varies (mac address
    //     16       Varies (dev name)
    //
    int tokenLengths[ 15 ] = { 2, 2, 4, 4, 2, 2, 2, 2, 4, 2, 4, 2, 2, 8, 2 };
    
    for (int j = 0; j < 15; j += 1)
        if (strlen( token[ j ] ) != tokenLengths[ j ]) {
            Logger_LogWarning( "Warning - token[%d] is [%s]. Actual length: %d, should be: %d\n",
                    j, token[ j ], strlen( token[ j ] ), tokenLengths[ j ] );
            //
            // return a value that should cause the whole line to be discarded
            return 0;       
        }
    
    return NUM_TOKENS_PER_STATE_CMD;
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
A pointer that will be assigned to each list element in succession (see example) in the case of 
 * iteration macros; or, the output pointer from the search macros; or the element to be prepended to or replaced.

like
An element pointer, having the same type as elt, for which the search macro seeks a 
 * match (if found, the match is stored in elt). A match is determined by the given cmp function.

cmp
pointer to comparison function which accepts two arguments-- these are pointers to two 
 * element structures to be compared. The comparison function must return an int that 
 * is negative, zero, or positive, which specifies whether the first item should sort before, 
 * equal to, or after the second item, respectively. (In other words, the same convention 
 * that is used by strcmp). Note that under Visual Studio 2008 you may need to declare the 
 * two arguments as void * and then cast them back to their actual types.

tmp
A pointer of the same type as elt. Used internally. Need not be initialized.

mbr
In the scalar search macro, the name of a member within the elt structure which will 
 * be tested (using ==) for equality with the value val.

val
In the scalar search macro, specifies the value of (of structure member field) of the element being sought.

count
integer which will be set to the length of the list
 */
