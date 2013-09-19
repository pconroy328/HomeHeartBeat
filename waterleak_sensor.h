/* 
 * File:   waterleak_sensor.h
 * Author: pconroy
 *
 * Created on August 29, 2013, 10:48 AM
 */



#ifndef WATERLEAK_SENSOR_H
#define	WATERLEAK_SENSOR_H

#ifdef	__cplusplus
extern "C" {
#endif


#include "hhb_structures.h"
    
extern  char    *WaterLeak_dumpDeviceRecord( HomeHeartBeatDevice_t *deviceRecPtr );
extern  void    WaterLeak_parseOneStateRecord( HomeHeartBeatDevice_t *deviceRecPtr );


#ifdef	__cplusplus
}
#endif

#endif	/* WATERLEAK_SENSOR_H */

