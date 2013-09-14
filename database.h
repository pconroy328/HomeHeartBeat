/* 
 * File:   database.h
 * Author: patrick.conroy
 *
 * Created on September 3, 2013, 2:01 PM
 */

#ifndef DATABASE_H
#define	DATABASE_H

#include "homeheartbeat.h"


#ifdef	__cplusplus
extern "C" {
#endif
    
    
    
extern  int     Database_openDatabase ();
extern  int     Database_closeDatabase ();
    
    
extern  int     Database_createDeviceStateLogTable();
extern  void    Database_dropDeviceStateLogTable ();
extern  void    Database_insertDeviceStateLogRecord (HomeHeartBeatDevice_t *recPtr);

extern  int     Database_createDeviceStateCurrentTable();
extern  void    Database_dropDeviceStateCurrentTable();
extern  void    Database_updateDeviceStateCurrentRecord (HomeHeartBeatDevice_t *recPtr);


#ifdef	__cplusplus
}
#endif

#endif	/* DATABASE_H */

