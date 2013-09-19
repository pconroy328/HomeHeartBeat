/* 
 * File:   helpers.h
 * Author: pconroy
 *
 * Created on August 29, 2013, 4:04 PM
 */

#ifndef HELPERS_H
#define	HELPERS_H

#ifdef	__cplusplus
extern "C" {
#endif
 
    
#ifndef     FALSE
# define    FALSE   (0)
# define    TRUE    (!FALSE)    
#endif
    
    
#include <stdio.h>
    
//
// Selective (but compile time) debugging 
#define DEBUG       1
    
#define debug_print(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)

    
    
extern  void    haltAndCatchFire( char * );
extern  void    warnAndKeepGoing( char *message );
extern  int     hexStringToInt( char *hexChars );



#ifdef	__cplusplus
}
#endif

#endif	/* HELPERS_H */

