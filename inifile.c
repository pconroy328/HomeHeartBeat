

#include <stdio.h>
#include <sys/stat.h>

#include <configIO.h>
#include "hhb_structures.h"
#include "helpers.h"
#include "homeheartbeat.h"

static  char    *INIFileName;

// ----------------------------------------------------------------------------
static  
int IniFile_fileExists (const char * fileName)
{
    struct  stat    buf;

    int i = stat ( fileName, &buf );
    if ( i == 0 )
        return TRUE;

    return FALSE;
}


// ------------------------------------------------------------------
void    IniFile_findIniFile ()
{
    //
    //  Look in some common places for the INI file
    if (IniFile_fileExists( "./homeheartbeat.ini" ))                      // 1. Local takes precedence
        INIFileName = "./homeheartbeat.ini";
    else if (IniFile_fileExists( "./INI/homeheartbeat.ini" ))             // 2. Go down one and see if there's a INI dir
        INIFileName = "./INI/Systems.ini";
    else if (IniFile_fileExists( "../INI/homeheartbeat.ini" ))            // 3. Go up one and see if there's a INI dir
        INIFileName = "../INI/Systems.ini";
    else if (IniFile_fileExists( "/usr/local/INI/homeheartbeat.ini" ))    // 4. look in a common place
        INIFileName = "/usr/local/INI/homeheartbeat.ini";
    else
        INIFileName = "homeheartbeat.ini";
}

// ------------------------------------------------------------------
void    IniFile_readIniFile (HomeHeartBeatSystem_t *aSystem)
{
    int     result;

    //
    //  Go looking for it first!!!
    IniFile_findIniFile();

    result = IniFiler_SearchCfg( INIFileName, "SYSTEM", "ID", &(aSystem->systemID), Cfg_Ushort );
    if (!result)
        fprintf( stderr, "Warning ! Could not open the INIFile [%s]\n", INIFileName );

    (void) IniFiler_SearchCfg( INIFileName, "SYSTEM", "debug", &(aSystem->debugValue), Cfg_Boolean );    


    (void) IniFiler_SearchCfg( INIFileName, "SYSTEM", "Name", &(aSystem->name[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "SYSTEM", "Address Line 1", &(aSystem->addressLine1[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "SYSTEM", "Address Line 2", &(aSystem->addressLine2[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "SYSTEM", "City", &(aSystem->city[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "SYSTEM", "StateOrProvinceCode", &(aSystem->stateOrProvinceCode[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "SYSTEM", "PostalCode", &(aSystem->postalCode[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "SYSTEM", "Latitude", &(aSystem->latitude), Cfg_Double );
    (void) IniFiler_SearchCfg( INIFileName, "SYSTEM", "Longitude", &(aSystem->longitude), Cfg_Double );
    (void) IniFiler_SearchCfg( INIFileName, "SYSTEM", "TZOffsetMins", &(aSystem->TZOffsetMins), Cfg_Short );
    (void) IniFiler_SearchCfg( INIFileName, "SYSTEM", "PortName", &(aSystem->portName[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "SYSTEM", "sleepSecsBetweenEventLoops", &(aSystem->sleepSecsBetweenEventLoops), Cfg_Ushort );
    

    //
    // Now the database stuff
    (void) IniFiler_SearchCfg( INIFileName, "Database", "HostName", &(aSystem->DBParameters.databaseHost[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "Database", "UserName", &(aSystem->DBParameters.databaseUserName[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "Database", "Password", &(aSystem->DBParameters.databasePassword[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "Database", "Schema", &(aSystem->DBParameters.databaseSchema[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "Database", "logAlarms", &(aSystem->DBParameters.logAlarms), Cfg_Boolean );    
    (void) IniFiler_SearchCfg( INIFileName, "Database", "logStatus", &(aSystem->DBParameters.logStatus), Cfg_Boolean );    
    (void) IniFiler_SearchCfg( INIFileName, "Database", "logHistory", &(aSystem->DBParameters.logHistory), Cfg_Boolean );    
    (void) IniFiler_SearchCfg( INIFileName, "Database", "dropAlarmTable", &(aSystem->DBParameters.dropAlarmTable), Cfg_Boolean );    
    (void) IniFiler_SearchCfg( INIFileName, "Database", "createAlarmTable", &(aSystem->DBParameters.createAlarmTable), Cfg_Boolean );    
    (void) IniFiler_SearchCfg( INIFileName, "Database", "dropStatusTable", &(aSystem->DBParameters.dropStatusTable), Cfg_Boolean );    
    (void) IniFiler_SearchCfg( INIFileName, "Database", "createStatusTable", &(aSystem->DBParameters.createStatusTable), Cfg_Boolean );    
    (void) IniFiler_SearchCfg( INIFileName, "Database", "dropHistoryTable", &(aSystem->DBParameters.dropHistoryTable), Cfg_Boolean );    
    (void) IniFiler_SearchCfg( INIFileName, "Database", "createHistoryTable", &(aSystem->DBParameters.createHistoryTable), Cfg_Boolean );    


    (void) IniFiler_SearchCfg( INIFileName, "Database", "maxMinutesOfHistoryStored", &(aSystem->DBParameters.maxMinutesOfHistoryStored), Cfg_Ushort );    
    
    //
    // MQTT Specific Information
    (void) IniFiler_SearchCfg( INIFileName, "MQTT", "logEventsToMQTT", &(aSystem->logEventsToMQTT), Cfg_Boolean ); 
    (void) IniFiler_SearchCfg( INIFileName, "MQTT", "HostName", &(aSystem->MQTTParameters.mqttBrokerHost[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "MQTT", "PortNumber", &(aSystem->MQTTParameters.mqttPortNumber), Cfg_Ushort );
    (void) IniFiler_SearchCfg( INIFileName, "MQTT", "KeepAliveValue", &(aSystem->MQTTParameters.mqttKeepAliveValue), Cfg_Ushort );
    (void) IniFiler_SearchCfg( INIFileName, "MQTT", "QoS", &(aSystem->MQTTParameters.mqttKeepAliveValue), Cfg_Ushort );
    (void) IniFiler_SearchCfg( INIFileName, "MQTT", "StatusTopic", &(aSystem->MQTTParameters.mqttStatusTopic[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "MQTT", "AlarmTopic", &(aSystem->MQTTParameters.mqttAlarmTopic[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "MQTT", "retainMsgs", &(aSystem->MQTTParameters.mqttRetainMsgs), Cfg_Boolean ); 
}


