/*
 * digest.c --
 *
 *	Implements and registers code common to all message digests.
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
static void             DeleteDecoder  _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     ClientData clientData));
static int              Decode         _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     unsigned int character,
						     Tcl_Interp* interp,
						     ClientData clientData));
static int              DecodeBuffer   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     unsigned char* buffer, int bufLen,
						     Tcl_Interp* interp,
						     ClientData clientData));
static int              FlushDecoder   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     Tcl_Interp* interp,
						     ClientData clientData));
static void             ClearDecoder   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     ClientData clientData));


/*
 * Generator definition.
 */

static Trf_TypeDefinition mdDefinition =
{
  NULL, /* filled later by Trf_RegisterMessageDigest (in a copy) */
  NULL, /* filled later by Trf_RegisterMessageDigest (in a copy) */
  NULL, /* filled later by Trf_RegisterMessageDigest (in a copy) */
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
 * Definition of the control blocks for en- and decoder.
 */

typedef struct _EncoderControl_ {
  Trf_WriteProc* write;
  ClientData     writeClientData;

  int            operation_mode;
  Tcl_Channel    dest;           /* target for ATTACH_WRITE. possibly NULL */
  VOID*          context;

} EncoderControl;

#define IMMEDIATE     (0)
#define ATTACH_ABSORB (1)
#define ATTACH_WRITE  (2)


typedef struct _DecoderControl_ {
  Trf_WriteProc* write;
  ClientData     writeClientData;

  int            operation_mode;
  Tcl_Channel    dest;           /* target for ATTACH_WRITE. possibly NULL  */
  VOID*          context;

  char*          matchFlag;      /* target for ATTACH_ABSORB */
  Tcl_Interp*    mfInterp;       /* interpreter containing matchFlag */

  unsigned char* digest_buffer;
  short          buffer_pos;
  unsigned short charCount;

} DecoderControl;




/*
 *------------------------------------------------------*
 *
 *	Trf_RegisterMessageDigest --
 *
 *	------------------------------------------------*
 *	Register the specified generator as transformer.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Allocates memory. As of 'Trf_Register'.
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

EXTERN int
Trf_RegisterMessageDigest (interp, md_desc)
Tcl_Interp*                         interp;
CONST Trf_MessageDigestDescription* md_desc;
{
  Trf_TypeDefinition* md;

  md = (Trf_TypeDefinition*) Tcl_Alloc (sizeof (Trf_TypeDefinition));

  memcpy ((VOID*) md, (VOID*) &mdDefinition, sizeof (Trf_TypeDefinition));

  md->name       = md_desc->name;
  md->clientData = (ClientData) md_desc;
  md->options    = TrfMDOptions ();

  return Trf_Register (interp, md);

  /* 'md' is a memory leak, it will never be released.
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
  EncoderControl*                c;
  TrfMDOptionBlock*              o = (TrfMDOptionBlock*) optInfo;
  Trf_MessageDigestDescription* md = (Trf_MessageDigestDescription*) clientData;

  c = (EncoderControl*) Tcl_Alloc (sizeof (EncoderControl));
  c->write           = fun;
  c->writeClientData = writeClientData;

  c->operation_mode = (o->behaviour == TRF_IMMEDIATE ?
		       IMMEDIATE :
		       (o->mode == TRF_ABSORB_HASH ?
			ATTACH_ABSORB :
			ATTACH_WRITE));

  c->dest = (c->operation_mode == ATTACH_WRITE ?
	     o->writeDest : (Tcl_Channel) NULL);

  /*
   * Create and initialize the context.
   */

  c->context = (VOID*) Tcl_Alloc (md->context_size);
  (*md->startProc) (c->context);

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
ClientData       clientData;
{
  EncoderControl* c = (EncoderControl*) ctrlBlock;

  Tcl_Free ((char*) c->context);
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
  EncoderControl*                c = (EncoderControl*) ctrlBlock;
  Trf_MessageDigestDescription* md = (Trf_MessageDigestDescription*) clientData;
  unsigned char                buf;

  buf = character;
  (*md->updateProc) (c->context, character);

  if (c->operation_mode == ATTACH_ABSORB) {
    /*
     * absorption mode: incoming characters flow unchanged through transformation.
     */

    return c->write (c->writeClientData, &buf, 1, interp);
  }

  return TCL_OK;
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
  EncoderControl*                c = (EncoderControl*) ctrlBlock;
  Trf_MessageDigestDescription* md = (Trf_MessageDigestDescription*) clientData;

  if (*md->updateBufProc) {
    (*md->updateBufProc) (c->context, buffer, bufLen);
  } else {
    unsigned int character, i;

    for (i=0; i < bufLen; i++) {
      character = buffer [i];
      (*md->updateProc) (c->context, character);
    }
  }

  if (c->operation_mode == ATTACH_ABSORB) {
    /*
     * absorption mode: incoming characters flow unchanged through transformation.
     */

    return c->write (c->writeClientData, buffer, bufLen, interp);
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
Tcl_Interp*      interp;
ClientData       clientData;
{
  EncoderControl*                c = (EncoderControl*) ctrlBlock;
  Trf_MessageDigestDescription* md = (Trf_MessageDigestDescription*) clientData;
  char*                     digest;
  int                          res = TCL_OK;

  digest = (char*) Tcl_Alloc (md->digest_size);
  (*md->finalProc) (c->context, digest);

  if (c->operation_mode == ATTACH_WRITE) {
    if (c->dest != (Tcl_Channel) NULL)
      /* Skip writing, if no channel available for result */
      res = Tcl_Write (c->dest, digest, md->digest_size);
    else
      res = 0;

    if (res < 0) {
      if (interp) {
	Tcl_AppendResult (interp, "error writing \"",
			  Tcl_GetChannelName (c->dest),
			  "\": ", Tcl_PosixError (interp),
			  (char *) NULL);
	res = TCL_ERROR;
      }
    }
  } else {
    /*
     * immediate execution or attached channel absorbs checksum.
     */

    res = c->write (c->writeClientData, digest, md->digest_size, interp);
  }

  Tcl_Free (digest);
  return res;
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
  EncoderControl*                c = (EncoderControl*) ctrlBlock;
  Trf_MessageDigestDescription* md = (Trf_MessageDigestDescription*) clientData;

  (*md->startProc) (c->context);
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
  DecoderControl*                c;
  TrfMDOptionBlock*              o = (TrfMDOptionBlock*) optInfo;
  Trf_MessageDigestDescription* md = (Trf_MessageDigestDescription*) clientData;

  c = (DecoderControl*) Tcl_Alloc (sizeof (DecoderControl));
  c->write           = fun;
  c->writeClientData = clientData;

  c->operation_mode = (o->mode == TRF_ABSORB_HASH ?
		       ATTACH_ABSORB :
		       ATTACH_WRITE);

  c->dest = (c->operation_mode == ATTACH_WRITE ?
	     o->readDest : (Tcl_Channel) NULL);

  c->matchFlag  = o->matchFlag;
  c->mfInterp   = o->mfInterp;
  o->matchFlag  = NULL;


  c->buffer_pos = 0;
  c->charCount  = 0;

  c->context = (VOID*) Tcl_Alloc (md->context_size);
  (*md->startProc) (c->context);

  c->digest_buffer = (unsigned char*) Tcl_Alloc (md->digest_size);
  memset (c->digest_buffer, '\0', md->digest_size);

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

  Tcl_Free ((char*) c->digest_buffer);
  Tcl_Free ((char*) c->context);
  Tcl_Free ((char*) c);
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
unsigned int     character;
Tcl_Interp*      interp;
ClientData       clientData;
{
  DecoderControl*                c = (DecoderControl*) ctrlBlock;
  Trf_MessageDigestDescription* md = (Trf_MessageDigestDescription*) clientData;
  char                         buf;

  if (c->operation_mode == ATTACH_WRITE) {
    buf = character;
    (*md->updateProc) (c->context, character);
  } else {
    if (c->charCount == md->digest_size) {
      /*
       * ringbuffer full, forward oldest character
       * and replace with new one.
       */

      buf = c->digest_buffer [c->buffer_pos];

      c->digest_buffer [c->buffer_pos] = character;
      c->buffer_pos ++;
      c->buffer_pos %= md->digest_size;

      character = buf;
      (*md->updateProc) (c->context, character);

      return c->write (c->writeClientData, &buf, 1, interp);
    } else {
      /*
       * Fill ringbuffer.
       */

      c->digest_buffer [c->buffer_pos] = character;

      c->buffer_pos ++;
      c->charCount  ++;
    }
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
  DecoderControl*                c = (DecoderControl*) ctrlBlock;
  Trf_MessageDigestDescription* md = (Trf_MessageDigestDescription*) clientData;

  if (c->operation_mode == ATTACH_WRITE) {

    if (*md->updateBufProc) {
      (*md->updateBufProc) (c->context, buffer, bufLen);
    } else {
      int character, i;

      for (i=0; i < bufLen; i++) {
	character = buffer [i];
	(*md->updateProc) (c->context, character);
      }
    }

  } else {
    /* try to use more than character at a time. */

    if (*md->updateBufProc != NULL) {

      /*
       * 2 cases:
       *
       * - Incoming buffer and data stored from previous calls are less
       *   or just enough to fill the ringbuffer. Copy the incoming bytes
       *   into the buffer and return.
       *
       * - Incoming data + ringbuffer contain more than reqired for a
       * digest. Concatenate both and use the oldest bytes to update the
       * hash context. Place the remaining 'digest_size' bytes into the
       * ringbuffer again.
       *
       * Both cases assume the ringbuffer data to be starting at index '0'.
       */

      if ((c->charCount + bufLen) <= md->digest_size) {
	/* extend ring buffer */

	memcpy ( (VOID*) (c->digest_buffer + c->charCount), (VOID*) buffer, bufLen);
	c->charCount += bufLen;
      } else {
	/*
	 * n contains the number of bytes we are allowed to hash into the context.
	 */

	int n = c->charCount + bufLen - md->digest_size;
	int res;

	if (c->charCount > 0) {
	  if (n <= c->charCount) {
	    /*
	     * update context, then shift down the remaining bytes
	     */

	    (*md->updateBufProc) (c->context, c->digest_buffer, n);

	    res = c->write (c->writeClientData, c->digest_buffer, n, interp);

	    memmove ((VOID*) c->digest_buffer,
		     (VOID*) (c->digest_buffer + n), c->charCount - n);
	    c->charCount -= n;
	    n             = 0;
	  } else {
	    /*
	     * Hash everything inside the buffer.
	     */

	    (*md->updateBufProc) (c->context, c->digest_buffer, c->charCount);

	    res = c->write (c->writeClientData, c->digest_buffer, c->charCount, interp);

	    n -= c->charCount;
	    c->charCount = 0;
	  }

	  if (res != TCL_OK)
	    return res;
	}

	if (n > 0) {
	  (*md->updateBufProc) (c->context, buffer, n);

	  res = c->write (c->writeClientData, buffer, n, interp);

	  memcpy ((VOID*) (c->digest_buffer + c->charCount),
		  (VOID*) (buffer + n),
		  (bufLen - n));
	  c->charCount = md->digest_size; /* <=> 'c->charCount += bufLen - n;' */

	  if (res != TCL_OK)
	    return res;
	}
      }
    } else {
      /*
       * no 'updateBufProc', simulate it using the
       * underlying single character routine
       */

      int character, i, res;
      char         buf;

      for (i=0; i < bufLen; i++) {
	buf       = c->digest_buffer [c->buffer_pos];
	character = buffer [i];

	if (c->charCount == md->digest_size) {
	  /*
	   * ringbuffer full, forward oldest character
	   * and replace with new one.
	   */

	  c->digest_buffer [c->buffer_pos] = character;
	  c->buffer_pos ++;
	  c->buffer_pos %= md->digest_size;

	  character = buf;
	  (*md->updateProc) (c->context, character);

	  res = c->write (c->writeClientData, &buf, 1, interp);
	  if (res != TCL_OK)
	    return res;
	} else {
	  /*
	   * Fill ringbuffer.
	   */
	
	  c->digest_buffer [c->buffer_pos] = character;

	  c->buffer_pos ++;
	  c->charCount  ++;
	}
      } /* for */
    }
  }

  return TCL_OK;
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
Tcl_Interp*      interp;
ClientData       clientData;
{
  DecoderControl*                c = (DecoderControl*) ctrlBlock;
  Trf_MessageDigestDescription* md = (Trf_MessageDigestDescription*) clientData;
  char* digest;
  int res= TCL_OK;

  digest = (char*) Tcl_Alloc (md->digest_size);

  (*md->finalProc) (c->context, digest);

  if (c->operation_mode == ATTACH_WRITE) {
    int resW;

    if (c->dest != (Tcl_Channel) NULL)
      /* Skip writing, if no channel available for result */
      resW = Tcl_Write (c->dest, (char*) digest, md->digest_size);
    else
      resW = 0;

    if (resW < 0) {
      if (interp) {
	Tcl_AppendResult (interp, "error writing \"",
			  Tcl_GetChannelName (c->dest),
			  "\": ", Tcl_PosixError (interp),
			  (char *) NULL);
      }

      res = TCL_ERROR;
    }
  } else if (c->charCount < md->digest_size) {
    /*
     * Not enough data in channel!
     */

    if (interp) {
      Tcl_AppendResult (interp, "not enough bytes in channel", (char*) NULL);
    }

    res = TCL_ERROR;
  } else {
    char* result_text;

    if (c->buffer_pos > 0) {
      /*
       * Reorder bytes in ringbuffer to form the correct digest.
       */

      char* temp;
      int i,j;

      temp = (char*) Tcl_Alloc (md->digest_size);

      for (i= c->buffer_pos, j=0;
	   j < md->digest_size;
	   i = (i+1) % md->digest_size, j++) {
	temp [j] = c->digest_buffer [i];
      }

      memcpy ((VOID*) &c->digest_buffer, (VOID*) temp, md->digest_size);
      Tcl_Free (temp);
    }

    /*
     * Compare computed and transmitted checksums
     */

    result_text = (0 == memcmp ((VOID*) digest, (VOID*) &c->digest_buffer, md->digest_size) ?
		   "ok" : "failed");

    Tcl_SetVar (c->mfInterp, c->matchFlag, result_text, TCL_GLOBAL_ONLY);
  }

  Tcl_Free (digest);
  return res;
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
ClientData       clientData;
{
  DecoderControl*                c = (DecoderControl*) ctrlBlock;
  Trf_MessageDigestDescription* md = (Trf_MessageDigestDescription*) clientData;

  c->buffer_pos = 0;
  c->charCount  = 0;

  (*md->startProc) (c->context);
  memset (c->digest_buffer, '\0', md->digest_size);
}
