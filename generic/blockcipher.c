/*
 * blockcipher.c --
 *
 *	Implements code common to all blockciphers.
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

static Trf_TypeDefinition bcDefinition =
{
  NULL, /* filled later by 'Trf_RegisterBlockcipher' */
  NULL, /* filled later by 'Trf_RegisterBlockcipher' */
  NULL, /* filled later by 'Trf_RegisterBlockcipher' */
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

  unsigned short operation_mode;
  unsigned short shift_width;
  unsigned short byteCount;

  unsigned char* key;    /* keyschedule */
  unsigned char* block;  /* register to contain the incoming characters */
  unsigned char* iv;     /* initialization vector, feedback register (stream modes) */
  unsigned char* output; /* register to en/decrypt the block (or iv) */
} EncoderControl;


typedef EncoderControl DecoderControl ;


/*
 * Additional internal helpers.
 */

static void
InitializeState _ANSI_ARGS_ ((EncoderControl*             c,
			      TrfBlockcipherOptionBlock*  optInfo,
			      int                         direction,
			      Trf_BlockcipherDescription* bc_desc));

/*
 *------------------------------------------------------*
 *
 *	Trf_RegisterBlockcipher --
 *
 *	------------------------------------------------*
 *	Register the specified blockcipher.
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
Trf_RegisterBlockcipher (interp, bc_desc)
Tcl_Interp*                       interp;
CONST Trf_BlockcipherDescription* bc_desc;
{
  Trf_TypeDefinition* bc;

  bc = (Trf_TypeDefinition*) Tcl_Alloc (sizeof (Trf_TypeDefinition));

  memcpy ((VOID*) bc, (VOID*) &bcDefinition, sizeof (Trf_TypeDefinition));

  bc->name       = bc_desc->name;
  bc->clientData = (ClientData) bc_desc;
  bc->options    = TrfBlockcipherOptions ();

  return Trf_Register (interp, bc);

  /* 'bc' is a memory leak, it will never be released.
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
  TrfBlockcipherOptionBlock*        o = (TrfBlockcipherOptionBlock*) optInfo;
  Trf_BlockcipherDescription* bc_desc = (Trf_BlockcipherDescription*) clientData;
  EncoderControl*                   c;

  c = (EncoderControl*) Tcl_Alloc (sizeof (EncoderControl));
  c->write           = fun;
  c->writeClientData = writeClientData;

  InitializeState (c, o, TRF_ENCRYPT, bc_desc);

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
  EncoderControl*             c       = (EncoderControl*) ctrlBlock;
  Trf_BlockcipherDescription* bc_desc = (Trf_BlockcipherDescription*) clientData;

  memset ((VOID*) c->key,    '\0', bc_desc->ks_size);
  memset ((VOID*) c->block,  '\0', bc_desc->block_size);
  memset ((VOID*) c->output, '\0', bc_desc->block_size);


  Tcl_Free (c->block);
  Tcl_Free (c->key);
  Tcl_Free (c->output);

  if (c->iv != NULL) {
    memset ((VOID*) c->iv, '\0', bc_desc->block_size);
    Tcl_Free (c->iv);
  }

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
  EncoderControl*             c       = (EncoderControl*) ctrlBlock;
  Trf_BlockcipherDescription* bc_desc = (Trf_BlockcipherDescription*) clientData;

  c->block [c->byteCount] = character;
  c->byteCount ++;

  switch (c->operation_mode) {
  case TRF_ECB_MODE:
    /*
     * Electronic Code Book.
     * Encrypt complete blocks only, each separately.
     */
    
    if (c->byteCount == bc_desc->block_size) {
      (*bc_desc->encryptProc) (c->block, c->output, c->key);

      c->byteCount = 0;
      /* not really required: memset ((VOID*) c->block, '\0', bc_desc->block_size); */

      return c->write (c->writeClientData, c->output, bc_desc->block_size, interp);
    }
    break;

  case TRF_CBC_MODE:
    /*
     * Cipher Block Chaining.
     * Encrypt complete blocks only, each dependent on its predecessor.
     */
    
    if (c->byteCount == bc_desc->block_size) {
      int res;

      Trf_XorBuffer (c->block, c->iv, bc_desc->block_size);

      (*bc_desc->encryptProc) (c->block, c->output, c->key);

      c->byteCount = 0;
      res = c->write (c->writeClientData, c->output, bc_desc->block_size, interp);

      /* use encryption result as new IV => next block depends on this one */
      memcpy ((VOID*) c->iv, (VOID*) c->output, bc_desc->block_size);

      return res;
    }
    break;

  case TRF_CFB_MODE:
    /*
     * Cipher FeedBack.
     * Encrypt blocks of 'shift_width' bytes, each dependent on its predecessor.
     */
    
    if (c->byteCount == c->shift_width) {
      (*bc_desc->encryptProc) (c->iv, c->output, c->key);

      Trf_XorBuffer     (c->block, c->output, c->shift_width);
      Trf_ShiftRegister (c->iv,    c->block,  c->shift_width, bc_desc->block_size);

      c->byteCount = 0;
      return c->write (c->writeClientData, c->block, c->shift_width, interp);
    }
    break;

  case TRF_OFB_MODE:
    /*
     * Output FeedBack.
     * Encrypt blocks of 'shift_width' bytes, each dependent on its predecessor.
     * Considered secure only for 'shift_width' being equal to bc_desc->block_size.
     */
    
    if (c->byteCount == c->shift_width) {
      (*bc_desc->encryptProc) (c->iv, c->output, c->key);

      Trf_XorBuffer     (c->block, c->output, c->shift_width);
      Trf_ShiftRegister (c->iv,    c->output, c->shift_width, bc_desc->block_size);

      c->byteCount = 0;
      return c->write (c->writeClientData, c->block, c->shift_width, interp);
    }
    break;

  default:
    panic ("unknown mode given to blockcipher.Encode");
    break;
  }

  return TCL_OK;
}

/*
 *------------------------------------------------------*
 *
 *	EncodeBuffer --
 *
 *	------------------------------------------------*
 *	Encode the buffer and write the result.
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
  EncoderControl*             c       = (EncoderControl*) ctrlBlock;
  Trf_BlockcipherDescription* bc_desc = (Trf_BlockcipherDescription*) clientData;

  int k, res;

  switch (c->operation_mode) {
  case TRF_ECB_MODE:
    /*
     * Electronic Code Book.
     * Encrypt complete blocks only, each separately.
     */

    k = bc_desc->block_size - c->byteCount;

    if (k > bufLen) {
      /*
       * We got less information than required to form a full block.
       * Extend the internal buffer and wait for more.
       */
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, bufLen);
      c->byteCount += bufLen;
      return TCL_OK;
    }

    if (k < bc_desc->block_size) {
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, k);

      (*bc_desc->encryptProc) (c->block, c->output, c->key);

      buffer += k;
      bufLen -= k;

      c->byteCount = 0;

      res = c->write (c->writeClientData, c->output, bc_desc->block_size, interp);
      if (res != TCL_OK)
	return res;
    } /* k == bc_desc->block_size => internal buffer was empty, so skip it entirely */

    /*
     * Process all blocks in the buffer.
     */

    while (bufLen >= bc_desc->block_size) {
      (*bc_desc->encryptProc) (buffer, c->output, c->key);

      buffer += bc_desc->block_size;
      bufLen -= bc_desc->block_size;

      res = c->write (c->writeClientData, c->output, bc_desc->block_size, interp);
      if (res != TCL_OK)
	return res;
    }
    break;


  case TRF_CBC_MODE:
    /*
     * Cipher Block Chaining.
     * Encrypt complete blocks only, each dependent on its predecessor.
     */
    
    k = bc_desc->block_size - c->byteCount;

    if (k > bufLen) {
      /*
       * We got less information than required to form a full block.
       * Extend the internal buffer and wait for more.
       */
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, bufLen);
      c->byteCount += bufLen;
      return TCL_OK;
    }

    if (k < bc_desc->block_size) {
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, k);

      buffer += k;
      bufLen -= k;

      Trf_XorBuffer (c->block, c->iv, bc_desc->block_size);
      (*bc_desc->encryptProc) (c->block, c->output, c->key);

      c->byteCount = 0;
      res = c->write (c->writeClientData, c->output, bc_desc->block_size, interp);

      /* use encryption result as new IV => next block depends on this one */
      memcpy ((VOID*) c->iv, (VOID*) c->output, bc_desc->block_size);

      if (res != TCL_OK)
	return res;
    } /* k == bc_desc->block_size => internal buffer was empty, so skip it entirely */

    /*
     * Process all blocks in the buffer.
     */

    while (bufLen >= bc_desc->block_size) {

      Trf_XorBuffer (buffer, c->iv, bc_desc->block_size);

      (*bc_desc->encryptProc) (buffer, c->output, c->key);

      buffer += bc_desc->block_size;
      bufLen -= bc_desc->block_size;

      res = c->write (c->writeClientData, c->output, bc_desc->block_size, interp);

      /* use encryption result as new IV => next block depends on this one */
      memcpy ((VOID*) c->iv, (VOID*) c->output, bc_desc->block_size);

      if (res != TCL_OK)
	return res;
    }
    break;


  case TRF_CFB_MODE:
    /*
     * Cipher FeedBack.
     * Encrypt blocks of 'shift_width' bytes, each dependent on its predecessor.
     */

    k = c->shift_width - c->byteCount;

    if (k > bufLen) {
      /*
       * We got less information than required to form a full block.
       * Extend the internal buffer and wait for more.
       */
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, bufLen);
      c->byteCount += bufLen;
      return TCL_OK;
    }

    if (k < c->shift_width) {
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, k);

      buffer += k;
      bufLen -= k;

      (*bc_desc->encryptProc) (c->iv, c->output, c->key);

      Trf_XorBuffer     (c->block, c->output, c->shift_width);
      Trf_ShiftRegister (c->iv,    c->block,  c->shift_width, bc_desc->block_size);

      c->byteCount = 0;
      res = c->write (c->writeClientData, c->block, c->shift_width, interp);

      if (res != TCL_OK)
	return res;
    } /* k == c->shift_width => internal buffer was empty, so skip it entirely */

    /*
     * Process all blocks in the buffer.
     */

    while (bufLen >= c->shift_width) {
      (*bc_desc->encryptProc) (c->iv, c->output, c->key);

      Trf_XorBuffer     (buffer, c->output, c->shift_width);
      Trf_ShiftRegister (c->iv,  buffer,  c->shift_width, bc_desc->block_size);

      res = c->write (c->writeClientData, buffer, c->shift_width, interp);

      buffer += c->shift_width;
      bufLen -= c->shift_width;

      if (res != TCL_OK)
	return res;
    }
    break;


  case TRF_OFB_MODE:
    /*
     * Output FeedBack.
     * Encrypt blocks of 'shift_width' bytes, each dependent on its predecessor.
     * Considered secure only for 'shift_width' being equal to bc_desc->block_size.
     */
    
    k = c->shift_width - c->byteCount;

    if (k > bufLen) {
      /*
       * We got less information than required to form a full block.
       * Extend the internal buffer and wait for more.
       */
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, bufLen);
      c->byteCount += bufLen;
      return TCL_OK;
    }

    if (k < c->shift_width) {
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, k);

      (*bc_desc->encryptProc) (c->iv, c->output, c->key);

      Trf_XorBuffer     (c->block, c->output, c->shift_width);
      Trf_ShiftRegister (c->iv,    c->output, c->shift_width, bc_desc->block_size);

      c->byteCount = 0;
      res = c->write (c->writeClientData, c->block, c->shift_width, interp);

      buffer += k;
      bufLen -= k;

      if (res != TCL_OK)
	return res;
    } /* k == c->shift_width => internal buffer was empty, so skip it entirely */

    /*
     * Process all blocks in the buffer.
     */

    while (bufLen >= c->shift_width) {
      (*bc_desc->encryptProc) (c->iv, c->output, c->key);

      Trf_XorBuffer     (buffer, c->output, c->shift_width);
      Trf_ShiftRegister (c->iv,    c->output, c->shift_width, bc_desc->block_size);

      res = c->write (c->writeClientData, buffer, c->shift_width, interp);

      buffer += c->shift_width;
      bufLen -= c->shift_width;

      if (res != TCL_OK)
	return res;
    }
    break;

  default:
    panic ("unknown mode given to blockcipher.EncodeBuffer");
    break;
  }


  /*
   * Remember incomplete data for next call.
   */
  
  if (bufLen > 0) {
    memcpy ((VOID*) c->block, (VOID*) buffer, bufLen);
    c->byteCount = bufLen;
  }

  return TCL_OK;
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
  EncoderControl*             c      = (EncoderControl*) ctrlBlock;
#if 0
  Trf_BlockcipherDescription* bc_desc = (Trf_BlockcipherDescription*) clientData;
#endif

  switch (c->operation_mode) {
  case TRF_ECB_MODE:
  case TRF_CFB_MODE:
  case TRF_OFB_MODE:
  case TRF_CBC_MODE:
    /*
     * No way to encrypt incomplete blocks!
     */

    if (c->byteCount > 0) {
      if (interp) {
	Tcl_AppendResult (interp,
			  "can not encrypt incomplete block at end of input",
			  (char*) NULL);
      }
      return TCL_ERROR;
    }
    break;

  default:
    panic ("illegal mode given to blockcipher.FlushEncoder");
    break;
  }

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
  EncoderControl*             c       = (EncoderControl*) ctrlBlock;
  Trf_BlockcipherDescription* bc_desc = (Trf_BlockcipherDescription*) clientData;

  c->byteCount = 0;
  memset ((VOID*) c->block,  '\0', bc_desc->block_size);

  /*
   ** Is problematic in stream modes, as the IV is
   * changed and cannot be restored (currently). Even
   * if restauration is available, this sort of reset has
   * its hazards.
   */
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
  TrfBlockcipherOptionBlock*        o = (TrfBlockcipherOptionBlock*) optInfo;
  Trf_BlockcipherDescription* bc_desc = (Trf_BlockcipherDescription*) clientData;
  EncoderControl*                   c;

  c = (EncoderControl*) Tcl_Alloc (sizeof (EncoderControl));
  c->write           = fun;
  c->writeClientData = writeClientData;

  InitializeState (c, o, TRF_DECRYPT, bc_desc);

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
  DecoderControl*             c       = (DecoderControl*) ctrlBlock;
  Trf_BlockcipherDescription* bc_desc = (Trf_BlockcipherDescription*) clientData;

  c->block [c->byteCount] = character;
  c->byteCount ++;

  switch (c->operation_mode) {
  case TRF_ECB_MODE:
    /*
     * Electronic Code Book.
     * Decrypt complete blocks only, each separately.
     */
    
    if (c->byteCount == bc_desc->block_size) {
      (*bc_desc->decryptProc) (c->block, c->output, c->key);

      c->byteCount = 0;
      return c->write (c->writeClientData, c->output, bc_desc->block_size, interp);
    }
    break;

  case TRF_CBC_MODE:
    /*
     * Cipher Block Chaining.
     * Decrypt complete blocks only, each dependent on its predecessor.
     */
    
    if (c->byteCount == bc_desc->block_size) {
      int res;

      (*bc_desc->decryptProc) (c->block, c->output, c->key);

      Trf_XorBuffer (c->output, c->iv, bc_desc->block_size);

      c->byteCount = 0;
      res = c->write (c->writeClientData, c->output, bc_desc->block_size, interp);

      /* use ENCRYPTED block as new IV !! */
      memcpy ((VOID*) c->iv, (VOID*) c->block, bc_desc->block_size);

      return res;
    }
    break;

  case TRF_CFB_MODE:
    /*
     * Cipher FeedBack.
     * Encrypt blocks of 'shift_width' bytes, each dependent on its predecessor.
     */
    
    if (c->byteCount == c->shift_width) {
      (*bc_desc->encryptProc) (c->iv, c->output, c->key);

      Trf_ShiftRegister (c->iv,    c->block,  c->shift_width, bc_desc->block_size);
      Trf_XorBuffer     (c->block, c->output, c->shift_width);

      c->byteCount = 0;
      return c->write (c->writeClientData, c->block, c->shift_width, interp);
    }
    break;

  case TRF_OFB_MODE:
    /*
     * Output FeedBack.
     * Encrypt blocks of 'shift_width' bytes, each dependent on its predecessor.
     * Considered secure only for 'shift_width' being equal to ;bc_desc->block_size'.
     */
    
    if (c->byteCount == c->shift_width) {
      (*bc_desc->encryptProc) (c->iv, c->output, c->key);

      Trf_XorBuffer     (c->block, c->output, c->shift_width);
      Trf_ShiftRegister (c->iv,    c->output, c->shift_width, bc_desc->block_size);

      c->byteCount = 0;
      return c->write (c->writeClientData, c->block, c->shift_width, interp);
    }
    break;

  default:
    panic ("unknown mode given to blockcipher.Decode");
    break;
  }

  return TCL_OK;
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
 *		As of the called writeFun.
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
  DecoderControl*             c       = (DecoderControl*) ctrlBlock;
  Trf_BlockcipherDescription* bc_desc = (Trf_BlockcipherDescription*) clientData;

  int res, k;


  switch (c->operation_mode) {
  case TRF_ECB_MODE:
    /*
     * Electronic Code Book.
     * Decrypt complete blocks only, each separately.
     */
    
    k = bc_desc->block_size - c->byteCount;

    if (k > bufLen) {
      /*
       * We got less information than required to form a full block.
       * Extend the internal buffer and wait for more.
       */
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, bufLen);
      c->byteCount += bufLen;
      return TCL_OK;
    }

    if (k < bc_desc->block_size) {
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, k);

      (*bc_desc->decryptProc) (c->block, c->output, c->key);

      buffer += k;
      bufLen -= k;

      c->byteCount = 0;

      res = c->write (c->writeClientData, c->output, bc_desc->block_size, interp);

      if (res != TCL_OK)
	return res;
    } /* k == bc_desc->block_size => internal buffer was empty, so skip it entirely */

    /*
     * Process all blocks in the buffer.
     */

    while (bufLen >= bc_desc->block_size) {
      (*bc_desc->decryptProc) (buffer, c->output, c->key);

      res = c->write (c->writeClientData, c->output, bc_desc->block_size, interp);
      
      buffer += bc_desc->block_size;
      bufLen -= bc_desc->block_size;

      if (res != TCL_OK)
	return res;
    }
    break;


  case TRF_CBC_MODE:
    /*
     * Cipher Block Chaining.
     * Decrypt complete blocks only, each dependent on its predecessor.
     */
    
    k = bc_desc->block_size - c->byteCount;

    if (k > bufLen) {
      /*
       * We got less information than required to form a full block.
       * Extend the internal buffer and wait for more.
       */
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, bufLen);
      c->byteCount += bufLen;
      return TCL_OK;
    }

    if (k < bc_desc->block_size) {
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, k);

      (*bc_desc->decryptProc) (c->block, c->output, c->key);

      Trf_XorBuffer (c->output, c->iv, bc_desc->block_size);

      buffer += k;
      bufLen -= k;

      c->byteCount = 0;

      res = c->write (c->writeClientData, c->output, bc_desc->block_size, interp);

      /* use ENCRYPTED block as new IV !! */
      memcpy ((VOID*) c->iv, (VOID*) c->block, bc_desc->block_size);

      if (res != TCL_OK)
	return res;
    } /* k == bc_desc->block_size => internal buffer was empty, so skip it entirely */

    /*
     * Process all blocks in the buffer.
     */

    while (bufLen >= bc_desc->block_size) {
      (*bc_desc->decryptProc) (buffer, c->output, c->key);

      Trf_XorBuffer (c->output, c->iv, bc_desc->block_size);

      res = c->write (c->writeClientData, c->output, bc_desc->block_size, interp);

      /* use ENCRYPTED block as new IV !! */
      memcpy ((VOID*) c->iv, (VOID*) buffer, bc_desc->block_size);

      buffer += bc_desc->block_size;
      bufLen -= bc_desc->block_size;

      if (res != TCL_OK)
	return res;
    }
    break;


  case TRF_CFB_MODE:
    /*
     * Cipher FeedBack.
     * Encrypt blocks of 'shift_width' bytes, each dependent on its predecessor.
     */
    
    k = c->shift_width - c->byteCount;

    if (k > bufLen) {
      /*
       * We got less information than required to form a full block.
       * Extend the internal buffer and wait for more.
       */
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, bufLen);
      c->byteCount += bufLen;
      return TCL_OK;
    }

    if (k < c->shift_width) {
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, k);

      (*bc_desc->encryptProc) (c->iv, c->output, c->key);

      Trf_ShiftRegister (c->iv,    c->block,  c->shift_width, bc_desc->block_size);
      Trf_XorBuffer     (c->block, c->output, c->shift_width);

      buffer += k;
      bufLen -= k;

      c->byteCount = 0;

      res = c->write (c->writeClientData, c->output, bc_desc->block_size, interp);

      if (res != TCL_OK)
	return res;
    } /* k == bc_desc->block_size => internal buffer was empty, so skip it entirely */

    /*
     * Process all blocks in the buffer.
     */

    while (bufLen >= c->shift_width) {
      (*bc_desc->encryptProc) (c->iv, c->output, c->key);

      Trf_ShiftRegister (c->iv,  buffer,    c->shift_width, bc_desc->block_size);
      Trf_XorBuffer     (buffer, c->output, c->shift_width);

      res = c->write (c->writeClientData, buffer, c->shift_width, interp);

      buffer += c->shift_width;
      bufLen -= c->shift_width;

      if (res != TCL_OK)
	return res;
    }
    break;


  case TRF_OFB_MODE:
    /*
     * Output FeedBack.
     * Encrypt blocks of 'shift_width' bytes, each dependent on its predecessor.
     * Considered secure only for 'shift_width' being equal to ;bc_desc->block_size'.
     */
    
    k = c->shift_width - c->byteCount;

    if (k > bufLen) {
      /*
       * We got less information than required to form a full block.
       * Extend the internal buffer and wait for more.
       */
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, bufLen);
      c->byteCount += bufLen;
      return TCL_OK;
    }

    if (k < c->shift_width) {
      memcpy ((VOID*) (c->block + c->byteCount), (VOID*) buffer, k);

      (*bc_desc->encryptProc) (c->iv, c->output, c->key);

      Trf_XorBuffer     (c->block, c->output, c->shift_width);
      Trf_ShiftRegister (c->iv,    c->output, c->shift_width, bc_desc->block_size);

      buffer += k;
      bufLen -= k;

      c->byteCount = 0;

      res = c->write (c->writeClientData, c->output, bc_desc->block_size, interp);

      if (res != TCL_OK)
	return res;
    } /* k == bc_desc->block_size => internal buffer was empty, so skip it entirely */

    /*
     * Process all blocks in the buffer.
     */

    while (bufLen >= c->shift_width) {
      (*bc_desc->encryptProc) (c->iv, c->output, c->key);

      Trf_XorBuffer     (buffer, c->output, c->shift_width);
      Trf_ShiftRegister (c->iv,  c->output, c->shift_width, bc_desc->block_size);

      res = c->write (c->writeClientData, buffer, c->shift_width, interp);

      buffer += c->shift_width;
      bufLen -= c->shift_width;

      if (res != TCL_OK)
	return res;
    }
    break;

  default:
    panic ("unknown mode given to blockcipher.Encode");
    break;
  }


  /*
   * Remember incomplete data for next call.
   */
  
  if (bufLen > 0) {
    memcpy ((VOID*) c->block, (VOID*) buffer, bufLen);
    c->byteCount = bufLen;
  }

  return TCL_OK;
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
  DecoderControl* c = (DecoderControl*) ctrlBlock;

  /* execute algorithm specific code here (XXX) */

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
  DecoderControl* c = (DecoderControl*) ctrlBlock;

  /* execute algorithm specific code here (XXX) */
}
#endif



static void
InitializeState (c, o, direction, bc_desc)
EncoderControl*             c;
TrfBlockcipherOptionBlock*  o;
int                         direction;
Trf_BlockcipherDescription* bc_desc;
{
  c->operation_mode = o->operation_mode;
  c->shift_width    = o->shift_width;
  c->byteCount      = 0;
  c->iv             = NULL;

  c->block  = (unsigned char*) Tcl_Alloc (bc_desc->block_size);
  c->output = (unsigned char*) Tcl_Alloc (bc_desc->block_size);

  memset ((VOID*) c->block,  '\0', bc_desc->block_size);
  memset ((VOID*) c->output, '\0', bc_desc->block_size);

  if (o->operation_mode >= TRF_CBC_MODE) {
    /*
     * Create and load IV.
     */

    c->iv = (unsigned char*) Tcl_Alloc (bc_desc->block_size);
    memcpy ((VOID*) c->iv, o->iv, bc_desc->block_size);
  }

  /*
   * Create and load keyschedule
   * (dependent on direction and operation_mode)
   *
   * direction  |	ENCRYPT	DECRYPT
   * -----------+----------------------
   * EBC	|	E	D
   * CBC	|	E	D
   * CFB	|	E	E
   * OFB	|	E	E
   */

  c->key = (unsigned char*) Tcl_Alloc (bc_desc->ks_size);

  if (o->operation_mode >= TRF_CFB_MODE) {
    direction = TRF_ENCRYPT;
  }

  if (direction == TRF_ENCRYPT) {
    if (o->encrypt_keyschedule == NULL) {
      (*bc_desc->scheduleProc) (o->key, o->key_length, direction,
				&(o->encrypt_keyschedule),
				&(o->decrypt_keyschedule));
      o->eks_length = bc_desc->ks_size;

      /* if decrypt_schedule was created too, then don't forget
       * to set the length information
       */

      if (o->decrypt_keyschedule != NULL) {
	o->dks_length = bc_desc->ks_size;
      }
    }

    memcpy ((VOID*) c->key, o->encrypt_keyschedule, bc_desc->ks_size);

  } else if (direction == TRF_DECRYPT) {
    if (o->decrypt_keyschedule == NULL) {
      (*bc_desc->scheduleProc) (o->key, o->key_length, direction,
				&(o->encrypt_keyschedule),
				&(o->decrypt_keyschedule));
      o->dks_length = bc_desc->ks_size;

      /* if encrypt_schedule was created too, then don't forget
       * to set the length information
       */

      if (o->encrypt_keyschedule != NULL) {
	o->eks_length = bc_desc->ks_size;
      }
    }

    memcpy ((VOID*) c->key, o->decrypt_keyschedule, bc_desc->ks_size);
  } else {
    panic ("wrong direction given to blockcipher.InitializeState");
  }
}

