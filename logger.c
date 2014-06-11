/* 
 * File:   logger.c
 * Author: patrick conroy
 *
 * Created on September 17, 2013, 10:03 AM
 * 
 *  Get things ready for a real logging framework.
 * (C) 2013 Patrick Conroy
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "logger.h"



static  int     Logger_Log (char *format, char *level, ...);
static  char    *getCurrentDateTime( void );
static  char    currentDateTimeBuffer[ 80 ];



static  FILE    *fp;
static  int     logFileOpen = FALSE;
static  int     debugValue;              // from the INI file

// ----------------------------------------------------------------------------
//
// How "debugValue" works.  It's an integer
// 0 = Log nothing         
// 5 = Log Fatal and Error and Warning and Debug and Info
// 4 = Log Fatal and Error and Warning and Debug
// 3 = Log Fatal and Error and Warning
// 2 = Log Fatal and Error
// 1 = Log Fatal

// ----------------------------------------------------------------------------
void    Logger_Initialize (char *fileName, int debugLevel)
{
    // logCat = log4c_category_get( "homeheartbeat" );
    // (void) log4c_init();
    printf( "ATTEMPTING TO OPEN [%s] DEBUG LEVEL [%d]\n", fileName, debugLevel ); fflush(stdout);
    if (fileName != (char *) 0 ) {
        debugValue = debugLevel;
        fp = fopen( fileName, "a" );
        if (fp != (FILE *) 0)
            logFileOpen = TRUE;
    }
}

// ----------------------------------------------------------------------------
void    Logger_Terminate()
{
    // (void) log4c_fini();
    if (logFileOpen)
        fclose( fp );
}

// ----------------------------------------------------------------------------
void    Logger_LogDebug (char *format, ... )
{
    if (!logFileOpen || debugValue < 4)
        return;
    
    va_list args;
    
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( fp, "DEBUG|%s|", getCurrentDateTime() );
    numWritten += vfprintf( fp, format, args );
    va_end( args );
    
    fflush( fp );

}

// ----------------------------------------------------------------------------
void    Logger_LogWarning (char *format, ... )
{
    if (!logFileOpen || debugValue < 3)
        return;
    
    va_list args;
    
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( fp, "WARNING|%s|", getCurrentDateTime() );
    numWritten += vfprintf( fp, format, args );
    va_end( args );
    
    fflush( fp );
}

// ----------------------------------------------------------------------------
void    Logger_LogError (char *format, ... )
{
    if (!logFileOpen || debugValue < 2)
        return;
    
    va_list args;
    
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( fp, "ERROR|%s|", getCurrentDateTime() );
    numWritten += vfprintf( fp, format, args );
    va_end( args );
    
    fflush( fp );
}

// ----------------------------------------------------------------------------
void    Logger_LogFatal (char *format, ... )
{
    if (!logFileOpen || debugValue < 1)
        return;
    
    va_list args;
    
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( fp, "FATAL|%s|", getCurrentDateTime() );
    numWritten += vfprintf( fp, format, args );
    va_end( args );
    
    fflush( fp );
    exit( 1 );
}

// ----------------------------------------------------------------------------
void    Logger_LogInfo (char *format, ... )
{
    if (!logFileOpen || debugValue < 5)
        return;
    
    va_list args;
    
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( fp, "INFO|%s|", getCurrentDateTime() );
    numWritten += vfprintf( fp, format, args );
    va_end( args );
    
    fflush( fp );
}

// ----------------------------------------------------------------------------
static  int    Logger_Log (char *level, char *format, ...)
{
    if (!logFileOpen)
        return 0;
    
    va_list args;
    
    va_start( args, format );                   // the last fixed parameter
   
    int numWritten = fprintf( fp, "%s|%s|", level, getCurrentDateTime() );
    numWritten += vfprintf( fp, format, args );

    va_end( args );
    
    return numWritten;
}

// ----------------------------------------------------------------------------
static  char    *getCurrentDateTime()
{
    //
    // Something quick and dirty... Fix this later - thread safe
    time_t  current_time;
    struct  tm  *tmPtr;
 
    memset( currentDateTimeBuffer, '\0', sizeof currentDateTimeBuffer );
    
    /* Obtain current time as seconds elapsed since the Epoch. */
    current_time = time( NULL );
    if (current_time > 0) {
        /* Convert to local time format. */
        tmPtr = localtime( &current_time );
 
        if (tmPtr != NULL) {
            strftime( currentDateTimeBuffer,
                    sizeof currentDateTimeBuffer,
                    "%F %T",
                    tmPtr );
            
        }
    }
    
    return &currentDateTimeBuffer[ 0 ];    
}
