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
    
typedef struct  MQTT_Parameters {
    //
    // MQTT Specific Information
    int     logEventsToMQTT;
    char    *mqttBrokerHost;
    int     mqttPortNumber;
    int     mqttKeepAliveValue;
    
    char    *mqttTopic;
} MQTT_Parameters_t;


extern  void    MQTT_setDefaults (HomeHeartBeatSystem_t *aSystem, char *brokerHostName);
extern  void    MQTT_initialize (HomeHeartBeatSystem_t *aSystem);
extern  void    MQTT_teardown( void );
extern  int     MQTT_SendReceive ( void );
extern  void    MQTT_CreateDeviceEvent( HomeHeartBeatDevice_t deviceRecPtr );


#ifdef	__cplusplus
}
#endif

#endif	/* MQTT_H */

