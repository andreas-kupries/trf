/*
 * c_util.c --
 *
 *	Implements helper procedures used by (block)ciphers.
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

#define min(a,b) ((a) < (b) ? (a) : (b))

#if (TCL_MAJOR_VERSION < 8)
static int
TrfGetImmediateData _ANSI_ARGS_ ((Tcl_Interp* interp, CONST char* dataName,
				  CONST char* data, unsigned short min_bytes,
				  unsigned short max_bytes, VOID** buf, int* length));
#else
static int
TrfGetImmediateData _ANSI_ARGS_ ((Tcl_Interp* interp, CONST char* dataName,
				  CONST Tcl_Obj* data, unsigned short min_bytes,
				  unsigned short max_bytes, VOID** buf, int* length));
#endif

#if (TCL_MAJOR_VERSION < 8)
static int
TrfGetChannelData _ANSI_ARGS_ ((Tcl_Interp* interp, CONST char* dataName,
				CONST char* data, unsigned short min_bytes,
				unsigned short max_bytes, VOID** buf, int* length));
#else
static int
TrfGetChannelData _ANSI_ARGS_ ((Tcl_Interp* interp, CONST char* dataName,
				CONST Tcl_Obj* data, unsigned short min_bytes,
				unsigned short max_bytes, VOID** buf, int* length));
#endif


/*
 *------------------------------------------------------*
 *
 *	TrfGetImmediateData --
 *
 *	------------------------------------------------*
 *	Extracts immediately binary information, either
 *	from a string or a Tcl_Obj, dependent on the Tcl
 *	version.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Allocates memory
 *
 *	Result:
 *		A standard TCL error code.
 *
 *------------------------------------------------------*
 */
#if (TCL_MAJOR_VERSION < 8)
static int
TrfGetImmediateData (interp, dataName, data, min_bytes, max_bytes, buf, length)
Tcl_Interp*     interp;
CONST char*     dataName;
CONST char*     data;
unsigned short  min_bytes;
unsigned short  max_bytes;
VOID**          buf;
int*            length;
{
  /* This truncates after the first embedded \0 */
  *length = strlen (data);

  if (*length < min_bytes) {
    Tcl_AppendResult (interp, dataName, " to short", (char*) NULL);
    return TCL_ERROR;
  }

  if (max_bytes != TRF_KEYSIZE_INFINITY) {
    *length = min (*length, max_bytes);
  }

  *buf = Tcl_Alloc (*length);
  memcpy (*buf, data, *length);

  return TCL_OK;
}
#else
static int
TrfGetImmediateData (interp, dataName, data, min_bytes, max_bytes, buf, length)
Tcl_Interp*     interp;
CONST char*     dataName;
CONST Tcl_Obj*  data;
unsigned short  min_bytes;
unsigned short  max_bytes;
VOID**          buf;
int*            length;
{
  unsigned char* tmp = Tcl_GetStringFromObj ((Tcl_Obj*) data, length);

  if (*length < min_bytes) {
    Tcl_AppendResult (interp, dataName, " to short", (char*) NULL);
    return TCL_ERROR;
  }

  if (max_bytes != TRF_KEYSIZE_INFINITY) {
    *length = min (*length, max_bytes);
  }

  *buf = Tcl_Alloc (*length);
  memcpy (*buf, tmp, *length);

  return TCL_OK;
}
#endif

/*
 *------------------------------------------------------*
 *
 *	TrfGetChannelData --
 *
 *	------------------------------------------------*
 *	Extracts binary information stored in a channel.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Allocates memory.
 *
 *	Result:
 *		A standard TCCL error code.
 *
 *------------------------------------------------------*
 */
static int
TrfGetChannelData (interp, dataName, dataChan, min_bytes, max_bytes, buf, length)
Tcl_Interp*     interp;
CONST char*     dataName;
#if (TCL_MAJOR_VERSION < 8)
CONST char*     dataChan;
#else
CONST Tcl_Obj*  dataChan;
#endif
unsigned short  min_bytes;
unsigned short  max_bytes;
VOID**          buf;
int*            length;
{
  CONST char* chanHandle;
  int         access;
  Tcl_Channel key;
  int         read_bytes;


#if (TCL_MAJOR_VERSION < 8)
  chanHandle = dataChan;
#else
  chanHandle = Tcl_GetStringFromObj ((Tcl_Obj*) dataChan, NULL);
#endif

  key = Tcl_GetChannel (interp, (char*) chanHandle, &access);

  if (key == (Tcl_Channel) NULL)
    return TCL_ERROR;

  if (! (access & TCL_READABLE)) {
    Tcl_AppendResult (interp, dataName, " channel \"",     (char*) NULL);
    Tcl_AppendResult (interp, chanHandle,                  (char*) NULL);
    Tcl_AppendResult (interp, "\" not opened for reading", (char*) NULL);
    return TCL_ERROR;
  }


  if (max_bytes != TRF_KEYSIZE_INFINITY) {
    /* We have an upper bound set upon the keysize. Preallocate this much
     * memory, then read as much as possible from the channel.
     */

    *buf       = Tcl_Alloc (max_bytes);
    *length    = Tcl_Read  (key, *buf, max_bytes);

    if (*length < 0) {
      Tcl_Free (*buf);
      *buf = NULL;

      Tcl_AppendResult (interp, "error reading ", dataName, " from channel \"", (char*) NULL);
      Tcl_AppendResult (interp, chanHandle,                                     (char*) NULL);
      Tcl_AppendResult (interp, "\": ",                                         (char*) NULL);
      Tcl_AppendResult (interp, Tcl_PosixError (interp),                        (char*) NULL);
      return TCL_ERROR;
    }

    if (*length < min_bytes) {
      Tcl_Free (*buf);
      *buf    = NULL;
      *length = 0;
      Tcl_AppendResult (interp, dataName, " to short", (char*) NULL);
      return TCL_ERROR;
    }
  } else {
    /* No upper bound, read everything available.
     */

    Tcl_DString    ds;
    int            read_now;
    unsigned char* tmp;

#ifndef READ_CHUNK_SIZE
#define READ_CHUNK_SIZE 4096
#endif

    Tcl_DStringInit (&ds);
    tmp        = Tcl_Alloc (READ_CHUNK_SIZE);
    read_bytes = 0;

    while (1) {
      read_now = Tcl_Read (key, tmp, READ_CHUNK_SIZE);

      if (read_now < 0) {
	Tcl_Free (tmp);
	Tcl_AppendResult (interp, "error reading ", dataName ," from channel \"", (char*) NULL);
	Tcl_AppendResult (interp, chanHandle,                                     (char*) NULL);
	Tcl_AppendResult (interp, "\": ",                                         (char*) NULL);
	Tcl_AppendResult (interp, Tcl_PosixError (interp),                        (char*) NULL);
	return TCL_ERROR;
      }

      if (read_now > 0) {
	Tcl_DStringAppend (&ds, tmp, read_now);
      }

      /* jump out of the loop upon a short read, either EOF or BLOCKED */
      if (read_now < READ_CHUNK_SIZE) break;
    }

    Tcl_Free (tmp);

    if (ds.length < min_bytes) {
      Tcl_AppendResult (interp, dataName, " to short", (char*) NULL);
      return TCL_ERROR;
    }

    read_bytes = ds.length;

    *length = read_bytes;
    *buf    = Tcl_Alloc (read_bytes);

    strncpy (*buf, ds.string, read_bytes);

    Tcl_DStringFree (&ds);
  }

  return TCL_OK;
}

/*
 *------------------------------------------------------*
 *
 *	TrfGetData --
 *
 *	------------------------------------------------*
 *	Extracts binary information, either immediately
 *	specified, or stored in a channel.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of 'TrfGetImmediateData' or 'TrfGetChannelData'.
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

EXTERN int
TrfGetData (interp, dataName, dataIsChannel, data, min_bytes, max_bytes, buf, length)
Tcl_Interp*     interp;
CONST char*     dataName;
int             dataIsChannel;
#if (TCL_MAJOR_VERSION < 8)
CONST char*     data;
#else
CONST Tcl_Obj*  data;
#endif
unsigned short  min_bytes;
unsigned short  max_bytes;
VOID**          buf;
int*            length;
{
  if (dataIsChannel)
    return TrfGetChannelData   (interp, dataName, data, min_bytes, max_bytes,
				buf, length);
  else
    return TrfGetImmediateData (interp, dataName, data, min_bytes, max_bytes,
				buf, length);
}

/*
 *------------------------------------------------------*
 *
 *	Trf_GetDataType --
 *
 *	------------------------------------------------*
 *	Determines from a string what type of data was
 *	given to the cipher.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		May leave an error message in the
 *		interpreter result area.
 *
 *	Result:
 *		A standard Tcl error code, in case of
 *		success 'isChannel' is set too.
 *
 *------------------------------------------------------*
 */

EXTERN int
TrfGetDataType (interp, dataName, typeString, isChannel)
Tcl_Interp* interp;
CONST char* dataName;
CONST char* typeString;
int*        isChannel;
{
  int len = strlen (typeString);

  switch (typeString [0]) {
  case 'd':
    if (0 == strncmp ("data", typeString, len)) {
      *isChannel = 0;
    } else
      goto unknown_type;
    break;

  case 'c':
    if (0 == strncmp ("channel", typeString, len)) {
      *isChannel = 1;
    } else
      goto unknown_type;
    break;

  default:
  unknown_type:
    Tcl_AppendResult (interp, "unknown ", dataName,
		      "-type \"", typeString, "\"",
		      (char*) NULL);
    return TCL_ERROR;
  }

  return TCL_OK;
}
