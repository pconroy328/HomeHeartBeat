/* 
 * File:   homeheartbeat.h
 * Author: pconroy
 *
 * Created on August 29, 2013, 9:00 AM
 */

#ifndef HOMEHEARTBEAT_H
#define	HOMEHEARTBEAT_H

#include "device.h"
#include "mqtt.h"

#ifdef	__cplusplus
extern "C" {
#endif


    
    
   
typedef struct  MQTT_Parameters {
    //
    // MQTT Specific Information
    int     logEventsToMQTT;
    char    mqttBrokerHost[ 256 ];
    int     mqttPortNumber;
    int     mqttKeepAliveValue;
    int     mqttQoS;
    char    mqttStatusTopic[ 256 ];
    char    mqttAlarmTopic[ 256 ];
    int     mqttRetainMsgs;
} MQTT_Parameters_t;

   
    
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
    MQTT_Parameters_t   MQTTParameters;
} HomeHeartBeatSystem_t;



//
// Home Heart Beat System Function Declarations 
extern  void        homeHeartBeatSystem_Initialize( void );
extern  void        homeHeartBeatSystem_SetPortName( char * );
extern  void        homeHeartBeatSystem_OpenPort( char * );
extern  void        homeHeartBeatSystem_eventLoop( void );
extern  void        homeHeartBeatSystem_Shutdown( void );



extern  void    MQTT_setDefaults( HomeHeartBeatSystem_t *aSystem, char *brokerHostName );
extern  void    MQTT_initialize( HomeHeartBeatSystem_t *aSystem );
extern  void    MQTT_teardown( void );
extern  int     MQTT_SendReceive ( void );
extern  int     MQTT_CreateDeviceEvent( HomeHeartBeatSystem_t *aSystem, HomeHeartBeatDevice_t *deviceRecPtr );
extern  int     MQTT_CreateDeviceAlarm( HomeHeartBeatSystem_t *aSystem, HomeHeartBeatDevice_t *deviceRecPtr );


#ifdef	__cplusplus
}
#endif

#endif	/* HOMEHEARTBEAT_H */

