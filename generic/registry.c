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
 * => Information required by a result buffer.
 * => Information stored for the complete channel.
 */

typedef struct _DirectionInfo_ {
  Trf_ControlBlock   control; /* control block of transformation */
  Trf_Vectors*       vectors; /* vectors used during the transformation */
} DirectionInfo;


typedef struct _ResultBuffer_ {
  unsigned char* buf;
  int            allocated;
  int            used;
} ResultBuffer;


typedef struct _TrfTransformationInstance_ {
  Tcl_Channel parent; /* The channel superceded by this one */

  int readIsFlushed; /* flag to note wether in.flushProc was called or not */

  int mode;          /* mode of parent channel,
		      * OR'ed combination of
		      * TCL_READABLE, TCL_WRITABLE */

  /* Tcl_Transformation standard;   data required for all transformation instances */
  DirectionInfo      in;         /* information for transformation of read data */
  DirectionInfo      out;        /* information for transformation of written data */
  ClientData         clientData; /* copy from entry->trfType->clientData */

  /*
   * internal result buffer used during transformations of incoming data.
   * Stores results waiting for retrieval too, i.e. state information
   * carried from call to call.
   */

  ResultBuffer result;

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
			      int objc, struct Tcl_Obj* CONST objv []));
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

#if (TCL_MAJOR_VERSION < 8)
static int
TrfReady _ANSI_ARGS_ ((ClientData instanceData, int mask));

static Tcl_File
TrfGetFile _ANSI_ARGS_ ((ClientData instanceData, int mask));

static int
TransformImmediate _ANSI_ARGS_ ((Tcl_Interp* interp, Trf_RegistryEntry* entry,
				 Tcl_Channel source, Tcl_Channel destination,
				 CONST char* in,
				 Trf_Options optInfo));

#else
static int
TrfGetFile _ANSI_ARGS_ ((ClientData instanceData, int direction, ClientData* handlePtr));

static int
TransformImmediate _ANSI_ARGS_ ((Tcl_Interp* interp, Trf_RegistryEntry* entry,
				 Tcl_Channel source, Tcl_Channel destination,
				 struct Tcl_Obj* CONST in,
				 Trf_Options optInfo));

#endif

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

static int
PutInterpResult _ANSI_ARGS_ ((ClientData clientData,
			      unsigned char* outString, int outLen,
			      Tcl_Interp* interp));

/*
 *------------------------------------------------------*
 *
 *	TrfGetRegistry --
 *
 *	------------------------------------------------*
 *	Accessor to the interpreter associated registry
 *	of transformations.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Allocates and initializes the hashtable
 *		during the first call and associates it
 *		with the specified interpreter.
 *
 *	Result:
 *		The internal registry of transformations.
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
 *	of transformations. Does not create the registry
 *	(in contrast to 'TrfGetRegistry').
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		None.
 *
 *	Result:
 *		The internal registry of transformations.
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
 *	Announce a transformation to the registry associated
 *	with the specified interpreter.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		May create the registry. Allocates and
 *		initializes the structure describing
 *		the announced transformation.
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
  assert ((type->encoder.convertProc != NULL) || (type->encoder.convertBufProc != NULL));
  assert (type->encoder.flushProc);
  assert (type->encoder.clearProc);

  assert (type->decoder.createProc);
  assert (type->decoder.deleteProc);
  assert ((type->decoder.convertProc != NULL) || (type->decoder.convertBufProc != NULL));
  assert (type->decoder.flushProc);
  assert (type->decoder.clearProc);

  /*
   * Generate command to execute transformations immediately or to generate filters.
   */

  entry             = (Trf_RegistryEntry*) Tcl_Alloc (sizeof (Trf_RegistryEntry));
  entry->transType  = (Tcl_ChannelType*)   Tcl_Alloc (sizeof (Tcl_ChannelType)); 
  entry->trfType    = (Trf_TypeDefinition*) type;
  entry->interp     = interp;
#if (TCL_MAJOR_VERSION >= 8)
  entry->trfCommand = Tcl_CreateObjCommand (interp, (char*) type->name, TrfExecuteObjCmd,
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
#if (TCL_MAJOR_VERSION < 8)
  entry->transType->watchChannelProc = TrfWatch;
  entry->transType->channelReadyProc = TrfReady;
  entry->transType->getFileProc      = TrfGetFile;
#else
  entry->transType->watchProc        = TrfWatch;
  entry->transType->getHandleProc    = TrfGetFile;
#endif

#if GT81
  /* No additional close procedure. It is not possible to partially close a
   * transformation channel.
   */
  entry->transType->close2Proc = NULL;
#endif

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
 *	Removes the transformation from the registry
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Releases the memory allocated in 'Trf_Register'.
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
 *	of transformations.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Releases the memory allocated in 'TrfGetRegistry'.
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
 *	Implementation procedure for all transformations.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Mostly as defined by 'Transform' or
 *		'AttachTransform'. Leaves a message in
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
  /*  Tcl_Channel        source, destination; */
  /*  int                src_mode, dst_mode;  */
  const char*        cmd;
  const char*        option;
  const char*        optarg;
  Trf_RegistryEntry* entry;
  Trf_Options        optInfo;
  Trf_BaseOptions    baseOpt;
  int                mode;
  int                wrong_mod2;

  baseOpt.attach      = (Tcl_Channel) NULL;
  baseOpt.attach_mode = 0;
  baseOpt.source      = (Tcl_Channel) NULL;
  baseOpt.destination = (Tcl_Channel) NULL;

  entry = (Trf_RegistryEntry*) clientData;
  cmd   = argv [0];

  argc --;
  argv ++;

  optInfo = CREATE_OPTINFO;

  while ((argc > 0) && (argv [0][0] == '-')) {
    /*
     * Process options, as long as they are found
     */

    if (argc < 2) {
      /* option, but without argument */
      Tcl_AppendResult (interp, cmd, ": wrong # args", (char*) NULL);
      goto cleanup_after_error;      
    }

    option = argv [0];
    optarg = argv [1];

    argc -= 2;
    argv += 2;

    len = strlen (option);
    
    if (len < 2)
      goto unknown_option;

    switch (option [1])
      {
      case 'a':
	if (0 != strncmp (option, "-attach", len))
	  goto check_for_trans_option;

	baseOpt.attach = Tcl_GetChannel (interp, (char*) optarg, &baseOpt.attach_mode);
	if (baseOpt.attach == (Tcl_Channel) NULL)
	  goto cleanup_after_error;
	break;

      case 'i':
	if (0 != strncmp (option, "-in", len))
	  goto check_for_trans_option;

	baseOpt.source = Tcl_GetChannel (interp, (char*) optarg, &mode);
	if (baseOpt.source == (Tcl_Channel) NULL)
	  goto cleanup_after_error;

	if (! (mode & TCL_READABLE)) {
	  Tcl_AppendResult (interp, cmd, ": source-channel not readable", (char*) NULL);
	  goto cleanup_after_error;
	}
	break;

      case 'o':
	if (0 != strncmp (option, "-out", len))
	  goto check_for_trans_option;

	baseOpt.destination = Tcl_GetChannel (interp, (char*) optarg, &mode);
	if (baseOpt.destination == (Tcl_Channel) NULL)
	  goto cleanup_after_error;

	if (! (mode & TCL_WRITABLE)) {
	  Tcl_AppendResult (interp, cmd, ": destination-channel not writable", (char*) NULL);
	  goto cleanup_after_error;
	}
	break;

      default:
      check_for_trans_option:
	res = SET_OPTION (option, optarg);
	if (res != TCL_OK)
	  goto cleanup_after_error;
	break;
      } /* switch option */
  } /* while options */

  /*
   * Check argument restrictions, insert defaults if necessary,
   * execute the required operation.
   *
   * -attach => -in, -out not allowed, and reverse.
   */

  if ((baseOpt.attach != (Tcl_Channel) NULL) &&
      ((baseOpt.source      != (Tcl_Channel) NULL) ||
       (baseOpt.destination != (Tcl_Channel) NULL))) {
    Tcl_AppendResult (interp, cmd,
		      ": inconsistent options, -in/-out not allowed with -attach",
		      (char*) NULL);
    goto cleanup_after_error;
  }

  if ((baseOpt.source == (Tcl_Channel) NULL) &&
      (baseOpt.attach == (Tcl_Channel) NULL))
    wrong_mod2 = 0;
  else
    wrong_mod2 = 1;

  if (wrong_mod2 == (argc % 2)) {
      Tcl_AppendResult (interp, cmd, ": wrong # args", (char*) NULL);
      goto cleanup_after_error;
  }

  res = CHECK_OPTINFO (baseOpt);
  if (res != TCL_OK)
    goto cleanup_after_error;

  if (baseOpt.attach == (Tcl_Channel) NULL) /* TRF_IMMEDIATE */ {
    /*
     * Immediate execution of transformation requested.
     *
     * 4 possible cases:
     * % argv [0] -> result
     * % argv [0] -> channel	(-out)
     * % channel  -> result	(-in)
     * % channel  -> channel	(-in, -out)
     */

    res = TransformImmediate (interp, entry,
			      baseOpt.source, baseOpt.destination,
			      argv [0], optInfo);
  } else /* TRF_ATTACH */ {
    /*
     * User requested attachment of transformation procedure to a channel.
     */

    res = AttachTransform (entry, baseOpt.attach, optInfo, interp);
  }

  DELETE_OPTINFO;
  return res;


unknown_option:
  Tcl_AppendResult (interp, cmd, ": unknown option '", option, "'",
		    (char*) NULL);
  /* fall through to cleanup */

cleanup_after_error:
  DELETE_OPTINFO;
  return TCL_ERROR;
}

#else /* -> TCL_MAJOR_VERSION >= 8 */
/*
 *------------------------------------------------------*
 *
 *	TrfExecuteObjCmd --
 *
 *	------------------------------------------------*
 *	Implementation procedure for all transformations.
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
struct Tcl_Obj* CONST * objv;
{
  /* (readable) shortcuts for calling the option processing vectors.
   * as defined in 'TrfExecuteCmd'.
   */

#define SET_OPTION_OBJ(opt,optval) (optInfo ? (*OPT->setObjProc) (optInfo, interp, opt, optval, CLT) : TCL_ERROR)

  int                res, len;
  /*  Tcl_Channel        source, destination;*/
  /*  int                src_mode, dst_mode;*/
  const char*        cmd;
  const char*        option;
  struct Tcl_Obj*    optarg;
  Trf_RegistryEntry* entry;
  Trf_Options        optInfo;
  Trf_BaseOptions    baseOpt;
  int                mode;
  int                wrong_mod2;


  baseOpt.attach      = (Tcl_Channel) NULL;
  baseOpt.attach_mode = 0;
  baseOpt.source      = (Tcl_Channel) NULL;
  baseOpt.destination = (Tcl_Channel) NULL;

  entry = (Trf_RegistryEntry*) clientData;
  cmd   = Tcl_GetStringFromObj (objv [0], NULL);

  objc --;
  objv ++;

  optInfo = CREATE_OPTINFO;

  while ((objc > 0) && (*Tcl_GetStringFromObj (objv [0], NULL) == '-')) {
    /*
     * Process options, as long as they are found
     */

    if (objc < 2) {
      /* option, but without argument */
      Tcl_AppendResult (interp, cmd, ": wrong # args", (char*) NULL);
      goto cleanup_after_error;      
    }

    option = Tcl_GetStringFromObj (objv [0], NULL);
    optarg = objv [1];

    objc -= 2;
    objv += 2;

    len = strlen (option);
    
    if (len < 2)
      goto unknown_option;

    switch (option [1])
      {
      case 'a':
	if (0 != strncmp (option, "-attach", len))
	  goto check_for_trans_option;

	baseOpt.attach = Tcl_GetChannel (interp,
					 Tcl_GetStringFromObj (optarg, NULL),
					 &baseOpt.attach_mode);
	if (baseOpt.attach == (Tcl_Channel) NULL)
	  goto cleanup_after_error;
	break;

      case 'i':
	if (0 != strncmp (option, "-in", len))
	  goto check_for_trans_option;

	baseOpt.source = Tcl_GetChannel (interp,
					 Tcl_GetStringFromObj (optarg, NULL),
					 &mode);
	if (baseOpt.source == (Tcl_Channel) NULL)
	  goto cleanup_after_error;

	if (! (mode & TCL_READABLE)) {
	  Tcl_AppendResult (interp, cmd,
			    ": source-channel not readable",
			    (char*) NULL);
	  goto cleanup_after_error;
	}
	break;

      case 'o':
	if (0 != strncmp (option, "-out", len))
	  goto check_for_trans_option;

	baseOpt.destination = Tcl_GetChannel (interp,
					      Tcl_GetStringFromObj (optarg, NULL),
					      &mode);
	if (baseOpt.destination == (Tcl_Channel) NULL)
	  goto cleanup_after_error;

	if (! (mode & TCL_WRITABLE)) {
	  Tcl_AppendResult (interp, cmd,
			    ": destination-channel not writable",
			    (char*) NULL);
	  goto cleanup_after_error;
	}
	break;

      default:
      check_for_trans_option:
	if ((*OPT->setObjProc) == NULL) {
	  res = SET_OPTION     (option, Tcl_GetStringFromObj (optarg, NULL));
	} else {
	  res = SET_OPTION_OBJ (option, optarg);
	}

	if (res != TCL_OK)
	  goto cleanup_after_error;
	break;
      } /* switch option */
  } /* while options */

  /*
   * Check argument restrictions, insert defaults if necessary,
   * execute the required operation.
   */

  if ((baseOpt.attach != (Tcl_Channel) NULL) &&
      ((baseOpt.source      != (Tcl_Channel) NULL) ||
       (baseOpt.destination != (Tcl_Channel) NULL))) {
    Tcl_AppendResult (interp, cmd,
		      ": inconsistent options, -in/-out not allowed with -attach",
		      (char*) NULL);
    goto cleanup_after_error;
  }

  if ((baseOpt.source == (Tcl_Channel) NULL) &&
      (baseOpt.attach == (Tcl_Channel) NULL))
    wrong_mod2 = 0;
  else
    wrong_mod2 = 1;

  if (wrong_mod2 == (objc % 2)) {
      Tcl_AppendResult (interp, cmd, ": wrong # args", (char*) NULL);
      goto cleanup_after_error;
  }

  res = CHECK_OPTINFO (baseOpt);
  if (res != TCL_OK) {
    DELETE_OPTINFO;
    return TCL_ERROR;
  }

  if (baseOpt.attach == (Tcl_Channel) NULL) /* TRF_IMMEDIATE */ {
    /*
     * Immediate execution of transformation requested.
     */

    res = TransformImmediate (interp, entry,
			      baseOpt.source, baseOpt.destination,
			      objv [0], optInfo);

  } else /* TRF_ATTACH */ {
    /*
     * User requested attachment of transformation procedure to a channel.
     */

    res = AttachTransform (entry, baseOpt.attach, optInfo, interp);
  }

  DELETE_OPTINFO;
  return res;


unknown_option:
  Tcl_AppendResult (interp, cmd, ": unknown option '", option, "'",
		    (char*) NULL);
  /* fall through to cleanup */

cleanup_after_error:
  DELETE_OPTINFO;
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
 *	transformation.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Removes the transformation from the registry.
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
 *	during destruction of the transformation channel.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Releases the memory allocated in
 *		'AttachTransform'.
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
   * Flush data waiting in transformation buffers to output.
   * Flush input too, maybe there are side effects other
   * parts do rely on (-> message digests).
   */


  if (trans->mode & TCL_WRITABLE) {
    trans->out.vectors->flushProc (trans->out.control,
				   (Tcl_Interp*) NULL,
				   trans->clientData);
  }

  if (trans->mode & TCL_READABLE) {
    if (!trans->readIsFlushed) {
      trans->readIsFlushed = 1;
      trans->in.vectors->flushProc (trans->in.control,
				    (Tcl_Interp*) NULL,
				    trans->clientData);
    }
  }

  if (trans->mode & TCL_WRITABLE) {
    trans->out.vectors->deleteProc (trans->out.control, trans->clientData);
  }

  if (trans->mode & TCL_READABLE) {
    trans->in.vectors->deleteProc  (trans->in.control,  trans->clientData);
  }

  if (trans->result.allocated)
    Tcl_Free (trans->result.buf);

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

  /* should assert (trans->mode & TCL_READABLE) */

  gotBytes = 0;

  while (toRead > 0) {
    if (trans->result.used > 0) {
      /*
       * First: copy as much possible from the result buffer.
       */

      if (trans->result.used >= toRead) {
	/*
	 * More than enough, take data from the internal buffer, then return.
	 * Don't forget to shift the remaining parts down.
	 */

	memcpy ((VOID*) buf, (VOID*) trans->result.buf, toRead);
	if (trans->result.used > toRead) {
	  memmove ((VOID*) trans->result.buf,
		   (VOID*) (trans->result.buf + toRead),
		   trans->result.used - toRead);
	}

	trans->result.used -= toRead;
	gotBytes    += toRead;
	toRead = 0;

	return gotBytes;

      } else {
	/*
	 * Not enough in buffer to satisfy the caller, take all, then try to read more.
	 */

	memcpy ((VOID*) buf, (VOID*) trans->result.buf, trans->result.used);

	buf      += trans->result.used;
	toRead   -= trans->result.used;
	gotBytes += trans->result.used;
	trans->result.used = 0;
      }
    }

    /*
     * trans->result.used == 0, toRead > 0 here 
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
	if (trans->readIsFlushed) {
	  /* already flushed, nothing to do anymore */
	  return gotBytes;
	}

	trans->readIsFlushed = 1;
	res = trans->in.vectors->flushProc (trans->in.control,
					    (Tcl_Interp*) NULL,
					    trans->clientData);
	if (trans->result.used == 0) {
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
 *		As defined by the transformation.
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

  /* should assert (trans->mode & TCL_WRITABLE) */

  /*
   * transformation results are automatically written to
   * the parent channel ('PutDestination' was configured
   * as write procedure in 'AttachTransformation').
   */

    if (trans->out.vectors->convertBufProc){ 
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

  if (trans->mode & TCL_WRITABLE) {
    trans->out.vectors->flushProc (trans->out.control,
				   (Tcl_Interp*) NULL,
				   trans->clientData);
  }

  if (trans->mode & TCL_READABLE) {
    trans->in.vectors->clearProc  (trans->in.control, trans->clientData);
    trans->readIsFlushed = 0;
  }

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

#if (TCL_MAJOR_VERSION < 8)
  p_type->watchChannelProc (p_instance, mask);
#else
  p_type->watchProc (p_instance, mask);
#endif
}

#if (TCL_MAJOR_VERSION < 8)
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
#endif /* (TCL_MAJOR_VERSION < 8) */

#if (TCL_MAJOR_VERSION < 8)
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
#else
/*
 *------------------------------------------------------*
 *
 *	TrfGetFile --
 *
 *	------------------------------------------------*
 *	Called from Tcl_GetChannelHandle to retrieve
 *	OS specific file handle from inside this channel.
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
static int
TrfGetFile (instanceData, direction, handlePtr)
ClientData  instanceData;	/* Channel to query */
int         direction;		/* Direction of interest */
ClientData* handlePtr;		/* Place to store the handle into */
{
  /*
   * return handle belonging to parent channel
   */

  TrfTransformationInstance* trans = (TrfTransformationInstance*) instanceData;

  return Tcl_GetChannelHandle (trans->parent, direction, handlePtr);
}
#endif

/*
 *------------------------------------------------------*
 *
 *	TransformImmediate --
 *
 *	------------------------------------------------*
 *	Read from source, apply the specified transformation
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
TransformImmediate (interp, entry, source, destination, in, optInfo)
Tcl_Interp*        interp;
Trf_RegistryEntry* entry;
Tcl_Channel        source;
Tcl_Channel        destination;
#if (TCL_MAJOR_VERSION < 8)
CONST char*        in;
#else
struct Tcl_Obj* CONST in;
#endif
Trf_Options        optInfo;
{
  Trf_Vectors*     v;
  Trf_ControlBlock control;
  int              res = TCL_OK;

  ResultBuffer r;

  if (ENCODE_REQUEST (entry, optInfo))
    v = &(entry->trfType->encoder);
  else
    v = &(entry->trfType->decoder);

  /* Take care of output (channel vs. interpreter result area).
   */

  if (destination == (Tcl_Channel) NULL) {
    r.buf       = (unsigned char*) NULL;
    r.used      = 0;
    r.allocated = 0;

    control = v->createProc ((ClientData) &r, PutInterpResult,
			     optInfo, interp,
			     entry->trfType->clientData);
  } else {
    control = v->createProc ((ClientData) destination, PutDestination,
			     optInfo, interp,
			     entry->trfType->clientData);
  }

  if (control == (Trf_ControlBlock) NULL) {
    return TCL_ERROR;
  }


  /* Now differentiate between immediate value and channel as input.
   */

  if (source == (Tcl_Channel) NULL) {
    /* Immediate value.
     * -- VERSION DEPENDENT CODE --
     */
    int            length;
    unsigned char* buf;

#if (TCL_MAJOR_VERSION < 8)
    /* 7.6, argument 'in' is a string.
     */
    length = strlen (in);
    buf    = (char*) in;
#else
    /* 8.x, argument 'in' is arbitrary object, its string rep. may contain \0.
     */
    buf = Tcl_GetStringFromObj (in, &length);
#endif

    if (v->convertBufProc) {
      /* play it safe, use a copy, avoid clobbering the input. */
      unsigned char* tmp;

      tmp = Tcl_Alloc (length);
      memcpy (tmp, buf, length);

      res = v->convertBufProc (control, tmp, length, interp,
			       entry->trfType->clientData);
      Tcl_Free (tmp);
    } else {
      unsigned int i, c;
      
      for (i=0; i < ((unsigned int) length); i++) {
	c = buf [i];
	res = v->convertProc (control, c, interp,
			      entry->trfType->clientData);
	
	if (res != TCL_OK)
	  break;
      }
    }

    if (res == TCL_OK)
      res = v->flushProc (control, interp, entry->trfType->clientData);
  } else {
    /* Read from channel.
     */

    unsigned char* buf;
    int            actuallyRead;

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
	
	for (i=0; i < ((unsigned int) actuallyRead); i++) {
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
  }

  v->deleteProc (control, entry->trfType->clientData);


  if (destination == (Tcl_Channel) NULL) {
    /* Now write into interpreter result area.
     */

    if (res != TCL_OK) {
      if (r.buf != NULL)
	Tcl_Free (r.buf);
    } else {
      Tcl_ResetResult (interp);

      if (r.buf != NULL) {
#if (TCL_MAJOR_VERSION < 8)
	/*
	 * r.buf is always overallocated, see 'PutInterpResult'.
	 * No danger in simply writing the '\0'. Do this to
	 * prevent setting a string longer than given as result.
	 */
 
	r.buf [r.used] = '\0';
	Tcl_AppendResult (interp, r.buf, (char*) NULL);
#else
	Tcl_Obj* o = Tcl_NewStringObj (r.buf, r.used);

	Tcl_IncrRefCount (o);
	Tcl_SetObjResult (interp, o);
	Tcl_DecrRefCount (o);
#endif
      }
    }
  }

  return res;
}

/*
 *------------------------------------------------------*
 *
 *	AttachTransform --
 *
 *	------------------------------------------------*
 *	Create an instance of a transformation and
 *	associate as filter it with the specified channel.
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
  trans->readIsFlushed    = 0;
  trans->mode             = Tcl_GetChannelMode (attach);

  if (ENCODE_REQUEST (entry, optInfo)) {
    /* ENCODE on write
     * DECODE on read
     */

    trans->out.vectors = ((trans->mode & TCL_WRITABLE) ? &entry->trfType->encoder : NULL);
    trans->in.vectors  = ((trans->mode & TCL_READABLE) ? &entry->trfType->decoder : NULL);

  } else /* mode == DECODE */ {
    /* DECODE on write
     * ENCODE on read
     */

    trans->out.vectors = ((trans->mode & TCL_WRITABLE) ? &entry->trfType->decoder : NULL);
    trans->in.vectors  = ((trans->mode & TCL_READABLE) ? &entry->trfType->encoder : NULL);
  }

  /* 'PutDestination' is ok for write, only read
   * requires 'PutTrans' and its internal buffer.
   */

  if (trans->mode & TCL_WRITABLE) {
    trans->out.control = trans->out.vectors->createProc ((ClientData) trans->parent,
							 PutDestination,
							 optInfo, interp,
							 trans->clientData);
  
    if (trans->out.control == (Trf_ControlBlock) NULL) {
      Tcl_Free ((char*) trans);
      return TCL_ERROR;
    }
  }

  if (trans->mode & TCL_READABLE) {
    trans->in.control  = trans->in.vectors->createProc  ((ClientData) trans,
							 PutTrans,
							 optInfo, interp,
							 trans->clientData);

    if (trans->in.control == (Trf_ControlBlock) NULL) {
      Tcl_Free ((char*) trans);
      return TCL_ERROR;
    }
  }

  trans->result.buf       = (unsigned char*) NULL;
  trans->result.allocated = 0;
  trans->result.used      = 0;

  /*
   * Build channel from converter definition and stack it upon the one we shall attach to.
   */

  new = Tcl_ReplaceChannel (interp,
			    entry->transType, (ClientData) trans,
			    trans->mode, attach);


  if (new == (Tcl_Channel) NULL) {
    Tcl_Free ((char*) trans);
    Tcl_AppendResult (interp, "internal error in Tcl_ReplaceChannel", (char*) NULL);
    return TCL_ERROR;
  }

  /*  Tcl_RegisterChannel (interp, new); */
  Tcl_AppendResult (interp, Tcl_GetChannelName (new), (char*) NULL);

  return TCL_OK;
}

/*
 *------------------------------------------------------*
 *
 *	PutDestination --
 *
 *	------------------------------------------------*
 *	Handler used by a transformation to write its results.
 *	Used during the explicit transformation done by 'Transform'.
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
      Tcl_AppendResult (interp, "error writing \"",               (char*) NULL);
      Tcl_AppendResult (interp, Tcl_GetChannelName (destination), (char*) NULL);
      Tcl_AppendResult (interp, "\": ",                           (char*) NULL);
      Tcl_AppendResult (interp, Tcl_PosixError (interp),          (char*) NULL);
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
 *	Handler used by a transformation to write its
 *	results (to be read later). Used by transformations
 *	acting as filter.
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

  if ((outLen + trans->result.used) > trans->result.allocated) {
    /*
     * Extension of internal buffer required.
     */

    if (trans->result.allocated == 0) {
      trans->result.allocated = outLen + INCREMENT;
      trans->result.buf    = (unsigned char*) Tcl_Alloc (trans->result.allocated);
    } else {
      trans->result.allocated += outLen + INCREMENT;
      trans->result.buf     = (unsigned char*) Tcl_Realloc (trans->result.buf, trans->result.allocated);
    }
  }

  /* now copy data */
  memcpy (trans->result.buf + trans->result.used, outString, outLen);
  trans->result.used += outLen;

  return TCL_OK;
}

/*
 *------------------------------------------------------*
 *
 *	PutInterpResult --
 *
 *	------------------------------------------------*
 *	Handler used by a transformation to write its
 *	results into the interpreter result area.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		changes the contents of the interpreter
 *		result area.
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

static int
PutInterpResult (clientData, outString, outLen, interp)
ClientData     clientData;
unsigned char* outString;
int            outLen;
Tcl_Interp*    interp;
{
  ResultBuffer* r = (ResultBuffer*) clientData;

#if 0
  printf ("-- %d '%s'\n", outLen, outString);
  fflush (stdout);
#endif

  if ((outLen+1 + r->used) > r->allocated) {
    /*
     * Extension of internal buffer required.
     */

    if (r->allocated == 0) {
      r->allocated = outLen + INCREMENT;
      r->buf    = (unsigned char*) Tcl_Alloc (r->allocated);
    } else {
      r->allocated += outLen + INCREMENT;
      r->buf     = (unsigned char*) Tcl_Realloc (r->buf, r->allocated);
    }
  }

  /* now copy data */

  memcpy (r->buf + r->used, outString, outLen);
  r->used += outLen;

  return TCL_OK;
}
