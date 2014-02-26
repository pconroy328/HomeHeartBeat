/* 
 * File:   tiltsensor.h
 * Author: pconroy
 *
 * Created on November 4, 2013, 8:31 PM
 */

#ifndef TILTSENSOR_H
#define	TILTSENSOR_H

#ifdef	__cplusplus
extern "C" {
#endif


#include "hhb_structures.h"
    
extern  char    *TiltSensor_dumpSensorDeviceRecord( HomeHeartBeatDevice_t *deviceRecPtr );
extern  void    TiltSensor_parseOneStateRecord( HomeHeartBeatDevice_t *deviceRecPtr );

    





#ifdef	__cplusplus
}
#endif

#endif	/* TILTSENSOR_H */

