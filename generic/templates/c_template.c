/*
 * xxx.c --
 *
 *	Implements and registers cipher XXX.
 *
 *
 * Copyright (c) 1995 Andreas Kupries (a.kupries@westend.com)
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
#include "xxx/xxx.h"

#define  MIN_KEYSIZE      (0)
#define  MAX_KEYSIZE      (0)
#define  KEYSCHEDULE_SIZE (0)

/*
 * Declarations of internal procedures.
 */

static void C_Schedule _ANSI_ARGS_ ((VOID*  key, int key_length, int direction,
				      VOID** e_schedule,
				      VOID** d_schedule));
static void C_Encrypt  _ANSI_ARGS_ ((unsigned char* inout, VOID* key /* schedule */));
static void C_Decrypt  _ANSI_ARGS_ ((unsigned char* inout, VOID* key /* schedule */));

/*
 * cipher definition.
 */

static Trf_CipherDescription cDescription = {
  "xxx",
  MIN_KEYSIZE,
  MAX_KEYSIZE,
  KEYSCHEDULE_SIZE,
  C_Schedule,
  C_Encrypt,
  C_Decrypt,
  NULL
};

/*
 *------------------------------------------------------*
 *
 *	TrfInit_XXX --
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
TrfInit_XXX (interp)
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
C_Schedule (key, key_length, direction, e_schedule, d_schedule)
VOID*  key;
int    key_length;
int    direction;
VOID** e_schedule;
VOID** d_schedule;
{
  if (*e_schedule == NULL) {
    *e_schedule = Tcl_Alloc (KEYSCHEDULE_SIZE);

  }

  if (direction == TRF_ENCRYPT) {
    /* nothing to do */
  } else if (direction == TRF_DECRYPT) {
    if (*d_schedule == NULL) {
      *d_schedule = Tcl_Alloc (KEYSCHEDULE_SIZE);


    }
  } else {
    panic ("unknown direction code given to idea::C_Schedule");
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
}

/*
 * External code from here on.
 */

#include "xxx/xxx.c"
