/*
 * c_opt.c --
 *
 *	Implements the C level procedures handling option processing
 *	for (stream) ciphers.
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
 *	TrfCipherOptions --
 *
 *	------------------------------------------------*
 *	Accessor to the set of vectors realizing option
 *	processing for (stream) ciphers.
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
TrfCipherOptions ()
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
 *	Create option structure for (stream) ciphers.
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
  TrfCipherOptionBlock* o;

  o = (TrfCipherOptionBlock*) Tcl_Alloc (sizeof (TrfCipherOptionBlock));

  o->direction      = TRF_UNKNOWN_MODE;
  o->key_length     = -1;
  o->key            = NULL;

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
 *	Delete option structure of a (stream) ciphers.
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
  TrfCipherOptionBlock*  o       = (TrfCipherOptionBlock*) options;
#if 0
  Trf_CipherDescription* c_desc = (Trf_CipherDescription*) clientData;
#endif

  if (o->key != NULL) {
    memset (o->key, '\0', o->key_length);
    Tcl_Free ((char*) o->key);
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
  TrfCipherOptionBlock*       o = (TrfCipherOptionBlock*) options;
  Trf_CipherDescription* c_desc = (Trf_CipherDescription*) clientData;

  /*
   * Call cipher dependent check of environment first.
   */

  if (c_desc->checkProc != NULL) {
    if (TCL_OK != (*c_desc->checkProc) (interp)) {
      return TCL_ERROR;
    }
  }

  if (o->direction == TRF_UNKNOWN_MODE) {
    Tcl_AppendResult (interp, "direction not specified", (char*) NULL);
    return TCL_ERROR;
  }

  if (o->key == NULL) {
    Tcl_AppendResult (interp, "key not specified", (char*) NULL);
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
CONST char* optvalue;
ClientData  clientData;
{
  /* Possible options:
   *
   * -direction	encrypt/decrypt
   * -mode	ecb/cc/cfb/ofb
   * -key	<readable channel>
   * -iv	<readable channel>
   * -shift	<number>
   */

  TrfCipherOptionBlock*  o      = (TrfCipherOptionBlock*) options;
  Trf_CipherDescription* c_desc = (Trf_CipherDescription*) clientData;

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

	char* tmp = Tcl_Alloc (c_desc->max_keysize);
	int res   = Tcl_Read  (key, tmp, c_desc->max_keysize);

	if (res < 0) {
	  Tcl_Free (tmp);
	  Tcl_AppendResult (interp, "error reading key from \"", optvalue,
			    "\": ", Tcl_PosixError (interp),
			    (char *) NULL);
	  return TCL_ERROR;
	} else if (res < c_desc->min_keysize) {
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
  TrfCipherOptionBlock* o = (TrfCipherOptionBlock*) options;

  return (o->direction == TRF_ENCRYPT ? 1 : 0);
}
