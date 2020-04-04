//-----------------------------------------------------------------------------
/*!
   \file
   \brief Driver to read/write from/to serial port.
*/
//-----------------------------------------------------------------------------
#ifndef SERPORT_H
#define SERPORT_H

/* -- Includes ------------------------------------------------------------ */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -- Defines ------------------------------------------------------------- */
#define SERPORT_RECEIVE_CODE_CLIENT    (-42)


/* -- Types --------------------------------------------------------------- */

/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */
int serport_init(const char * dev);

void serport_flush(void);

int serport_send(unsigned char const * payload,
                 unsigned int len);

int serport_receive(unsigned char * buffer,
                    unsigned int size);

void serport_shutdown(void);




/* -- Implementation ------------------------------------------------------ */



#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif
