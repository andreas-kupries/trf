/*
 * blowfish.c --
 *
 *	Implements and registers blockcipher BLOWFISH.
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
#include "blowfish/blowfish.h"

#define  BLOCK_SIZE       (8)
#define  MIN_KEYSIZE      (1)
#define  MAX_KEYSIZE      (MAXKEYBYTES)
#define  KEYSCHEDULE_SIZE (sizeof (Blowfish_keyschedule))

/*
 * Declarations of internal procedures.
 */

static void BC_Schedule _ANSI_ARGS_ ((VOID*  key, int key_length, int direction,
				      VOID** e_schedule,
				      VOID** d_schedule));
static void BC_Encrypt  _ANSI_ARGS_ ((VOID* in, VOID* out, VOID* key /* schedule */));
static void BC_Decrypt  _ANSI_ARGS_ ((VOID* in, VOID* out, VOID* key /* schedule */));

/*
 * cipher definition.
 */

static Trf_BlockcipherDescription bcDescription = {
  "blowfish",
  BLOCK_SIZE,
  MIN_KEYSIZE,
  MAX_KEYSIZE,
  KEYSCHEDULE_SIZE,
  BC_Schedule,
  BC_Encrypt,
  BC_Decrypt,
  NULL
};

/*
 *------------------------------------------------------*
 *
 *	TrfInit_BLOWFISH --
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
TrfInit_BLOWFISH (interp)
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
	InitializeBlowfish ((unsigned char*) key, key_length,
			    (Blowfish_keyschedule*) *e_schedule);
      }
    }
  } else if (direction == TRF_DECRYPT) {
    if (*d_schedule == NULL) {
      *d_schedule = Tcl_Alloc (KEYSCHEDULE_SIZE);

      if (*e_schedule != NULL) {
	memcpy (*d_schedule, *e_schedule, KEYSCHEDULE_SIZE);
      } else {
	InitializeBlowfish ((unsigned char*) key, key_length,
			    (Blowfish_keyschedule*) *d_schedule);
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
  unsigned char* i = (unsigned char*) in;
  unsigned char* o = (unsigned char*) out;

  union aword left;
  union aword right;

  /* assume implicit bigendian order of incoming byte stream */

  left.word     = 0;
  right.word    = 0;

  left.w.byte0  = i [0];
  left.w.byte1  = i [1];
  left.w.byte2  = i [2];
  left.w.byte3  = i [3];
  right.w.byte0 = i [4];
  right.w.byte1 = i [5];
  right.w.byte2 = i [6];
  right.w.byte3 = i [7];

  Blowfish_encipher ((Blowfish_keyschedule*) key,
		     (UWORD_32bits*)        &left,
		     (UWORD_32bits*)        &right);

  o [0] = left.w.byte0;
  o [1] = left.w.byte1;
  o [2] = left.w.byte2;
  o [3] = left.w.byte3;
  o [4] = right.w.byte0;
  o [5] = right.w.byte1;
  o [6] = right.w.byte2;
  o [7] = right.w.byte3;
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
  unsigned char* i = (unsigned char*) in;
  unsigned char* o = (unsigned char*) out;

  union aword left;
  union aword right;

  /* assume implicit bigendian order of incoming byte stream */

  left.word     = 0;
  right.word    = 0;

  left.w.byte0  = i [0];
  left.w.byte1  = i [1];
  left.w.byte2  = i [2];
  left.w.byte3  = i [3];
  right.w.byte0 = i [4];
  right.w.byte1 = i [5];
  right.w.byte2 = i [6];
  right.w.byte3 = i [7];

  Blowfish_decipher ((Blowfish_keyschedule*) key,
		     (UWORD_32bits*) &left,
		     (UWORD_32bits*) &right);

  o [0] = left.w.byte0;
  o [1] = left.w.byte1;
  o [2] = left.w.byte2;
  o [3] = left.w.byte3;
  o [4] = right.w.byte0;
  o [5] = right.w.byte1;
  o [6] = right.w.byte2;
  o [7] = right.w.byte3;
}

/*
 * External code from here on.
 */

#include "blowfish/blowfish.c"
