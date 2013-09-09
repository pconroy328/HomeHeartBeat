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

    
    

// ----------------------------------------------------------------------------
//  Device State (Field 5) values specific to an Open/Close Sensor
//
#define     ISOPEN_BITMASK      (0x02)
#define     ISCLOSED_BITMASK    (0x01)
typedef enum  ocDeviceState { ocOpen, ocClosed, ocUnknown } ocDeviceState_t;
    

typedef struct  OpenCloseSensor {

    int             stateRecordID;              // Field 1
    int             zigbeeBindingID;            // Field 2
    
    ocDeviceState_t currentState;               // Field 5
    int             isOpen;                     // derived
    // char            *deviceName;                // Field 8  in supertype
    
    int             alarmOnOpen;                // Field 9
    int             alarmOnClose;               // Field 9
    int             callOnOpen;                 // Field 9
    int             callOnClose;                // Field 9
    
    long            aliveUpdateTimerSecs;       // Field 10
    
    long        pendingUpdateTimer;             // Field 14
                                                // ....5....0....5.
    // char            macAddress[ 17 ];           // 000D6F0000011367  Field 16 in supertype
    char            *usersDeviceName[ MAX_DEVICE_NAME_LEN ];        // Field 17
} OpenCloseSensor_t;




#ifdef	__cplusplus
}
#endif

#endif	/* OPENCLOSE_SENSOR_H */

