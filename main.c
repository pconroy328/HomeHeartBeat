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


extern  void    setHHBPort (char *aPortName);
extern  void    openHHBPort (char *aPortName);
extern  void    closeHHBPort();
extern  void    getOneStateRecord ();

/*
 * 
 */
static  int useDatabase = 0;

int main(int argc, char** argv) 
{
    //
    //  I store my Timer data as seconds which means I need room for about 100 days worth of seconds
    //  which is a value of almost 9,000,000.  I need to make sure and "int" type will hold that much
    assert( sizeof( int ) >= 4 );
    assert( (long) INT_MAX > 9000000L);
    
    if (useDatabase) {
        Database_setDatabaseHost( "amhq-webdev-04" );
        Database_setDatabaseUserName( "otterbox" );
        Database_setDatabasePassword( "otterbox" );
        Database_setDatabaseSchema( "Conroy" );
        Database_setFailOnDatabaseErrors( 1 );
        Database_openDatabase();
        Database_dropDeviceStateLogTable(); Database_createDeviceStateLogTable();
        Database_dropDeviceStateCurrentTable(); Database_createDeviceStateCurrentTable();
    }
    
    ;
    
    HomeHeartBeatSystem_Initialize();
    HomeHeartBeatSystem_SetPortName( "/dev/hhb" );
    HomeHeartBeatSystem_OpenPort( NULL );
    HomeHeartBeatSystem_EventLoop( 1 );
    HomeHeartBeatSystem_Shutdown();
    HomeHeartBeat_ReleaseMemory();

    
    
    if (useDatabase) {
        Database_closeDatabase();
    }

    return (EXIT_SUCCESS);
}

