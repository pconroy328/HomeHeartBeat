/* 
 * File:   powersensor.h
 * Author: pconroy
 *
 * Created on February 25, 2014, 4:52 PM
 */

#ifndef POWERSENSOR_H
#define	POWERSENSOR_H

#ifdef	__cplusplus
extern "C" {
#endif


#include "hhb_structures.h"
    
extern  char    *PowerSensor_dumpSensorDeviceRecord( HomeHeartBeatDevice_t *deviceRecPtr );
extern  void    PowerSensor_parseOneStateRecord( HomeHeartBeatDevice_t *deviceRecPtr );




#ifdef	__cplusplus
}
#endif

#endif	/* POWERSENSOR_H */

