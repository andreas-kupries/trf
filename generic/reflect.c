/*
 * reflect.c --
 *
 *	Implements and registers conversion channel relying on
 *	tcl-scripts to do the conversion. In other words: The
 *	transformation functionality is reflected up into the
 *	tcl-level. In case of binary data this will be usable
 *	only with tcl 8.0 and up.
 *
 *
 * Copyright (c) 1995 Andreas Kupries (a.kupries@westend.com)
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
 * Converter description
 * ---------------------
 */


/*
 * Declarations of internal procedures.
 */

static Trf_ControlBlock CreateEncoder  _ANSI_ARGS_ ((ClientData writeClientData,
						     Trf_WriteProc fun,
						     Trf_Options optInfo,
						     Tcl_Interp*   interp,
						     ClientData clientData));
static void             DeleteEncoder  _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     ClientData clientData));
/*static int              Encode         _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     int character,
						     Tcl_Interp* interp,
						     ClientData clientData));*/
static int              EncodeBuffer   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     unsigned char*   buffer,
						     int              bufLen,
						     Tcl_Interp*      interp,
						     ClientData       clientData));
static int              FlushEncoder   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     Tcl_Interp* interp,
						     ClientData clientData));
static void             ClearEncoder   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     ClientData clientData));

static Trf_ControlBlock CreateDecoder  _ANSI_ARGS_ ((ClientData writeClientData,
						     Trf_WriteProc fun,
						     Trf_Options optInfo,
						     Tcl_Interp*   interp,
						     ClientData clientData));
static void             DeleteDecoder  _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     ClientData clientData));
/*static int              Decode         _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     int character,
						     Tcl_Interp* interp,
						     ClientData clientData));*/
static int              DecodeBuffer   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     unsigned char* buffer,
						     int bufLen,
						     Tcl_Interp* interp,
						     ClientData clientData));
static int              FlushDecoder   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     Tcl_Interp* interp,
						     ClientData clientData));
static void             ClearDecoder   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     ClientData clientData));

/*#define CreateDecoder CreateEncoder*/
/*#define DeleteDecoder DeleteEncoder*/

/*
 * Converter definition.
 */

static Trf_TypeDefinition reflectDefinition =
{
  "transform",
  NULL, /* filled later (TrfInit_XXX) */
  NULL, /* filled later (TrfInit_XXX) */
  {
    CreateEncoder,
    DeleteEncoder,
    NULL,
    EncodeBuffer,
    FlushEncoder,
    ClearEncoder
  }, {
    CreateDecoder,
    DeleteDecoder,
    NULL,
    DecodeBuffer,
    FlushDecoder,
    ClearDecoder
  }
};

/*
 * Definition of the control blocks for en- and decoder.
 */

typedef struct _EncoderControl_ {
  Trf_WriteProc* write;
  ClientData     writeClientData;

#if (TCL_MAJOR_VERSION >= 8)
  Tcl_Obj*       command; /* tcl code to execute for a buffer */
#else
  unsigned char* command; /* tcl code to execute for a buffer */
#endif

  Tcl_Interp* interp; /* interpreter creating the channel */

} EncoderControl;


typedef EncoderControl DecoderControl;

/*
typedef struct _DecoderControl_ {
  Trf_WriteProc* write;
  ClientData     writeClientData;

#if (TCL_MAJOR_VERSION >= 8)
  Tcl_Obj*       command; / * tcl code to execute for a buffer * /
#else
  unsigned char* command; / * tcl code to execute for a buffer * /
#endif

} DecoderControl;
*/


/*
 * Execute callback for buffer and operation.
 */
static int Execute _ANSI_ARGS_ ((EncoderControl* ctrl, Tcl_Interp* interp, unsigned char* op,
				 unsigned char* buf, int bufLen, int transmit));



/*
 *------------------------------------------------------*
 *
 *	TrfInit_Transform --
 *
 *	------------------------------------------------*
 *	Register the conversion implemented in this file.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of 'Trf_Register'.
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

int
TrfInit_Transform (interp)
Tcl_Interp* interp;
{
  reflectDefinition.options = TrfTransformOptions ();

  return Trf_Register (interp, &reflectDefinition);
}

/*
 *------------------------------------------------------*
 *
 *	CreateEncoder --
 *
 *	------------------------------------------------*
 *	Allocate and initialize the control block of a
 *	data encoder.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Allocates memory.
 *
 *	Result:
 *		An opaque reference to the control block.
 *
 *------------------------------------------------------*
 */

static Trf_ControlBlock
CreateEncoder (writeClientData, fun, optInfo, interp, clientData)
ClientData    writeClientData;
Trf_WriteProc fun;
Trf_Options   optInfo;
Tcl_Interp*   interp;
ClientData clientData;
{
  EncoderControl*          c;
  TrfTransformOptionBlock* o = (TrfTransformOptionBlock*) optInfo;

  c = (EncoderControl*) Tcl_Alloc (sizeof (EncoderControl));
  c->write           = fun;
  c->writeClientData = writeClientData;
  c->interp          = interp;

#if (TCL_MAJOR_VERSION >= 8)
    /* Store reference, tell the interpreter about it. */
    c->command      = o->command;
    Tcl_IncrRefCount (c->command);
#else
    /* Generate local copy of command string */
    c->command = strcpy (Tcl_Alloc (1+strlen (o->command)), o->command);
#endif

    if (TCL_OK != Execute (c, interp, "create/write", NULL, 0, 0)) {
#if (TCL_MAJOR_VERSION >= 8)
      Tcl_DecrRefCount (c->command);
#else
      Tcl_Free ((VOID*) c->command);
#endif

      Tcl_Free ((VOID*) c);
      return (ClientData) NULL;
    }

  return (ClientData) c;
}

/*
 *------------------------------------------------------*
 *
 *	DeleteEncoder --
 *
 *	------------------------------------------------*
 *	Destroy the control block of an encoder.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Releases the memory allocated by 'CreateEncoder'
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
DeleteEncoder (ctrlBlock, clientData)
Trf_ControlBlock ctrlBlock;
ClientData clientData;
{
  EncoderControl* c = (EncoderControl*) ctrlBlock;

  Execute (c, NULL, "delete/write", NULL, 0, 0);

#if (TCL_MAJOR_VERSION >= 8)
  Tcl_DecrRefCount (c->command);
#else
  Tcl_Free ((VOID*) c->command);
#endif

  Tcl_Free ((VOID*) c);
}

/*
 *------------------------------------------------------*
 *
 *	EncodeBuffer --
 *
 *	------------------------------------------------*
 *	Encode the given buffer and write the result.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of the called WriteFun.
 *
 *	Result:
 *		Generated bytes implicitly via WriteFun.
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

static int
EncodeBuffer (ctrlBlock, buffer, bufLen, interp, clientData)
Trf_ControlBlock ctrlBlock;
unsigned char* buffer;
int bufLen;
Tcl_Interp* interp;
ClientData clientData;
{
  EncoderControl* c = (EncoderControl*) ctrlBlock;

  return Execute (c, interp, "write", buffer, bufLen, 1);
}

/*
 *------------------------------------------------------*
 *
 *	FlushEncoder --
 *
 *	------------------------------------------------*
 *	Encode an incomplete character sequence (if possible).
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of the called WriteFun.
 *
 *	Result:
 *		Generated bytes implicitly via WriteFun.
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

static int
FlushEncoder (ctrlBlock, interp, clientData)
Trf_ControlBlock ctrlBlock;
Tcl_Interp* interp;
ClientData clientData;
{
  EncoderControl* c = (EncoderControl*) ctrlBlock;

  return Execute (c, interp, "flush/write", NULL, 0, 1);
}

/*
 *------------------------------------------------------*
 *
 *	ClearEncoder --
 *
 *	------------------------------------------------*
 *	Discard an incomplete character sequence.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		See above.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
ClearEncoder (ctrlBlock, clientData)
Trf_ControlBlock ctrlBlock;
ClientData clientData;
{
  EncoderControl* c = (EncoderControl*) ctrlBlock;

  Execute (c, (Tcl_Interp*) NULL, "clear_write", NULL, 0, 0);
}

/*
 *------------------------------------------------------*
 *
 *	CreateDecoder --
 *
 *	------------------------------------------------*
 *	Allocate and initialize the control block of a
 *	data decoder.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Allocates memory.
 *
 *	Result:
 *		An opaque reference to the control block.
 *
 *------------------------------------------------------*
 */

static Trf_ControlBlock
CreateDecoder (writeClientData, fun, optInfo, interp, clientData)
ClientData    writeClientData;
Trf_WriteProc fun;
Trf_Options   optInfo;
Tcl_Interp*   interp;
ClientData clientData;
{
  DecoderControl*          c;
  TrfTransformOptionBlock* o = (TrfTransformOptionBlock*) optInfo;

  c = (DecoderControl*) Tcl_Alloc (sizeof (DecoderControl));
  c->write           = fun;
  c->writeClientData = writeClientData;
  c->interp          = interp;

#if (TCL_MAJOR_VERSION >= 8)
    /* Store reference, tell the interpreter about it. */
    c->command      = o->command;
    Tcl_IncrRefCount (c->command);
#else
    /* Generate local copy of command string */
    c->command = strcpy (Tcl_Alloc (1+strlen (o->command)), o->command);
#endif

    if (TCL_OK != Execute (c, interp, "create/read", NULL, 0, 0)) {
#if (TCL_MAJOR_VERSION >= 8)
      Tcl_DecrRefCount (c->command);
#else
      Tcl_Free ((VOID*) c->command);
#endif

      Tcl_Free ((VOID*) c);
      return (ClientData) NULL;
    }

  return (ClientData) c;
}

/*
 *------------------------------------------------------*
 *
 *	DeleteDecoder --
 *
 *	------------------------------------------------*
 *	Destroy the control block of an decoder.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Releases the memory allocated by 'CreateDecoder'
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
DeleteDecoder (ctrlBlock, clientData)
Trf_ControlBlock ctrlBlock;
ClientData clientData;
{
  DecoderControl* c = (DecoderControl*) ctrlBlock;

  Execute (c, NULL, "delete/read", NULL, 0, 0);

#if (TCL_MAJOR_VERSION >= 8)
  Tcl_DecrRefCount (c->command);
#else
  Tcl_Free ((VOID*) c->command);
#endif

  Tcl_Free ((VOID*) c);
}

/*
 *------------------------------------------------------*
 *
 *	DecodeBuffer --
 *
 *	------------------------------------------------*
 *	Decode the given buffer and write the result.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of the called WriteFun.
 *
 *	Result:
 *		Generated bytes implicitly via WriteFun.
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

static int
DecodeBuffer (ctrlBlock, buffer, bufLen, interp, clientData)
Trf_ControlBlock ctrlBlock;
unsigned char* buffer;
int bufLen;
Tcl_Interp* interp;
ClientData clientData;
{
  DecoderControl* c = (DecoderControl*) ctrlBlock;

  return Execute (c, interp, "read", buffer, bufLen, 1);
}

/*
 *------------------------------------------------------*
 *
 *	FlushDecoder --
 *
 *	------------------------------------------------*
 *	Decode an incomplete character sequence (if possible).
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of the called WriteFun.
 *
 *	Result:
 *		Generated bytes implicitly via WriteFun.
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

static int
FlushDecoder (ctrlBlock, interp, clientData)
Trf_ControlBlock ctrlBlock;
Tcl_Interp* interp;
ClientData clientData;
{
  DecoderControl* c = (DecoderControl*) ctrlBlock;

  return Execute (c, interp, "flush/read", NULL, 0, 1);
}

/*
 *------------------------------------------------------*
 *
 *	ClearDecoder --
 *
 *	------------------------------------------------*
 *	Discard an incomplete character sequence.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		See above.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
ClearDecoder (ctrlBlock, clientData)
Trf_ControlBlock ctrlBlock;
ClientData clientData;
{
  DecoderControl* c = (DecoderControl*) ctrlBlock;

  Execute (c, (Tcl_Interp*) NULL, "clear_read", NULL, 0, 0);
}

/*
 *------------------------------------------------------*
 *
 *	Execute --
 *
 *	------------------------------------------------*
 *	Execute callback for buffer and operation.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Everything possible, depending on the
 *		script executed.
 *
 *	Result:
 *		A standard TCL error code. In case of an
 *		error a message is left in the result area
 *		of the specified interpreter.
 *
 *------------------------------------------------------*
 */

static int
Execute (c, interp, op, buf, bufLen, transmit)
EncoderControl* c;
Tcl_Interp*     interp;
unsigned char*  op;
unsigned char*  buf;
int             bufLen;
int             transmit;
{
  /* Generate the real command from -command by appending name of
   * operation and buffer to operate upon, evaluate it at the global
   * level, then use its result as data to write (in case of transmit != 0).
   */

  int res;

#if (TCL_MAJOR_VERSION >= 8)
  Tcl_Obj* command;

  command = Tcl_DuplicateObj (c->command);

  if (command == (Tcl_Obj*) NULL) {
    return TCL_ERROR;
  }

  res = Tcl_ListObjAppendElement (interp, command, Tcl_NewStringObj (op, strlen (op)));
  if (res != TCL_OK)
    goto cleanup;

  res = Tcl_ListObjAppendElement (interp, command, Tcl_NewStringObj (buf, bufLen));
  if (res != TCL_OK)
    goto cleanup;

  res = Tcl_GlobalEvalObj (c->interp, command);

  Tcl_DecrRefCount (command);

  if (res != TCL_OK) {
    /* copy error message from 'c->interp' to actual 'interp'. */

    if ((interp != (Tcl_Interp*) NULL) && (c->interp != interp)) {
      Tcl_SetObjResult (interp, Tcl_GetObjResult (c->interp));
    }

    return res;
  }

  if (transmit) {
    /* Caller said to expect data in interpreter result area.
     * Take it, then write it out to the channel system.
     */

    unsigned char* newbuf;
    int newbufLen;

    newbuf = (unsigned char*) Tcl_GetStringFromObj (Tcl_GetObjResult (c->interp), &newbufLen);

    res = c->write (c->writeClientData, newbuf, newbufLen, interp);
    Tcl_ResetResult (c->interp);
  }

  return res;

cleanup:
  Tcl_DecrRefCount (command);
  return res;

#else

  Tcl_DString command;

  Tcl_DStringInit          (&command);
  Tcl_DStringAppend        (&command, c->command, -1);
  Tcl_DStringAppend        (&command, " ", -1);
  Tcl_DStringAppend        (&command, op,  -1);
  Tcl_DStringAppend        (&command, " ", -1);
  Tcl_DStringAppendElement (&command, buf);
  Tcl_DStringAppend        (&command, "",   1); /* terminate buffer for sure */

  res = Tcl_GlobalEval (c->interp, command.string);

  Tcl_DStringFree (&command);

  if (res != TCL_OK) {
    /* copy error message from 'c->interp' to actual 'interp'. */

    if ((interp != (Tcl_Interp*) NULL) && (c->interp != interp)) {
      Tcl_SetResult (interp, c->interp->result, TCL_VOLATILE);
    }

    return res;
  }

  if (transmit) {
    /* Caller said to expect data in interpreter result area.
     * Take it, then write it out to the channel system.
     */

    res = c->write (c->writeClientData, c->interp->result,
		    strlen (c->interp->result), interp);
    Tcl_ResetResult (c->interp);
  }

  return TCL_OK;
#endif
}
