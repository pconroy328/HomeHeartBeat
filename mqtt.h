/* 
 * File:   mqtt.h
 * Author: pconroy
 *
 * Created on September 6, 2013, 6:08 PM
 */

#ifndef MQTT_H
#define	MQTT_H



#ifdef	__cplusplus
extern "C" {
#endif

#include "hhb_structures.h"
    
    
#ifndef FALSE
# define FALSE  0
# define TRUE (!FALSE)
#endif
    
#define MQTT_NOT_CONNECTED      (-1)
    

extern  void    MQTT_SetDefaults( MQTT_Parameters_t *mqttParams );
extern  int     MQTT_Initialize( MQTT_Parameters_t *mqttParams );
extern  void    MQTT_Teardown( void );
extern  int     MQTT_SendReceive( int * );
extern  int     MQTT_HandleError( MQTT_Parameters_t *mqttParams, int errorCode );
extern  int     MQTT_Publish( char *topic, char *payload, int payloadLength );

extern  int     MQTT_createDeviceAlarm (HomeHeartBeatSystem_t *aSystem, HomeHeartBeatDevice_t *deviceRecPtr);
extern  int     MQTT_createDeviceEvent (HomeHeartBeatSystem_t *aSystem, HomeHeartBeatDevice_t *deviceRecPtr);


#ifdef	__cplusplus
}
#endif

#endif	/* MQTT_H */

