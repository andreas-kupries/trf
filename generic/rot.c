/*
 * rot.c --
 *
 *	Implements and registers cipher ROT.
 *
 *
 * Copyright (c) 1996 Andreas Kupries (a.kupries@westend.com)
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL I LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL,
 * INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS
 * SOFTWARE AND ITS DOCUMENTATION, EVEN IF I HAVE BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * I SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND
 * I HAVE NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
 * ENHANCEMENTS, OR MODIFICATIONS.
 *
 * CVS: $Id$
 */

#include "transformInt.h"

#define  MIN_KEYSIZE      (1)
#define  MAX_KEYSIZE      (1)
#define  KEYSCHEDULE_SIZE (sizeof (char))

/*
 * Declarations of internal procedures.
 */

static void C_Schedule _ANSI_ARGS_ ((VOID*  key, int key_length,
				     Trf_Options cOptions, int direction,
				     VOID** e_schedule, VOID** d_schedule));
static void C_Encrypt  _ANSI_ARGS_ ((unsigned char* inout, VOID* key /* schedule */));
static void C_Decrypt  _ANSI_ARGS_ ((unsigned char* inout, VOID* key /* schedule */));

/*
 * cipher definition.
 */

static Trf_CipherDescription cDescription = {
  "rot",
  MIN_KEYSIZE,
  MAX_KEYSIZE,
  KEYSCHEDULE_SIZE,
  C_Schedule,
  C_Encrypt,
  C_Decrypt,
  NULL,
  NULL /* no additional options */
};

/*
 *------------------------------------------------------*
 *
 *	TrfInit_ROT --
 *
 *	------------------------------------------------*
 *	Register the cipher implemented in this file.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of 'Trf_RegisterCipher'.
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

int
TrfInit_ROT (interp)
Tcl_Interp* interp;
{
  return Trf_RegisterCipher (interp, &cDescription);
}

/*
 *------------------------------------------------------*
 *
 *	C_Schedule --
 *
 *	------------------------------------------------*
 *	Generate keyschedules for cipher from
 *	specified key.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of the called procedure.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
C_Schedule (keyptr, key_length, cOptions, direction, e_schedule, d_schedule)
VOID*  keyptr;
int    key_length;
Trf_Options cOptions;
int    direction;
VOID** e_schedule;
VOID** d_schedule;
{
  char* key = (char*) keyptr;

  if (direction == TRF_ENCRYPT) {

    if (*e_schedule == NULL) {
      *e_schedule = Tcl_Alloc (KEYSCHEDULE_SIZE);

      if (*d_schedule != NULL) {
	*((char*) *e_schedule) = *((char*) *d_schedule);
      } else {
	*((char*) *e_schedule) = (char) (*key);
      }
    }
  } else if (direction == TRF_DECRYPT) {
    if (*d_schedule == NULL) {
      *d_schedule = Tcl_Alloc (KEYSCHEDULE_SIZE);

      if (*e_schedule != NULL) {
	*((char*) *d_schedule) = *((char*) *e_schedule);
      } else {
	*((char*) *d_schedule) = (char) (*key);
      }

    }
  } else {
    panic ("unknown direction code given to rot::C_Schedule");
  }
}

/*
 *------------------------------------------------------*
 *
 *	C_Encrypt --
 *
 *	------------------------------------------------*
 *	Encrypt a single character with the implemented cipher.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of the called procedure.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
C_Encrypt (inout, key)
unsigned char* inout;
VOID*          key;
{
  (*inout) = (*inout) + *((char*) key);
}

/*
 *------------------------------------------------------*
 *
 *	C_Decrypt --
 *
 *	------------------------------------------------*
 *	Decrypt a single block with the implemented cipher.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of the called procedure.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
C_Decrypt (inout, key)
unsigned char* inout;
VOID*          key;
{
  (*inout) = (*inout) - *((char*) key);
}

