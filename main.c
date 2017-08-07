/* 
 * File:   main.c
 * Author: patrick.conroy
 *
 * Created on August 27, 2013, 8:50 AM
 * (C) 2013 Patrick Conroy
 * 
 * 
 * 
 * To Do: when a sensor is off-line, ignore the warning that it is reporting both states
 * * sometime in 2017 - removing TOPIC from mqtt payload prequel
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <limits.h>

#include "homeheartbeat.h"
#include "logger.h"


//
// V3.0 - Better recovery from MQTT errors (I hope)
//  V3.0.1  -   have sensors ignore both states set warning if they're offline
//  V4.0.0  -   Use Alternate Device Names from TXT file
static  char                *version = "v4.1.0 [ MQTT topic inside of JSON  ]";




// -----------------------------------------------------------------------------
int main(int argc, char** argv) 
{
    puts( "Home HeartBeat System" );
    printf( "Version: %s\n", version );


    //
    //  I store my Timer data a seconds which means I need room for about 100 days worth of seconds
    //  which is a value of almost 9,000,000.  I need to make sure and "int" type will hold that much
    assert( sizeof( int ) >= 4 );
    assert( (long) INT_MAX > 9000000L);
    
    HomeHeartBeatSystem_initialize();

    HomeHeartBeatSystem_openPort( NULL );           // use the INI file values!!
    HomeHeartBeatSystem_eventLoop( );
    HomeHeartBeatSystem_shutdown();

    return (EXIT_SUCCESS);
}

