/*
 * uucode.c --
 *
 *	Implements and registers conversion from and to uuencoded representation.
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
 * Converter description
 * ---------------------
 *
 * Encoding:
 *	Each sequence of 3 bytes is expanded into 4 printable characters
 *	using the 4 6bit-sequences contained in the 3 bytes. The mapping
 *	from 6bit value to printable characters is done with the UU map.
 *	Special processing is done for incomplete byte sequences at the
 *	end of the input (1,2 bytes).
 *
 * Decoding:
 *	Each sequence of 4 characters is mapped into 4 6bit values using
 *	the reverse UU map and then concatenated to form 3 8bit bytes.
 *	Special processing is done for incomplete character sequences at
 *	the end of the input (1,2,3 bytes).
 */


/*
 * Declarations of internal procedures.
 */

static Trf_ControlBlock CreateEncoder  _ANSI_ARGS_ ((ClientData writeClientData, Trf_WriteProc *fun,
						     Trf_Options optInfo, Tcl_Interp*   interp,
						     ClientData clientData));
static void             DeleteEncoder  _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     ClientData clientData));
static int              Encode         _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     unsigned int character,
						     Tcl_Interp* interp,
						     ClientData clientData));
static int              FlushEncoder   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     Tcl_Interp* interp,
						     ClientData clientData));
static void             ClearEncoder   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     ClientData clientData));


static Trf_ControlBlock CreateDecoder  _ANSI_ARGS_ ((ClientData writeClientData, Trf_WriteProc *fun,
						     Trf_Options optInfo, Tcl_Interp*   interp,
						     ClientData clientData));
static void             DeleteDecoder  _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     ClientData clientData));
static int              Decode         _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     unsigned int character,
						     Tcl_Interp* interp,
						     ClientData clientData));
static int              FlushDecoder   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     Tcl_Interp* interp,
						     ClientData clientData));
static void             ClearDecoder   _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						     ClientData clientData));


/*
 * Converter definition.
 */

static Trf_TypeDefinition convDefinition =
{
  "uuencode",
  NULL, /* clientData not used by converters */
  NULL, /* set later by Trf_InitUU */
  {
    CreateEncoder,
    DeleteEncoder,
    Encode,
    NULL,
    FlushEncoder,
    ClearEncoder
  }, {
    CreateDecoder,
    DeleteDecoder,
    Decode,
    NULL,
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

  /* add conversion specific items here (uuencode) */

  unsigned char charCount;
  unsigned char buf [3];

} EncoderControl;


typedef struct _DecoderControl_ {
  Trf_WriteProc* write;
  ClientData     writeClientData;

  /* add conversion specific items here (uudecode) */

  unsigned char charCount;
  unsigned char buf [4];
  unsigned char expectFlush;

} DecoderControl;


/*
 * Character mapping for uuencode (bin -> ascii)
 *
 * Index this array by a 6-bit value to obtain the corresponding
 * 8-bit character. The last character (index 64) is the pad char (~)
 *                                                                                              |
 *                                       1         2         3         4         5          6   6
 *                            01 2345678901234567890123456789012345678901234567890123456789 01234 */
static CONST char* uuMap   = "`!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_~";

#define PAD '~'

/*
 * Character mappings for uudecode (ascii -> bin)
 *
 * Index this array by a 8 bit value to get the 6-bit binary field
 * corresponding to that value.  Any illegal characters have high bit set.
 */

static CONST char uuMapReverse [] = {
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0001,0002,0003,0004,0005,0006,0007,
	0010,0011,0012,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0024,0025,0026,0027,
	0030,0031,0032,0033,0034,0035,0036,0037,
	0040,0041,0042,0043,0044,0045,0046,0047,
	0050,0051,0052,0053,0054,0055,0056,0057,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0072,0073,0074,0075,0076,0077,
	0000,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	/* */
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200,
	0200,0200,0200,0200,0200,0200,0200,0200
};


/*
 *------------------------------------------------------*
 *
 *	TrfInit_UU --
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
TrfInit_UU (interp)
Tcl_Interp* interp;
{
  convDefinition.options = Trf_ConverterOptions ();

  return Trf_Register (interp, &convDefinition);
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
ClientData clientData;
{
  EncoderControl* c;

  c = (EncoderControl*) Tcl_Alloc (sizeof (EncoderControl));
  c->write           = fun;
  c->writeClientData = writeClientData;

  /* initialize conversion specific items here (uuencode) */

  c->charCount = 0;
  memset (c->buf, '\0', 3);

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

  /* release conversion specific items here (uuencode) */

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
unsigned int character;
Tcl_Interp* interp;
ClientData clientData;
{
  EncoderControl* c = (EncoderControl*) ctrlBlock;

  /* execute conversion specific code here (uuencode) */

  c->buf [c->charCount] = character;
  c->charCount ++;

  if (c->charCount == 3) {
    unsigned char buf [4];

    TrfSplit3to4     (c->buf, buf, 3);
    TrfApplyEncoding (buf, 4, uuMap);

    c->charCount = 0;
    memset (c->buf, '\0', 3);

    return c->write (c->writeClientData, buf, 4, interp);
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
  EncoderControl* c = (EncoderControl*) ctrlBlock;

  /* execute conversion specific code here (uuencode) */

  if (c->charCount > 0) {
    unsigned char buf [4];

    TrfSplit3to4     (c->buf, buf, c->charCount);
    TrfApplyEncoding (buf, 4, uuMap);

    c->charCount = 0;
    memset (c->buf, '\0', 3);

    return c->write (c->writeClientData, buf, 4, interp);
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
ClientData clientData;
{
  EncoderControl* c = (EncoderControl*) ctrlBlock;

  /* execute conversion specific code here (uuencode) */

  c->charCount = 0;
  memset (c->buf, '\0', 3);
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
ClientData clientData;
{
  DecoderControl* c;

  c = (DecoderControl*) Tcl_Alloc (sizeof (DecoderControl));
  c->write           = fun;
  c->writeClientData = writeClientData;

  /* initialize conversion specific items here (uudecode) */

  c->charCount = 0;
  memset (c->buf, '\0', 4);
  c->expectFlush = 0;

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

  /* release conversion specific items here (uudecode) */

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
unsigned int character;
Tcl_Interp* interp;
ClientData clientData;
{
  DecoderControl* c = (DecoderControl*) ctrlBlock;

  /* execute conversion specific code here (uudecode) */

  if (c->expectFlush) {
    /*
     * We had a quadruple with pad characters at the last call,
     * this had to be the last characters in input!  coming here
     * now indicates, that the padding characters were in the
     * middle of the string, therefore illegal.
     */

    if (interp) {
      ADD_RES (interp, "illegal padding inside the string");
    }
    return TCL_ERROR;
  }


  c->buf [c->charCount] = character;
  c->charCount ++;

  if (c->charCount == 4) {
    int res, hasPadding;
    unsigned char buf [3];

    hasPadding = 0;
    res = TrfReverseEncoding (c->buf, 4, uuMapReverse,
			      PAD, &hasPadding);

    if (res != TCL_OK) {
      if (interp) {
	ADD_RES (interp, "illegal characters in string");
      }
      return res;
    }

    if (hasPadding)
      c->expectFlush = 1;

    TrfMerge4to3 (c->buf, buf);

    c->charCount = 0;
    memset (c->buf, '\0', 4);

    return c->write (c->writeClientData, buf, 3-hasPadding, interp);
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
Tcl_Interp* interp;
ClientData clientData;
{
  DecoderControl* c = (DecoderControl*) ctrlBlock;

  /* execute conversion specific code here (uudecode) */

  /*
   * expectFlush && c->charcount > 0 impossible, catched
   * in 'Decode' already.
   */

  if (c->charCount > 0) {
    /*
     * Convert, as if padded with the pad-character.
     */

    int res, hasPadding;
    unsigned char buf [3];

    hasPadding = 0;
    res = TrfReverseEncoding (c->buf, c->charCount, uuMapReverse,
			      PAD, &hasPadding);

    if (res != TCL_OK) {
      if (interp) {
	ADD_RES (interp, "illegal characters in string");
      }
      return res;
    }

    TrfMerge4to3 (c->buf, buf);

    c->charCount = 0;
    memset (c->buf, '\0', 4);

    return c->write (c->writeClientData, buf, 3-hasPadding, interp);
  }

  return TCL_OK;
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

  /* execute conversion specific code here (uudecode) */

  c->charCount = 0;
  memset (c->buf, '\0', 4);
  c->expectFlush = 0;
}
