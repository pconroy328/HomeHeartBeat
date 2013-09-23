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
extern  void    Database_setFailOnDatabaseErrors( int newValue );
extern  void    Database_setDefaults( HomeHeartBeatSystem_t *aSystem );
extern  void    Database_initialize( Database_Parameters_t dbParameters );
extern  void    Database_updateDeviceTables( HomeHeartBeatDevice_t *deviceRecPtr );
    
    
extern  int     Database_openDatabase ();
extern  int     Database_closeDatabase ();
    

static  int     createAlarmTable( void );
static  void    dropAlarmTable( void );
static  void    insertAlarmRecord( HomeHeartBeatDevice_t *recPtr );

static  int     createHistoryTable( void );
static  void    dropHistoryTable( void );
static  void    insertHistoryRecord( HomeHeartBeatDevice_t *recPtr );
static  void    deleteHistoryRecords( void );


static  int     createStatusTable( void );
static  void    dropStatusTable( void );
static  int     statusRecordExists( char *macAddress );
static  void    updateStatusRecord( HomeHeartBeatDevice_t *recPtr );


#ifdef	__cplusplus
}
#endif

#endif	/* DATABASE_H */

