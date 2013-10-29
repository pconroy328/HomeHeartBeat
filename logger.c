#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "logger.h"

static  int    Logger_Log (char *format, char *level, ...);

// ----------------------------------------------------------------------------
void    Logger_Initialize ()
{
     // logCat = log4c_category_get( "homeheartbeat" );
    // (void) log4c_init();
  
}

// ----------------------------------------------------------------------------
void    Logger_Terminate()
{
    // (void) log4c_fini();

}

// ----------------------------------------------------------------------------
void    Logger_LogDebug (char *format, ... )
{
    va_list args;
    
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( stderr, "DEBUG|" );
    numWritten += vfprintf( stderr, format, args );
    va_end( args );
    
    fflush( stderr );

}

// ----------------------------------------------------------------------------
void    Logger_LogWarning (char *format, ... )
{
    va_list args;
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( stderr, "WARNING|" );
    numWritten += vfprintf( stderr, format, args );
    va_end( args );
    
    fflush( stderr );
}

// ----------------------------------------------------------------------------
void    Logger_LogError (char *format, ... )
{
    va_list args;
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( stderr, "ERROR|" );
    numWritten += vfprintf( stderr, format, args );
    va_end( args );
    
    fflush( stderr );
}

// ----------------------------------------------------------------------------
void    Logger_LogFatal (char *format, ... )
{
    va_list args;
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( stderr, "FATAL|" );
    numWritten += vfprintf( stderr, format, args );
    va_end( args );
    
    fflush( stderr );
    exit( 1 );
}

// ----------------------------------------------------------------------------
void    Logger_LogInfo (char *format, ... )
{
    va_list args;
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( stderr, "INFO|" );
    numWritten += vfprintf( stderr, format, args );
    va_end( args );
    
    fflush( stderr );
}

#if 0
void    Logger_LogFunctionStart (char *format, ...)
{
    
}

void    Logger_LogFunctionEnd (char *format, ...)
{
    Logger_LogDebug( 
}
#endif

// ----------------------------------------------------------------------------
static  int    Logger_Log (char *level, char *format, ...)
{
    va_list args;
    va_start( args, format );                   // the last fixed parameter
   
    int numWritten = fprintf( stderr, "%s|", level );
    numWritten += vfprintf( stderr, format, args );

    va_end( args );
    
    return numWritten;
}