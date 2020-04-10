//-----------------------------------------------------------------------------
/*!
   \file
   \brief Bridge-Tool to relay data from serial com port to tcp socket (and vice versa).
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include "serport.h"


/* -- Defines ------------------------------------------------------------- */
#define TTY_NAME_MAX          (32)
#define RX_BUFFER_HEADER      (16)
#define RX_BUFFER_SIZE        (128)


/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */
static int mCtrlC;

/* -- Module Global Function Prototypes ----------------------------------- */
static const char * exec_command(unsigned char * rxBuffer, unsigned int rxLen);


/* -- Implementation ------------------------------------------------------ */
static void m_signal_handler(int a)
{
   mCtrlC = 1;
}




int main(int argc, const char * argv[])
{
   unsigned char rxBuffer[RX_BUFFER_HEADER + RX_BUFFER_SIZE + TTY_NAME_MAX + 1];
   const char * const tty = "/dev/ttyUSB0";
   const char * sysCall;
   int greets = 0;
   int serFd;
   int rxLen;

   //register signal handler, to quit program usin CTRL+C
   signal(SIGINT, &m_signal_handler);
   printf("Press [CTRL + C] to quit\n\n");
   printf("Trying to operate serial device %s, at 115200bps 8n1\n", tty);
   while (!mCtrlC)
   {
      //init serial interface
      serFd = serport_init(tty);
      if (serFd < 0)
      {
         printf("Failed to open serial device %s\n", tty);
         return -2;
      }

      //send greetings
      static const char * const greetings = "\r\nminlux zterm\r\n" \
                                            "Type \"zdir\" + ENTER to get list of available files\r\n" \
                                            "Type \"zsend <file>\" + ENTER to request reception of a file\r\n" \
                                            "Type \"CLIENT\" to quit the server\r\n";
      if (greets == 0)
      {
         serport_send(greetings, strlen(greetings)); //entire greetings
         greets = 1; //only (the first time) once
      }
      else
      {
         serport_send(greetings, 2); //only a new-line
      }


      //start reception
      do
      {
         //spare out some leading bytes of rxBuffer, because i will prefix some chars in "exec_command" later on...
         rxLen = serport_receive(&rxBuffer[RX_BUFFER_HEADER], RX_BUFFER_SIZE);
         if (rxLen > 0)
         {
            sysCall = exec_command(&rxBuffer[RX_BUFFER_HEADER], (unsigned int)rxLen);
         }
         //a direct cable connection request was received -> quit program
         if (rxLen == SERPORT_RECEIVE_CODE_CLIENT)
         {
            printf("Quit program due to \"CLIENT\" request\n");
            mCtrlC = 1;
         }
      } while (!mCtrlC && (sysCall == NULL)); //neither [CTRL + C] nor system call

      //shutdown serial port (so that it is available for the sys-call)
      serport_shutdown();

      //do requested system call (if requested)
      if (!mCtrlC && (sysCall != NULL)) //no [CTRL + C] but system call
      {
         //call system command
         printf("Calling ");
         puts(sysCall);
         int err = system(sysCall);
         printf("Error=%d\n", err);
      }
   }
   return 0;
}



static const char * exec_command(unsigned char * rxBuffer, unsigned int rxLen)
{
   //zdir command
   if ((rxLen >= 4) && (strncmp(rxBuffer, "zdir", 4) == 0))
   {
      //add a prefix (must not me more than RX_BUFFER_HEADER)
      rxBuffer[-2] = '.';
      rxBuffer[-1] = '/'; //command shall be ./zdir
      rxBuffer[4] = 0;
      return (const char *)&rxBuffer[-2]; //system call desired!
   }

   //zsend command
   if ((rxLen >= 7) && (strncmp(rxBuffer, "zsend ", 6) == 0))
   {
      //add a prefix (must not me more than RX_BUFFER_HEADER)
      rxBuffer[-2] = '.';
      rxBuffer[-1] = '/'; //command shall be ./zsend ...
      return (const char *)&rxBuffer[-2]; //system call desired!
   }

   //other commands
   //todo

   //otherwise
   return NULL; //no system call desired
}
