/*
 * util.c --
 *
 *	Implements helper procedures used by 3->4 encoders (uu, base64)
 *	and other useful things.
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

static void
Split _ANSI_ARGS_ ((CONST char* in, char* out));


/*
 *------------------------------------------------------*
 *
 *	TrfSplit3to4 --
 *
 *	------------------------------------------------*
 *	Splits every 3 bytes of input into 4 bytes,
 *	actually 6-bit values and places them in the
 *	target.  Padding at the end is done with a value
 *	of '64' (6 bit -> values in range 0..63).
 *	This feature is used by 'TrfApplyEncoding'.
 *	'length' must be in the range 1, ..., 3.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		'out' is changed.
 *
 *	Result:
 *		See above.
 *		
 *
 *------------------------------------------------------*
 */

void
TrfSplit3to4 (in, out, length)
CONST unsigned char*  in;
unsigned char*       out;
int               length;
{
  if (length == 3) {
    Split ((char*) in, (char*) out);
  } else {
    char buf [3];

    /* expand incomplete sequence with with '\0' */
    memset (buf, '\0', 3);
    memcpy (buf, in,   length);

    Split (buf, (char*) out);

    switch (length) {
    case 1:
      out [2] = 64;
      out [3] = 64;
      break;

    case 2:
      out [3] = 64;
      break;

    case 0:
    default:
      /* should not happen */
      panic ("illegal length given to TrfSplit3to4");
    }
  }
}

/*
 *------------------------------------------------------*
 *
 *	TrfMerge4to3 --
 *
 *	------------------------------------------------*
 *	takes 4 bytes from 'in' (6-bit values) and
 *	merges them into 3 bytes (8-bit values).
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		The generated bytes are written to 'out'.
 *
 *	Results:
 *		See above.
 *
 *------------------------------------------------------*
 */

void
TrfMerge4to3 (in, out)
CONST unsigned char* in;
unsigned char*       out;
{
#define	MASK(i,by,mask) ((in [i] by) & mask)

  /*
   * use temp storage to prevent problems in case of
   * 'in', 'out' overlapping each other.
   */

  unsigned char o1, o2, o3;

  o1 = MASK (0, << 2, 0374) | MASK (1, >> 4, 003);
  o2 = MASK (1, << 4, 0360) | MASK (2, >> 2, 017);
  o3 = MASK (2, << 6, 0300) | MASK (3, >> 0, 077);

  out [0] = o1;
  out [1] = o2;
  out [2] = o3;

#undef MASK
}

/*
 *------------------------------------------------------*
 *
 *	 --
 *
 *	------------------------------------------------*
 *	transform 6-bit values into real characters
 *	according to the specified character-mapping.
 *	The map HAS TO contain at least 65 characters,
 *	the last one being the PAD character to use.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		The characters are read from and written
 *		to 'buf'.
 *
 *	Results:
 *		See above.
 *
 *------------------------------------------------------*
 */

void
TrfApplyEncoding (buf, length, map)
unsigned char* buf;
int            length;
CONST char*    map;
{
  int i;

  for (i=0; i < length; i++) {
    buf [i] = map [buf [i]];
  }
}

/*
 *------------------------------------------------------*
 *
 *	TrfReverseEncoding --
 *
 *	------------------------------------------------*
 *	The given string is converted in place into its
 *	equivalent binary representation.  The procedure
 *	assummes the string to be encoded with a 3->4
 *	byte scheme (such as uuencding, base64).
 *
 *	The map HAS TO contain at least 256 characters.
 *      It is indexed by an 8 bit value to get the 6-bit
 *	binary field corresponding to that value.  Any
 *	illegal characters must have the high bit set.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		The characters are read from and written
 *		to 'buf'.
 *
 *	Result:
 *		A standard TCL error code
 *		'hasPadding' returns the number unused
 *		bytes in buf (0..3).
 *
 *------------------------------------------------------*
 */

int
TrfReverseEncoding (buf, length, reverseMap, padChar, hasPadding)
unsigned char* buf;
int            length;
CONST char*    reverseMap;
unsigned int   padChar;
int*           hasPadding;
{
  /*
   * length has to be in the range 1..4.
   */

  int i, pad, maplen;

  if ((length < 1) || (length > 4))
    panic ("illegal length given to TrfReverseEncoding");

  pad = 4 - length;

  /* check for more pad chars */

  for (i=length-1;
       (i >= 0) && (padChar == buf [i]);
       pad++, i--) {
    buf [i] = '\0';
  }

  if (pad > 2)
    /*
     * Only xxxx, xxx= and xx== allowed
     * (with x as legal character and = as pad-char.
     */
    return TCL_ERROR;

  *hasPadding = pad;

  maplen = i+1;

  /* convert characters to 6-bit values */

  for (i=0; i < maplen; i++) {
    char tmp = reverseMap [buf [i]];

    if (tmp & 0x80)
      /* high-bit set? => illegal character */
      return TCL_ERROR;

    buf [i] = tmp;
  }

  return TCL_OK;
}

/*
 *------------------------------------------------------*
 *
 *	Split --
 *
 *	------------------------------------------------*
 *	takes 3 bytes from 'in', splits them into
 *	4 6-bit values and places them then into 'out'.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		The generated characters are written to
 *		'out'.
 *
 *	Results:
 *		See above.
 *
 *------------------------------------------------------*
 */

static void
Split (in, out)
CONST char* in;
char*       out;
{
  out [0] = (077 &   (in [0] >> 2));
  out [1] = (077 & (((in [0] << 4) & 060) | ((in [1] >> 4) & 017)));
  out [2] = (077 & (((in [1] << 2) & 074) | ((in [2] >> 6) &  03)));
  out [3] = (077 &   (in [2] & 077));
}

/*
 *------------------------------------------------------*
 *
 *	Trf_XorBuffer --
 *
 *	------------------------------------------------*
 *	Do an 'exclusive or' of all 'length' bytes in
 *	'buffer' with the corresponding bytes in 'mask'.
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

void
Trf_XorBuffer (buffer, mask, length)
VOID* buffer;
VOID* mask;
int   length;
{
  unsigned char* b = (unsigned char*) buffer;
  unsigned char* m = (unsigned char*) mask;

  while (length > 0) {
    *b++ ^= *m++;
    length --;
  }
}

/*
 *------------------------------------------------------*
 *
 *	Trf_ShiftRegister --
 *
 *	------------------------------------------------*
 *	Take the 'shift' leftmost bytes of 'mask' and
 *	shift them into the rightmost bytes of 'buffer'.
 *	The leftmost bytes of 'buffer' are lost.  Both
 *	buffers are assumed to be of size 'length'.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		See above.
 *
 *	Result:
 *		none.
 *
 *------------------------------------------------------*
 */

void
Trf_ShiftRegister (buffer, mask, shift, length)
VOID* buffer;
VOID* mask;
int   shift;
int   length;
{
  assert (shift > 0);

  if (shift == length) {
      /*
       * Special case: Drop the whole old register.
       */

    memcpy (buffer, mask, length);
  } else {
    unsigned char* b = (unsigned char*) buffer;
    unsigned char* m = (unsigned char*) mask;

    int retained;

    /* number bytes in 'buffer' to retain */
    retained = length - shift;

    /* left-shift retained bytes of 'buffer' over by
     * 'shift' bytes to create space for new bytes
     */

    while (retained --) {
      *b = *(b + shift);
      b ++;
    }

    /* now copy 'shift' bytes from 'input' to shifted tail of 'buffer' */
    do {
      *b++ = *m++;
    } while (--shift);
  }
}

/*
 *------------------------------------------------------*
 *
 *	Trf_FlipRegisterShort --
 *
 *	------------------------------------------------*
 *	Swap the bytes for all 2-Byte words contained in
 *	the register.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		See above.
 *
 *	Result:
 *		none.
 *
 *------------------------------------------------------*
 */

void
Trf_FlipRegisterShort (buffer, length)
VOID* buffer;
int   length;
{
  unsigned char  tmp;
  unsigned char* b = (unsigned char*) buffer;
  int n_shorts     = length / 2;
  int i;
  
  for (i=0; i < n_shorts; i++, b+= 2) {
    tmp = b [0]; b [0] = b [1]; b [1] = tmp;
  }
}

/*
 *------------------------------------------------------*
 *
 *	Trf_FlipRegisterLong --
 *
 *	------------------------------------------------*
 *	Swap the bytes for all 4-Byte words contained in
 *	the register.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		See above.
 *
 *	Result:
 *		none.
 *
 *------------------------------------------------------*
 */

void
Trf_FlipRegisterLong (buffer, length)
VOID* buffer;
int   length;
{
  unsigned char  tmp;
  unsigned char* b = (unsigned char*) buffer;
  int n_longs      = length / 4;
  int i;
  
  /*
   * 0 -> 3
   * 1 -> 2
   * 2 -> 1
   * 3 -> 0
   */

  for (i=0; i < n_longs; i++, b+= 4) {
    tmp = b [0]; b [0] = b [3]; b [3] = tmp;
    tmp = b [1]; b [1] = b [2]; b [2] = tmp;
  }
}

/* internal procedures, for testing */

void
TrfDumpHex (f, buffer, length, next)
FILE* f;
VOID* buffer;
int   length;
int   next;
{
  short i;
  unsigned char* b = (unsigned char*) buffer;

  for (i=0; i < length; i++) fprintf (f, "%02x", (unsigned int) (b [i]));
  switch (next) {
  case 0: break;
  case 1: fprintf (f, "   ");  break;
  case 2: fprintf (f, "\n"); break;
  }
}


void
TrfDumpShort (f, buffer, length, next)
FILE* f;
VOID* buffer;
int   length;
int   next;
{
  short i;
  unsigned short* b = (unsigned short*) buffer;

  for (i=0; i < (length/2); i++) fprintf (f, "%06d ", (unsigned int) (b [i]));
  switch (next) {
  case 0: break;
  case 1: fprintf (f, "   ");  break;
  case 2: fprintf (f, "\n"); break;
  }
}
