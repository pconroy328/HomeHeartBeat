/* 
 * File:   helpers.h
 * Author: pconroy
 *
 * Created on August 29, 2013, 4:04 PM
 */

#ifndef HELPERS_H
#define	HELPERS_H

#ifdef	__cplusplus
extern "C" {
#endif
 
    
#ifndef     FALSE
# define    FALSE   (0)
# define    TRUE    (!FALSE)    
#endif
    
    
#include <stdio.h>
    
//
// Selective (but compile time) debugging 
#define DEBUG       1
    
#define debug_print(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)

    
    
extern  void    haltAndCatchFire( char * );
extern  void    warnAndKeepGoing( char *message );

extern  int     HomeHeartBeat_parseStateRecordID (char *token);
extern  int     HomeHeartBeat_parseZigbeeBindingID (char *token);
extern  int     HomeHeartBeat_parseDeviceCapabilties (char *token);
extern  int     HomeHeartBeat_parseDeviceType (char *token);
extern  int     HomeHeartBeat_parseDeviceState( char *token, int deviceType );
extern  long    HomeHeartBeat_parseDeviceStateTimer (char *token);
extern  int     HomeHeartBeat_parseDeviceAlerts (char *token);
extern  int     HomeHeartBeat_parseDeviceNameIndex( char * );
extern  int     HomeHeartBeat_deviceAliveUpdateTimer( char * );
extern  char    *HomeHeartBeat_parseMacAddress( char *token );
extern  char    *HomeHeartBeat_parseDeviceName( char *token );


#ifdef	__cplusplus
}
#endif

#endif	/* HELPERS_H */

