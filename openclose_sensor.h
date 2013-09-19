/* 
 * File:   openclose_sensor.h
 * Author: pconroy
 *
 * Created on August 29, 2013, 10:47 AM
 */


#ifndef OPENCLOSE_SENSOR_H
#define	OPENCLOSE_SENSOR_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "hhb_structures.h"
    
extern  char    *OpenClose_dumpSensorDeviceRecord( HomeHeartBeatDevice_t *deviceRecPtr );
extern  void    OpenClose_parseOneStateRecord( HomeHeartBeatDevice_t *deviceRecPtr );

    



#ifdef	__cplusplus
}
#endif

#endif	/* OPENCLOSE_SENSOR_H */

