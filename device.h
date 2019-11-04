/* 
 * File:   device.h
 * Author: pconroy
 *
 * Created on September 9, 2013, 10:28 AM
 * 
 *  From our perspective, Home HeartBeat devices include:
 *      Base Station, Home Key, Open/Closed Sensor, Power Sensor
 *      Water Sensor, Reminder Sensor, Attention Sensor, Base Station Modem
 *      Motion Sensor, Tilt Sensor (aka Garage Door Sensor)
 * 
 *  All of these devices have some thingn in common and that will be captured 
 *  here
 */

#ifndef DEVICE_H
#define	DEVICE_H

#ifdef	__cplusplus
extern "C" {
#endif

    
typedef enum    deviceAlert { AlarmTriggered, DeviceOffline, LowBattery, daUnknown } deviceAlert_t;




#include "openclose_sensor.h"
#include "waterleak_sensor.h"
#include "motion_sensor.h"


// -----------------------------------------------------------------------------
//
//  There are times when you really wish you could do some OO things in 'C' easier.
//  This is the Device Superclass.  All Eaton Home Heartbeat devices are of this
//  type.
//


//
//

extern  char                    *Device_dumpDeviceRecord( HomeHeartBeatDevice_t *deviceRecPtr );
extern  HomeHeartBeatDevice_t   *Device_newDeviceRecord( char *macAddress );

extern  int     Device_parseTokens( HomeHeartBeatDevice_t *deviceRecPtr, char token[NUM_TOKENS_PER_STATE_CMD][MAX_TOKEN_LENGTH] );
extern  int     Device_parseStateRecordID( char *token );
extern  int     Device_parseZigbeeBindingID (char *token);
extern  int     Device_parseDeviceState( char *token );
extern  int     Device_parseDeviceCapabilties( char *token );
extern  int     Device_parseDeviceType( char *token );
extern  int     Device_parseDeviceStateTimer( char *token );
extern  int     Device_parseDeviceAlarmTriggered( char *token );
extern  int     Device_parseDeviceOffline( char *token );
extern  int     Device_parseDeviceLowBattery( char *token );
extern  int     Device_parseDeviceBatteryCharing( char *token );
extern  int     Device_parseDeviceOnBatteryBackup( char *token );
extern  int     Device_parseDeviceNameIndex( char * );
extern  int     Device_parseDeviceConfiguration( char *token );
extern  int     Device_parseAliveUpdateTimer( char * );
extern  int     Device_parseUpdateFlags( char *token );
extern  int     Device_parseDeviceParameter( char *token );
extern  int     Device_parsePendingUpdateTimer( char * );


extern  char    *Device_parseMacAddress( char *token );
extern  char    *Device_parseDeviceName( char *token );

extern  void    Device_readDeviceInfoFromFile( HomeHeartBeatSystem_t *aSystem );


#ifdef	__cplusplus
}
#endif

#endif	/* DEVICE_H */

