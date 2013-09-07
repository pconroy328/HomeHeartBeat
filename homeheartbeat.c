// ----------------------------------------------------------------------------
// 27-Aug-2013
//
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <ctype.h>



//
// Forward declarations
static  char    *readLine( int * );
static  int     parseStateRecordID( char * );
static  int     parseZigbeeBindingID( char * );
static  int     parseDeviceCapabilties (char *token);
static  int     parseDeviceType (char *token);
static  int     parseDeviceState (char *token, int type);


/*
// ----------------------------------------------------------------------------
void    setHHBPort (char *aPortName)
{
    if (debug) 
        printf( "DEBUG:homeheartbeat:setHHBPort(): aPortName [%s]\n", (aPortName != NULL ? aPortName : "NULL" ) );
        
    strncpy( portName, aPortName, sizeof( portName ) );
}

// ----------------------------------------------------------------------------
void    openHHBPort (char *aPortName)
{
    if (debug) 
        printf( "DEBUG:homeheartbeat:setHHBPort(): openHHBPort [%s]\n", (aPortName != NULL ? aPortName : "NULL" ) );

    if (aPortName != NULL)
        setHHBPort( aPortName );
    
    if (RS232_Open() > 0)
        portIsOpen = TRUE;
    // if (RS232_Open2() > 0)
    //    portIsOpen = TRUE;
    //if (RS232_Open3() > 0)
    //    portIsOpen = TRUE;
}

// ----------------------------------------------------------------------------
void    closeHHBPort()
{
    if (debug) 
        printf( "DEBUG:homeheartbeat:closeHHBPort()" );

    RS232_Close();
}

*/



// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
