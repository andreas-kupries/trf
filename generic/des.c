/*
 * des.c --
 *
 *	Implements and registers blockcipher DES.
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
/*#include "des/des.h"*/

#define  BLOCK_SIZE       (8)
#define  MIN_KEYSIZE      (8)
#define  MAX_KEYSIZE      (8)
#define  KEYSCHEDULE_SIZE (DES_SCHEDULE_SZ)

/*
 * Declarations of internal procedures.
 */

static void BC_Schedule _ANSI_ARGS_ ((VOID*  key, int key_length, int direction,
				      VOID** e_schedule,
				      VOID** d_schedule));
static void BC_Encrypt  _ANSI_ARGS_ ((VOID* in, VOID* out, VOID* key /* schedule */));
static void BC_Decrypt  _ANSI_ARGS_ ((VOID* in, VOID* out, VOID* key /* schedule */));
static int  BC_Check    _ANSI_ARGS_ ((Tcl_Interp* interp));

/*
 * cipher definition.
 */

static Trf_BlockcipherDescription bcDescription = {
  "des",
  BLOCK_SIZE,
  MIN_KEYSIZE,
  MAX_KEYSIZE,
  KEYSCHEDULE_SIZE,
  BC_Schedule,
  BC_Encrypt,
  BC_Decrypt,
  BC_Check
};

/*
 *------------------------------------------------------*
 *
 *	TrfInit_DES --
 *
 *	------------------------------------------------*
 *	Register the blockcipher implemented in this file.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of 'Trf_RegisterBlockcipher'.
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

int
TrfInit_DES (interp)
Tcl_Interp* interp;
{
  return Trf_RegisterBlockcipher (interp, &bcDescription);
}

/*
 *------------------------------------------------------*
 *
 *	BC_Schedule --
 *
 *	------------------------------------------------*
 *	Generate keyschedules for blockcipher from
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
BC_Schedule (key, key_length, direction, e_schedule, d_schedule)
VOID*  key;
int    key_length;
int    direction;
VOID** e_schedule;
VOID** d_schedule;
{
  if (direction == TRF_ENCRYPT) {

    if (*e_schedule == NULL) {
      *e_schedule = Tcl_Alloc (KEYSCHEDULE_SIZE);

      if (*d_schedule != NULL) {
	memcpy (*e_schedule, *d_schedule, KEYSCHEDULE_SIZE);
      } else {
	ld.des_set_key ((des_cblock*) key,
			*((des_key_schedule*) *e_schedule));
      }
    }
  } else if (direction == TRF_DECRYPT) {
    if (*d_schedule == NULL) {
      *d_schedule = Tcl_Alloc (KEYSCHEDULE_SIZE);

      if (*e_schedule != NULL) {
	memcpy (*d_schedule, *e_schedule, KEYSCHEDULE_SIZE);
      } else {
	ld.des_set_key ((des_cblock*) key,
			*((des_key_schedule*) *d_schedule));
      }

    }
  } else {
    panic ("unknown direction code given to idea::BC_Schedule");
  }
}

/*
 *------------------------------------------------------*
 *
 *	BC_Encrypt --
 *
 *	------------------------------------------------*
 *	Encrypt a single block with the implemented cipher.
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
BC_Encrypt (in, out, key)
VOID* in;
VOID* out;
VOID* key;
{
  ld.des_ecb_encrypt ((des_cblock*) in,
		      (des_cblock*) out,
		      * (des_key_schedule*) key,
		      1);
}

/*
 *------------------------------------------------------*
 *
 *	BC_Decrypt --
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
BC_Decrypt (in, out, key)
VOID* in;
VOID* out;
VOID* key;
{
  ld.des_ecb_encrypt ((des_cblock*) in,
		      (des_cblock*) out,
		      * (des_key_schedule*) key,
		      0);
}

/*
 *------------------------------------------------------*
 *
 *	BC_Check --
 *
 *	------------------------------------------------*
 *	Check for existence of libdes, load it.
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

static int
BC_Check (interp)
Tcl_Interp* interp;
{
  return TrfLoadLibdes (interp);
}

/*
 * External code from here on.
 */

/*#include "des/set_key.c"*/
/*#include "des/ecb_enc.c"*/
