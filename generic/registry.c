/*
 * registry.c --
 *
 *	Implements the C level procedures handling the registry
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
 * Code used to associate the registry with an interpreter.
 */

#define ASSOC "binTrf"

/*
 * Structures used by an attached transformation procedure
 *
 * => Information stored for a single direction of the channel.
 * => Information stored for the complete channel.
 */

typedef struct _DirectionInfo_ {
  Trf_ControlBlock   control; /* control block of conversion */
  Trf_Vectors*       vectors; /* vectors used during the conversion */
} DirectionInfo;

typedef struct _TrfTransformationInstance_ {
  Tcl_Channel parent; /* The channel superceded by this one */

  /* Tcl_Transformation standard;   data required for all transformation instances */
  DirectionInfo      in;         /* information for conversion of read data */
  DirectionInfo      out;        /* information for conversion of written data */
  ClientData         clientData; /* copy from entry->trfType->clientData */

  /*
   * internal result buffer used during conversions of incoming data.
   * Used to store results waiting for retrieval too, i.e. state
   * information carried from call to call.
   */

  unsigned char* result;
  int   allocated;
  int   used;

} TrfTransformationInstance;

#define INCREMENT (512)
#define READ_CHUNK_SIZE 4096

/*
 * forward declarations of all internally used procedures.
 */

static void
TrfDeleteRegistry _ANSI_ARGS_ ((ClientData clientData, Tcl_Interp *interp));

#if (TCL_MAJOR_VERSION >= 8)
static int
TrfExecuteObjCmd _ANSI_ARGS_((ClientData clientData, Tcl_Interp* interp,
			      int objc, struct Tcl_Obj* objv []));
#else
static int
TrfExecuteCmd _ANSI_ARGS_((ClientData clientData, Tcl_Interp* interp,
			   int argc, char** argv));
#endif

static void
TrfDeleteCmd _ANSI_ARGS_((ClientData clientData));

static int
TrfClose _ANSI_ARGS_ ((ClientData instanceData, Tcl_Interp* interp));

static int
TrfInput _ANSI_ARGS_ ((ClientData instanceData,
		       char* buf, int toRead,
		       int*       errorCodePtr));

static int
TrfOutput _ANSI_ARGS_ ((ClientData instanceData,
			char*  buf, int toWrite,
			int*        errorCodePtr));

static int
TrfSeek _ANSI_ARGS_ ((ClientData instanceData, long offset,
		      int mode, int* errorCodePtr));
static void
TrfWatch _ANSI_ARGS_ ((ClientData instanceData, int mask));

static int
TrfReady _ANSI_ARGS_ ((ClientData instanceData, int mask));

static Tcl_File
TrfGetFile _ANSI_ARGS_ ((ClientData instanceData, int mask));

static int
TransformImmediate _ANSI_ARGS_ ((Tcl_Interp* interp, Trf_RegistryEntry* entry,
				 Tcl_Channel source, Tcl_Channel destination,
				 Trf_Options optInfo));

static int
AttachTransform _ANSI_ARGS_ ((Trf_RegistryEntry* entry,
			      Tcl_Channel        attach,
			      Trf_Options        optInfo,
			      Tcl_Interp*        interp));

static int
PutDestination _ANSI_ARGS_ ((ClientData clientData,
			     unsigned char* outString, int outLen,
			     Tcl_Interp* interp));

static int
PutTrans _ANSI_ARGS_ ((ClientData clientData,
		       unsigned char* outString, int outLen,
		       Tcl_Interp* interp));

/*
 *------------------------------------------------------*
 *
 *	TrfGetRegistry --
 *
 *	------------------------------------------------*
 *	Accessor to the interpreter associated registry
 *	of conversions.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Allocates and initializes the hashtable
 *		during the first call and associates it
 *		with the specified interpreter.
 *
 *	Result:
 *		The internal registry of conversions.
 *
 *------------------------------------------------------*
 */

EXTERN Tcl_HashTable*
TrfGetRegistry (interp)
Tcl_Interp* interp;
{
  Tcl_HashTable* hTablePtr;

  hTablePtr = TrfPeekForRegistry (interp);

  if (hTablePtr == (Tcl_HashTable*) NULL) {
    hTablePtr = (Tcl_HashTable*) Tcl_Alloc (sizeof (Tcl_HashTable));

    Tcl_InitHashTable (hTablePtr, TCL_STRING_KEYS);

    Tcl_SetAssocData (interp, ASSOC, TrfDeleteRegistry, (ClientData) hTablePtr);
  }

  return hTablePtr;
}

/*
 *------------------------------------------------------*
 *
 *	TrfPeekForRegistry --
 *
 *	------------------------------------------------*
 *	Accessor to the interpreter associated registry
 *	of conversions. Does not create the registry (in
 *	contrast to TrfGetRegistry).
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		None.
 *
 *	Result:
 *		The internal registry of conversions.
 *
 *------------------------------------------------------*
 */

EXTERN Tcl_HashTable*
TrfPeekForRegistry (interp)
Tcl_Interp* interp;
{
  Tcl_InterpDeleteProc* proc;

  proc = TrfDeleteRegistry;

 return (Tcl_HashTable*) Tcl_GetAssocData (interp, ASSOC, &proc);
}

/*
 *------------------------------------------------------*
 *
 *	Trf_Register --
 *
 *	------------------------------------------------*
 *	Announce a conversion to the registry associated
 *	with the specified interpreter.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		May create the registry. Allocates and
 *		initializes the structure describing
 *		the announced conversion
 *
 *	Result:
 *		A standard TCL error code.
 *
 *------------------------------------------------------*
 */

EXTERN int
Trf_Register (interp, type)
Tcl_Interp*               interp;
CONST Trf_TypeDefinition* type;
{
  Trf_RegistryEntry* entry;
  Tcl_HashTable*     hTablePtr;
  Tcl_HashEntry*     hPtr;
  int                new;

  hTablePtr = TrfGetRegistry (interp);

  /*
   * Already defined ?
   */

  hPtr = Tcl_FindHashEntry (hTablePtr, (char*) type->name);

  if (hPtr != (Tcl_HashEntry*) NULL) {
    return TCL_ERROR;
  }

  /*
   * Check validity of given structure
   */

#define IMPLY(a,b) ((! (a)) || (b))

  /* assert (type->options); */
  assert (IMPLY(type->options != NULL, type->options->createProc != NULL));
  assert (IMPLY(type->options != NULL, type->options->deleteProc != NULL));
  assert (IMPLY(type->options != NULL, type->options->checkProc  != NULL));
  assert (IMPLY(type->options != NULL,
		(type->options->setProc   != NULL) ||
		(type->options->setObjProc != NULL)));
  assert (IMPLY(type->options != NULL, type->options->queryProc  != NULL));

  assert (type->encoder.createProc);
  assert (type->encoder.deleteProc);
  assert (type->encoder.convertProc);
  assert (type->encoder.flushProc);
  assert (type->encoder.clearProc);

  assert (type->decoder.createProc);
  assert (type->decoder.deleteProc);
  assert (type->decoder.convertProc);
  assert (type->decoder.flushProc);
  assert (type->decoder.clearProc);

  /*
   * Generate command to execute conversions and/or generate transformations
   */

  entry             = (Trf_RegistryEntry*) Tcl_Alloc (sizeof (Trf_RegistryEntry));
  entry->transType  = (Tcl_ChannelType*)   Tcl_Alloc (sizeof (Tcl_ChannelType)); 
  entry->trfType    = (Trf_TypeDefinition*) type;
  entry->interp     = interp;
#if (TCL_MAJOR_VERSION >= 8)
  entry->trfCommand = Tcl_CreateObjCommand (interp, (char*) type->name,
					    strlen (type->name), TrfExecuteObjCmd,
					    (ClientData) entry, TrfDeleteCmd);
#else
  entry->trfCommand = Tcl_CreateCommand (interp, (char*) type->name, TrfExecuteCmd,
					 (ClientData) entry, TrfDeleteCmd);
#endif

  /*
   * Set up channel type
   */

  entry->transType->typeName         = (char*) type->name;
  entry->transType->blockModeProc    = NULL;
  entry->transType->closeProc        = TrfClose;
  entry->transType->inputProc        = TrfInput;
  entry->transType->outputProc       = TrfOutput;
  entry->transType->seekProc         = TrfSeek;
  entry->transType->setOptionProc    = NULL;
  entry->transType->getOptionProc    = NULL;
  entry->transType->watchChannelProc = TrfWatch;
  entry->transType->channelReadyProc = TrfReady;
  entry->transType->getFileProc      = TrfGetFile;

  /*
   * Add entry to internal registry.
   */

  hPtr = Tcl_CreateHashEntry (hTablePtr, (char*) type->name, &new);
  Tcl_SetHashValue (hPtr, entry);

  return TCL_OK;
}

/*
 *------------------------------------------------------*
 *
 *	Trf_Unregister --
 *
 *	------------------------------------------------*
 *	Removess the conversion from the registry
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Releases memory allocated in 'Trf_Register'.
 *
 *	Result:
 *		A standard TCL error code.
 *
 *------------------------------------------------------*
 */

EXTERN int
Trf_Unregister (interp, entry)
Tcl_Interp*        interp;
Trf_RegistryEntry* entry;
{
  Tcl_HashEntry* hPtr;
  Tcl_HashTable* hTablePtr;

  hTablePtr = TrfGetRegistry    (interp);
  hPtr      = Tcl_FindHashEntry (hTablePtr, (char*) entry->trfType->name);

  Tcl_Free ((char*) entry->transType);
  Tcl_Free ((char*) entry);

  Tcl_DeleteHashEntry (hPtr);

  return TCL_OK;
}

/*
 *------------------------------------------------------*
 *
 *	TrfDeleteRegistry --
 *
 *	------------------------------------------------*
 *	Trap handler. Called by the Tcl core during
 *	interpreter destruction. Destroys the registry
 *	of conversions.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Releases memory allocated in 'TrfGetRegistry'.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
TrfDeleteRegistry (clientData, interp)
ClientData  clientData;
Tcl_Interp* interp;
{
  Tcl_HashTable* hTablePtr;
  hTablePtr = (Tcl_HashTable*) clientData;

  /*
   * The commands are already deleted, therefore the hashtable is empty here.
   */

  Tcl_DeleteHashTable (hTablePtr);
}

/* (readable) shortcuts for calling the option processing vectors.
 */

#define CLT  (entry->trfType->clientData)
#define OPT  (entry->trfType->options)

#define CREATE_OPTINFO         (OPT ? (*OPT->createProc) (CLT) : NULL)
#define DELETE_OPTINFO         if (optInfo) (*OPT->deleteProc) (optInfo, CLT)
#define CHECK_OPTINFO(baseOpt) (optInfo ? (*OPT->checkProc) (optInfo, interp, &baseOpt, CLT) : TCL_OK)
#define SET_OPTION(opt,optval) (optInfo ? (*OPT->setProc) (optInfo, interp, opt, optval, CLT) : TCL_ERROR)
#define ENCODE_REQUEST(entry,optInfo) (optInfo ? (*OPT->queryProc) (optInfo, CLT) : 1)

#if (TCL_MAJOR_VERSION < 8)
/*
 *------------------------------------------------------*
 *
 *	TrfExecuteCmd --
 *
 *	------------------------------------------------*
 *	Implementation procedure for all conversion commands.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Mostly as defined by 'Transform' or
 *		'CreateTransform'. Leaves a message in
 *		the interpreter result area in case of
 *		an error.
 *
 *	Result:
 *		A standard TCL error code.
 *
 *------------------------------------------------------*
 */

static int
TrfExecuteCmd (clientData, interp, argc, argv)
ClientData  clientData;
Tcl_Interp* interp;
int         argc;
char**      argv;
{
  int                res, len;
  Tcl_Channel        source, destination;
  int                src_mode, dst_mode;
  const char*        cmd;
  const char*        option;
  const char*        optarg;
  Trf_RegistryEntry* entry;
  Trf_Options        optInfo;
  Trf_BaseOptions    baseOpt;

  baseOpt.attach      = (Tcl_Channel) NULL;
  baseOpt.attach_mode = 0;

  entry = (Trf_RegistryEntry*) clientData;
  cmd   = argv [0];

  argc --;
  argv ++;


  if (1 == (argc % 2)) {
      Tcl_AppendResult (interp, cmd, ": wrong # args", (char*) NULL);
      return TCL_ERROR;
  }

  optInfo = CREATE_OPTINFO;

  while ((argc > 0) && (argv [0][0] == '-')) {
    /*
     * Process options, as long as they are found
     */

    option = argv [0];
    optarg = argv [1];

    argc -= 2;
    argv += 2;

    len = strlen (option+1);
    
    if (len < 1)
      goto unknown_option;

    switch (option [1])
      {
      case 'a':
	if (0 != strncmp (option, "-attach", len))
	  goto unknown_option;

	baseOpt.attach = Tcl_GetChannel (interp, (char*) optarg, &baseOpt.attach_mode);
	if (baseOpt.attach == (Tcl_Channel) NULL)
	  return TCL_ERROR;
	break;

      default:
	res = SET_OPTION (option, optarg);
	if (res != TCL_OK) {
	  DELETE_OPTINFO;
	  return TCL_ERROR;
	}
	break;
      } /* switch option */
  } /* while options */

  /*
   * Check argument restrictions, insert defaults if necessary,
   * execute the required operation.
   */

  res = CHECK_OPTINFO (baseOpt);
  if (res != TCL_OK) {
    DELETE_OPTINFO;
    return TCL_ERROR;
  }

  if (baseOpt.attach == (Tcl_Channel) NULL) /* TRF_IMMEDIATE */ {
    /*
     * Direct rendering requested
     */

    if (argc < 1) {
      Tcl_AppendResult (interp, cmd, ": source, destination missing",
			(char*) NULL);
      return TCL_ERROR;
    } else if (argc < 2) {
      Tcl_AppendResult (interp, cmd, ": destination missing",
			(char*) NULL);
      return TCL_ERROR;
    }

    /* Determine channels to use */

    source = Tcl_GetChannel (interp, argv [0], &src_mode);
    if (source == (Tcl_Channel) NULL)
      return TCL_ERROR;

    destination = Tcl_GetChannel (interp, argv [1], &dst_mode);
    if (destination == (Tcl_Channel) NULL)
      return TCL_ERROR;

    if (! (src_mode & TCL_READABLE)) {
      Tcl_AppendResult (interp, cmd, ": source-channel not readable",
			(char*) NULL);
      return TCL_ERROR;
    }

    if (! (dst_mode & TCL_WRITABLE)) {
      Tcl_AppendResult (interp, cmd, ": destination-channel not writable",
			(char*) NULL);
      return TCL_ERROR;
    }

    /*
     * Transform input now.
     */

    res = TransformImmediate (interp, entry, source, destination, optInfo);

  } else /* TRF_ATTACH */ {
    /*
     * User requested attachment of transformation procedure to a channel.
     */

    res = AttachTransform (entry, baseOpt.attach, optInfo, interp);
  }

  DELETE_OPTINFO;
  return res;

 unknown_option:
  DELETE_OPTINFO;
  Tcl_AppendResult (interp, cmd, ": unknown option '", option, "'",
		    (char*) NULL);
  return TCL_ERROR;
}

#else /* -> TCL_MAJOR_VERSION >= 8 */
/*
 *------------------------------------------------------*
 *
 *	TrfExecuteObjCmd --
 *
 *	------------------------------------------------*
 *	Implementation procedure for all conversion commands.
 *	Equivalent to 'TrfExecuteCmd', but using the new
 *	Object interfaces.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		See 'TrfExecuteCmd'.
 *
 *	Result:
 *		A standard TCL error code.
 *
 *------------------------------------------------------*
 */

static int
TrfExecuteObjCmd (clientData, interp, objc, objv)
ClientData       clientData;
Tcl_Interp*      interp;
int              objc;
struct Tcl_Obj** objv;
{
  /* (readable) shortcuts for calling the option processing vectors.
   * as defined in 'TrfExecuteCmd'.
   */

#define SET_OPTION_OBJ(opt,optval) (optInfo ? (*OPT->setObjProc) (optInfo, interp, opt, optval, CLT) : TCL_ERROR)

  int                res, len;
  Tcl_Channel        source, destination;
  int                src_mode, dst_mode;
  const char*        cmd;
  const char*        option;
  struct Tcl_Obj*    optarg;
  Trf_RegistryEntry* entry;
  Trf_Options        optInfo;
  Trf_BaseOptions    baseOpt;

  baseOpt.attach      = (Tcl_Channel) NULL;
  baseOpt.attach_mode = 0;

  entry = (Trf_RegistryEntry*) clientData;
  cmd   = Tcl_GetStringFromObj (objv [0], NULL);

  objc --;
  objv ++;


  if (1 == (objc % 2)) {
    ADD_RES (interp, cmd);
    ADD_RES (interp, ": wrong # args");
    return TCL_ERROR;
  }

  optInfo = CREATE_OPTINFO;

  while ((objc > 0) && (*Tcl_GetStringFromObj (objv [0], NULL) == '-')) {
    /*
     * Process options, as long as they are found
     */

    option = Tcl_GetStringFromObj (objv [0], NULL);
    optarg = objv [1];

    objc -= 2;
    objv += 2;

    len = strlen (option+1);
    
    if (len < 1)
      goto unknown_option;

    switch (option [1])
      {
      case 'a':
	if (0 != strncmp (option, "-attach", len))
	  goto unknown_option;

	baseOpt.attach = Tcl_GetChannel (interp,
					 Tcl_GetStringFromObj (optarg, NULL),
					 &baseOpt.attach_mode);
	if (baseOpt.attach == (Tcl_Channel) NULL)
	  return TCL_ERROR;
	break;

      default:
	if ((*OPT->setObjProc) == NULL) {
	  res = SET_OPTION     (option, Tcl_GetStringFromObj (optarg, NULL));
	} else {
	  res = SET_OPTION_OBJ (option, optarg);
	}

	if (res != TCL_OK) {
	  DELETE_OPTINFO;
	  return TCL_ERROR;
	}
	break;
      } /* switch option */
  } /* while options */

  /*
   * Check argument restrictions, insert defaults if necessary,
   * execute the required operation.
   */

  res = CHECK_OPTINFO (baseOpt);
  if (res != TCL_OK) {
    DELETE_OPTINFO;
    return TCL_ERROR;
  }

  if (baseOpt.attach == (Tcl_Channel) NULL) /* TRF_IMMEDIATE */ {
    /*
     * Direct rendering requested
     */

    if (objc < 1) {
      ADD_RES (interp, cmd);
      ADD_RES (interp, ": source, destination missing");
      return TCL_ERROR;
    } else if (objc < 2) {
      ADD_RES (interp, cmd);
      ADD_RES (interp, ": destination missing");
      return TCL_ERROR;
    }

    /* Determine channels to use */

    source = Tcl_GetChannel (interp, Tcl_GetStringFromObj (objv [0], NULL), &src_mode);
    if (source == (Tcl_Channel) NULL)
	  return TCL_ERROR;

    destination = Tcl_GetChannel (interp, Tcl_GetStringFromObj (objv [1], NULL), &dst_mode);
    if (destination == (Tcl_Channel) NULL)
      return TCL_ERROR;

    if (! (src_mode & TCL_READABLE)) {
      ADD_RES (interp, cmd);
      ADD_RES (interp, ": source-channel not readable");
      return TCL_ERROR;
    }

    if (! (dst_mode & TCL_WRITABLE)) {
      ADD_RES (interp, cmd);
      ADD_RES (interp, ": destination-channel not writable");
      return TCL_ERROR;
    }

    /*
     * Transform input now.
     */

    res = TransformImmediate (interp, entry, source, destination, optInfo);

  } else /* TRF_ATTACH */ {
    /*
     * User requested attachment of transformation procedure to a channel.
     */

    res = AttachTransform (entry, baseOpt.attach, optInfo, interp);
  }

  DELETE_OPTINFO;
  return res;

 unknown_option:
  DELETE_OPTINFO;
  ADD_RES (interp, cmd);
  ADD_RES (interp, ": unknown option '");
  ADD_RES (interp, option);
  ADD_RES (interp, "'");
  return TCL_ERROR;
}
#endif /* TCL_MAJOR_VERSION < 8 */

/*
 *------------------------------------------------------*
 *
 *	TrfDeleteCmd --
 *
 *	------------------------------------------------*
 *	Trap handler. Called by the Tcl core during
 *	destruction of the command for invocation of a
 *	conversion.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Removes the conversion from the registry.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
TrfDeleteCmd (clientData)
ClientData clientData;
{
  Trf_RegistryEntry* entry;

  entry = (Trf_RegistryEntry*) clientData;

  Trf_Unregister (entry->interp, entry);
}

/*
 *------------------------------------------------------*
 *
 *	TrfClose --
 *
 *	------------------------------------------------*
 *	Trap handler. Called by the generic IO system
 *	during destruction of the conversion channel.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Releases memory allocated in 'CreateTransform'.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static int
TrfClose (instanceData, interp)
ClientData  instanceData;
Tcl_Interp* interp;
{
  /*
   * The parent channel will be removed automatically
   * (if necessary and/or desired).
   */

  TrfTransformationInstance* trans = (TrfTransformationInstance*) instanceData;

  /*
   * Flush data waiting in conversion buffers to output.
   * Flush input too, maybe there are side effects other
   * parts do rely on (-> message digests).
   */

  trans->out.vectors->flushProc (trans->out.control, (Tcl_Interp*) NULL, trans->clientData);
  trans->in.vectors->flushProc  (trans->in.control,  (Tcl_Interp*) NULL, trans->clientData);

  trans->out.vectors->deleteProc (trans->out.control, trans->clientData);
  trans->in.vectors->deleteProc  (trans->in.control,  trans->clientData);

  if (trans->allocated)
    Tcl_Free (trans->result);

  return TCL_OK;
}

/*
 *------------------------------------------------------*
 *
 *	TrfInput --
 *
 *	------------------------------------------------*
 *	Called by the generic IO system to convert read data.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As defined by the converiosn.
 *
 *	Result:
 *		A transformed buffer.
 *
 *------------------------------------------------------*
 */

static int
TrfInput (instanceData, buf, toRead, errorCodePtr)
ClientData instanceData;
char*      buf;
int        toRead;
int*       errorCodePtr;
{
  TrfTransformationInstance* trans = (TrfTransformationInstance*) instanceData;
  int gotBytes, read, i, res;

  gotBytes = 0;

  while (toRead > 0) {
    if (trans->used > 0) {
      /*
       * First: copy as much possible from the result buffer.
       */

      if (trans->used >= toRead) {
	/*
	 * More than enough, take data from the internal buffer, then return.
	 * Don't forget to shift the remaining parts down.
	 */

	memcpy ((VOID*) buf, (VOID*) trans->result, toRead);
	if (trans->used > toRead) {
	  memmove ((VOID*) trans->result, (VOID*) (trans->result + toRead), toRead - trans->used);
	}

	trans->used -= toRead;
	gotBytes    += toRead;
	toRead = 0;

	return gotBytes;

      } else {
	/*
	 * Not enough in buffer to satisfy the caller, take all, then try to read more.
	 */

	memcpy ((VOID*) buf, (VOID*) trans->result, trans->used);

	buf      += trans->used;
	toRead   -= trans->used;
	gotBytes += trans->used;
	trans->used = 0;
      }
    }

    /*
     * trans->used == 0, toRead > 0 here 
     * Use 'buf'! as target to store the intermediary
     * information read from the parent channel.
     */

    read = Tcl_Read (trans->parent, buf, toRead);

    if (read < 0) {
      /* Report errors to caller.
       */

      *errorCodePtr = Tcl_GetErrno ();
      return -1;      
    }

    if (read == 0) {
      /* check wether we hit on EOF in 'trans->parent' or
       * not. If not we are in non-blocking mode and ran
       * temporarily out of data. Return the part we got
       * and let the caller wait for more. On the other
       * hand, if we got an Eof we have to convert and
       * flush all waiting partial data.
       */

      if (! Tcl_Eof (trans->parent)) {
	return gotBytes;
      } else {
	res = trans->in.vectors->flushProc (trans->in.control,
					    (Tcl_Interp*) NULL,
					    trans->clientData);
	if (trans->used == 0) {
	  /* we had nothing to flush */
	  return gotBytes;
	}
	continue; /* at: while (toRead > 0) */
      }
    }

    /* transform the read chunk */

    if (trans->in.vectors->convertBufProc){ 
      res = trans->in.vectors->convertBufProc (trans->in.control,
					       buf, read,
					       (Tcl_Interp*) NULL,
					       trans->clientData);
    } else {
      res = TCL_OK;
      for (i=0; i < read; i++) {
	res = trans->in.vectors->convertProc (trans->in.control, buf [i],
					      (Tcl_Interp*) NULL,
					      trans->clientData);
	if (res != TCL_OK) {
	  break;
	}
      }
    }

    if (res != TCL_OK) {
      *errorCodePtr = EINVAL;
      return -1;
    }
  } /* while toRead > 0 */

  return gotBytes;
}

/*
 *------------------------------------------------------*
 *
 *	TrfOutput --
 *
 *	------------------------------------------------*
 *	Called by the generic IO system to convert data
 *	waiting to be written.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As defined by the conversion.
 *
 *	Result:
 *		A transformed buffer.
 *
 *------------------------------------------------------*
 */

static int
TrfOutput (instanceData, buf, toWrite, errorCodePtr)
ClientData instanceData;
char*      buf;
int        toWrite;
int*       errorCodePtr;
{
  TrfTransformationInstance* trans = (TrfTransformationInstance*) instanceData;
  int i, res;

  /*
   * transformation results are automatically written to the parent channel
   * ('PutDestination' was configured as write procedure in 'AttachTransformation')
   */

    if (trans->in.vectors->convertBufProc){ 
      res = trans->out.vectors->convertBufProc (trans->out.control,
					       buf, toWrite,
						(Tcl_Interp*) NULL,
					       trans->clientData);
    } else {
      res = TCL_OK;
      for (i=0; i < toWrite; i++) {
	res = trans->out.vectors->convertProc (trans->out.control, buf [i],
					       (Tcl_Interp*) NULL,
					       trans->clientData);
	if (res != TCL_OK) {
	  break;
	}
      }
    }

  if (res != TCL_OK) {
    *errorCodePtr = EINVAL;
    return -1;
  }

  return toWrite;
}

/*
 *------------------------------------------------------*
 *
 *	TrfSeek --
 *
 *	------------------------------------------------*
 *	This procedure is called by the generic IO level
 *	to move the access point in a channel.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Moves the location at which the channel
 *		will be accessed in future operations.
 *		Flushes all transformation buffers, then
 *		forwards it to the underlying channel.
 *
 *	Result:
 *		-1 if failed, the new position if
 *		successful. An output argument contains
 *		the POSIX error code if an error
 *		occurred, or zero.
 *
 *------------------------------------------------------*
 */

static int
TrfSeek (instanceData, offset, mode, errorCodePtr)
ClientData instanceData;	/* The channel to manipulate */
long       offset;		/* Size of movement. */
int        mode;		/* How to move */
int*       errorCodePtr;	/* Location of error flag. */
{
  int result;
  TrfTransformationInstance* trans = (TrfTransformationInstance*) instanceData;

  /*
   * Flush data waiting for output, discard everything in the input buffers.
   */

  trans->out.vectors->flushProc (trans->out.control, (Tcl_Interp*) NULL, trans->clientData);
  trans->in.vectors->clearProc  (trans->in.control, trans->clientData);


  result = Tcl_Seek (trans->parent, offset, mode);
  *errorCodePtr = (result == -1) ? Tcl_GetErrno ():0;
  return result;
}

/*
 *------------------------------------------------------*
 *
 *	TrfWatch --
 *
 *	------------------------------------------------*
 *	Initialize the notifier to watch Tcl_Files from
 *	this channel.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Sets up the notifier so that a future
 *		event on the channel will be seen by Tcl.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */
	/* ARGSUSED */
static void
TrfWatch (instanceData, mask)
ClientData instanceData;	/* Channel to watch */
int        mask;		/* Events of interest */
{
  /*
   * Forward request to channel we are stacked upon.
   */

  TrfTransformationInstance* trans      = (TrfTransformationInstance*) instanceData;
  Tcl_ChannelType*           p_type     = Tcl_GetChannelType         (trans->parent);
  ClientData                 p_instance = Tcl_GetChannelInstanceData (trans->parent);

  p_type->watchChannelProc (p_instance, mask);
}

/*
 *------------------------------------------------------*
 *
 *	TrfReady --
 *
 *	------------------------------------------------*
 *	Called by the notifier to check whether events
 *	of interest are present on the channel.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		None.
 *
 *	Result:
 *		Returns OR-ed combination of TCL_READABLE,
 *		TCL_WRITABLE and TCL_EXCEPTION to indicate
 *		which events of interest are present.
 *
 *------------------------------------------------------*
 */

static int
TrfReady (instanceData, mask)
ClientData instanceData;	/* Channel to query */
int        mask;		/* Mask of queried events */
{
  /*
   * Forward request to channel we are stacked upon.
   */

  TrfTransformationInstance* trans      = (TrfTransformationInstance*) instanceData;
  Tcl_ChannelType*           p_type     = Tcl_GetChannelType         (trans->parent);
  ClientData                 p_instance = Tcl_GetChannelInstanceData (trans->parent);

  return p_type->channelReadyProc (p_instance, mask);
}

/*
 *------------------------------------------------------*
 *
 *	TrfGetFile --
 *
 *	------------------------------------------------*
 *	Called from Tcl_GetChannelFile to retrieve
 *	Tcl_Files from inside this channel.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		None.
 *
 *	Result:
 *		The appropriate Tcl_File or NULL if not
 *		present. 
 *
 *------------------------------------------------------*
 */
	/* ARGSUSED */
static Tcl_File
TrfGetFile (instanceData, mask)
ClientData instanceData;	/* Channel to query */
int        mask;		/* Direction of interest */
{
  /*
   * return file belonging to parent channel
   */

  TrfTransformationInstance* trans = (TrfTransformationInstance*) instanceData;

  return Tcl_GetChannelFile (trans->parent, mask);
}

/*
 *------------------------------------------------------*
 *
 *	TransformImmediate --
 *
 *	------------------------------------------------*
 *	Read from source, apply the specified conversion
 *	and write the result to destination.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		The access points of source and destination
 *		change, data is added to destination too.
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

static int
TransformImmediate (interp, entry, source, destination, optInfo)
Tcl_Interp*        interp;
Trf_RegistryEntry* entry;
Tcl_Channel        source;
Tcl_Channel        destination;
Trf_Options        optInfo;
{
  Trf_Vectors*     v;
  Trf_ControlBlock control;
  unsigned char*   buf;
  int              res = TCL_OK;
  int actuallyRead;

  if (ENCODE_REQUEST (entry, optInfo))
    v = &(entry->trfType->encoder);
  else
    v = &(entry->trfType->decoder);

  control = v->createProc ((ClientData) destination, PutDestination,
			   optInfo, interp,
			   entry->trfType->clientData);

  if (control == (Trf_ControlBlock) NULL) {
    return TCL_ERROR;
  }

  buf = (unsigned char*) Tcl_Alloc (READ_CHUNK_SIZE);

  while (1) {
    if (Tcl_Eof (source))
      break;

    actuallyRead = Tcl_Read (source, buf, READ_CHUNK_SIZE);

    if (actuallyRead <= 0)
      break;

    if (v->convertBufProc) {
      res = v->convertBufProc (control, buf, actuallyRead, interp,
			       entry->trfType->clientData);
    } else {
      unsigned int i, c;

      for (i=0; i < ((int) actuallyRead); i++) {
	c = buf [i];
	res = v->convertProc (control, c, interp,
			      entry->trfType->clientData);
	  
	if (res != TCL_OK)
	  break;
      }
    }

    if (res != TCL_OK)
      break;
  }

  Tcl_Free (buf);

  if (res == TCL_OK)
    res = v->flushProc (control, interp, entry->trfType->clientData);

  v->deleteProc (control, entry->trfType->clientData);

  return res;
}

/*
 *------------------------------------------------------*
 *
 *	AttachTransform --
 *
 *	------------------------------------------------*
 *	Create an instance of a conversion transformation
 *	and associate it with the specified channel.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Allocates memory, changes the internal
 *		state of the channel.
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

static int
AttachTransform (entry, attach, optInfo, interp)
Trf_RegistryEntry* entry;
Tcl_Channel        attach;
Trf_Options        optInfo;
Tcl_Interp*        interp;
{
  Tcl_Channel                new;
  TrfTransformationInstance* trans;

  trans = (TrfTransformationInstance*) Tcl_Alloc (sizeof (TrfTransformationInstance));

  /* trans->standard.typePtr = entry->transType; */
  trans->clientData       = entry->trfType->clientData;
  trans->parent           = attach;

  if (ENCODE_REQUEST (entry, optInfo)) {
    /* ENCODE on write
     * DECODE on read
     */

    trans->out.vectors = &entry->trfType->encoder;
    trans->in.vectors  = &entry->trfType->decoder;

  } else /* mode == DECODE */ {
    /* DECODE on write
     * ENCODE on read
     */

    trans->out.vectors = &entry->trfType->decoder;
    trans->in.vectors  = &entry->trfType->encoder;
  }

  trans->out.control = trans->out.vectors->createProc ((ClientData) trans->parent,
						       PutDestination,
						       optInfo, interp,
						       trans->clientData);

  if (trans->out.control == (Trf_ControlBlock) NULL) {
    Tcl_Free ((char*) trans);
    return TCL_ERROR;
  }

  trans->in.control  = trans->in.vectors->createProc  ((ClientData) trans, PutTrans,
						       optInfo, interp,
						       trans->clientData);

  if (trans->in.control == (Trf_ControlBlock) NULL) {
    Tcl_Free ((char*) trans);
    return TCL_ERROR;
  }


  trans->result    = (unsigned char*) NULL;
  trans->allocated = 0;
  trans->used      = 0;

  /*
   * Build channel from converter definition and stack it upon the one we shall attach to.
   */

  new = Tcl_ReplaceChannel (interp,
			    entry->transType, (ClientData) trans,
			    Tcl_GetChannelMode (attach), attach);


  if (new == (Tcl_Channel) NULL) {
    Tcl_Free ((char*) trans);
    ADD_RES (interp, "internal error in Tcl_ReplaceChannel");
    return TCL_ERROR;
  }

  /*  Tcl_RegisterChannel (interp, new); */
  ADD_RES (interp, Tcl_GetChannelName (new));

  return TCL_OK;
}

/*
 *------------------------------------------------------*
 *
 *	PutDestination --
 *
 *	------------------------------------------------*
 *	Handler used by a conversion to write its results.
 *	Used during the explicit conversion done by 'Transform'.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Writes to the channel.
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

static int
PutDestination (clientData, outString, outLen, interp)
ClientData     clientData;
unsigned char* outString;
int            outLen;
Tcl_Interp*    interp;
{
  Tcl_Channel destination = (Tcl_Channel) clientData;
  int         res;

  res = Tcl_Write (destination, outString, outLen);

  if (res < 0) {
    if (interp) {
      ADD_RES (interp, "error writing \"");
      ADD_RES (interp, Tcl_GetChannelName (destination));
      ADD_RES (interp, "\": ");
      ADD_RES (interp, Tcl_PosixError (interp));
    }
    return TCL_ERROR;
  }

  return TCL_OK;
}

/*
 *------------------------------------------------------*
 *
 *	PutTrans --
 *
 *	------------------------------------------------*
 *	Handler used by a conversion to write its results
 *	(to be read later). Used by conversion transformations.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		May allocate memory.
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

static int
PutTrans (clientData, outString, outLen, interp)
ClientData     clientData;
unsigned char* outString;
int            outLen;
Tcl_Interp*    interp;
{
  TrfTransformationInstance* trans = (TrfTransformationInstance*) clientData;
  unsigned char*             actual;

  if ((outLen + trans->used) > trans->allocated) {
    /*
     * Extension of internal buffer required.
     */

    if (trans->allocated == 0) {
      trans->allocated = outLen + INCREMENT;
      trans->result    = (unsigned char*) Tcl_Alloc (trans->allocated);
    } else {
      trans->allocated += outLen + INCREMENT;
      trans->result     = (unsigned char*) Tcl_Realloc (trans->result, trans->allocated);
    }
  }

  /* now copy data */

  for (actual = trans->result + trans->used, trans->used += outLen;
       outLen > 0;
       outLen --, actual ++, outString ++)
    *actual = *outString;

  return TCL_OK;
}
