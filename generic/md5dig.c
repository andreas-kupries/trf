/*
 * md5.c --
 *
 *	Implements and registers message digest generator MD5.
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
#include "loadman.h"
/*#include "md5/md5.h"*/

/*
 * Generator description
 * ---------------------
 *
 * The MD5 alogrithm is used to compute a cryptographically strong
 * message digest.
 */

#define MD5_CTX  struct md5_ctx
#define MD5_CTXP (struct md5_ctx*)

#ifndef OTP
#define DIGEST_SIZE               (16)
#else
#define DIGEST_SIZE               (8)
#endif
#define CTX_TYPE                  MD5_CTX

/*
 * Declarations of internal procedures.
 */

static void MD_Start     _ANSI_ARGS_ ((VOID* context));
static void MD_Update    _ANSI_ARGS_ ((VOID* context, unsigned int character));
static void MD_UpdateBuf _ANSI_ARGS_ ((VOID* context,
				       unsigned char* buffer, int bufLen));
static void MD_Final     _ANSI_ARGS_ ((VOID* context, VOID* digest));
static int  MD_Check     _ANSI_ARGS_ ((Tcl_Interp* interp));

/*
 * Generator definition.
 */

static Trf_MessageDigestDescription
mdDescription = { /* THREADING: constant, read-only => safe */
#ifndef OTP 
  "md5",
#else
  "otp_md5",
#endif
  sizeof (CTX_TYPE),
  DIGEST_SIZE,
  MD_Start,
  MD_Update,
  MD_UpdateBuf,
  MD_Final,
  MD_Check
};

/*
 *------------------------------------------------------*
 *
 *	TrfInit_MD5 --
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
#ifndef	OTP
TrfInit_MD5 (interp)
#else
TrfInit_OTP_MD5 (interp)
#endif
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
  /*  MD5Init ((MD5_CTX*) context);*/
  md5f.init (MD5_CTXP context);

#ifdef TRF_DEBUG
  {
    MD5_CTX* c = MD5_CTXP context;
    PRINT ("Init ABCD = %d %d %d %d\n", c->A, c->B, c->C, c->D); FL;
  }
#endif
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

  /*  MD5Update ((MD5_CTX*) context, &buf, 1); */

  md5f.update (&buf, 1, MD5_CTXP context);
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
  /*  MD5Update ((MD5_CTX*) context, (unsigned char*) buffer, bufLen);*/

  PRTSTR ("update by %d (%s)\n", bufLen, buffer);
#ifdef TRF_DEBUG
  {
    MD5_CTX* c = MD5_CTXP context;
    PRINT ("Upd1 ABCD = %d %d %d %d\n", c->A, c->B, c->C, c->D); FL;
  }
#endif

  md5f.update ((unsigned char*) buffer, bufLen, MD5_CTXP context);

#ifdef TRF_DEBUG
  {
    MD5_CTX* c = MD5_CTXP context;
    PRINT ("Upd2 ABCD = %d %d %d %d\n", c->A, c->B, c->C, c->D); FL;
  }
#endif
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
#ifndef OTP
  /*  MD5Final ((unsigned char*) digest, (MD5_CTX*) context); */
  md5f.final (MD5_CTXP context, (unsigned char*) digest);
#else
    int    i;
    unsigned char result[16];

    /*    MD5Final ((unsigned char*) result, (MD5_CTX*) context);*/
    md5f.final (MD5_CTXP context, (unsigned char*) result);

    for (i = 0; i < 8; i++)
        result[i] ^= result[i + 8];

    memcpy ((VOID *) digest, (VOID *) result, DIGEST_SIZE);
#endif

#ifdef TRF_DEBUG
  {
    MD5_CTX* c = MD5_CTXP context;
    PRINT ("Flsh ABCD = %d %d %d %d\n", c->A, c->B, c->C, c->D); FL;
  }
#endif
}

/*
 *------------------------------------------------------*
 *
 *	MD_Check --
 *
 *	------------------------------------------------*
 *	Do global one-time initializations of the message
 *	digest generator.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Loads the shared library containing the
 *		SHA1 functionality
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

static int
MD_Check (interp)
Tcl_Interp* interp;
{
  return TrfLoadMD5 (interp);
}

#if 0
/*
 * External code from here on.
 */

#ifndef OTP
#include "md5/md5.c" /* THREADING: import of one constant var, read-only => safe */
#endif
#endif
