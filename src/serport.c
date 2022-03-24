//-----------------------------------------------------------------------------
/*!
   \file
   \brief Driver to read/write from/to serial port.
*/
//-----------------------------------------------------------------------------


/* -- Includes ------------------------------------------------------------ */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>
#include "serport.h"

/* -- Defines ------------------------------------------------------------- */

/* -- Types --------------------------------------------------------------- */

/* -- Global Variables ---------------------------------------------------- */

/* -- Module Global Variables --------------------------------------------- */
static int mSerFd;


/* -- Module Global Function Prototypes ----------------------------------- */


/* -- Implementation ------------------------------------------------------ */

//See: https://www.gnu.org/software/libc/manual/html_node/Terminal-Modes.html#Terminal-Modes
static int m_set_interface_attribs (int fd, int speed)
{
   struct termios tty = { 0 }; //for forwared compatibility: initialize all member to 0 (just to be sure, everything has a defined value)

   //read out current settings
   if (tcgetattr (fd, &tty) != 0)
   {
      return -1;
   }

   //configure "raw" mode (8n1, no parity, no flow-control)
   //set input mode
   tty.c_iflag &= ~( //clearing of all those flags will have the mentiond effect ...
      INPCK    |  //parity off
      IGNPAR   |  //don't care as parity is off
      PARMRK   |  //don't care as parity is off
      ISTRIP   |  //don't stip the input down to 7bit - leaf it 8bit
      IGNCR    |  //don't discard \r
      ICRNL    |  //don't translate \r to \n
      INLCR    |  //don't translate \n to \r
      IXOFF    |  //don't use start/stop control on input
      IXON     |  //don't use start/stop control on output
      IXANY    |  //don't use start/stop control on input/output
      IMAXBEL  |  //BSD extension: don't send BEL character when terminal buffer gets filled
      IGNBRK   |  //do not ignore break character -> forward it as 0 character (in this application, this is an exception in normal communication, an will detected by CRC of higher level protocol)
      BRKINT   |  //don't clear input/output queues on reception of break character
      0
   );
   //set output mode
   tty.c_oflag &= ~( //clearing of all those flags will have the mentiond effect ...
      OPOST    |  //output data is not processed for terminal applications
      ONLCR    |  //don't care as post processing is off (don't translate \n to \r\n)
      // OXTABS   |  //don't care as post processing is off (don\t translate TAB to SPACES)
      // ONOEOT   |  //don't care as post processing is off (don't discard C-d (ASCII 0x04))
      0
   );
   //set control mode
   tty.c_cflag &= ~( //clearing of all those flags will have the mentiond effect ...
      HUPCL    | //don't generate modem disconnect events
      CSTOPB   | //use only one stop bit
      PARENB   | //no parity
      PARODD   | //don't care as parity is off
      CRTSCTS  | //no hw flow control
      // CCTS_OFLOW | //don't use flow control based on CTS wire
      // CRTS_IFLOW | //don't use flow control based on RTS wire
      // MDMBUF   | //don't use flow control based on carrier
      // CIGNORE  | //MUST BE CLEAR, otherwise the driver doesn't take the control mode settings!!!
      CSIZE    | //clear bitfield, that specifies the bits per character (will be set to the desired value some lines below...)
      0
   );
   tty.c_cflag |= ( //setting of all those flags will have the mentiond effect ...
      CLOCAL   | //terminal is connected “locally” and that the modem status lines (such as carrier detect) shall be ignored.
      CREAD    | //input can be read from the terminal (Otherwise, input would be discarded when it arrives)
      CS8      | //eight bits per byte
      0
   );
   //set local modes
   tty.c_lflag &= ~(
      ICANON   |   //input is processed in noncanonical mode (das ist WICHTIG! sonst wird der Input verandert!!!)
      ECHO     | //don't echo input characters back to the terminal
      ECHOE    | //don't handle ERASE character
      ECHOPRT  | //BSD extension: don't handle ERASE character (in display)
      ECHOK    | //don't handle KILL character
      ECHOKE   | //don't handle KILL character (in a special way)
      ECHONL   | //don't echo \n (don't care as canonical is OFF)
      ECHOCTL  | //BSD extension: don't echo control characters with leading '^' (don't care as canonical is off)
      ISIG     | //don't recognize INTR, QUIT and SUSP characters
      IEXTEN   | //ACHTUNG: don't use implementation-defined feature (on linux: don't use the LNEXT and DISCARD characters)
      NOFLSH   | //don't clear the input/output buffer on INTR, QUIT and SUSP
      TOSTOP   | //BSD extension: don't use SIGTTOU signals are generated by background processes that attempt to write to the terminal
      // ALTWERASE | //meaning of WERASE charater -> beginning of a word is a nonwhitespace character following a whitespace character
      FLUSHO   | //While this bit is set, all output is discarded -> deswegen shalte ich es aus!
      PENDIN   | //indicates that there is a line of input that needs to be reprinted -> brauch ich nicht
      0
   );
   tty.c_lflag |= ( //setting of all those flags will have the mentiond effect ...
      // NOKERNINFO | //disables handling of the STATUS character
      0
   );

   //set timeout behaviour (wait up to TIME*100ms while RX buffer is empty (MIN=0))
   tty.c_cc[VMIN]  = 0;
   tty.c_cc[VTIME] = 0;   //multiple of 100ms (0 -> no timeout)

   //adjust baudrate
   cfsetspeed (&tty, speed); //set input and output speed to same baudrate

   //write back the modified  settings
   if (tcsetattr (fd, TCSANOW, &tty) != 0)
   {
      return -1;
   }
   return 0;
}



int serport_init(const char * dev)
{
   mSerFd = open(dev, O_RDWR | O_NOCTTY);
   if (mSerFd >= 0)
   {
      //configure interface
      m_set_interface_attribs(mSerFd, B14400);
      //drop all data in input and output buffer
      serport_flush();
   }

   //return file descriptor
   return mSerFd;
}


//flush tx and rx buffer
void serport_flush(void)
{
   tcflush(mSerFd, TCIOFLUSH);
}



//return status:
//0 data sent successfully
//otherwise: failed to send send data
int serport_send(unsigned char const * payload,
                 unsigned int len)
{
   return (int)write(mSerFd, payload, len);
}


//receive line (delimited by carriage-return ('\r'))
//<0 no or invalid frame received
//-1 buffer too small
//-2 buffer overflow
//SERPORT_RECEIVE_CODE_CLIENT keyword "CLIENT" was received
//>= 0 length of frame
int serport_receive(unsigned char * buffer,
                    unsigned int size)
{
   static const unsigned char newLine[4] = { '\r', '\n', '\n', '\n' };
   static const unsigned char backspace[2] = { ' ', '\b' };
   unsigned char data;
   unsigned long rd;
   int len = 0;
   int esc = 0;
   fd_set rfds;


   //check for minimal buffer size
   if (size < 1)
   {
      return -1;
   }

   //
   do
   {
      FD_ZERO(&rfds);
      FD_SET(mSerFd, &rfds);

      //sleep until receiption of data
      select(mSerFd+1, &rfds, NULL, NULL, NULL);
      //read data
      rd = read(mSerFd, &data, 1);
      if (rd != 0) //data received?
      {
         //echo each received char
         write(mSerFd, &data, 1);

         //end of line
         if (data == '\r') //ASCII 13
         {
            write(mSerFd, &newLine[1], 1); //echo an extra newline char
            buffer[len] = 0; //add zero termination
            return len;
         }

         //backspace
         if (data == '\b')
         {
            if (len > 0)
            {
               write(mSerFd, &backspace, sizeof(backspace)); //clear char from terminal
               --len; //remove previous char
            }
            continue;
         }

         //otherwise: push received character into buffer
         if ((unsigned int)len < (size - 1)) //one more required to add the trailing zero-termination
         {
            //add to buffer
            buffer[len++] = data;

            //check if the last 6 received chars build the keyword "CLIENT"
            //beim Aufbau eine PC-Direktverbindung, sendet Windows naemlich mehrmals "CLIENT" um eine PPP Verbindung aufzubauen...
            if ((len >= 6) && (memcmp(&buffer[len-6], "CLIENT", 6) == 0))
            {
               buffer[len] = 0; //add zero termination
               return SERPORT_RECEIVE_CODE_CLIENT;
            }

            //otherwise
            continue;
         }

         //buffer overflow
         write(mSerFd, &newLine, sizeof(newLine)); //echo an extra newline char
         return -3;
      }
   } while(1);
}


void serport_shutdown(void)
{
   close(mSerFd);
}
