/*
 * idea.c --
 *
 *	Implements and registers blockcipher IDEA.
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
#include "idea/idea.h"

#define  BLOCK_SIZE       (Idea_dataSize)
#define  MIN_KEYSIZE      (Idea_userKeySize)
#define  MAX_KEYSIZE      (MIN_KEYSIZE)
#define  KEYSCHEDULE_SIZE (Idea_keySize)

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
  "idea",
  BLOCK_SIZE,
  MIN_KEYSIZE,
  MAX_KEYSIZE,
  KEYSCHEDULE_SIZE,
  BC_Schedule,
  BC_Encrypt,
  BC_Decrypt,
  NULL
};

#ifdef WORDS_BIGENDIAN
#define FLIP(buf,n)
#else
#define FLIP(buf,n) Trf_FlipRegisterShort (buf, n)
#endif

/*
 *------------------------------------------------------*
 *
 *	TrfInit_IDEA --
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
TrfInit_IDEA (interp)
Tcl_Interp* interp;
{
  /* DEBUG */
  printf ("block size   = %5d [byte]\n", BLOCK_SIZE);
  printf ("min key size = %5d [byte]\n", MIN_KEYSIZE);	
  printf ("max key size = %5d [byte]\n", MAX_KEYSIZE);
  printf ("keyschedule  = %5d [byte]\n", KEYSCHEDULE_SIZE);
  /* DEBUG */

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
  Idea_UserKey K;

  memcpy (K, key, MIN_KEYSIZE);
  FLIP   (K,      MIN_KEYSIZE);

  /* DEBUG */  
  printf ("user_key'= "); TrfDumpHex   (stdout, K, MIN_KEYSIZE, 2);
  printf ("user_key'= "); TrfDumpShort (stdout, K, MIN_KEYSIZE, 2);
  printf ("dir      = %d (e%d/d%d)\n", direction, TRF_ENCRYPT, TRF_DECRYPT);
  /* DEBUG */

  /* Generate encryption schedule every time!
   */

  if (*e_schedule == NULL) {
    *e_schedule = Tcl_Alloc (KEYSCHEDULE_SIZE);
    Idea_ExpandUserKey (K, * ((Idea_Key*) *e_schedule));

      /* DEBUG */
      printf ("key expansion (e,%d) {\n", KEYSCHEDULE_SIZE);
      {
	short i;
	unsigned char* b = (unsigned char*) *e_schedule;
	for (i=0; i < KEYSCHEDULE_SIZE; i+=8, b+= 8) {
	  printf ("\t"); TrfDumpHex (stdout, b, 8, 1); TrfDumpShort (stdout, b, 8, 2);
	}
      }
      printf ("}\n");
      /* DEBUG */
  }

  if (direction == TRF_ENCRYPT) {
    /* nothing to do */
  } else if (direction == TRF_DECRYPT) {
    if (*d_schedule == NULL) {
      *d_schedule = Tcl_Alloc (KEYSCHEDULE_SIZE);

      Idea_InvertKey (*((Idea_Key*) *e_schedule),
		      *((Idea_Key*) *d_schedule));
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
  FLIP       (in, Idea_dataSize);
  Idea_Crypt (*(Idea_Data*) in, *(Idea_Data*) out, *(Idea_Key*) key);
  FLIP       (out, Idea_dataSize);
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
  Idea_Data i;

  memcpy     (i, in, Idea_dataSize);
  FLIP       (i,     Idea_dataSize);
  Idea_Crypt (i,   *(Idea_Data*) out, *(Idea_Key*) key);
  FLIP       (out, Idea_dataSize);
}

/*
 * External code from here on.
 */

#include "idea/idea.c"
