#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
    
    int len = strlen( hexChars );
    assert( (len == 2) || (len == 4) );
    
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

// -----------------------------------------------------------------------------
int     HomeHeartBeat_parseStateRecordID (char *token)
{
    assert( token != NULL );
    //debug_print( "Entering. Token [%s]\n", token );
    
    
    //
    // This comes into us with "STATExx=" prepended.  Let's whack that off
    char  *cPtr = strchr( token, '=' );
    cPtr++;                                     // skipover "="
    cPtr++;                                     // skip over double quote mark
    int recordID = hexStringToInt( cPtr );
    return recordID;
}


// ----------------------------------------------------------------------------
int HomeHeartBeat_parseZigbeeBindingID (char *token)
{
    assert( token != NULL );
    //debug_print( "Entering. Token [%s]\n", token );
    
    int zigbeeID = hexStringToInt( token );
    return zigbeeID;
}

// ----------------------------------------------------------------------------
int     HomeHeartBeat_parseDeviceCapabilties (char *token)
{
    assert( token != NULL );
    //debug_print( "Entering. Token [%s]\n", token );

    int capFlags = hexStringToInt( token );
    /* from Kolinahr
     *  Bit 0 (0x01)  -  unknown
        Bit 1 (0x02)  -  unknown
        Bit 2 (0x04)  -  Is a sensor
        Bit 3 (0x08)  -  Is the Base Station
        Bit 4 (0x10)  -  Can receive parameters (Motion, Power, and Reminder sensors)
        Bit 5 (0x20)  -  Is a sensor
        Bit 6 (0x40)  -  Is a Home Key
        Bit 7 (0x80)  -  Is AC powered (Base Station, Modem, and Power Sensor)
     */
    int isSensor        = (capFlags & 0x04);
    int isBaseStaton    = (capFlags & 0x08);
    int canReceive      = (capFlags & 0x10);
    int isSensor2       = (capFlags & 0x20);
    int isHomeKey       = (capFlags & 0x40);
    int isACPowered     = (capFlags & 0x80);

    //debug_print( "Token [%s], value: %d\n", &token[ len - 2 ], capFlags );
    //debug_print( "isSensor: %s isSensor(2): %s isHomeKey: %s\n",
    //             (isSensor ? "YES" : "NO"), (isSensor2 ? "YES" : "NO"), (isHomeKey ? "YES" : "NO") );
    //debug_print( "isBaseStation: %s canReceive: %s isACPowered: %s\n",
    //            (isBaseStaton ? "YES" : "NO"), (canReceive ? "YES" : "NO"), (isACPowered ? "YES" : "NO") );

    return capFlags;
}

// ----------------------------------------------------------------------------
int     HomeHeartBeat_parseDeviceType (char *token)
{
    assert( token != NULL );
    //debug_print( "Entering. Token [%s]\n", token );

    int deviceType = hexStringToInt( token );


    /* from Kolinahr
        1 (0x0001)  -  Base Station
        2 (0x0002)  -  Home Key
        3 (0x0003)  -  Open/Closed
        4 (0x0004)  –   Power
        5 (0x0005)  -  Water
        6 (0x0006)  -  Reminder
        7 (0x0007)  -  Attention
        16 (0x0010)  -  Modem
        23 (0x0017)  -  Motion
        24 (0x0018)  -  Tilt

    int isBaseStation   = (deviceType == 0x0001);
    int isHomeKey       = (deviceType == 0x0002);
    int isOpenClose     = (deviceType == 0x0003);
    int isPower         = (deviceType == 0x0004);
    int isWater         = (deviceType == 0x0005);
    int isReminder      = (deviceType == 0x0006);
    int isAttention     = (deviceType == 0x0007);
    int isModem         = (deviceType == 0x0010);
    int itMotion        = (deviceType == 0x0017);
    int isTilt          = (deviceType == 0x0018);
                
    char    *cPtr = "???";
    switch (deviceType) {
        case    1:  cPtr = "BASE STATION";  break;
        case    2:  cPtr = "HOME KEY";  break;
        case    3:  cPtr = "OPEN-CLOSE SENSOR";  break;
        case    4:  cPtr = "POWER SENSOR";  break;
        case    5:  cPtr = "WATER LEAK SENSOR";  break;
        case    6:  cPtr = "REMINDER";  break;
        case    7:  cPtr = "ATTENTION";  break;
        case    16:  cPtr = "MODEM";  break;
        case    23:  cPtr = "MOTION SENSOR";  break;
        case    24:  cPtr = "TILT SENSOR";  break;
    }
    debug_print( "Device Type [%s]\n", cPtr );
    */

    return deviceType;
}
/*
// -----------------------------------------------------------------------------
int HomeHeartBeat_parseDeviceState (char *token, int deviceType) 
{
    assert( token != NULL );
    debug_print( "Entering. Token [%s]\n", token );
    
    int deviceState = hexStringToInt( token );
    
    if (token != NULL) {
        int len = strlen( token );
        
        //
        // Last two characters should be hexadecimal value
        if (len > 1) {
            if ( isxdigit(token[ len - 2 ]) && isxdigit( token[ len - 1 ]) ) {
                //
                // strtol can parse hex strings!
                deviceState = (int) strtol( &token[ len - 2 ], NULL, 16 );
                int bitOneSet   = (deviceState & 0x01);
                int bitTwoSet   = (deviceState & 0x02);
                
                //
                // Now the interpretation depends on the device type
                int state = -1;
                if (deviceType == 3) {      // OpenClose Sensor
                    if (bitOneSet)
                        state = 0;          // closed
                    else if (bitTwoSet)
                        state = 1;          // open
                    
                    debug_print( "Open/Close Sensor is: [%s]\n", (state == 0 ? "CLOSED" : "OPEN" ) );
                    
                } else if (deviceType == 5) {   // Water Leak Sensor
                    if (bitOneSet)
                        state = 1;          // wet
                    else if (bitTwoSet)
                        state = 0;          // dry
                    
                    debug_print( "Water Leak Sensor is: [%s]\n", (state == 0 ? "DRY" : "WET" ) );
                    
                }
                
            }
        }
    }    
    
    return deviceState;
}
*/

// ----------------------------------------------------------------------------
long    HomeHeartBeat_parseDeviceStateTimer (char *token)
{
    
    //
    //  The devices keep track of time for up to about 100 days. Since we're tracking everything in
    //  seconds, we need room for almost 9 million seconds.
    //
    // According to Kolinahr:
    //  Values from 0 (0x00) to 59 (0x3B) represent 0 to 59 seconds
    //  Values from 64 (0x40) to 123 (0x7B) represent 0 to 59 minutes
    //  Values from 128 (0x80) to 151 (0x97) represent 0 to 23 hours
    //  Values from 160 (0xA0) to 255 (0xFF) represent 0 to 95 days
    return HomeHeartBeat_parseTimerValue( token );
}

// ----------------------------------------------------------------------------
int     HomeHeartBeat_parseDeviceAlerts (char *token)
{
    assert( token != NULL );
    /*
        Bit 0 (0x01)  -  Alarm triggered; controlled by Field 9: Device Configuration
        Bit 1 (0x02)  -  Device off-line, out of range, or battery dead
        Bit 2 (0x04)  -  Low Battery signal sent during “I’m Alive” message from device
    */
    int tmpValue = hexStringToInt( token );

    int alarmTriggered = (tmpValue & ALARMTRIGGERED_BITMASK);
    int deviceOffLine  = (tmpValue & DEVICEOFFLINE_BITMASK);
    int lowBattery     = (tmpValue & LOWBATTERY_BITMASK);

    //debug_print( "Alert flags - Alarm Triggered: [%s], Offline: [%s], Low Battery: [%s]\n",
    //                    (alarmTriggered ? "YES" : "NO"),
    //                    (deviceOffLine ? "YES" : "NO"),
    //                    (lowBattery ? "YES" : "NO") );
    
    return tmpValue;
}


// ----------------------------------------------------------------------------
int     HomeHeartBeat_parseDeviceNameIndex (char *token)
{
    assert( token != NULL );
    
    int indexID = hexStringToInt( token );
    return indexID;
}

// ----------------------------------------------------------------------------
int     HomeHeartBeat_parseDeviceConfiguration (char *token)
{
    /*
     * Each sensor, Home Key, and the Base Station have configurations values contained in this field. 
     * Please see the Home Key, and the Base Station documents for their specific configuration . Each of the 
     * sensors follow the same pattern for configuration as described in the following generalized discussion. 
     * See the device specific documents for the actual sensor configurations.

        Sensor Alarms – Each sensor can be configured to alarm on either of two different conditions:

        Bit 0 (0x0001)  -  Alarm enabled on condition #1
        Bit 1 (0x0002)  -  Alarm enabled on condition #2

        Only one or the other of the bits may be set at a time. When the sensor’s state matches the selected 
        alarm condition, an alarm is generated on the Home Key and bit 0 of field 7 will be set to true.

        Sensor Call me – Each sensor can be configured for “Call me” on either of two conditions:

        Bit 8 (0x0100)  -  Call me enabled on condition #1
        Bit 9 (0x0200)  -  Call me enabled on condition #2

        Only one or the other of the bits may be set at a time. When the sensor’s state matches the 
        selected  “Call me” condition, an alert message is transmitted via the Base Station Modem to the 
        home owner’s email or cell phone. This functionality is no longer available since Eaton disabled the 
       service in June 2011.
       */
    return -1;
}

// ----------------------------------------------------------------------------
int     HomeHeartBeat_parseAliveUpdateTimer (char *token)
{
    /*
     * This is the elapsed time since that last “I’m Alive” message received from the device. 
     * All devices send these messages to the Base Station at regular intervals. For battery operated devices, 
     * this message contains a status of the battery. For all devices, this message serves to inform the Base 
     * Station of device readiness. See Appendix A: Timer Encoding, for the format of this field. 
     * Battery operated devices, excluding the Home Key, send the “I’m alive” message every 30 minutes. The Home 
     * Key sends its message according to its current battery update setting. AC powered devices, such 
     * as the Power Sensor, sends the “I’m alive” once every minute.
     * 
     * If this timer reaches two hours, the Base Station will assume there is a problem (possibly 
     * a dead battery) with the device. The Base Station will then raise an alert by setting bit 1 (0x02) of field 7 
     * to true. When a fresh battery is inserted into a device, it will immediately transmit an “I’m alive” 
     * message, clearing the alert in field 7.
     */
    return HomeHeartBeat_parseTimerValue( token );
}

// ----------------------------------------------------------------------------
int     HomeHeartBeat_parseUpdateFlags (char *token)
{
    /*
     * 
        This field is normally zero (0x00) for all devices. It has been observed to contain the value 0x80 when 
        there is pending data needing to be sent to the device. Please read the description 
        under Field 13: Device Parameter section below.
     * */
    return hexStringToInt( token );
}

// ----------------------------------------------------------------------------
int     HomeHeartBeat_parseDeviceParameter (char *token)
{
    return hexStringToInt( token );
}

// ----------------------------------------------------------------------------
int     HomeHeartBeat_parsePendingUpdateTimer (char *token)
{
    /*
     * This is the elapsed time since the user changed the device configuration parameter defined 
     * in Field 13. The fact that there is a non-zero value in this field is indication that the 
     * value has not yet been transmitted to the device. See Appendix A: Timer Encoding, for 
     * the format of this field.
     */
    return HomeHeartBeat_parseTimerValue( token );
}

// ----------------------------------------------------------------------------
char    *HomeHeartBeat_parseMacAddress (char *token)
{
    assert( token != NULL );
    
    /*
     *  The IEEE 64 bit address known as the EUI-64(tm). The first 6 digits (24 bits) of 
     *  this address consist of the Organizational Unique Identifier (OUI) as assigned by the 
     *  IEEE to Eaton Electrical as 0x000D6F. This sequence appears at the beginning of all 
     *  MAC addresses in a Home Heartbeat network. The remaining 10 digit portion (40 bits) 
     *  serves to uniquely identify an individual device on Home Heartbeat network.
     */
    //
    // Since th first six never change -- skip them
    //
    //  12 34 56   78 90  
    // [00 0D 6F   00 00 01 02 28]
    long  macValue = strtol( &token[ 6 ], NULL, 16 );
    
    //debug_print( "MAC value [%s] %lx\n", &token[ 6 ], macValue );
    
    return ( &token[ 6 ] );
}

// ----------------------------------------------------------------------------
char *HomeHeartBeat_parseDeviceName (char *token)
{
    assert( token != NULL );
    int len = strlen( token );
   
    if (token[ 0 ] == '"')
        token++;
    
    //
    // There may be an extra \"\r\n on the device name
    char    *cPtr;
    if ( (cPtr = strrchr( token, '"')) != NULL) {
        *cPtr = '\0';
    }
    
    return token;
}


// ----------------------------------------------------------------------------
int     HomeHeartBeat_parseTimerValue (char *token)
{
    assert( token != NULL );

    //
    //  The devices keep track of time for up to about 100 days. Since we're tracking everything in
    //  seconds, we need room for almost 9 million seconds.
    //
    // According to Kolinahr:
    /*
     *  The Home Heartbeat uses a single 8-bit counter to track the passing of time. Each timer starts 
     *  out counting in seconds, and gradually looses resolution as it advances. Once a timer passes 
     *  the 59th second, it will only increment for each minute. Then once it passes the 59th minute, 
     *  it increments by just an hour per count. After 24 hours, the timer will increment by one day.
     * 
     *  The value of the timer fields, and their meaning are as follows:
     *          Values from 0 (0x00) to 59 (0x3B) represent 0 to 59 seconds
     *          Values from 64 (0x40) to 123 (0x7B) represent 0 to 59 minutes
     *          Values from 128 (0x80) to 151 (0x97) represent 0 to 23 hours
     *          Values from 160 (0xA0) to 255 (0xFF) represent 0 to 95 days
     * 
     *  Note that I don’t know what the values above 167 really represent. I’m just guessing, 
     *  as I have yet to let a timer run for more than a week.
     * */
    
    long        timerValue = (long) hexStringToInt( token );
    
    int seconds = 0;
    int minutes = 0;
    int hours = 0;
    int days = 0;

    if (timerValue < 64L) {
        seconds = (int) timerValue;
        
    } else if (timerValue < 128L) {
        minutes = (int) (timerValue - 64L);
        seconds = 0;
        
    } else if (timerValue < 160L) {
        hours = (int) (timerValue - 128L);
        minutes = 0;
        seconds = 0;
        
    } else {
        days = (int) (timerValue - 160L);
        hours = 0;
        minutes = 0;
        seconds = 0;
    }

    timerValue = (days * 24L * 60L * 60L) +
                 (hours * 60L * 60L) +
                 (minutes * 60L) +
                 seconds;
    
    //debug_print( "----------totalSeconds: %ld    days %d , hours %d, minutes: %d, seconds: %d\n", timerValue, days, hours, minutes, seconds );
    
    return timerValue;
}

