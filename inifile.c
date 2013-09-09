

#include <stdio.h>
#include <sys/stat.h>

#include <configIO.h>
#include "homeheartbeat.h"
#include "helpers.h"



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
    (void) IniFiler_SearchCfg( INIFileName, "Database", "logEventsToDatabase", &(aSystem->logEventsToDatabase), Cfg_Boolean );    
    (void) IniFiler_SearchCfg( INIFileName, "Database", "HostName", &(aSystem->databaseHost[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "Database", "UserName", &(aSystem->databaseUserName[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "Database", "Password", &(aSystem->databasePassword[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "Database", "Schema", &(aSystem->databaseSchema[ 0 ]), Cfg_String );
    
    //
    // MQTT Specific Information
    (void) IniFiler_SearchCfg( INIFileName, "MQTT", "logEventsToMQTT", &(aSystem->logEventsToMQTT), Cfg_Boolean );    
    (void) IniFiler_SearchCfg( INIFileName, "MQTT", "HostName", &(aSystem->mqttBrokerHost[ 0 ]), Cfg_String );
    (void) IniFiler_SearchCfg( INIFileName, "MQTT", "PortNumber", &(aSystem->mqttPortNumber), Cfg_Ushort );
    (void) IniFiler_SearchCfg( INIFileName, "MQTT", "KeepAliveValue", &(aSystem->mqttKeepAliveValue), Cfg_Ushort );
    
    debug_print( "aSystem->databaseHost [%s], mqtt Host[%s]\n", aSystem->databaseHost, aSystem->mqttBrokerHost );
    //int i = 614;
}


