/*
 * cipher.c --
 *
 *	Implements code common to all (stream) ciphers.
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
 * Declarations of internal procedures.
 */

static Trf_ControlBlock CreateEncoder  _ANSI_ARGS_ ((ClientData writeClientData,
						     Trf_WriteProc *fun,
						     Trf_Options optInfo,
						     Tcl_Interp*   interp,
						     ClientData clientData));
static void             DeleteEncoder  _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     ClientData clientData));
static int              Encode         _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     unsigned int character,
						     Tcl_Interp* interp,
						     ClientData clientData));
static int              EncodeBuffer   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     unsigned char* buffer, int bufLen,
						     Tcl_Interp* interp,
						     ClientData clientData));
static int              FlushEncoder   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     Tcl_Interp* interp,
						     ClientData clientData));
static void             ClearEncoder   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     ClientData clientData));


static Trf_ControlBlock CreateDecoder  _ANSI_ARGS_ ((ClientData writeClientData,
						     Trf_WriteProc *fun,
						     Trf_Options optInfo,
						     Tcl_Interp*   interp,
						     ClientData clientData));
#define DeleteDecoder DeleteEncoder
static int              Decode         _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     unsigned int character,
						     Tcl_Interp* interp,
						     ClientData clientData));
static int              DecodeBuffer   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     unsigned char* buffer, int bufLen,
						     Tcl_Interp* interp,
						     ClientData clientData));

#if 0
static int              FlushDecoder   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     Tcl_Interp* interp,
						     ClientData clientData));
#endif
#define FlushDecoder FlushEncoder
#if 0
static void             ClearDecoder   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     ClientData clientData));
#endif
#define ClearDecoder ClearEncoder

/*
 * Algorithm definition.
 */

static Trf_TypeDefinition cDefinition =
{
  NULL, /* filled later by 'Trf_RegisterCipher' */
  NULL, /* filled later by 'Trf_RegisterCipher' */
  NULL, /* filled later by 'Trf_RegisterCipher' */
  {
    CreateEncoder,
    DeleteEncoder,
    Encode,
    EncodeBuffer,
    FlushEncoder,
    ClearEncoder
  }, {
    CreateDecoder,
    DeleteDecoder,
    Decode,
    DecodeBuffer,
    FlushDecoder,
    ClearDecoder
  }
};

/*
 * Definition of the control blocks for en- and decryption.
 */

typedef struct _EncoderControl_ {
  Trf_WriteProc* write;
  ClientData     writeClientData;

  /* state information */
  unsigned char* key;  /* keyschedule, contains state information
			* of stream cipher too! */
} EncoderControl;


typedef EncoderControl DecoderControl ;


/*
 * Additional internal helpers.
 */

static void
InitializeState _ANSI_ARGS_ ((EncoderControl*             c,
			      TrfCipherOptionBlock*  optInfo,
			      int                    direction,
			      Trf_CipherDescription* bc_desc));

/*
 *------------------------------------------------------*
 *
 *	Trf_RegisterCipher --
 *
 *	------------------------------------------------*
 *	Register the specified cipher.
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
Trf_RegisterCipher (interp, c_desc)
Tcl_Interp*                  interp;
CONST Trf_CipherDescription* c_desc;
{
  Trf_TypeDefinition* c;

  c = (Trf_TypeDefinition*) Tcl_Alloc (sizeof (Trf_TypeDefinition));

  memcpy ((VOID*) c, (VOID*) &cDefinition, sizeof (Trf_TypeDefinition));

  c->name       = c_desc->name;
  c->clientData = (ClientData) c_desc;
  c->options    = TrfCipherOptions ();

  return Trf_Register (interp, c);

  /* 'c' is a memory leak, it will never be released.
   */
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
Trf_WriteProc *fun;
Trf_Options   optInfo;
Tcl_Interp*   interp;
ClientData    clientData;
{
  TrfCipherOptionBlock*       o = (TrfCipherOptionBlock*) optInfo;
  Trf_CipherDescription* c_desc = (Trf_CipherDescription*) clientData;
  EncoderControl*             c;

  c = (EncoderControl*) Tcl_Alloc (sizeof (EncoderControl));
  c->write           = fun;
  c->writeClientData = writeClientData;

  InitializeState (c, o, TRF_ENCRYPT, c_desc);

  return (Trf_ControlBlock) c;
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
ClientData      clientData;
{
  EncoderControl*        c      = (EncoderControl*) ctrlBlock;
  Trf_CipherDescription* c_desc = (Trf_CipherDescription*) clientData;

  memset ((VOID*) c->key, '\0', c_desc->ks_size);
  Tcl_Free (c->key);

  memset ((VOID*) c, '\0', sizeof (EncoderControl));
  Tcl_Free ((char*) c);
}

/*
 *------------------------------------------------------*
 *
 *	Encode --
 *
 *	------------------------------------------------*
 *	Encode the given character and write the result.
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
Encode (ctrlBlock, character, interp, clientData)
Trf_ControlBlock ctrlBlock;
unsigned int     character;
Tcl_Interp*      interp;
ClientData       clientData;
{
  unsigned char buf;

  EncoderControl*        c      = (EncoderControl*) ctrlBlock;
  Trf_CipherDescription* c_desc = (Trf_CipherDescription*) clientData;

  buf = character;
  (*c_desc->encryptProc) (&buf, c->key);

  return c->write (c->writeClientData, &buf, 1, interp);
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
unsigned char*   buffer;
int              bufLen;
Tcl_Interp*      interp;
ClientData       clientData;
{
  unsigned char* bufStart;
  int i;

  EncoderControl*        c      = (EncoderControl*) ctrlBlock;
  Trf_CipherDescription* c_desc = (Trf_CipherDescription*) clientData;

  for (i = 0, bufStart = buffer; i < bufLen; i++, buffer++) {
    (*c_desc->encryptProc) (buffer, c->key);
  }

  return c->write (c->writeClientData, bufStart, bufLen, interp);
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
  /* nothing to do */
  return TCL_OK;
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
ClientData       clientData;
{
  /* nothing to do, impossible (no access to state information in cipher) */
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
Trf_WriteProc *fun;
Trf_Options   optInfo;
Tcl_Interp*   interp;
ClientData    clientData;
{
  TrfCipherOptionBlock*       o = (TrfCipherOptionBlock*) optInfo;
  Trf_CipherDescription* c_desc = (Trf_CipherDescription*) clientData;
  EncoderControl*             c;

  c = (EncoderControl*) Tcl_Alloc (sizeof (EncoderControl));
  c->write           = fun;
  c->writeClientData = writeClientData;

  InitializeState (c, o, TRF_DECRYPT, c_desc);

  return (Trf_ControlBlock) c;
}

/*
 *------------------------------------------------------*
 *
 *	Decode --
 *
 *	------------------------------------------------*
 *	Decode the given character and write the result.
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
Decode (ctrlBlock, character, interp, clientData)
Trf_ControlBlock ctrlBlock;
unsigned int character;
Tcl_Interp* interp;
ClientData clientData;
{
  unsigned char buf;

  EncoderControl*             c = (EncoderControl*) ctrlBlock;
  Trf_CipherDescription* c_desc = (Trf_CipherDescription*) clientData;

  buf = character;
  (*c_desc->decryptProc) (&buf, c->key);

  return c->write (c->writeClientData, &buf, 1, interp);
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
unsigned char*   buffer;
int              bufLen;
Tcl_Interp*      interp;
ClientData       clientData;
{
  unsigned char* bufStart;
  int i;

  EncoderControl*             c = (EncoderControl*) ctrlBlock;
  Trf_CipherDescription* c_desc = (Trf_CipherDescription*) clientData;

  for (i=0, bufStart = buffer; i < bufLen; i++, buffer++) {
    (*c_desc->decryptProc) (buffer, c->key);
  }

  return c->write (c->writeClientData, bufStart, bufLen, interp);
}

#if 0
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
FlushDecoder (ctrlBlock, interp)
Trf_ControlBlock ctrlBlock;
Tcl_Interp* interp;
{
  return TCL_OK;
}
#endif

#if 0
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
ClearDecoder (ctrlBlock)
Trf_ControlBlock ctrlBlock;
{
}
#endif



static void
InitializeState (c, o, direction, c_desc)
EncoderControl*             c;
TrfCipherOptionBlock*  o;
int                         direction;
Trf_CipherDescription* c_desc;
{
  c->key = (unsigned char*) Tcl_Alloc (c_desc->ks_size);

  if (direction == TRF_ENCRYPT) {
    if (o->encrypt_keyschedule == NULL) {
      (*c_desc->scheduleProc) (o->key, o->key_length, direction,
				&(o->encrypt_keyschedule),
				&(o->decrypt_keyschedule));
      o->eks_length = c_desc->ks_size;
    }

    memcpy ((VOID*) c->key, o->encrypt_keyschedule, c_desc->ks_size);

  } else if (direction == TRF_DECRYPT) {
    if (o->decrypt_keyschedule == NULL) {
      (*c_desc->scheduleProc) (o->key, o->key_length, direction,
				&(o->encrypt_keyschedule),
				&(o->decrypt_keyschedule));
      o->dks_length = c_desc->ks_size;
    }

    memcpy ((VOID*) c->key, o->decrypt_keyschedule, c_desc->ks_size);
  } else {
    panic ("wrong direction given to cipher.InitializeState");
  }
}

