/* 
 * File:   serialport.h
 * Author: pconroy
 *
 * Created on August 30, 2013, 9:57 AM
 */

#ifndef SERIALPORT_H
#define	SERIALPORT_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>    

#define BAUDRATE    B38400
    

extern  int     SerialPort_Open( char *portName );
extern  int     SerialPort_Close( int portID );
extern  void    SerialPort_FlushBuffers (int portID);
extern  int     SerialPort_SendData( int, char *, int );
extern  int     SerialPort_ReadData( int, char *, int );
extern  int     SerialPort_ReadLine( int, char *, int );

//
// debugging
extern  int     SerialPort_Open2( char * );
extern  int     SerialPort_Open3( char * );


#ifdef	__cplusplus
}
#endif

#endif	/* SERIALPORT_H */

