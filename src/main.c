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
#define RX_BUFFER_HEADER      (16)
#define RX_BUFFER_SIZE        (128)


/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */
static char mTtyDevice[32];
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
   int serFd;
   static unsigned char rxBuffer[RX_BUFFER_HEADER + RX_BUFFER_SIZE];
   int rxLen;
   const char * sysCall;


   sprintf(mTtyDevice, "/dev/ttyUSB0"); //set default device - TODO
   //process command line arguments
   printf("Usage: zterm [tty-device]\n");
   printf("Default tty-device: %s\n", mTtyDevice);
   if (argc >= 2)
   {
      strncpy(mTtyDevice, argv[1], sizeof(mTtyDevice));
      mTtyDevice[sizeof(mTtyDevice) - 1] = 0; //ensure termination
   }


   //register signal handler, to quit program usin CTRL+C
   signal(SIGINT, &m_signal_handler);
   printf("Press [CTRL + C] to quit\n\n");
   while (!mCtrlC)
   {
      //init serial interface
      serFd = serport_init(mTtyDevice);
      if (serFd < 0)
      {
         printf("Failed to initialize serial device %s\n", mTtyDevice);
         return -1;
      }
      printf("Operating serial device %s, at 115200bps 8n1\n", mTtyDevice);

      //send greetings
      static const char * const greetings = "\r\nminlux zterm\r\n" \
                                            "Type \"zdir\" + ENTER to get list of available files\r\n" \
                                            "Type \"zsend <file>\" + ENTER to request reception of a file\r\n";
                                          //   "This tool will terminate automatically on direct cable connection request (\"CLIENT\")\r\n";
#if 1
      serport_send(greetings, 2);
#else
      serport_send(greetings, strlen(greetings));
#endif
      //start reception
      do
      {
         //ich lasse hier die am Anfang des Buffers noch etwas platz, damit ich da dann noch was in "exec_command" voranstellen kann
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
   //todo - currently there is only one command implemented (zsend)

   //otherwise
   return NULL; //no system call desired
}
