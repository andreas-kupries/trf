/*
 * safer.c --
 *
 *	Implements and registers blockcipher SAFER.
 *
 * Copyright (c) 1997 Andreas Kupries (a.kupries@westend.com)
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

/* Transform tcl knowledge into something useful to the external sources.
 */
#ifdef _USING_PROTOTYPES_
#undef NOT_ANSI_C
#else
#define NOT_ANSI_C
#endif
#include "safer/safer.h"

/*
 * Keyschedule as used by wrapper code in this file.
 */

#define  BLOCK_SIZE       (sizeof (safer_block_t)) /* 8 byte */
#define  MIN_KEYSIZE      (BLOCK_SIZE)
#define  MAX_KEYSIZE      (2*BLOCK_SIZE)
#define  KEYSCHEDULE_SIZE (sizeof (safer_key_t))

#define  SAFER_KEYSIZE    BLOCK_SIZE

/*
 * Declarations of internal procedures.
 */

static void BC_Schedule _ANSI_ARGS_ ((VOID*  key, int key_length,
				      Trf_Options cOptions, int direction,
				      VOID** e_schedule, VOID** d_schedule));
static void BC_Encrypt  _ANSI_ARGS_ ((VOID* in, VOID* out, VOID* key /* schedule */));
static void BC_Decrypt  _ANSI_ARGS_ ((VOID* in, VOID* out, VOID* key /* schedule */));

/*
 * More internal procedures: SAFER specific option processing.
 */

static Trf_Options CreateOptions _ANSI_ARGS_ ((ClientData clientData));
static void        DeleteOptions _ANSI_ARGS_ ((Trf_Options options,
					       ClientData clientData));
static int         CheckOptions  _ANSI_ARGS_ ((Trf_Options options, Tcl_Interp* interp,
					       CONST Trf_BaseOptions* baseOptions,
					       ClientData clientData));
#if (TCL_MAJOR_VERSION < 8)
static int         SetOption     _ANSI_ARGS_ ((Trf_Options options, Tcl_Interp* interp,
					       CONST char* optname, CONST char* optvalue,
					       ClientData clientData));
#else
static int         SetOption     _ANSI_ARGS_ ((Trf_Options options, Tcl_Interp* interp,
					       CONST char* optname, CONST Tcl_Obj* optvalue,
					       ClientData clientData));
#endif
/* ** NOT USED **
   static int QueryOptions  _ANSI_ARGS_ ((Trf_Options options, ClientData clientData));
   */

typedef struct _SaferOptions_ {
  short rounds;         /* number of rounds to perform */
  short strongSchedule; /* boolean flag indicating wether the strengthened
			 * schedule is to be used or not */
} SaferOptions;


/*
 * cipher definition.
 */

static Trf_OptionVectors bcOptions = {
  CreateOptions,
  DeleteOptions,
  CheckOptions,
#if (TCL_MAJOR_VERSION < 8)
  SetOption,
  NULL,      /* no object procedure */
#else
  NULL,      /* no string procedure */
  SetOption,
#endif
  NULL /* QueryOptions not required for cipher specific option processor */
};


static Trf_BlockcipherDescription bcDescription = {
  "safer",
  BLOCK_SIZE,
  MIN_KEYSIZE,
  MAX_KEYSIZE,
  KEYSCHEDULE_SIZE,
  BC_Schedule,
  BC_Encrypt,
  BC_Decrypt,
  NULL,
  &bcOptions
};

/*
 *------------------------------------------------------*
 *
 *	TrfInit_SAFER --
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
TrfInit_SAFER (interp)
Tcl_Interp* interp;
{
  Safer_Init_Module ();
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
BC_Schedule (key, key_length, cOptions, direction, e_schedule, d_schedule)
VOID*       key;
int         key_length;
Trf_Options cOptions;
int         direction;
VOID**      e_schedule;
VOID**      d_schedule;
{
  SaferOptions* so = (SaferOptions*) cOptions;

  safer_block_t  b1;
  safer_block_t  b2;

  memcpy (b1, key, SAFER_KEYSIZE);

  if (key_length > SAFER_KEYSIZE) {
    memcpy (b2, ((char*)key) + SAFER_KEYSIZE, SAFER_KEYSIZE);
  } else {
    memcpy (b2, b1, SAFER_KEYSIZE);
  }

  if (direction == TRF_ENCRYPT) {
    if (*e_schedule == NULL) {
      *e_schedule = Tcl_Alloc (KEYSCHEDULE_SIZE);

      if (*d_schedule != NULL) {
	memcpy (*e_schedule, *d_schedule, KEYSCHEDULE_SIZE);
      } else {
	Safer_Expand_Userkey (b1, b2, so->rounds,
			      so->strongSchedule,
			      *(safer_key_t*) *e_schedule);
      }
    }
  } else if (direction == TRF_DECRYPT) {
    if (*d_schedule == NULL) {
      *d_schedule = Tcl_Alloc (KEYSCHEDULE_SIZE);

      if (*e_schedule != NULL) {
	memcpy (*d_schedule, *e_schedule, KEYSCHEDULE_SIZE);
      } else {
	Safer_Expand_Userkey (b1, b2, so->rounds,
			      so->strongSchedule,
			      *(safer_key_t*) *d_schedule);
      }
    }
  } else {
    panic ("unknown direction code given to safer.c::BC_Schedule");
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
  Safer_Encrypt_Block (*(safer_block_t*) in,
		       *(safer_key_t*)   key,
		       *(safer_block_t*) out);
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
  Safer_Decrypt_Block (*(safer_block_t*) in,
		       *(safer_key_t*)   key,
		       *(safer_block_t*) out);
}

/*
 * Option processing facilities
 */
/*------------------------------------------------------*
 *
 *	CreateOptions --
 *
 *	------------------------------------------------*
 *	Create option structure specific to SAFER.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Allocates memory and initializes it as
 *		option structure for (stream) ciphers.
 *
 *	Result:
 *		A reference to the allocated block of
 *		memory.
 *
 *------------------------------------------------------*
 */

static Trf_Options
CreateOptions (clientData)
ClientData clientData;
{
  /*  TrfBlockcipherOptionBlock* o = (TrfBlockcipherOptionBlock*) clientData; */
  SaferOptions*             so;

  so = (SaferOptions*) Tcl_Alloc (sizeof (SaferOptions));

  so->rounds         = -1;
  so->strongSchedule =  0;

  return (Trf_Options) so;
}

/*
 *------------------------------------------------------*
 *
 *	DeleteOptions --
 *
 *	------------------------------------------------*
 *	Delete option structure specific to SAFER.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		A memory block allocated by 'CreateOptions'
 *		is released.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
DeleteOptions (options, clientData)
Trf_Options options;
ClientData clientData;
{
  /*  TrfBlockcipherOptionBlock* o = (TrfBlockcipherOptionBlock*) clientData; */

  Tcl_Free ((char*) options);
}

/*
 *------------------------------------------------------*
 *
 *	CheckOptions --
 *
 *	------------------------------------------------*
 *	Check the given option structure for errors.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		May modify the given structure to set
 *		default values into uninitialized parts.
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

static int
CheckOptions (options, interp, baseOptions, clientData)
Trf_Options            options;
Tcl_Interp*            interp;
CONST Trf_BaseOptions* baseOptions;
ClientData             clientData;
{
  TrfBlockcipherOptionBlock* o = (TrfBlockcipherOptionBlock*) clientData;
  SaferOptions*             so = (SaferOptions*) options;

  /*
   * Performs additional check and manipulation of standard structure too!
   * SAFER either uses an 8- or 16-byte key, but nothing in between.
   */

  if (o->key_length < (2*SAFER_KEYSIZE))
    o->key_length = SAFER_KEYSIZE;

  if (so->rounds < 0) {
    /*
     * Add in the default number of rounds. Done here, and not in
     * 'CreateOptions' because the exact value depends on the length
     * of the key and chosen keyschedule.
     */

      if (so->strongSchedule)
	so->rounds = (o->key_length == SAFER_KEYSIZE ?
		      SAFER_SK64_DEFAULT_NOF_ROUNDS  :
		      SAFER_SK128_DEFAULT_NOF_ROUNDS);
      else
	so->rounds = (o->key_length == SAFER_KEYSIZE ?
		      SAFER_K64_DEFAULT_NOF_ROUNDS   :
		      SAFER_K128_DEFAULT_NOF_ROUNDS);
  }

  /*
   * Check number of rounds against allowed lower and upper bounds.
   */

  if (so->rounds > SAFER_MAX_NOF_ROUNDS) {
    Tcl_AppendResult (interp, "safer: value of -rounds exceeded upper limit",
		      (char*) NULL);
    return TCL_ERROR;
  }

  return TCL_OK;
}

/*
 *------------------------------------------------------*
 *
 *	SetOption --
 *
 *	------------------------------------------------*
 *	Define value of given option.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Sets the given value into the option
 *		structure
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

static int
SetOption (options, interp, optname, optvalue, clientData)
Trf_Options options;
Tcl_Interp* interp;
CONST char* optname;
#if (TCL_MAJOR_VERSION < 8)
CONST char*    optvalue;
#else
CONST Tcl_Obj* optvalue;
#endif
ClientData  clientData;
{
  /* Possible options:
   *
   * -rounds          integer
   * -strong-schedule boolean
   */

  /*  TrfBlockcipherOptionBlock* o = (TrfBlockcipherOptionBlock*) clientData; */
  SaferOptions*             so = (SaferOptions*) options;

  CONST char*            value;

  int len = strlen (optname+1);

#if (TCL_MAJOR_VERSION < 8)
  value = optvalue;
#else
  value = Tcl_GetStringFromObj ((Tcl_Obj*) optvalue, NULL);
#endif

  switch (optname [1]) {
  case 'r':
    if (0 == strncmp (optname+1, "rounds", len)) {
      int r;

      if (TCL_OK != Tcl_GetInt (interp, (char*) value, &r)) {
	return TCL_ERROR;
      }

      so->rounds = r;
    } else
      goto unknown_option;
    break;

  case 's':
    if (0 == strncmp (optname+1, "strong-schedule", len)) {
      int s;

      if (TCL_OK != Tcl_GetBoolean (interp, (char*) value, &s)) {
	return TCL_ERROR;
      }

      so->strongSchedule = s;
    } else
      goto unknown_option;
    break;

  default:
    goto unknown_option;
    break;
  }

  return TCL_OK;

 unknown_option:
  Tcl_AppendResult (interp, "unknown option '", (char*) NULL);
  Tcl_AppendResult (interp, optname, (char*) NULL);
  Tcl_AppendResult (interp, "'", (char*) NULL);
  return TCL_ERROR;
}

/*
 * External code from here on.
 */

#include "safer/safer.c"
