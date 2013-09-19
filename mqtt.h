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
    
#define MQTT_NOT_CONNECTED      (-1)
    
    
extern  void    MQTT_setDefaults( HomeHeartBeatSystem_t *aSystem, char *brokerHostName );
extern  void    MQTT_initialize (HomeHeartBeatSystem_t *aSystem );
extern  void    MQTT_teardown(  void );
extern  int     MQTT_sendReceive( void );
extern  int     MQTT_createDeviceAlarm( HomeHeartBeatSystem_t *aSystem, HomeHeartBeatDevice_t *deviceRecPtr );
extern  int     MQTT_createDeviceEvent( HomeHeartBeatSystem_t *aSystem, HomeHeartBeatDevice_t *deviceRecPtr );
extern  int     MQTT_handleError( HomeHeartBeatSystem_t *aSystem, int errorCode );


#ifdef	__cplusplus
}
#endif

#endif	/* MQTT_H */

