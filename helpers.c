#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "homeheartbeat.h"
#include "helpers.h"

// -----------------------------------------------------------------------------
int     hexStringToInt (char *hexChars)
{
    assert( hexChars != NULL );

    int len = strlen( hexChars );
    assert( (len == 2) || (len == 4) );
    
    
    int intValue = -1;
    
    if (len == 2) {
        if ( isxdigit(hexChars[ len - 2 ]) && isxdigit( hexChars[ len - 1 ]) ) {

            //
            // strtol can parse hex strings!
            intValue = (int) strtol( &hexChars[ len - 2 ], NULL, 16 );
        }
        
    } else if (len == 4) {
        if ( isxdigit(hexChars[ len - 4 ]) && isxdigit( hexChars[ len - 3 ])  &&
             isxdigit(hexChars[ len - 2 ]) && isxdigit( hexChars[ len - 1 ]) ) {

            //
            // strtol can parse hex strings!
            intValue = (int) strtol( &hexChars[ len - 4 ], NULL, 16 );
        }
    }
    
    return intValue;
}

