// -----------------------------------------------------------------------------
//
//  The Eaton Home Heartbeat (HHB) system has a USB port on the side of it for a
//  connection to a computer. Inside the HHB is a FTDI branded "USB to Serial" chip
//  that essential turns the USB port into a serial port. 
//
//  After the FTDI driver has been loaded by the operating system - we can access
//  the HHB system by sending data to/from a port the same way we did it years ago
//  talking to modems and other SerialPort devices.
//
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "homeheartbeat.h"
#include "serialport.h"
#include "helpers.h"


// static  int     hhbPortID;
//static  char    *portName;
static  struct  termios oldtio;
// static  char    outBuffer[ 8192 ];          // fix this!



// -----------------------------------------------------------------------------
int SerialPort_Open (char *portName)
{
    assert( portName != NULL );
    
    int             portID;
    struct termios  newtio;    
    
    return SerialPort_Open2( portName );
    
    
    /*                                                                            
      Open modem device for reading and writing and not as controlling tty        
      because we don't want to get killed if linenoise sends CTRL-C.              
    */                                                                            
     portID = open( portName, (O_RDWR | O_NOCTTY) );                                  
     if (portID < 0) {
         fprintf( stderr, "Error - unable to open the port [%s]\n", portName );
         return -1;
     }                                 
                                                                                      
    tcgetattr( portID, &oldtio );        /* save current serial port settings */               
    memset( &newtio, 0, sizeof( newtio ) );     /* clear struct for new port settings */     

    // newtio.c_cflag = (BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD);                  
    newtio.c_cflag = (BAUDRATE |  CS8 | CLOCAL | CREAD);   

    newtio.c_cflag &= ~CRTSCTS;                         // turn off hardware flow control

    
    newtio.c_iflag = IGNPAR;                            // ignore parity
    newtio.c_iflag &= ~(IXON | IXOFF | IXANY);           // ignore software flow control
    newtio.c_oflag = 0;                                                          

    /*  disable canonical input!
     *  Now VMIN and VTIME mean something!
     */                                                                            
    newtio.c_lflag = 0;                                                     
    
    /*
     * 
     *  When waiting for input, the driver returns when:
     *      VTIME tenths of a second elapses between bytes
     *          An internal timer is started when a character arrives, and it counts up in tenths of a second units. 
     *          It's reset whenever new data arrives, so rapidly-arriving data never gives the intercharacter timer 
     *          a chance to count very high.  It's only after the last character of a burst — when the line is quiet — that the 
     *          timer really gets counting. When it reaches VTIME tenths, the user's request has been satisfied and the read() returns.
     *          This provides exactly the behavior we want when dealing with bursty data: collect data while it's arriving rapidly, 
     *          but when it calms down, give us what you got.
     * 
     * VMIN characters have been received, with no more data available
     *          At first this appears duplicative to the nbytes parameter to the read() system call, but it's not 
     *          quite the same thing. The serial driver maintains an input queue of data received but not transferred to 
     *          the user — clearly data can arrive even when we're not asking for it — and this is always first copied to the 
     *          user's buffer on a read() without having to wait for anything. But if we do end up blocking for I/O (because 
     *          the kernel's input queue is empty), then VMIN kicks in: When that many bytes have been received, the read() 
     *          request returns that data. In this respect we can think of the nbytes parameter as being the amount of data 
     *          we hope to get, but we'll settle for VMIN.   
     * 
     * NBYTES   The user's requested number of bytes has been satisfied
     *          This rule trumps all the others: there is no circumstance where the system will provide more data than 
     *          was actually asked for by the user. If the user asks for (say) ten bytes in the read() system call, and 
     *          that much data is already waiting in the kernel's input queue, then it's returned to the caller immediately 
     *          and without having VMIN and VTIME participate in any way.
     * 
     * These are certainly confusing to one who is new to termios, but it's not really poorly defined Instead, they solve 
     * problems that are not obvious to the newcomer. It's only when one is actually dealing with terminal I/O and running into issues of 
     * either performance or timing that one really must dig in.
     * 
     * Keep in mind that the tty driver maintains an input queue of bytes already read from the serial line 
     * and not passed to the user, so not every read() call waits for actual I/O - the read may very well be 
     * satisfied directly from the input queue.
     * 
     * 
     *   VMIN = 0 and VTIME = 0
     *      This is a completely non-blocking read - the call is satisfied immediately directly from the 
     *      driver's input queue. If data are available, it's transferred to the caller's buffer up to nbytes 
     *      and returned. Otherwise zero is immediately returned to indicate "no data". We'll note 
     *      that this is "polling" of the serial port, and it's almost always a bad idea. If done repeatedly, 
     *      it can consume enormous amounts of processor time and is highly inefficient. Don't 
     *      use this mode unless you really, really know what you're doing.
     * 
     *  VMIN = 0 and VTIME > 0
     *      This is a pure timed read. If data are available in the input queue, it's transferred to the caller's 
     *      buffer up to a maximum of nbytes, and returned immediately to the caller. Otherwise the driver blocks 
     *      until data arrives, or when VTIME tenths expire from the start of the call. If the timer expires 
     *      without data, zero is returned. A single byte is sufficient to satisfy this read call, but if more 
     *      is available in the input queue, it's returned to the caller. Note that this is an overall timer, not 
     *      an intercharacter one.
     * 
     *  VMIN > 0 and VTIME > 0
     *      A read() is satisfied when either VMIN characters have been transferred to the caller's buffer, or 
     *      when VTIME tenths expire between characters. Since this timer is not started until the first character arrives, 
     *      this call can block indefinitely if the serial line is idle. This is the most common mode of operation, 
     *      and we consider VTIME to be an intercharacter timeout, not an overall one. This call should never return zero bytes read.
     * 
     *  VMIN > 0 and VTIME = 0
     *      This is a counted read that is satisfied only when at least VMIN characters have been transferred to the 
     *      caller's buffer - there is no timing component involved. This read can be satisfied from the driver's 
     *      input queue (where the call could return immediately), or by waiting for new data to arrive: in this 
     *      respect the call could block indefinitely. We believe that it's undefined behavior if nbytes is less then VMIN.
     */
    
    //
    // Based on the above, I'll wait up to 0.1 second for some data; I'll handle '0' data returned
    newtio.c_cc[ VTIME ]    = 20;     
    newtio.c_cc[ VMIN ]     = 1;     
    
    //
    // Emperical results
    // VTIME    VMIN    date        results
    //  0       0       28Aug13     0 bytes returned.  sniff shows two \n's before STATE
    //  0       1       28Aug13     4 bytes returned.  sniff shows "ATE=..."
    //  0       10                  12 bytes returned  sniff shows "02,0024,0005..."
    
    

    /* now clean the com line and activate the settings for the port */                                                                            
    tcflush( portID,  TCIFLUSH );                                                       
    tcsetattr( portID, TCSANOW, &newtio );                                               

    return portID;
}   // SerialPort_Open

// ----------------------------------------------------------------------------
int SerialPort_Close (int portID)
{
    int status;

    //
    // Don't worry too much about errors when we're closing or shutting down
    (void) ioctl( portID, TIOCMGET, &status );

    status &= ~TIOCM_DTR;    /* turn off DTR */
    status &= ~TIOCM_RTS;    /* turn off RTS */

    (void) ioctl( portID, TIOCMSET, &status );

    close( portID );
    tcsetattr( portID, TCSANOW, &oldtio );
}

// ----------------------------------------------------------------------------
void    SerialPort_FlushBuffers (int portID)
{
    tcflush( portID,  TCIFLUSH );                                                       
    tcflush( portID,  TCOFLUSH );     
}


// ----------------------------------------------------------------------------
int SerialPort_SendData (int portID, char *data, int numBytes)
{
    int n;
    n = write( portID, data, numBytes );
    return n;
}

// ----------------------------------------------------------------------------
int SerialPort_ReadData (int portID, char *data, int numBytes)
{
    int n;
    n = read( portID, data, numBytes );
    int errorVal = errno;
    
    if (n < 0) {
        switch (errorVal) {
            case    EAGAIN: perror( "Eagain" ); break;
            //case    EWOULDBLOCK:  perror( "EWOULDBLOCK" ); break;
            case    EBADF:  perror( "EBADF" );  break;
            case    EFAULT: perror( "EFAULT" ); break;
            case    EINTR:  perror( "EINTR" );  break;
            case    EINVAL: perror( "EINVAL" ); break;
            case    EIO:    perror( "EIO" );    break;
            case    EISDIR: perror( "EISDIR" ); break;
            default:        perror( "???" );        break;
        }
    }
    return n;
}


// ----------------------------------------------------------------------------
int     SerialPort_ReadLine (int portID, char *dataBuffer, int bufSize)
{
    int     done = FALSE;
    int     numRead = 0;
    int     totalRead = 0;
    int     numLoops = 0;
    char    inBuffer[ 1024 ];   

    //debug_print( "entering\n", 0 );
    assert( dataBuffer != NULL );
    assert( bufSize > 0 );
    
  
    memset( dataBuffer, '\0', sizeof bufSize );
 
    while (!done) {
        //
        // Start by trying to pull some bytes off an into a local buffer
        memset( inBuffer, '\0', sizeof inBuffer );
        // numRead = SerialPort_ReadData( portID, inBuffer, sizeof inBuffer );
        numRead = read( portID, inBuffer, 1 );
        inBuffer[ numRead ] ='\0';

        
        //
        // If we got something, copy it into the buffer going back
        if (numRead > 0) {
            totalRead += numRead;
            strncat( dataBuffer, inBuffer, sizeof bufSize );
            
        }
        
        //
        // Look for CR/LF from device
        if (totalRead > 1) {
            done = (dataBuffer[ numRead - 2 ] == 0x0D) && (dataBuffer[ numRead - 1 ] == 0x0A);
            if (done)
                //debug_print( "EOL FOUND at end of buffer - totalRead: %d\n", totalRead )
                ;

            if (!done) {
                 done = (strstr( &dataBuffer[ 2 ], "\r\n" ) != NULL);
                if (done)
                    //debug_print( "EOL FOUND in buffer (via strstr) - totalRead: %d\n", totalRead )
                            ;
            }
        }
        
        numLoops += 1;
        
        if (numLoops > 250) {
            done = TRUE;
            debug_print( "TOO MANY LOOPS - EXITING \n", 0 );
        }
    }

    // debug_print( "Final OutBuffer[%s]\n", dataBuffer );
        
    return totalRead;
}   // SerialPort_ReadLine


// ----------------------------------------------------------------------------
int SerialPort_Open2 (char *portName)
{
    int             portID;
    int             portstatus;
    struct termios  newtio;    
    
    debug_print( "v2.2 Entering [%s]\n", portName );
    
    /*                                                                            
      Open modem device for reading and writing and not as controlling tty        
      because we don't want to get killed if linenoise sends CTRL-C.              
    */                                                                            
     portID = open( portName, (O_RDWR | O_NOCTTY) );                                  
     if (portID < 0) {
         perror( portName );
         return -1;
     }   
     
    //We want full control of what is set and simply reset the entire newtio struct
    memset(&newtio, 0, sizeof(newtio));
    
    // Serial control options
    newtio.c_cflag &= ~PARENB;      // No parity
    newtio.c_cflag &= ~CSTOPB;      // One stop bit
    newtio.c_cflag &= ~CSIZE;       // Character size mask
    newtio.c_cflag |= CS8;          // Character size 8 bits
    newtio.c_cflag |= CREAD;        // Enable Receiver
    newtio.c_cflag &= ~HUPCL;       // No "hangup"
    newtio.c_cflag &= ~CRTSCTS;     // No flowcontrol
    newtio.c_cflag |= CLOCAL;       // Ignore modem control lines

    // Baudrate, for newer systems
    cfsetispeed(&newtio, BAUDRATE);
    cfsetospeed(&newtio, BAUDRATE);

    // Serial local options: newtio.c_lflag
    // Raw input = clear ICANON, ECHO, ECHOE, and ISIG
    // Disable misc other local features = clear FLUSHO, NOFLSH, TOSTOP, PENDIN, and IEXTEN
    // So we actually clear all flags in newtio.c_lflag
    newtio.c_lflag = 0;

    // Serial input options: newtio.c_iflag
    // Disable parity check = clear INPCK, PARMRK, and ISTRIP
    // Disable software flow control = clear IXON, IXOFF, and IXANY
    // Disable any translation of CR and LF = clear INLCR, IGNCR, and ICRNL
    // Ignore break condition on input = set IGNBRK
    // Ignore parity errors just in case = set IGNPAR;
    // So we can clear all flags except IGNBRK and IGNPAR
    newtio.c_iflag = IGNBRK|IGNPAR;
    newtio.c_iflag &= ~(IXON | IXOFF | IXANY);           // ignore software flow control

    // Serial output options
    // Raw output should disable all other output options
    newtio.c_oflag &= ~OPOST;

    newtio.c_cc[ VTIME ] = 10;        // timer 1s
    newtio.c_cc[ VMIN ] = 0;        // blocking read until 1 char

    if (tcsetattr(portID, TCSANOW, &newtio) < 0)
    {
        perror( "Unable to initialize serial device" );
        //exit(0);
                return 0;
    }

    tcflush(portID, TCIOFLUSH);

    // Set DTR low and RTS high and leave other ctrl lines untouched

    //ioctl(portID, TIOCMGET, &portstatus);    // get current port status
    //portstatus &= ~TIOCM_DTR;
    //portstatus |= TIOCM_RTS;
    //ioctl(portID, TIOCMSET, &portstatus);    // set current port status

    /*
    int n = write( portID, "i", 1 );           // send 7 character greeting
    debug_print( ">>Num written: %d\n", n ); fflush(stdout); fflush( stderr );
    char buf [2048];
    //sleep( 1 );
    n = read (portID, buf, sizeof buf);  // read up to 100 characters if ready to read
    debug_print( ">>Num read: %d\n", n ); fflush(stdout); fflush( stderr );
    sleep( 5 );
    */
    
    return portID;
}

//-----------------------------------------------------------------------------
static int set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                fprintf( stderr, "error %d from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // ignore break signal
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                fprintf( stderr, "error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}

void
set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                fprintf( stderr, "error %d from tggetattr", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                fprintf( stderr, "error %d setting term attributes", errno);
}

int SerialPort_Open3 (char *portName)
{
    int     portID;
 
    portID = open (portName, O_RDWR | O_NOCTTY | O_SYNC);
    if (portID < 0) {
        fprintf( stderr, "error %d opening %s: %s", errno, portName, strerror (errno));
        return;
    }

    set_interface_attribs (portID, BAUDRATE, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (portID, 0);                // set no blocking

    /*
    int n = write( portID, "i", 1 );           // send 7 character greeting
    printf( ">>>Num written: %d\n", n ); fflush(stdout); fflush( stderr );
    char buf [2048];
    //sleep( 1 );
    n = read (portID, buf, sizeof buf);  // read up to 100 characters if ready to read
    printf( ">>>Num read: %d\n", n ); fflush(stdout); fflush( stderr );
    sleep( 5 );
    */
    return portID;
}

