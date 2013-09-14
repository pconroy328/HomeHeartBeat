// -----------------------------------------------------------------------------
//
//
//

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mysql/mysql.h>

//#include "homeheartbeat.h"
#include "helpers.h"
#include "database.h"

//
// We can keep a log of all device readings
static  char    *deviceStateLogTableName = "HHBDeviceStateLog";

//
// We can keep a log of only state changes
static  char    *deviceStateChangeLogTableName = "HHBDeviceChangeStateLog";

//
// We can keep a table with the *current* states
static  char    *deviceStateCurrentTableName = "HHBDeviceCurrentState";


static  int     databaseIsOpen = FALSE;
static  MYSQL   *connection = NULL;
static  char    *dbHostName;
static  char    *dbUserName;
static  char    *dbPassword;
static  char    *dbSchemaName;
static  int     dbFailOnErrors = FALSE;

// -----------------------------------------------------------------------------
void    Database_setDatabaseHost (char *hostName)
{
    assert( hostName != NULL );
    dbHostName = hostName;
}
// -----------------------------------------------------------------------------
void    Database_setDatabaseUserName (char *userName)
{
    assert( userName != NULL );
    dbUserName = userName;
}
// -----------------------------------------------------------------------------
void    Database_setDatabasePassword (char *password)
{
    assert( password != NULL );
    dbPassword = password;
}

// -----------------------------------------------------------------------------
void    Database_setDatabaseSchema (char *schemaName)
{
    assert( schemaName != NULL );
    dbSchemaName = schemaName;
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
    
    
    connection = mysql_init( NULL );
    if (connection == NULL) {
        fprintf( stderr, "Unable to establish a client database session.\n" );
        if (dbFailOnErrors)
            exit( 1 );
    }

    if (mysql_real_connect( connection, dbHostName, dbUserName, dbPassword, dbSchemaName, 0, NULL, 0) == NULL) {
        fprintf( stderr, "Unable to establish a connection to the database.\n" );
        fprintf( stderr, "%s\n", mysql_error( connection ));
        if (dbFailOnErrors) {
            mysql_close( connection );
            exit( 1 );
        }
    } else {
        databaseIsOpen = TRUE;
    }
    
    return TRUE;
}        

// -----------------------------------------------------------------------------
int     Database_closeDatabase ()
{
    mysql_close( connection );
    databaseIsOpen = FALSE;
    connection = NULL;
    return TRUE;
}

// -----------------------------------------------------------------------------
int     Database_createDeviceStateLogTable()
{
    char *sql = "`ID` INT NOT NULL AUTO_INCREMENT ,        \
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
    char  buffer[ 1024 ];
    
    
    memset( buffer, '\0', sizeof buffer );
    snprintf( buffer, sizeof buffer, "CREATE TABLE `%s`.`%s` ( %s", dbSchemaName, deviceStateLogTableName, sql );
    
    if (mysql_query( connection, buffer ) != 0) {
        fprintf( stderr, "Unable to create the device table.\n" );
        fprintf( stderr, "%s\n", mysql_error( connection ));
        if (dbFailOnErrors) {
            mysql_close( connection );
            exit( 1 );
        }
    }
}

// -----------------------------------------------------------------------------
void    Database_dropDeviceStateLogTable ()
{
    char buffer[ 1024 ];
    
    memset( buffer, '\0', sizeof buffer );
    
    snprintf( buffer, sizeof buffer, "DROP TABLE `%s`.`%s` ", dbSchemaName, deviceStateLogTableName );
    
    if (mysql_query( connection, buffer ) != 0) {
        fprintf( stderr, "Unable to drop the device table.\n" );
        fprintf( stderr, "%s\n", mysql_error( connection ));
        if (dbFailOnErrors) {
            //mysql_close( connection );
            //exit( 1 );
        }
    }
}

// -----------------------------------------------------------------------------
void    Database_insertDeviceStateLogRecord (HomeHeartBeatDevice_t *recPtr)
{
    char    buffer[ 8192 ];
    char    *sql = "INSERT INTO `%s`.`%s` (\
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


    /*
    if (recPtr->deviceType == DT_OPEN_CLOSE_SENSOR)
     * 
     */
    memset( buffer, '\0', sizeof buffer );
    snprintf( buffer, sizeof buffer, sql,
                            dbSchemaName, deviceStateLogTableName,
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
                            recPtr->deviceName );
    
    
    if (mysql_query( connection, buffer ) != 0) {
        fprintf( stderr, "Unable to insert the record into the device table.\n" );
        fprintf( stderr, "SQL: [%s]\n", buffer );
        fprintf( stderr, "%s\n", mysql_error( connection ));
        if (dbFailOnErrors) {
            mysql_close( connection );
            exit( 1 );
        }
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
int     Database_createDeviceStateCurrentTable()
{
    char *sql = "`ID` INT NOT NULL AUTO_INCREMENT ,        \
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
    char  buffer[ 1024 ];
    
    
    memset( buffer, '\0', sizeof buffer );
    snprintf( buffer, sizeof buffer, "CREATE TABLE `%s`.`%s` ( %s", dbSchemaName, deviceStateCurrentTableName, sql );
    
    if (mysql_query( connection, buffer ) != 0) {
        fprintf( stderr, "Unable to create the device table.\n" );
        fprintf( stderr, "%s\n", mysql_error( connection ));
        if (dbFailOnErrors) {
            mysql_close( connection );
            exit( 1 );
        }
    }
}

// -----------------------------------------------------------------------------
void    Database_dropDeviceStateCurrentTable ()
{
    char buffer[ 1024 ];
    
    memset( buffer, '\0', sizeof buffer );
    
    snprintf( buffer, sizeof buffer, "DROP TABLE `%s`.`%s` ", dbSchemaName, deviceStateCurrentTableName );
    
    if (mysql_query( connection, buffer ) != 0) {
        fprintf( stderr, "Unable to drop the device table.\n" );
        fprintf( stderr, "%s\n", mysql_error( connection ));
        if (dbFailOnErrors) {
            //mysql_close( connection );
            //exit( 1 );
        }
    }
}

// -----------------------------------------------------------------------------
void    Database_updateDeviceStateCurrentRecord (HomeHeartBeatDevice_t *recPtr)
{
    char    buffer[ 8192 ];
    char    *updateSQL = "UPDATE `%s`.`%s` SET \
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
    
    char    *insertSQL = "INSERT INTO `%s`.`%s` (\
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
    //
    // Darned SQL...  
    char    *countSQL = "SELECT COUNT(*) FROM `%s`.`%s` WHERE macAddress='%s'";
    memset( buffer, '\0', sizeof buffer );
    snprintf( buffer, sizeof buffer, countSQL,
                            dbSchemaName, deviceStateCurrentTableName,
                            recPtr->macAddress );
    
    if (mysql_query( connection, buffer ) != 0) {
        fprintf( stderr, "Unable to see if the record is already in the device table.\n" );
        fprintf( stderr, "SQL: [%s]\n", buffer );
        fprintf( stderr, "%s\n", mysql_error( connection ));
        if (dbFailOnErrors) {
            mysql_close( connection );
            exit( 1 );
        }
    }
    
    //
    //  Is the device already there?
    MYSQL_RES *result = mysql_store_result( connection );
    if (result == NULL)  {
        fprintf( stderr, "%s\n", mysql_error( connection ));
    }

    int         num_fields = mysql_num_fields( result );
    MYSQL_ROW   row = mysql_fetch_row( result );
    char        *data = row[ 0 ];
  
    int     doInsertNotUpdate = (data[ 0 ] == '0');
    mysql_free_result( result );    
    
    //
    //
    memset( buffer, '\0', sizeof buffer );
    if (doInsertNotUpdate) {
        snprintf( buffer, sizeof buffer, insertSQL,
                            dbSchemaName, deviceStateCurrentTableName,
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
                            recPtr->deviceName );
    } else { 
        snprintf( buffer, sizeof buffer, updateSQL,
                            dbSchemaName, deviceStateCurrentTableName,
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
                            recPtr->deviceName,
                            recPtr->macAddress );
    }
    
    if (mysql_query( connection, buffer ) != 0) {
        fprintf( stderr, "Unable to insert the record into the device table.\n" );
        fprintf( stderr, "SQL: [%s]\n", buffer );
        fprintf( stderr, "%s\n", mysql_error( connection ));
        if (dbFailOnErrors) {
            mysql_close( connection );
            exit( 1 );
        }
    }
}
