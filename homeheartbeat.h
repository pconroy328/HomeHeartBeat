/* 
 * File:   homeheartbeat.h
 * Author: pconroy
 *
 * Created on August 29, 2013, 9:00 AM
 */

#ifndef HOMEHEARTBEAT_H
#define	HOMEHEARTBEAT_H


#ifdef	__cplusplus
extern "C" {
#endif


    

#include "device.h"
    
// -----------------------------------------------------------------------------
//
//  A system consists of the Base Station, Modem (in the base station), the Home
//  key and all of the devices
//

typedef struct  HomeHeartBeatSystem {
    int     systemID;                                   // use this as you see fit
    char    name[ 80 ];                                 // give this system a name, maybe you have > 1
    
    char    addressLine1[ 80 ];                         // where your system is located
    char    addressLine2[ 80 ];                         //      sorry for the N.A. centrc
    char    city[ 80 ];                                 //      approach to locations
    char    stateOrProvinceCode[ 80 ];
    char    postalCode[ 80 ];
    
    double  latitude;
    double  longitude;
    int     TZOffsetMins;                               // difference in minutes between UTC and *here*

    int     debugValue; 
    
    //
    // Now the collection of devices is a linked list, so we need a 'head' pointer
    HomeHeartBeatDevice_t   *deviceListHead;
    HomeHeartBeatDevice_t   deviceArray[ MAX_DEVICES_IN_SYSTEM ];
    int                     deviceArrayIndex;
    
    //
    // Port specifics
    char    portName[ 256 ];                            // e.g. /dev/hhb or /dev/ttyUSB0
    int     hhbFD;                                      // file descriptor
    int     portOpen;
    int     pollForEvents;
    int     sleepSecsBetweenEventLoops;
    
    //
    // Database Specific Information
    int     logEventsToDatabase;
    char    databaseHost[ 256 ];
    char    databaseUserName[ 256 ];
    char    databasePassword[ 256 ];
    char    databaseSchema[ 256 ];
    
    //
    // MQTT Specific Information
    int     logEventsToMQTT;
    char    mqttBrokerHost[ 256 ];
    int     mqttPortNumber;
    int     mqttKeepAliveValue;
} HomeHeartBeatSystem_t;



//
// Home Heart Beat System Function Declarations 
extern  void        homeHeartBeatSystem_Initialize( void );
extern  void        homeHeartBeatSystem_SetPortName( char * );
extern  void        homeHeartBeatSystem_OpenPort( char * );
extern  void        homeHeartBeatSystem_eventLoop( void );
extern  void        homeHeartBeatSystem_Shutdown( void );



#ifdef	__cplusplus
}
#endif

#endif	/* HOMEHEARTBEAT_H */

