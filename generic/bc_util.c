/*
 * bc_util.c --
 *
 *	Utilities for blockciphers implementation.
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
