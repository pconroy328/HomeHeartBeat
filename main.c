/* 
 * File:   main.c
 * Author: patrick.conroy
 *
 * Created on August 27, 2013, 8:50 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <limits.h>

#include "homeheartbeat.h"
#include "logger.h"

/*
 * 
 */
static  int useDatabase = 0;
// log4c_category_t    *logCat;


int main(int argc, char** argv) 
{
    //
    //  I store my Timer data as seconds which means I need room for about 100 days worth of seconds
    //  which is a value of almost 9,000,000.  I need to make sure and "int" type will hold that much
    assert( sizeof( int ) >= 4 );
    assert( (long) INT_MAX > 9000000L);
    
    //
    Logger_Initialize();
    
    HomeHeartBeatSystem_initialize();
    // SerialPort_findBaseStation();

    HomeHeartBeatSystem_openPort( NULL );           // use the INI file values!!
    HomeHeartBeatSystem_eventLoop( );
    HomeHeartBeatSystem_shutdown();

    Logger_Terminate();
    return (EXIT_SUCCESS);
}

