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

#pragma once                            // widely supported.    

// ----------------------------------------------------------------------------
//
//
#define     ISWET_BITMASK      (0x01)
#define     ISDRY_BITMASK      (0x02)
typedef enum  wlDeviceState { wlWet, wlDry, wlUnknown } wlDeviceState_t;


typedef struct  WaterLeakSensor {
    wlDeviceState_t currentState;               // Field 5   
    int             wetnessDetected;
    
    int             alarmOnWet;                 // Field 9
    int             alarmOnDry;                 // Field 9
    int             callOnWet;                  // Field 9
    int             callOnDry;                  // Field 9
    
    char            *usersDeviceName[ MAX_DEVICE_NAME_LEN ];        // Field 17
} WaterLeakSensor_t;


#ifdef	__cplusplus
}
#endif

#endif	/* WATERLEAK_SENSOR_H */

