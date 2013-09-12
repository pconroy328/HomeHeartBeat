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

#include "device.h"

// ----------------------------------------------------------------------------
//  Device State (Field 5) values specific to an Motion Sensor
//
#define     ISNOMOTION_BITMASK    (0x01)
#define     ISMOTION_BITMASK      (0x02)

typedef enum  motDeviceState { motMotion, motNoMotion, motUnknown } motDeviceState_t;
    

typedef struct  MotionSensor {
    motDeviceState_t    currentState;
    int             motionDetected;             // derived
    
    int             alarmOnMotion;
    int             alarmOnNoMotion;
    int             callOnMotion;  
    int             callOnNoMotion;
    
    int             motionDelayValueSecs;       // field 13
    
    char            *usersDeviceName[ MAX_DEVICE_NAME_LEN ];
} MotionSensor_t;



#ifdef	__cplusplus
}
#endif

#endif	/* MOTION_SENSOR_H */

