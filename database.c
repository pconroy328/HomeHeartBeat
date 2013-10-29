/* 
 * File:   database.c
 * Author: patrick.conroy
 *
 * Created on September 3, 2013, 2:01 PM
 * (C) 2013 Patrick Conroy
 */


// -----------------------------------------------------------------------------
//
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mysql/mysql.h>

#include "hhb_structures.h"
#include "logger.h"
#include "homeheartbeat.h"
#include "database.h"

//
// We can keep a log of all device readings
static  char    *historyTableName = "HHBHistory";
static  char    *createHistoryTableSQL = "`ID` INT NOT NULL AUTO_INCREMENT ,        \
`deviceType` INT NOT NULL ,           \
`stateRecordID` INT NOT NULL ,\
`zigbeeBindingID` INT NULL ,\
`deviceCapabilities` INT NULL ,\
`deviceState` INT NULL ,\
`deviceStateTimer` INT NULL ,\
`deviceAlerts` INT NULL ,\
`deviceNameIndex` INT NULL ,\
`deviceConfiguration` INT NULL ,\
`aliveUpdateTimer` INT NULL ,\
`updateFlags` INT NULL ,\
`field12` INT NULL ,\
`deviceParameter` INT NULL ,\
`field14` INT NULL ,\
`pendingUpdateTimer` INT NULL ,\
`macAddress` VARCHAR(20) NULL ,\
`deviceName` VARCHAR(80) NULL ,\
`lastUpdate` TIMESTAMP NULL DEFAULT NOW() ,\
PRIMARY KEY (`ID`) ); ";

static  char    *insertHistoryRecordSQL = "INSERT INTO `%s`.`%s` (\
`deviceType`, \
`stateRecordID`, \
`zigbeeBindingID`, \
`deviceCapabilities`, \
`deviceState` , \
`deviceStateTimer`, \
`deviceAlerts` ,\
`deviceNameIndex`  ,\
`deviceConfiguration`  ,\
`aliveUpdateTimer`  ,\
`updateFlags`  ,\
`field12`  ,\
`deviceParameter`  ,\
`field14`  ,\
`pendingUpdateTimer`  ,\
`macAddress`,\
`deviceName` ) VALUES ( \
%d, %d, %d, \
%d, %d, %d,  \
%d, %d, %d,  \
%d, %d, %d,  \
%d, %d, %d,  \
'%s', '%s' );";    


//
// We can keep a log of only state changes
static  char    *alarmTableName = "HHBAlarms";
static  char    *createAlarmTableSQL = "`ID` INT NOT NULL AUTO_INCREMENT ,        \
`deviceType` INT NOT NULL ,           \
`deviceName` VARCHAR(80) NULL ,\
`status` VARCHAR( 40 ) NULL,\
`duration` INT NULL, \
`macAddress` VARCHAR(20) NULL ,\
`lastUpdate` TIMESTAMP NULL DEFAULT NOW() ,\
PRIMARY KEY (`ID`) ); ";

static  char    *insertAlarmRecordSQL = "INSERT INTO `%s`.`%s` (\
`deviceType`,\
`deviceName`,\
`status`,\
`duration`,\
`macAddress`) VALUES ( %d, '%s', '%s', %d, '%s' );";




//
// We can keep a table with the *current* states
static  char    *statusTableName = "HHBStatus";
static  char    *createStatusTableSQL = "`ID` INT NOT NULL AUTO_INCREMENT ,        \
`deviceType` INT NOT NULL ,           \
`stateRecordID` INT NOT NULL ,\
`zigbeeBindingID` INT NULL ,\
`deviceCapabilities` INT NULL ,\
`deviceState` INT NULL ,\
`deviceStateTimer` INT NULL ,\
`deviceAlerts` INT NULL ,\
`deviceNameIndex` INT NULL ,\
`deviceConfiguration` INT NULL ,\
`aliveUpdateTimer` INT NULL ,\
`updateFlags` INT NULL ,\
`field12` INT NULL ,\
`deviceParameter` INT NULL ,\
`field14` INT NULL ,\
`pendingUpdateTimer` INT NULL ,\
`macAddress` VARCHAR(20) NULL ,\
`deviceName` VARCHAR(80) NULL ,\
`lastUpdate` TIMESTAMP NULL DEFAULT NOW() ,\
PRIMARY KEY (`ID`) ); ";

static  char    *updateStatusRecordSQL = "UPDATE `%s`.`%s` SET \
deviceType=%d, \
stateRecordID=%d, \
zigbeeBindingID=%d, \
deviceCapabilities=%d, \
deviceState=%d, \
deviceStateTimer=%d, \
deviceAlerts=%d,\
deviceNameIndex=%d,\
deviceConfiguration=%d,\
aliveUpdateTimer=%d,\
updateFlags=%d,\
field12=%d,\
deviceParameter=%d,\
field14=%d,\
pendingUpdateTimer=%d,\
deviceName='%s', lastUpdate=NOW()  WHERE macAddress='%s';"; 
    
static  char    *insertStatusRecordSQL = "INSERT INTO `%s`.`%s` (\
`deviceType`,\
`stateRecordID`,\
`zigbeeBindingID`,\
`deviceCapabilities`,\
`deviceState`,\
`deviceStateTimer`,\
`deviceAlerts`,\
`deviceNameIndex`,\
`deviceConfiguration`,\
`aliveUpdateTimer`,\
`updateFlags`,\
`field12`,\
`deviceParameter`,\
`field14`,\
`pendingUpdateTimer`,\
`macAddress`,\
`deviceName`) VALUES (\
%d,%d,%d,%d,%d,\
%d,%d,%d,%d,%d,\
%d,%d,%d,%d,%d,\
'%s','%s');";

    
    
    
// -----------------------------------------------------------------------------
static  int     databaseIsOpen = FALSE;
static  MYSQL   *connection = NULL;
static  char    dbHostName[ 256 ];
static  char    dbUserName[ 256 ];
static  char    dbPassword[ 256 ];
static  char    dbSchemaName[ 256 ];
static  int     dbFailOnErrors = FALSE;
static  int     logAlarms = FALSE;
static  int     logStatus = FALSE;
static  int     logHistory = FALSE;
static  int     maxMinutesOfHistoryStored = 180;

// -----------------------------------------------------------------------------
void    Database_setDatabaseHost (char *hostName)
{
    assert( hostName != NULL );
    strncpy( dbHostName, hostName, sizeof( dbHostName ) );
}
// -----------------------------------------------------------------------------
void    Database_setDatabaseUserName (char *userName)
{
    assert( userName != NULL );
    strncpy( dbUserName, userName, sizeof( dbUserName ) );
}
// -----------------------------------------------------------------------------
void    Database_setDatabasePassword (char *password)
{
    assert( password != NULL );
    strncpy( dbPassword, password, sizeof( dbPassword ) );
}

// -----------------------------------------------------------------------------
void    Database_setDatabaseSchema (char *schemaName)
{
    assert( schemaName != NULL );
    strncpy( dbSchemaName, schemaName, sizeof( dbSchemaName ) );
}

// -----------------------------------------------------------------------------
void    Database_setFailOnDatabaseErrors (int newValue)
{
    dbFailOnErrors = newValue;
}

// -----------------------------------------------------------------------------
int     Database_openDatabase ()
{
    if (databaseIsOpen)
        return TRUE;
    
    databaseIsOpen = FALSE;
    connection = mysql_init( NULL );
    if (connection == NULL) {
        Logger_LogError( "Unable to create a client database session (mysql_init call failed).\n" );
        if (dbFailOnErrors)
            exit( 1 );
    }

    if (mysql_real_connect( connection, dbHostName, dbUserName, dbPassword, dbSchemaName, 0, NULL, 0) == NULL) {
        Logger_LogError( "Unable to establish a connection to the database. Host: [%s], User: [%s], Schema: [%s].\n", dbHostName, dbUserName, dbSchemaName );
        Logger_LogError( "%s\n", mysql_error( connection ));
        if (dbFailOnErrors) {
            mysql_close( connection );
            exit( 1 );
        }
    } else {
        databaseIsOpen = TRUE;
    }
    
    return databaseIsOpen;
}        

// -----------------------------------------------------------------------------
int     Database_closeDatabase (void)
{
    if (!databaseIsOpen)
        return TRUE;
    
    mysql_close( connection );
    databaseIsOpen = FALSE;
    connection = NULL;
    return TRUE;
}

// -----------------------------------------------------------------------------
void    Database_setDefaults (HomeHeartBeatSystem_t *aSystem)
{
    aSystem->DBParameters.dropAlarmTable = FALSE;
    aSystem->DBParameters.createAlarmTable = FALSE;
    aSystem->DBParameters.dropStatusTable = FALSE;
    aSystem->DBParameters.createStatusTable = FALSE;
    aSystem->DBParameters.dropHistoryTable = FALSE;
    aSystem->DBParameters.createHistoryTable = FALSE;
    aSystem->DBParameters.maxMinutesOfHistoryStored = 180;
    aSystem->DBParameters.logAlarms = FALSE;
    aSystem->DBParameters.logStatus = FALSE;
    aSystem->DBParameters.logHistory = FALSE;
}

// -----------------------------------------------------------------------------
void    Database_initialize (Database_Parameters_t dbParameters)
{
    if (dbParameters.logAlarms || dbParameters.logStatus || dbParameters.logHistory) {
        Database_setDatabaseHost( &(dbParameters.databaseHost[ 0 ] ) );
        Database_setDatabaseUserName( &(dbParameters.databaseUserName[ 0 ] ) );
        Database_setDatabasePassword( &(dbParameters.databasePassword[ 0 ] ) );
        Database_setDatabaseSchema( &(dbParameters.databaseSchema[ 0 ] ) );
        Database_setFailOnDatabaseErrors( 1 );
        
        if (!Database_openDatabase()) {
            Logger_LogWarning( "Unable to open the database -- continuing\n" );
            return;
        }
        
        if (dbParameters.dropAlarmTable)
            dropAlarmTable();
        if (dbParameters.createAlarmTable)
            createAlarmTable();

        if (dbParameters.dropStatusTable)
            dropStatusTable();
        if (dbParameters.createStatusTable)
            createStatusTable();

        if (dbParameters.dropHistoryTable)
            dropHistoryTable();
        if (dbParameters.createHistoryTable)
            createHistoryTable();

        
        //
        // Make some local copies of the data...
        logAlarms = (dbParameters.logAlarms);
        logHistory = (dbParameters.logHistory);
        logStatus = (dbParameters.logStatus);

        maxMinutesOfHistoryStored = dbParameters.maxMinutesOfHistoryStored;
    }
}

// -----------------------------------------------------------------------------
void    Database_updateDeviceTables (HomeHeartBeatDevice_t *deviceRecPtr)
{
    if (!databaseIsOpen)
        return;
    
    if (logAlarms) {
        //
        // "deviceInAlarm" is NOT the derived field that I came up - it's set by the HHB if the device state is set.
        //  For example:
        //      Motions Sensor:  (Alarm on Motion and Motion Detected) or (Alarm on No Motion and No Motion Detected)
        //      Open / Close Sensor: (Open and Alarm on Open) or (Closed and Alarm on Close)
        //
        //  So far the only device that seems to give me trouble is the Motion Sensor.  I haven't quite figured out how they
        //  clear themselves.  Because I poll the HHB, I'll find a "(Alarm on Motion and Motion Detected)" condition every time I poll.
        //  That's not what I want for motions sensors.  But it may be a good thing for Water Leak sensors!
        //  
        //  So I have a derived field 'deviceRecPtr->stateHasChanged' which only gets set once each state change.
        //     
        
        if (deviceRecPtr->deviceInAlarm && deviceRecPtr->stateHasChanged)
            insertAlarmRecord( deviceRecPtr );
        else
            //
            //  I need to put some thought into the definition of an Alarm.  It should probably mirror what's been
            //  defined by the Base Station.  But, in MQTT, I'm sending actually state change events...
            // Logger_LogDebug( "POSSIBLE PROGRAMMER FAUX PAUS: - logAlarms is TRUE but deviceInAlarm is FALSE\n", 0 );
            ;
    }
    
    if (logStatus)
        updateStatusRecord( deviceRecPtr );
    if (logHistory)
        insertHistoryRecord( deviceRecPtr );
    if (maxMinutesOfHistoryStored > 0)
        deleteHistoryRecords();
}

// -----------------------------------------------------------------------------
static
int     createAlarmTable()
{
    char  buffer[ 1024 ];
    
    if (!databaseIsOpen)
        return FALSE;
    
    memset( buffer, '\0', sizeof buffer );
    snprintf( buffer, sizeof buffer, "CREATE TABLE `%s`.`%s` ( %s", dbSchemaName, alarmTableName, createAlarmTableSQL );
    
    if (mysql_query( connection, buffer ) != 0) {
        Logger_LogError( "Unable to create the alarm table.\n" );
        Logger_LogError( "MySQL Error Message [%s]\n", mysql_error( connection ));
        Logger_LogError( "SQL [%s]\n", buffer );
        return FALSE;
    }
    
    return TRUE;
}

// -----------------------------------------------------------------------------
static
void   dropAlarmTable ()
{
    char buffer[ 1024 ];
    
    if (!databaseIsOpen)
        return;
    
    memset( buffer, '\0', sizeof buffer );
    snprintf( buffer, sizeof buffer, "DROP TABLE `%s`.`%s` ", dbSchemaName, alarmTableName );
    
    if (mysql_query( connection, buffer ) != 0) {
        Logger_LogError( "Unable to drop the alarm table.\n" );
        Logger_LogError( "MySQL Error Message [%s]\n", mysql_error( connection ));
        Logger_LogError( "SQL [%s]\n", buffer );
        if (dbFailOnErrors) {
            //mysql_close( connection );
            //exit( 1 );
        }
    }
}

// -----------------------------------------------------------------------------
static
void    insertAlarmRecord (HomeHeartBeatDevice_t *recPtr)
{
    char    *stateString;
    char    buffer[ 8192 ];

    if (!databaseIsOpen)
        return;
    
    //
    // Type, Name, Status, Duration, MAC address

    stateString = "?unknown?";
    if (recPtr->deviceType == DT_OPEN_CLOSE_SENSOR) {
        stateString = (recPtr->ocSensor->isOpen ? "OPEN" : "CLOSED" );
    } else if (recPtr->deviceType == DT_MOTION_SENSOR) {
        stateString = (recPtr->motSensor->motionDetected ? "MOTION" : "NO MOTION" );
    } else if (recPtr->deviceType == DT_WATER_LEAK_SENSOR) {
        stateString = (recPtr->wlSensor->wetnessDetected ? "WET" : "DRY" );
    } else if (recPtr->deviceType == DT_HOME_KEY) {
        return;
    }
    
        
    memset( buffer, '\0', sizeof buffer );
    
    //
    //  We need to worry about Quoet Marks in the device Name that will screw up SQL
    //  Escape any special characters in the string!
    // unsigned long mysql_real_escape_string(MYSQL *mysql, char *to, const char *from, unsigned long length)    
    char    safeDeviceName[ MAX_DEVICE_NAME_LEN ];
    (void) mysql_real_escape_string( connection, safeDeviceName, recPtr->deviceName, strlen( recPtr->deviceName ) );
    
    int bufferLength = snprintf( buffer, sizeof buffer, insertAlarmRecordSQL,
                            dbSchemaName, alarmTableName,
                            recPtr->deviceType,
                            safeDeviceName,
                            stateString,
                            recPtr->deviceStateTimer,
                            recPtr->macAddress );
    
    
    if (mysql_query( connection, buffer ) != 0) {
        Logger_LogError( "Unable to insert the record into the alarm table.\n" );
        Logger_LogError( "%s\n", mysql_error( connection ));
        Logger_LogError( "SQL: [%s]\n", buffer );
        if (dbFailOnErrors) {
            mysql_close( connection );
            exit( 1 );
        }
    }
}    

// -----------------------------------------------------------------------------
static
int     createHistoryTable()
{
    char  buffer[ 1024 ];
    
    if (!databaseIsOpen)
        return FALSE;
    
    memset( buffer, '\0', sizeof buffer );
    snprintf( buffer, sizeof buffer, "CREATE TABLE `%s`.`%s` ( %s", dbSchemaName, historyTableName, createHistoryTableSQL );
    
    if (mysql_query( connection, buffer ) != 0) {
        Logger_LogError( "Unable to create the history table.\n" );
        Logger_LogError( "%s\n", mysql_error( connection ));
        Logger_LogError( "SQL [%s]\n", buffer );
        return FALSE;
    }
    
    return TRUE;
}

// -----------------------------------------------------------------------------
static
void   dropHistoryTable ()
{
    char buffer[ 1024 ];
    
    if (!databaseIsOpen)
        return;
    
    memset( buffer, '\0', sizeof buffer );
    snprintf( buffer, sizeof buffer, "DROP TABLE `%s`.`%s` ", dbSchemaName, historyTableName );
    
    if (mysql_query( connection, buffer ) != 0) {
        Logger_LogError( "Unable to drop the history table.\n" );
        Logger_LogError( "%s\n", mysql_error( connection ));
        Logger_LogError( "SQL [%s]\n", buffer );
        if (dbFailOnErrors) {
            //mysql_close( connection );
            //exit( 1 );
        }
    }
}

// -----------------------------------------------------------------------------
static
void    insertHistoryRecord (HomeHeartBeatDevice_t *recPtr)
{
    char    buffer[ 8192 ];

    if (!databaseIsOpen)
        return;

    /*
    if (recPtr->deviceType == DT_OPEN_CLOSE_SENSOR)
     * 
     */
    memset( buffer, '\0', sizeof buffer );
    
    //
    //  We need to worry about Quoet Marks in the device Name that will screw up SQL
    //  Escape any special characters in the string!
    // unsigned long mysql_real_escape_string(MYSQL *mysql, char *to, const char *from, unsigned long length)    
    char    safeDeviceName[ MAX_DEVICE_NAME_LEN ];
    (void) mysql_real_escape_string( connection, safeDeviceName, recPtr->deviceName, strlen( recPtr->deviceName ) );
    
    int bufferLength = snprintf( buffer, sizeof buffer, insertHistoryRecordSQL,
                            dbSchemaName, historyTableName,
                            recPtr->deviceType,
                            recPtr->stateRecordID,
                            recPtr->zigbeeBindingID,
                            recPtr->deviceCapabilities,
                            recPtr->deviceState,
                            recPtr->deviceStateTimer,
                            recPtr->deviceAlerts,
                            recPtr->deviceNameIndex,
                            recPtr->deviceConfiguration,
                            recPtr->aliveUpdateTimer,
                            recPtr->updateFlags,
                            0,      // `field12`  
                            recPtr->deviceParameter,
                            0,      //  `field14`  
                            recPtr->pendingUpdateTimer,
                            recPtr->macAddress,
                            safeDeviceName );
    
    
    if (mysql_query( connection, buffer ) != 0) {
        Logger_LogError( "Unable to insert the record into the history table.\n" );
        Logger_LogError( "%s\n", mysql_error( connection ));
        Logger_LogError( "SQL: [%s]\n", buffer );
        if (dbFailOnErrors) {
            mysql_close( connection );
            exit( 1 );
        }
    }
}

// -----------------------------------------------------------------------------
static
void    deleteHistoryRecords (void)
{
    char    *deleteRecordsSQL = "DELETE FROM `%s`.`%s` WHERE lastUpdate < NOW() - INTERVAL %d MINUTE";
    char    buffer[ 1024 ];
    
    if (!databaseIsOpen)
        return;
    
    memset( buffer, '\0', sizeof buffer );
    snprintf( buffer, sizeof buffer, deleteRecordsSQL, dbSchemaName, historyTableName, maxMinutesOfHistoryStored );
    
    if (mysql_query( connection, buffer ) != 0) {
        Logger_LogError( "Unable to delete all but the last %d minutes of history stored in the table.\n", maxMinutesOfHistoryStored );
        Logger_LogError( "%s\n", mysql_error( connection ));
        Logger_LogError( "SQL [%s]\n", buffer );
        if (dbFailOnErrors) {
            //mysql_close( connection );
            //exit( 1 );
        }
    }
    
    Logger_LogDebug( "History records deleted!\n", 0 );
}
// -----------------------------------------------------------------------------
static
int     createStatusTable(void)
{
    char    buffer[ 4096 ];
    
    if (!databaseIsOpen)
        return FALSE;
    
    memset( buffer, '\0', sizeof buffer );
    int bufferLength = snprintf( buffer, sizeof buffer, "CREATE TABLE `%s`.`%s` ( %s", 
                                dbSchemaName, statusTableName, createStatusTableSQL );
    
    if (mysql_query( connection, buffer ) != 0) {
        Logger_LogError( "Unable to create the status table.\n" );
        Logger_LogError( "%s\n", mysql_error( connection ));
        return FALSE;
    }
    return TRUE;
}

// -----------------------------------------------------------------------------
static
void    dropStatusTable (void)
{
    char buffer[ 1024 ];
    
    memset( buffer, '\0', sizeof buffer );
    
    snprintf( buffer, sizeof buffer, "DROP TABLE `%s`.`%s` ", dbSchemaName, statusTableName );
    
    if (mysql_query( connection, buffer ) != 0) {
        Logger_LogError( "Unable to drop the status table.\n" );
        Logger_LogError( "%s\n", mysql_error( connection ));
        if (dbFailOnErrors) {
            //mysql_close( connection );
            //exit( 1 );
        }
    }
}

// -----------------------------------------------------------------------------
static
int     statusRecordExists (char *macAddress)
{
    //
    // Darned SQL...  
    char    buffer[ 1024 ];
    
    char    *countSQL = "SELECT COUNT(*) FROM `%s`.`%s` WHERE macAddress='%s'";
    memset( buffer, '\0', sizeof buffer );
    snprintf( buffer, sizeof buffer, countSQL,
                            dbSchemaName, statusTableName,
                            macAddress );
    
    if (mysql_query( connection, buffer ) != 0) {
        Logger_LogError( "Unable to see if the record is already in the status table.\n" );
        Logger_LogError( "SQL: [%s]\n", buffer );
        Logger_LogError( "%s\n", mysql_error( connection ));
        if (dbFailOnErrors) {
            mysql_close( connection );
            exit( 1 );
        }
    }
    
    //
    //  Is the device already there?
    MYSQL_RES *result = mysql_store_result( connection );
    if (result == NULL)  {
        Logger_LogError( "%s\n", mysql_error( connection ));
    }

    int         num_fields = mysql_num_fields( result );
    MYSQL_ROW   row = mysql_fetch_row( result );
    char        *data = row[ 0 ];
    mysql_free_result( result );    
  
    return (data[ 0 ] == '0');
}


// -----------------------------------------------------------------------------
static
void    updateStatusRecord (HomeHeartBeatDevice_t *recPtr)
{
    char    buffer[ 8192 ];
    char    buffer2[ 8192 ];
    
    
    if (!databaseIsOpen)
        return;

    int     doInsertNotUpdate = statusRecordExists( recPtr->macAddress );

    //
    //  We need to worry about quote Marks in the device Name that will screw up SQL
    //  Escape any special characters in the string!
    // unsigned long mysql_real_escape_string(MYSQL *mysql, char *to, const char *from, unsigned long length)    
    char    safeDeviceName[ MAX_DEVICE_NAME_LEN ];
    (void) mysql_real_escape_string( connection, safeDeviceName, recPtr->deviceName, strlen( recPtr->deviceName ) );


    //
    //
    int bufferLength = 0;
    memset( buffer, '\0', sizeof buffer );
    if (doInsertNotUpdate) {
        bufferLength = snprintf( buffer, sizeof buffer, insertStatusRecordSQL,
                            dbSchemaName, statusTableName,
                            recPtr->deviceType,
                            recPtr->stateRecordID,
                            recPtr->zigbeeBindingID,
                            recPtr->deviceCapabilities,
                            recPtr->deviceState,
                            recPtr->deviceStateTimer,
                            recPtr->deviceAlerts,
                            recPtr->deviceNameIndex,
                            recPtr->deviceConfiguration,
                            recPtr->aliveUpdateTimer,
                            recPtr->updateFlags,
                            0, // `field12`  
                            recPtr->deviceParameter,
                            0,  //  `field14`  
                            recPtr->pendingUpdateTimer,
                            recPtr->macAddress, 
                            safeDeviceName );
    } else { 
        bufferLength = snprintf( buffer, sizeof buffer, updateStatusRecordSQL,
                            dbSchemaName, statusTableName,
                            recPtr->deviceType,
                            recPtr->stateRecordID,
                            recPtr->zigbeeBindingID,
                            recPtr->deviceCapabilities,
                            recPtr->deviceState,
                            recPtr->deviceStateTimer,
                            recPtr->deviceAlerts,
                            recPtr->deviceNameIndex,
                            recPtr->deviceConfiguration,
                            recPtr->aliveUpdateTimer,
                            recPtr->updateFlags,
                            0, // `field12`  
                            recPtr->deviceParameter,
                            0,  //  `field14`  
                            recPtr->pendingUpdateTimer,
                            safeDeviceName,
                            recPtr->macAddress );
    }
    
    
    if (mysql_query( connection, buffer ) != 0) {
        Logger_LogError( "Unable to insert or update the record into the device table.\n" );
        Logger_LogError( "%s\n", mysql_error( connection ));
        Logger_LogError( "SQL: [%s]\n", buffer );
        if (dbFailOnErrors) {
            mysql_close( connection );
            exit( 1 );
        }
    }
}
