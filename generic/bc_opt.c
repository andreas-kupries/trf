/*
 * bc_opt.c --
 *
 *	Implements the C level procedures handling option processing
 *	for block ciphers.
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

/*
 * forward declarations of all internally used procedures.
 */

static Trf_Options CreateOptions _ANSI_ARGS_ ((ClientData clientData));
static void        DeleteOptions _ANSI_ARGS_ ((Trf_Options options,
					       ClientData clientData));
static int         CheckOptions  _ANSI_ARGS_ ((Trf_Options options, Tcl_Interp* interp,
					       CONST Trf_BaseOptions* baseOptions,
					       ClientData clientData));
static int         SetOption     _ANSI_ARGS_ ((Trf_Options options, Tcl_Interp* interp,
					       CONST char* optname, CONST char* optvalue,
					       ClientData clientData));
static int         QueryOptions  _ANSI_ARGS_ ((Trf_Options options,
					       ClientData clientData));


/*
 *------------------------------------------------------*
 *
 *	TrfBlockcipherOptions --
 *
 *	------------------------------------------------*
 *	Accessor to the set of vectors realizing option
 *	processing for blockciphers.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		None.
 *
 *	Result:
 *		See above.
 *
 *------------------------------------------------------*
 */

Trf_OptionVectors*
TrfBlockcipherOptions ()
{
  static Trf_OptionVectors optVec =
    {
      CreateOptions,
      DeleteOptions,
      CheckOptions,
      SetOption,
      QueryOptions
    };

  return &optVec;
}

/*
 *------------------------------------------------------*
 *
 *	CreateOptions --
 *
 *	------------------------------------------------*
 *	Create option structure for block ciphers.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Allocates memory and initializes it as
 *		option structure for blockciphers.
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
  TrfBlockcipherOptionBlock* o;

  o = (TrfBlockcipherOptionBlock*) Tcl_Alloc (sizeof (TrfBlockcipherOptionBlock));

  o->direction      = TRF_UNKNOWN_MODE;
  o->operation_mode = TRF_UNKNOWN_MODE;
  o->key_length     = -1;
  o->key            = NULL;
  o->iv             = NULL;
  o->shift_width    = -1;

  /* ---- derived information ---- */

  o->eks_length          = -1;
  o->dks_length          = -1;
  o->encrypt_keyschedule = NULL;
  o->decrypt_keyschedule = NULL;

  return (Trf_Options) o;
}

/*
 *------------------------------------------------------*
 *
 *	DeleteOptions --
 *
 *	------------------------------------------------*
 *	Delete option structure of a blockciphers.
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
  TrfBlockcipherOptionBlock*  o       = (TrfBlockcipherOptionBlock*) options;
  Trf_BlockcipherDescription* bc_desc = (Trf_BlockcipherDescription*) clientData;

  if (o->key != NULL) {
    memset (o->key, '\0', o->key_length);
    Tcl_Free ((char*) o->key);
  }

  if (o->iv != NULL) {
    memset (o->iv, '\0', bc_desc->block_size);
    Tcl_Free ((char*) o->iv);
  }

  if (o->encrypt_keyschedule != NULL) {
    memset (o->encrypt_keyschedule, '\0', o->eks_length);
    Tcl_Free ((char*) o->encrypt_keyschedule);
  }

  if (o->decrypt_keyschedule != NULL) {
    memset (o->decrypt_keyschedule, '\0', o->dks_length);
    Tcl_Free ((char*) o->decrypt_keyschedule);
  }

  Tcl_Free ((char*) o);
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
  TrfBlockcipherOptionBlock*        o = (TrfBlockcipherOptionBlock*) options;
  Trf_BlockcipherDescription* bc_desc = (Trf_BlockcipherDescription*) clientData;

  /*
   * Call cipher dependent check of environment first.
   */

  if (bc_desc->checkProc != NULL) {
    if (TCL_OK != (*bc_desc->checkProc) (interp)) {
      return TCL_ERROR;
    }
  }


  if (o->direction == TRF_UNKNOWN_MODE) {
    Tcl_AppendResult (interp, "direction not specified", (char*) NULL);
    return TCL_ERROR;
  }

  if (o->operation_mode == TRF_UNKNOWN_MODE) {
    Tcl_AppendResult (interp, "mode not specified", (char*) NULL);
    return TCL_ERROR;
  }

  if (o->key == NULL) {
    Tcl_AppendResult (interp, "key not specified", (char*) NULL);
    return TCL_ERROR;
  }

  if (o->operation_mode >= TRF_CBC_MODE) {
    if (o->iv == NULL) {
      Tcl_AppendResult (interp, "iv not specified for stream mode", (char*) NULL);
      return TCL_ERROR;
    }
  }

  if (o->operation_mode >= TRF_CFB_MODE) {
    if (o->shift_width < 0) {
      Tcl_AppendResult (interp, "shift not specified for feedback mode", (char*) NULL);
      return TCL_ERROR;
    }
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
CONST char* optvalue;
ClientData  clientData;
{
  /* Possible options:
   *
   * -direction	encrypt/decrypt
   * -mode	ecb/cbc/cfb/ofb
   * -key	<readable channel>
   * -iv	<readable channel>
   * -shift	<number>
   */

  TrfBlockcipherOptionBlock*  o       = (TrfBlockcipherOptionBlock*) options;
  Trf_BlockcipherDescription* bc_desc = (Trf_BlockcipherDescription*) clientData;

  int len = strlen (optname + 1);

  switch (optname [1]) {
  case 'd':
    if (0 == strncmp (optname, "-direction", len)) {
      len = strlen (optvalue);

      switch (optvalue [0]) {
      case 'e':
	if (0 == strncmp ("encrypt", optvalue, len)) {
	  o->direction = TRF_ENCRYPT;
	} else
	  goto unknown_direction;
	break;

      case 'd':
	if (0 == strncmp ("decrypt", optvalue, len)) {
	  o->direction = TRF_DECRYPT;
	} else
	  goto unknown_direction;
	break;

      default:
      unknown_direction:
	Tcl_AppendResult (interp, "unknown direction \"", optvalue, "\"", (char*) NULL);
	return TCL_ERROR;
      }
    } else
      goto unknown_option;
    break;

  case 'm':
    if (0 == strncmp (optname, "-mode", len)) {
      len = strlen (optvalue);

      switch (optvalue [0]) {
      case 'e':
	if (0 == strncmp ("ecb", optvalue, len)) {
	  o->operation_mode = TRF_ECB_MODE;
	} else
	  goto unknown_mode;
	break;

      case 'c':
	if (len < 2)
	  goto unknown_mode;
	else if   (0 == strncmp ("cbc", optvalue, len)) {
	  o->operation_mode = TRF_CBC_MODE;
	} else if (0 == strncmp ("cfb", optvalue, len)) {
	  o->operation_mode = TRF_CFB_MODE;
	} else
	  goto unknown_mode;
	break;

      case 'o':
	if (0 == strncmp ("ofb", optvalue, len)) {
	  o->operation_mode = TRF_OFB_MODE;
	} else
	  goto unknown_mode;
	break;

      default:
      unknown_mode:
	Tcl_AppendResult (interp, "unknown mode \"", optvalue, "\"", (char*) NULL);
	return TCL_ERROR;
      }
    } else
      goto unknown_option;
    break;

  case 'i':
    if (0 == strncmp (optname, "-iv", len)) {
      int         access;
      Tcl_Channel iv;

      iv = Tcl_GetChannel (interp, (char*) optvalue, &access);

      if (iv == (Tcl_Channel) NULL)
	return TCL_ERROR;
      else if (! (access & TCL_READABLE)) {
	Tcl_AppendResult (interp, "iv \"", optvalue,
			  "\" not opened for reading",
			  (char*) NULL);
	return TCL_ERROR;
      } else {
	/*
	 * Extract iv information from channel.
	 */

	char* tmp = Tcl_Alloc (bc_desc->block_size);
	int res   = Tcl_Read  (iv, tmp, bc_desc->block_size);

	if (res < 0) {
	  Tcl_Free (tmp);
	  Tcl_AppendResult (interp, "error reading iv from \"",
			    optvalue, "\": ", Tcl_PosixError (interp),
			    (char *) NULL);
	  return TCL_ERROR;
	} else if (res < bc_desc->block_size) {
	  Tcl_Free (tmp);
	  Tcl_AppendResult (interp, "iv to short (< blocksize)", (char*) NULL);
	  return TCL_ERROR;
	}

	o->iv = tmp;
      }
    } else
      goto unknown_option;
    break;

  case 'k':
    if (0 == strncmp (optname, "-key", len)) {
      int         access;
      Tcl_Channel key;

      key = Tcl_GetChannel (interp, (char*) optvalue, &access);
      if (key == (Tcl_Channel) NULL)
	return TCL_ERROR;
      else if (! (access & TCL_READABLE)) {
	Tcl_AppendResult (interp, "key \"", optvalue,
			  "\" not opened for reading",
			  (char*) NULL);
	return TCL_ERROR;
      } else {
	/*
	 * Extract key from channel.
	 */

	char* tmp = Tcl_Alloc (bc_desc->max_keysize);
	int res   = Tcl_Read  (key, tmp, bc_desc->max_keysize);

	if (res < 0) {
	  Tcl_Free (tmp);
	  Tcl_AppendResult (interp, "error reading key from \"", optvalue,
			    "\": ", Tcl_PosixError (interp),
			    (char *) NULL);
	  return TCL_ERROR;
	} else if (res < bc_desc->min_keysize) {
	  Tcl_Free (tmp);
	  Tcl_AppendResult (interp, "key to short (< minimal keysize)", (char*) NULL);
	  return TCL_ERROR;
	}

	o->key        = tmp;
	o->key_length = res;
      }
    } else
      goto unknown_option;
    break;

  case 's':
    if (0 == strncmp (optname, "-shift", len)) {
      int res;

      res = Tcl_GetInt (interp, (char*) optvalue, &o->shift_width);
      if (res != TCL_OK)
	return res;
      else if (o->shift_width <= 0) {
	Tcl_AppendResult (interp, "shift must be > 0", (char*) NULL);
	return TCL_ERROR;
      } else if (o->shift_width > bc_desc->block_size) {
	Tcl_AppendResult (interp, "shift to large (> blocksize)", (char*) NULL);
	return TCL_ERROR;
      } else if ((bc_desc->block_size % o->shift_width) != 0) {
	Tcl_AppendResult (interp, "shift not a divisor of blocksize", (char*) NULL);
	return TCL_ERROR;
      }
    } else
      goto unknown_option;
    break;

  default:
    goto unknown_option;
    break;
  }

  return TCL_OK;

 unknown_option:
  Tcl_AppendResult (interp, "unknown option '", optname, "'",
		    (char*) NULL);
  return TCL_ERROR;
}

/*
 *------------------------------------------------------*
 *
 *	QueryOptions --
 *
 *	------------------------------------------------*
 *	Returns a value indicating wether the encoder or
 *	decoder set of vectors is to be used by immediate
 *	execution.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		None
 *
 *	Result:
 *		1 - use encoder vectors.
 *		0 - use decoder vectors.
 *
 *------------------------------------------------------*
 */

static int
QueryOptions (options, clientData)
Trf_Options options;
ClientData  clientData;
{
  TrfBlockcipherOptionBlock* o = (TrfBlockcipherOptionBlock*) options;

  return (o->direction == TRF_ENCRYPT ? 1 : 0);
}
