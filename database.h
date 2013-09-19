/* 
 * File:   database.h
 * Author: patrick.conroy
 *
 * Created on September 3, 2013, 2:01 PM
 */

#ifndef DATABASE_H
#define	DATABASE_H

#include "hhb_structures.h"


#ifdef	__cplusplus
extern "C" {
#endif
    
extern  void    Database_setDatabaseHost( char *hostName );
extern  void    Database_setDatabaseUserName( char *userName );
extern  void    Database_setDatabasePassword( char *password );
extern  void    Database_setDatabaseSchema( char *schemaName );
extern  void    Database_setFailOnDatabaseErrors(int newValue );
    
    
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

