/* 
 * File:   motion_sensor.h
 * Author: pconroy
 *
 * Created on September 11, 2013, 9:03 AM
 */

#ifndef MOTION_SENSOR_H
#define	MOTION_SENSOR_H

#ifdef	__cplusplus
extern "C" {
#endif


#include "hhb_structures.h"
    
extern  char    *MotionSensor_dumpSensorDeviceRecord (HomeHeartBeatDevice_t *deviceRecPtr);
extern  void    Motion_parseOneStateRecord( HomeHeartBeatDevice_t *deviceRecPtr );
   
    




#ifdef	__cplusplus
}
#endif

#endif	/* MOTION_SENSOR_H */

