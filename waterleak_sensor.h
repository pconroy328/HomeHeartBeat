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

/*
//
// Device Alerts (field 7))
#define         ALARMTRIGGERED_BITMASK  (0x01)
#define         DEVICEOFFLINE_BITMASK   (0x02)
#define         LOWBATTERY_BITMASK      (0x04)
typedef enum    wlDeviceAlert { AlarmTriggered, DeviceOffline, LowBattery, UnknownDA } wlDeviceAlert_t;


//
// Device Configuration BitMasks (field 9))
#define     ALARMONWET_BITMASK      (0x0001)
#define     ALARMONDRY_BITMASK      (0x0002)
#define     CALLONWET_BITMASK       (0x0100)
#define     CALLONDRY_BITMASK       (0x0200)
*/

typedef struct  WaterLeakSensor {
    
    
    int             stateRecordID;              // Field 1
    int             zigbeeBindingID;            // Field 2
    
    wlDeviceState_t currentState;               // Field 5
    long            stateTimerSecs;             // Field 6
    
    int             deviceNameIndex;            // Field 8
    char            *deviceName;                // Field 8
    
    int             alarmOnWet;                 // Field 9
    int             alarmOnDry;                 // Field 9
    int             callOnWet;                  // Field 9
    int             callOnDry;                  // Field 9
    
    long            aliveUpdateTimerSecs;       // Field 10
                                                // ....5....0....5.
    char            maxAddress[ 17 ];           // 000D6F0000011367  Field 16
    char            *usersDeviceName[ MAX_DEVICE_NAME_LEN ];        // Field 17
} WaterLeakSensor_t;


#ifdef	__cplusplus
}
#endif

#endif	/* WATERLEAK_SENSOR_H */

