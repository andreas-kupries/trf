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
#if (TCL_MAJOR_VERSION < 8)
static int         SetOption     _ANSI_ARGS_ ((Trf_Options options, Tcl_Interp* interp,
					       CONST char* optname, CONST char* optvalue,
					       ClientData clientData));
#else
static int         SetOption     _ANSI_ARGS_ ((Trf_Options options, Tcl_Interp* interp,
					       CONST char* optname, CONST Tcl_Obj* optvalue,
					       ClientData clientData));
#endif
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
#if (TCL_MAJOR_VERSION < 8)
      SetOption,
      NULL,      /* no object procedure */
#else
      NULL,      /* no string procedure */
      SetOption,
#endif
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
  o->keyDataIsChan  = 0;
  o->keyData        = NULL;
  o->ivDataIsChan   = 0;
  o->ivData         = NULL;

  /* ---- derived information ---- */

  o->key_length     = -1;
  o->key            = NULL;
  o->iv             = NULL;
  o->shift_width    = -1;

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

  if (o->keyData != NULL) {
#if (TCL_MAJOR_VERSION < 8)
    Tcl_Free (o->keyData);
#else
    Tcl_DecrRefCount(o->keyData);
#endif
  }

  if (o->ivData != NULL) {
#if (TCL_MAJOR_VERSION < 8)
    Tcl_Free (o->ivData);
#else
    Tcl_DecrRefCount(o->ivData);
#endif
  }

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
  int res;

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

  if (o->keyData == NULL) {
    Tcl_AppendResult (interp, "key not specified", (char*) NULL);
    return TCL_ERROR;
  }

  if (o->operation_mode >= TRF_CBC_MODE) {
    if (o->ivData == NULL) {
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

  /*
   * Interpret '-key' in dependance of '-key-type'.
   */

  res = TrfGetData (interp, "key", o->keyDataIsChan, o->keyData,
		     bc_desc->min_keysize, bc_desc->max_keysize,
		     &o->key, &o->key_length);

  if (res != TCL_OK)
    return res;

  if (o->operation_mode >= TRF_CBC_MODE) {
    /*
     * Interpret '-iv' in dependance of '-iv-type'.
     */
    int dummy;

    return TrfGetData (interp, "iv", o->ivDataIsChan, o->ivData,
		       bc_desc->block_size, bc_desc->block_size,
		       &o->iv, &dummy);
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
   * -direction	encrypt/decrypt
   * -mode	ecb/cbc/cfb/ofb
   * -shift	<number>
   *
   * -key-type  data|channel
   * -key	<value>, either channel handle or immediate
   *                     information (dependent on value of
   *                     -key-type).
   *
   * -iv-type	data|channel
   * -iv	<value>, either channel handle or immediate
   *                     information (dependent on value of
   *                     -iv-type).
   */

  TrfBlockcipherOptionBlock*  o       = (TrfBlockcipherOptionBlock*) options;
  Trf_BlockcipherDescription* bc_desc = (Trf_BlockcipherDescription*) clientData;
  int                         len = strlen (optname + 1);
  CONST char*                 value;

#if (TCL_MAJOR_VERSION < 8)
  value = optvalue;
#else
  value = Tcl_GetStringFromObj ((Tcl_Obj*) optvalue, NULL);
#endif

  switch (optname [1]) {
  case 'd':
    if (0 == strncmp (optname, "-direction", len)) {
      len = strlen (value);

      switch (value [0]) {
      case 'e':
	if (0 == strncmp ("encrypt", value, len)) {
	  o->direction = TRF_ENCRYPT;
	} else
	  goto unknown_direction;
	break;

      case 'd':
	if (0 == strncmp ("decrypt", value, len)) {
	  o->direction = TRF_DECRYPT;
	} else
	  goto unknown_direction;
	break;

      default:
      unknown_direction:
	Tcl_AppendResult (interp, "unknown direction \"", (char*) NULL);
	Tcl_AppendResult (interp, value, (char*) NULL);
	Tcl_AppendResult (interp, "\"", (char*) NULL);
	return TCL_ERROR;
      }
    } else
      goto unknown_option;
    break;

  case 'm':
    if (0 == strncmp (optname, "-mode", len)) {
      len = strlen (value);

      switch (value [0]) {
      case 'e':
	if (0 == strncmp ("ecb", value, len)) {
	  o->operation_mode = TRF_ECB_MODE;
	} else
	  goto unknown_mode;
	break;

      case 'c':
	if (len < 2)
	  goto unknown_mode;
	else if   (0 == strncmp ("cbc", value, len)) {
	  o->operation_mode = TRF_CBC_MODE;
	} else if (0 == strncmp ("cfb", value, len)) {
	  o->operation_mode = TRF_CFB_MODE;
	} else
	  goto unknown_mode;
	break;

      case 'o':
	if (0 == strncmp ("ofb", value, len)) {
	  o->operation_mode = TRF_OFB_MODE;
	} else
	  goto unknown_mode;
	break;

      default:
      unknown_mode:
	Tcl_AppendResult (interp, "unknown mode \"", (char*) NULL);
	Tcl_AppendResult (interp, value, (char*) NULL);
	Tcl_AppendResult (interp, "\"", (char*) NULL);
	return TCL_ERROR;
      }
    } else
      goto unknown_option;
    break;

  case 'i':
    if (0 == strncmp (optname, "-iv", len)) {
      /*
       * Save information, interpret later (CheckOption)
       */
#if (TCL_MAJOR_VERSION < 8)
      o->ivData = strcpy (Tcl_Alloc (1 + strlen (optvalue)), optvalue);
#else
      o->ivData = (Tcl_Obj*) optvalue;
      Tcl_IncrRefCount (o->ivData);
#endif
    } else if (0 == strncmp (optname, "-iv-type", len)) {
      return TrfGetDataType (interp, "iv", value, &o->ivDataIsChan);
    } else
      goto unknown_option;
    break;

  case 'k':
    if (0 == strncmp (optname, "-key", len)) {
      /*
       * Save information, interpret later (CheckOption)
       */
#if (TCL_MAJOR_VERSION < 8)
      o->keyData = strcpy (Tcl_Alloc (1 + strlen (optvalue)), optvalue);
#else
      o->keyData = (Tcl_Obj*) optvalue;
      Tcl_IncrRefCount (o->keyData);
#endif
    } else if (0 == strncmp (optname, "-key-type", len)) {
      return TrfGetDataType (interp, "key", value, &o->keyDataIsChan);
    } else
      goto unknown_option;
    break;

  case 's':
    if (0 == strncmp (optname, "-shift", len)) {
      int res;

#if (TCL_MAJOR_VERSION < 8)
      res = Tcl_GetInt (interp, (char*) optvalue, &o->shift_width);
#else
      int sw;
      res = Tcl_GetIntFromObj (interp, (Tcl_Obj*) optvalue, &sw);
      o->shift_width = sw;
#endif

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
  Tcl_AppendResult (interp, "unknown option '", (char*) NULL);
  Tcl_AppendResult (interp, optname, (char*) NULL);
  Tcl_AppendResult (interp, "'", (char*) NULL);
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
