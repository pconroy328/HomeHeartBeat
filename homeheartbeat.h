/* 
 * File:   homeheartbeat.h
 * Author: pconroy
 *
 * Created on August 29, 2013, 9:00 AM
 */

#ifndef HOMEHEARTBEAT_H
#define	HOMEHEARTBEAT_H

//#include "device.h"
// #include "mqtt.h"

#ifdef	__cplusplus
extern "C" {
#endif

//
// Home Heart Beat System Function Declarations 
extern  void        HomeHeartBeatSystem_initialize( void );
extern  void        HomeHeartBeatSystem_setPortName( char * );
extern  void        HomeHeartBeatSystem_openPort( char * );
extern  void        HomeHeartBeatSystem_eventLoop( void );
extern  void        HomeHeartBeatSystem_shutdown( void );


#ifdef	__cplusplus
}
#endif

#endif	/* HOMEHEARTBEAT_H */

