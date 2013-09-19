#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "homeheartbeat.h"
#include "helpers.h"


// -----------------------------------------------------------------------------
void    haltAndCatchFire (char *message)
{
    fputs( message, stderr );
    exit( 1 );
}

// -----------------------------------------------------------------------------
void    warnAndKeepGoing (char *message)
{
    fputs( message, stderr );
}

// -----------------------------------------------------------------------------
int     hexStringToInt (char *hexChars)
{
    assert( hexChars != NULL );
    //debug_print( "entering [%s]\n", hexChars );
    int len = strlen( hexChars );
    assert( (len == 2) || (len == 4) );
    
    //if (len != 2 && len != 4)
    //    printf( "!!!!!!!!! LENGHT %d [%s]\n", len, hexChars );
    
    int intValue = -1;
    
    if (len == 2) {
        if ( isxdigit(hexChars[ len - 2 ]) && isxdigit( hexChars[ len - 1 ]) ) {

            //
            // strtol can parse hex strings!
            intValue = (int) strtol( &hexChars[ len - 2 ], NULL, 16 );
            //debug_print( "Token [%s], value: %d\n", &token[ len - 2 ], recordID );
        }
        
    } else if (len == 4) {
        if ( isxdigit(hexChars[ len - 4 ]) && isxdigit( hexChars[ len - 3 ])  &&
             isxdigit(hexChars[ len - 2 ]) && isxdigit( hexChars[ len - 1 ]) ) {

            //
            // strtol can parse hex strings!
            intValue = (int) strtol( &hexChars[ len - 4 ], NULL, 16 );
            //debug_print( "Token [%s], value: %d\n", &token[ len - 2 ], recordID );
        }
    }
    
    return intValue;
}

