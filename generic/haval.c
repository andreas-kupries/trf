/*
 * haval.c --
 *
 *	Implements and registers message digest generator HAVAL.
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
#include "haval/haval.h"

/*
 * Generator description
 * ---------------------
 *
 * The HAVAL alogrithm is used to compute a cryptographically strong
 * message digest.
 */

#define DIGEST_SIZE               (32)
#define CTX_TYPE                  haval_state

/*
 * Declarations of internal procedures.
 */

static void MD_Start     _ANSI_ARGS_ ((VOID* context));
static void MD_Update    _ANSI_ARGS_ ((VOID* context, unsigned int character));
static void MD_UpdateBuf _ANSI_ARGS_ ((VOID* context, unsigned char* buffer, int bufLen));
static void MD_Final     _ANSI_ARGS_ ((VOID* context, VOID* digest));

/*
 * Generator definition.
 */

static Trf_MessageDigestDescription mdDescription = {
  "haval",
  sizeof (CTX_TYPE),
  DIGEST_SIZE,
  MD_Start,
  MD_Update,
  MD_UpdateBuf,
  MD_Final,
  NULL
};

/*
 *------------------------------------------------------*
 *
 *	TrfInit_HAVAL --
 *
 *	------------------------------------------------*
 *	Register the generator implemented in this file.
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
TrfInit_HAVAL (interp)
Tcl_Interp* interp;
{
  return Trf_RegisterMessageDigest (interp, &mdDescription);
}

/*
 *------------------------------------------------------*
 *
 *	MD_Start --
 *
 *	------------------------------------------------*
 *	Initialize the internal state of the message
 *	digest generator.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of the called procedure.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
MD_Start (context)
VOID* context;
{
  haval_start ((CTX_TYPE*) context);
}

/*
 *------------------------------------------------------*
 *
 *	MD_Update --
 *
 *	------------------------------------------------*
 *	Update the internal state of the message digest
 *	generator for a single character.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of the called procedure.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
MD_Update (context, character)
VOID* context;
unsigned int   character;
{
  unsigned char buf = character;

  haval_hash ((CTX_TYPE*) context, &buf, 1);
}

/*
 *------------------------------------------------------*
 *
 *	MD_UpdateBuf --
 *
 *	------------------------------------------------*
 *	Update the internal state of the message digest
 *	generator for a character buffer.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of the called procedure.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
MD_UpdateBuf (context, buffer, bufLen)
VOID* context;
unsigned char* buffer;
int   bufLen;
{
  haval_hash ((CTX_TYPE*) context, (unsigned char*) buffer, bufLen);
}

/*
 *------------------------------------------------------*
 *
 *	MD_Final --
 *
 *	------------------------------------------------*
 *	Generate the digest from the internal state of
 *	the message digest generator.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of the called procedure.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
MD_Final (context, digest)
VOID* context;
VOID* digest;
{
  haval_end ((CTX_TYPE*) context, (unsigned char*) digest);
}

/*
 * External code from here on.
 */

#include "haval/haval.c"
