/* -*- c -*-
 * loadman.h -
 *
 * internal definitions for loading of shared libraries required by Trf.
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
 * IN NO EVENT SHALL I BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL,
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

#ifndef TRF_LOADMANAGER_H
#define TRF_LOADMANAGER_H

/*
 * The procedures defined here manage the loading of libraries required
 * by various glue-code for crytographic algorithms. Dependent on the
 * functionality requested more than one library will be tried before
 * giving up entirely.
 *
 * All following sections define a structure for each algorithm to fill
 * with the addresses of the functions required here, plus a procedure
 * to do the filling.
 */

#include "transformInt.h"

#ifdef HAVE_DES_H
#   include <des.h>
#else
#   include "compat/des.h"
#endif
#ifdef HAVE_MD2_H
#   include <md2.h>
#else
#   include "compat/md2.h"
#endif
#ifdef HAVE_RC2_H
#   include <rc2.h>
#else
#   include "compat/rc2.h"
#endif
#ifdef HAVE_SHA_H
#   include <sha.h>
#else
#   include "compat/sha.h"
#endif


/* DES, symmetric block cipher.
 * Affected command in case of failure: des
 */

typedef struct DesFunctions {
  long loaded;
  void (* set_key)     _ANSI_ARGS_ ((des_cblock *key, des_key_schedule schedule));
  void (* ecb_encrypt) _ANSI_ARGS_ ((des_cblock *input,des_cblock *output,
				     des_key_schedule schedule, int enc));
} desFunctions;

EXTERN desFunctions desf;

EXTERN int
TrfLoadDes _ANSI_ARGS_ ((Tcl_Interp *interp));


/* RC2, symmetric block cipher.
 * Affected command in case of failure: rc2
 */

typedef struct Rc2Functions {
  long loaded;
  void (* set_key)     _ANSI_ARGS_ ((RC2_KEY *ks, int len, unsigned char* key, int bits));
  void (* ecb_encrypt) _ANSI_ARGS_ ((unsigned char* in, unsigned char* out,
				     RC2_KEY* schedule, int enc));
} rc2Functions;

EXTERN rc2Functions rc2f;

EXTERN int
TrfLoadRC2 _ANSI_ARGS_ ((Tcl_Interp *interp));


/* MD2, message digest.
 * Affected command in case of failure: md2
 */

typedef struct Md2Functions {
  long loaded;
  void (* init)   _ANSI_ARGS_ ((MD2_CTX* c));
  void (* update) _ANSI_ARGS_ ((MD2_CTX* c, unsigned char* data, unsigned long length));
  void (* final)  _ANSI_ARGS_ ((unsigned char* digest, MD2_CTX* c));
} md2Functions;

EXTERN md2Functions md2f;

EXTERN int
TrfLoadMD2 _ANSI_ARGS_ ((Tcl_Interp *interp));


/* SHA1, message digest.
 * Affected command in case of failure: sha1
 */

typedef struct Sha1Functions {
  long loaded;
  void (* init)   _ANSI_ARGS_ ((SHA_CTX* c));
  void (* update) _ANSI_ARGS_ ((SHA_CTX* c, unsigned char* data, unsigned long length));
  void (* final)  _ANSI_ARGS_ ((unsigned char* digest, SHA_CTX* c));
} sha1Functions;

EXTERN sha1Functions sha1f;

EXTERN int
TrfLoadSHA1 _ANSI_ARGS_ ((Tcl_Interp *interp));



#endif /* TRF_LOADMANAGER_H */

