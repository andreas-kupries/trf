/*
 * convert.c --
 *
 *	Implements the C level procedures handling option processing
 *	of transformation reflecting the work to the tcl level.
 *
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

/*
 * forward declarations of all internally used procedures.
 */

static Trf_Options CreateOptions _ANSI_ARGS_ ((ClientData clientData));
static void        DeleteOptions _ANSI_ARGS_ ((Trf_Options options, ClientData clientData));
static int         CheckOptions  _ANSI_ARGS_ ((Trf_Options options, Tcl_Interp* interp,
					       CONST Trf_BaseOptions* baseOptions,
					       ClientData clientData));
#if (TCL_MAJOR_VERSION >= 8)
static int         SetOption     _ANSI_ARGS_ ((Trf_Options options, Tcl_Interp* interp,
					       CONST char* optname, CONST Tcl_Obj* optvalue,
					       ClientData clientData));
#else
static int         SetOption     _ANSI_ARGS_ ((Trf_Options options, Tcl_Interp* interp,
					       CONST char* optname, CONST char* optvalue,
					       ClientData clientData));
#endif
static int         QueryOptions  _ANSI_ARGS_ ((Trf_Options options, ClientData clientData));


/*
 *------------------------------------------------------*
 *
 *	TrfTransformOptions --
 *
 *	------------------------------------------------*
 *	Accessor to the set of vectors realizing option
 *	processing for reflecting transformation.
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
TrfTransformOptions ()
{
  static Trf_OptionVectors optVec =
    {
      CreateOptions,
      DeleteOptions,
      CheckOptions,
#if (TCL_MAJOR_VERSION >= 8)
      NULL,      /* no string procedure */
      SetOption,
#else
      SetOption,
      NULL,      /* no object procedure */
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
 *	Create option structure for reflecting transformation.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Allocates memory and initializes it as
 *		option structure for reflecting
 *		transformations.
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
  TrfTransformOptionBlock* o;

  o = (TrfTransformOptionBlock*) Tcl_Alloc (sizeof (TrfTransformOptionBlock));
  o->mode = TRF_UNKNOWN_MODE;

#if (TCL_MAJOR_VERSION >= 8)
  o->command = (Tcl_Obj*) NULL;
#else
  o->command = (unsigned char*) NULL;
#endif

  return (Trf_Options) o;
}

/*
 *------------------------------------------------------*
 *
 *	DeleteOptions --
 *
 *	------------------------------------------------*
 *	Delete option structure of a reflecting transformation
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
ClientData  clientData;
{
  TrfTransformOptionBlock* o = (TrfTransformOptionBlock*) options;

  if (o->command != NULL) {
#if (TCL_MAJOR_VERSION >= 8)
    Tcl_DecrRefCount (o->command);
#else
    Tcl_Free ((VOID*) o->command);
#endif
  }

  Tcl_Free ((VOID*) o);
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
  TrfTransformOptionBlock* o = (TrfTransformOptionBlock*) options;

  if (o->command == NULL) {
    Tcl_AppendResult (interp, "command not specified", (char*) NULL);
    return TCL_ERROR;
  }

#if (TCL_MAJOR_VERSION >= 8)
  if ((o->command->bytes == 0) && (o->command->typePtr == NULL)) {
    /* object defined, but empty, reject this too */
    Tcl_AppendResult (interp, "command specified, but empty", (char*) NULL);
    return TCL_ERROR;
  }
#endif

  if (baseOptions->attach == (Tcl_Channel) NULL) /* IMMEDIATE? */ {
    if (o->mode == TRF_UNKNOWN_MODE) {
      Tcl_AppendResult (interp, "mode not defined", (char*) NULL);
      return TCL_ERROR;
    }
  } else /* ATTACH */ {
    if (o->mode != TRF_UNKNOWN_MODE) {
      /* operation mode irrelevant for attached transformation,
       * and specification therefore ruled as illegal.
       */
      Tcl_AppendResult (interp, "mode illegal for attached transformation", (char*) NULL);
      return TCL_ERROR;
    }
    o->mode = TRF_WRITE_MODE;
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
#if (TCL_MAJOR_VERSION >= 8)
CONST Tcl_Obj* optvalue;
#else
CONST char*    optvalue;
#endif
ClientData  clientData;
{
  TrfTransformOptionBlock* o = (TrfTransformOptionBlock*) options;
  int                     len;
  CONST char*             value;

  len = strlen (optname+1);

  switch (optname [1]) {
  case 'm':
    if (0 != strncmp (optname, "-mode", len))
      goto unknown_option;

#if (TCL_MAJOR_VERSION >= 8)
    value = Tcl_GetStringFromObj ((Tcl_Obj*) optvalue, NULL);
#else
    value = optvalue;
#endif
    len = strlen (value);

    switch (value [0]) {
    case 'r':
      if (0 != strncmp (value, "read", len))
	goto unknown_mode;
      
      o->mode = TRF_READ_MODE;
      break;

    case 'w':
      if (0 != strncmp (value, "write", len))
	goto unknown_mode;
      
      o->mode = TRF_WRITE_MODE;
      break;

    default:
    unknown_mode:
      Tcl_AppendResult (interp, "unknown mode '", (char*) NULL);
      Tcl_AppendResult (interp, value, (char*) NULL);
      Tcl_AppendResult (interp, "'", (char*) NULL);
      return TCL_ERROR;
      break;
    } /* switch optvalue */
    break;

  case 'c':
    if (0 != strncmp (optname, "-command", len))
      goto unknown_option;

    /* 'optvalue' contains the command to execute for a buffer */

#if (TCL_MAJOR_VERSION >= 8)
    /*
     * Store reference, tell the interpreter about it.
     * We have to unCONST it explicitly to allow modification
     * of its reference counter
     */
    o->command = (Tcl_Obj*) optvalue;
    Tcl_IncrRefCount (o->command);
#else
    /* Generate local copy of command string */
    o->command = strcpy (Tcl_Alloc (1+strlen (optvalue)), optvalue);
#endif
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
ClientData clientData;
{
  TrfTransformOptionBlock* o = (TrfTransformOptionBlock*) options;

  return (o->mode == TRF_WRITE_MODE ? 1 : 0);
}

