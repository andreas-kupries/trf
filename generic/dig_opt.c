/*
 * dig_opt.c --
 *
 *	Implements the C level procedures handling option processing
 *	for message digest generators.
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
 *	TrfMDOptions --
 *
 *	------------------------------------------------*
 *	Accessor to the set of vectors realizing option
 *	processing for message digest generators.
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
TrfMDOptions ()
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
 *	Create option structure for message digest generators.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Allocates memory and initializes it as
 *		option structure for message digest generators.
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
  TrfMDOptionBlock* o;

  o            = (TrfMDOptionBlock*) Tcl_Alloc (sizeof (TrfMDOptionBlock));
  o->behaviour = TRF_IMMEDIATE; /* irrelevant until set by 'CheckOptions' */
  o->mode      = TRF_UNKNOWN_MODE;
  o->readDest  = (Tcl_Channel) NULL;
  o->writeDest = (Tcl_Channel) NULL;
  o->matchFlag = (char*) NULL;
  o->mfInterp  = (Tcl_Interp*) NULL;

  return (Trf_Options) o;
}

/*
 *------------------------------------------------------*
 *
 *	DeleteOptions --
 *
 *	------------------------------------------------*
 *	Delete option structure of a message digest generators.
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
  TrfMDOptionBlock* o = (TrfMDOptionBlock*) options;

  if (o->matchFlag) {
    Tcl_Free ((char*) o->matchFlag);
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
  TrfMDOptionBlock*                   o = (TrfMDOptionBlock*) options;
  Trf_MessageDigestDescription* md_desc = (Trf_MessageDigestDescription*) clientData;

  /*
   * Call digest dependent check of environment first.
   */

  if (md_desc->checkProc != NULL) {
    if (TCL_OK != (*md_desc->checkProc) (interp)) {
      return TCL_ERROR;
    }
  }

  /* TRF_IMMEDIATE: no options allowed
   * TRF_ATTACH:    -mode required
   *                TRF_ABSORB_HASH: -matchflag required (only if channel is read)
   *                TRF_WRITE_HASH:  -write/read-dest required according to access
   *                                  mode of attched channel. specified channels
   *                                  must be writable (already checked by 'SetOption')
   */

  if (baseOptions->attach == (Tcl_Channel) NULL) /* IMMEDIATE */ {
    if ((o->mode      != TRF_UNKNOWN_MODE)   ||
	(o->matchFlag != (char*) NULL)       ||
	(o->readDest  != (Tcl_Channel) NULL) ||
	(o->writeDest != (Tcl_Channel) NULL)) {
      Tcl_AppendResult (interp, "immediate: no options allowed",
			(char*) NULL);
      return TCL_ERROR;
    }
  } else /* ATTACH */ {
    if (o->mode == TRF_UNKNOWN_MODE) {
      Tcl_AppendResult (interp, "attach: -mode not defined",
			(char*) NULL);
      return TCL_ERROR;
    } else if (o->mode == TRF_ABSORB_HASH) {
      if ((baseOptions->attach_mode & TCL_READABLE) &&
	  (o->matchFlag == (char*) NULL)) {
	Tcl_AppendResult (interp, "attach: -matchflag not defined",
			  (char*) NULL);
	return TCL_ERROR;
      }
    } else if (o->mode == TRF_WRITE_HASH) {
      if (o->matchFlag != (char*) NULL) {
	Tcl_AppendResult (interp, "attach, external: -matchflag not allowed",
			  (char*) NULL);
	return TCL_ERROR;
      }

      if ((baseOptions->attach_mode & TCL_READABLE) &&
	  (o->readDest == (Tcl_Channel) NULL)) {
	Tcl_AppendResult (interp, "attach, external: -read-dest missing",
			  (char*) NULL);
	return TCL_ERROR;
      }

      if ((baseOptions->attach_mode & TCL_WRITABLE) &&
	  (o->writeDest == (Tcl_Channel) NULL)) {
	Tcl_AppendResult (interp, "attach, external: -write-dest missing",
			  (char*) NULL);
	return TCL_ERROR;
      }

    } else {
      panic ("unknown mode-code given to message-digest::CheckOptions");
    }
  }

  o->behaviour = (baseOptions->attach == (Tcl_Channel) NULL ?
		  TRF_IMMEDIATE :
		  TRF_ATTACH);

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
   *	-mode		absorb|write
   *	-matchflag	<varname>
   *	-write-dest	<channel>
   *	-read-dest	<channel>
   */

  TrfMDOptionBlock* o = (TrfMDOptionBlock*) options;

  int len = strlen (optname + 1);

  switch (optname [1]) {
  case 'm':
    if (len == 1) {
      goto unknown_option;
    } else if (0 == strncmp (optname, "-mode", len)) {
      len = strlen (optvalue);

      switch (optvalue [0]) {
      case 'a':
	if (0 == strncmp (optvalue, "absorb", len)) {
	  o->mode = TRF_ABSORB_HASH;
	} else {
	  goto unknown_mode;
	}
	break;

      case 'w':
	if (0 == strncmp (optvalue, "write", len)) {
	  o->mode = TRF_WRITE_HASH;
	} else {
	  goto unknown_mode;
	}
	break;

      default:
      unknown_mode:
	Tcl_AppendResult (interp, "unknown mode '", optvalue, "'", (char*) NULL);
	return TCL_ERROR;
      } /* switch (optvalue) */

    } else if (0 == strncmp (optname, "-matchflag", len)) {
      if (o->matchFlag)
	Tcl_Free (o->matchFlag);

      o->matchFlag = (char*) Tcl_Alloc (1 + strlen (optvalue));
      o->mfInterp  = interp;
      strcpy (o->matchFlag, optvalue);

    } else {
      goto unknown_option;
    }
    break;

  case 'w':
    if (0 == strncmp (optname, "-write-dest", len)) {
      int access;

      o->writeDest = Tcl_GetChannel (interp, (char*) optvalue, &access);
      if (o->writeDest == (Tcl_Channel) NULL)
	return TCL_ERROR;
      else if (! (access & TCL_WRITABLE)) {
	Tcl_AppendResult (interp, optvalue, " not opened for writing", (char*) NULL);
	return TCL_ERROR;
      }
    } else {
      goto unknown_option;
    }
    break;

  case 'r':
    if (0 == strncmp (optname, "-read-dest", len)) {
      int access;

      o->readDest = Tcl_GetChannel (interp, (char*) optvalue, &access);
      if (o->readDest == (Tcl_Channel) NULL)
	return TCL_ERROR;
      else if (! (access & TCL_WRITABLE)) {
	Tcl_AppendResult (interp, optvalue, " not opened for writing", (char*) NULL);
	return TCL_ERROR;
      }
    } else {
      goto unknown_option;
    }
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
  /* Always use encoder for immediate execution */
  return 1;
}

