/* -*- c -*-
 * transformInt.h - internal definitions
 *
 * Copyright (C) 1996, 1997 Andreas Kupries (a.kupries@westend.com)
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

#ifndef TRF_INT_H
#define TRF_INT_H

#include "transform.h"
#include <errno.h>
#include <string.h>
#include <assert.h>

#ifndef ___MAKEDEPEND___
/*
 * Hide external references during runs of 'makedepend'.
 * It may fail on some systems, if the files are not installed.
 */
#ifdef MAC_TCL
#include "compat:dlfcn.h"
#include "compat:zlib.h"
#include "compat:des.h"
#else
#ifdef HAVE_DLFCN_H
#   include <dlfcn.h>
#else
#   include "compat/dlfcn.h"
#endif
#ifdef HAVE_ZLIB_H
#   include <zlib.h>
#else
#   include "compat/zlib.h"
#endif
#endif
#ifdef HAVE_STDLIB_H
#   include <stdlib.h>
#else
#   include "compat/stdlib.h"
#endif
#endif

/* make internal procedure of tcl available */
EXTERN void
panic _ANSI_ARGS_ ((CONST char* format, ...));


/*
 * A structure of the type below is created and maintained
 * for every registered transformer (and every interpreter).
 */

typedef struct _Trf_RegistryEntry_ {
  Trf_TypeDefinition* trfType;    /* reference to transformer specification */
  Tcl_ChannelType*    transType;  /* reference to derived channel type specification */
  Tcl_Command         trfCommand; /* command associated to the transformer */
  Tcl_Interp*         interp;     /* interpreter the command is registered in */
} Trf_RegistryEntry;


/*
 * Procedures to access the registry of transformers for a specified
 * interpreter. The registry is a hashtable mapping from transformer
 * names to structures of type 'Trf_RegistryEntry' (see above).
 */

EXTERN Tcl_HashTable* 
TrfGetRegistry  _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN Tcl_HashTable*
TrfPeekForRegistry _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN int
Trf_Unregister _ANSI_ARGS_ ((Tcl_Interp*        interp,
			     Trf_RegistryEntry* entry));


/*
 * Procedures used by 3->4 encoders (uu, base64).
 */

EXTERN void TrfSplit3to4 _ANSI_ARGS_ ((CONST unsigned char* in, unsigned char* out, int length));
EXTERN void TrfMerge4to3 _ANSI_ARGS_ ((CONST unsigned char* in, unsigned char* out));

EXTERN void TrfApplyEncoding   _ANSI_ARGS_ ((unsigned char* buf, int length, CONST char* map));
EXTERN int  TrfReverseEncoding _ANSI_ARGS_ ((unsigned char* buf, int length, CONST char* reverseMap,
					     unsigned int padChar, int* hasPadding));


/*
 * Helper procedures for ciphers.
 */

#if (TCL_MAJOR_VERSION < 8)
EXTERN int
TrfGetData _ANSI_ARGS_ ((Tcl_Interp* interp, CONST char* dataName, int dataIsChannel,
			 CONST char* data, unsigned short min_bytes, unsigned short max_bytes,
			 VOID** buf, int* length));
#else
EXTERN int
TrfGetData _ANSI_ARGS_ ((Tcl_Interp* interp, CONST char* dataName, int dataIsChannel,
			 CONST Tcl_Obj* data, unsigned short min_bytes, unsigned short max_bytes,
			 VOID** buf, int* length));
#endif

EXTERN int
TrfGetDataType _ANSI_ARGS_ ((Tcl_Interp* interp, CONST char* dataName,
			     CONST char* typeString, int* isChannel));



/*
 * Definition of option information for message digests and accessor
 * to set of vectors processing these.
 */


typedef struct _TrfMDOptionBlock {
  int         behaviour; /* IMMEDIATE vs. ATTACH, got from checkProc */
  int         mode;      /* what to to with the generated hashvalue */

  char*       readDestination;	/* Name of channel (or global variable)
				 * to write the hash of read data to
				 * (mode = TRF_WRITE_HASH) */
  char*       writeDestination;	/* Name of channel (or global variable)
				 * to write the hash of written data to
				 * (mode = TRF_WRITE_HASH) */

  int        rdIsChannel; /* typeflag for 'readDestination',  true for a channel */
  int        wdIsChannel; /* typeflag for 'writeDestination', true for a channel */

  char*       matchFlag; /* Name of global variable to write the match-
			  * result into (TRF_ABSORB_HASH) */

  Tcl_Interp* vInterp;	/* Interpreter containing the variable named in
			 * 'matchFlag', or '*Destination'. */

  /* derived information */

  Tcl_Channel rdChannel;  /* Channel associated to 'readDestination' */
  Tcl_Channel wdChannel;  /* Channel associated to 'writeDestination' */
} TrfMDOptionBlock;

#define TRF_IMMEDIATE (1)
#define TRF_ATTACH    (2)

#define TRF_ABSORB_HASH (1)
#define TRF_WRITE_HASH  (2)

EXTERN Trf_OptionVectors*
TrfMDOptions _ANSI_ARGS_ ((void));



/*
 * Definition of option information for blockciphers and accessor
 * to set of vectors processing these.
 */

typedef struct _TrfBlockcipherOptionBlock {
  int   direction;      /* encryption, decryption (TRF_ENCRYPT / TRF_DECRYPT) */
  int   operation_mode; /* ECB, CBC, CFB, OFB */
  int   shift_width;    /* shift per operation for feedback modes.
			 * given in bytes. */

  int   keyDataIsChan; /* Flag for interpretation of keyData */
  int   ivDataIsChan;  /* Flag for interpretation of ivData  */

#if (TCL_MAJOR_VERSION < 8)
  unsigned char* keyData;  /* Key information */
  unsigned char* ivData;   /* IV  information */
#else
  Tcl_Obj*       keyData;  /* Key information */
  Tcl_Obj*       ivData;   /* IV  information */
#endif

  /* ---- derived information ----
   *
   * Area used for communication between vectors
   *
   *	Trf_TypeDefinition.encoder.create,
   *	Trf_TypeDefinition.decoder.create
   *
   * to avoid duplicate execution of complex and/or common operations.
   */

  int   key_length;     /* length of key (required for ciphers with
			 * variable keysize) */
  VOID* key;            /* key data */
  VOID* iv;             /* initialization vector for stream modes. */

  int eks_length;
  int dks_length;

  VOID* encrypt_keyschedule; /* expansion of key into subkeys for encryption */
  VOID* decrypt_keyschedule; /* expansion of key into subkeys for decryption */

} TrfBlockcipherOptionBlock;

#define TRF_ECB_MODE (1)
#define TRF_CBC_MODE (2)
#define TRF_CFB_MODE (3)
#define TRF_OFB_MODE (4)

EXTERN Trf_OptionVectors*
TrfBlockcipherOptions _ANSI_ARGS_ ((void));


/*
 * Definition of option information for ciphers and accessor
 * to set of vectors processing these.
 */

typedef struct _TrfCipherOptionBlock {
  int   direction;      /* encryption, decryption */

  int   keyDataIsChan; /* Flag for interpretation of keyData */

#if (TCL_MAJOR_VERSION < 8)
  unsigned char* keyData;  /* Key information */
#else
  Tcl_Obj*       keyData;  /* Key information */
#endif

  /* ---- derived information ----
   *
   * Area used for communication between vectors
   *
   *	Trf_TypeDefinition.encoder.create,
   *	Trf_TypeDefinition.decoder.create
   *
   * to avoid duplicate execution of complex and/or common operations.
   */

  int   key_length;     /* length of key (required for ciphers with
			 * variable keysize) */
  VOID* key;            /* key data */

  int eks_length;
  int dks_length;

  VOID* encrypt_keyschedule; /* expansion of key into subkeys for encryption */
  VOID* decrypt_keyschedule; /* expansion of key into subkeys for decryption */

} TrfCipherOptionBlock;


EXTERN Trf_OptionVectors*
TrfCipherOptions _ANSI_ARGS_ ((void));


/*
 * Definition of option information for general transformation (reflect.c, ref_opt.c)
 * to set of vectors processing these.
 */

typedef struct _TrfTransformOptionBlock {
  int mode; /* operation to execute (transform for read or write) */

#if (TCL_MAJOR_VERSION >= 8)
  Tcl_Obj*       command; /* tcl code to execute for a buffer */
#else
  unsigned char* command; /* tcl code to execute for a buffer */
#endif
} TrfTransformOptionBlock;

/*#define TRF_UNKNOWN_MODE (0) -- transform.h */
#define TRF_WRITE_MODE (1)
#define TRF_READ_MODE  (2)



EXTERN Trf_OptionVectors*
TrfTransformOptions _ANSI_ARGS_ ((void));


/*
 * Definition of option information for ZIP compressor
 * + accessor to set of vectors processing them
 */

typedef struct _TrfZipOptionBlock {
  int mode;   /* compressor mode: compress/decompress */
  int level;  /* compression level (1..9, -1 = default) */
} TrfZipOptionBlock;

EXTERN Trf_OptionVectors*
TrfZIPOptions _ANSI_ARGS_ ((void));

#define TRF_COMPRESS   (1)
#define TRF_DECOMPRESS (2)

#define TRF_MIN_LEVEL      (1)
#define TRF_MAX_LEVEL      (9)
#define TRF_DEFAULT_LEVEL (-1)

#define TRF_MIN_LEVEL_STR "1"
#define TRF_MAX_LEVEL_STR "9"


/*
 * General purpose library loader functionality.
 * Used by -> TrfLoadZlib, -> TrfLoadLibdes.
 */

EXTERN int
TrfLoadLibrary _ANSI_ARGS_ ((Tcl_Interp* interp, CONST char* libName,
			    VOID** handlePtr, char** symbols, int num));

EXTERN void
TrfLoadFailed _ANSI_ARGS_ ((VOID** handlePtr));


/*
 * 'zlib' will be dynamically loaded. Following a structure to
 * contain the addresses of all functions required by this extension.
 *
 * Affected commands are: zip, adler, crc-zlib.
 * They will fail, if the library could not be loaded.
 */

typedef struct ZFunctions {
  VOID *handle;
  int (* deflate)           _ANSI_ARGS_ ((z_streamp strm, int flush));
  int (* deflateEnd)        _ANSI_ARGS_ ((z_streamp strm));
  int (* deflateInit_)      _ANSI_ARGS_ ((z_streamp strm, int level, CONST char *version, int stream_size));
  int (* deflateReset)      _ANSI_ARGS_ ((z_streamp strm));
  int (* inflate)           _ANSI_ARGS_ ((z_streamp strm, int flush));
  int (* inflateEnd)        _ANSI_ARGS_ ((z_streamp strm));
  int (* inflateInit_)      _ANSI_ARGS_ ((z_streamp strm, CONST char *version, int stream_size));
  int (* inflateReset)      _ANSI_ARGS_ ((z_streamp strm));
  unsigned long (* adler32) _ANSI_ARGS_ ((unsigned long adler, CONST unsigned char *buf, unsigned int len));
  unsigned long (* crc32)   _ANSI_ARGS_ ((unsigned long crc, CONST unsigned char *buf, unsigned int len));
} zFunctions;

EXTERN zFunctions z;

EXTERN int
TrfLoadZlib _ANSI_ARGS_ ((Tcl_Interp *interp));


/*
 * Internal initialization procedures for all transformers implemented here.
 */


EXTERN int TrfInit_Bin       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_Oct       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_Hex       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_UU        _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_B64       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_Ascii85   _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN int TrfInit_CRC       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_MD5       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_MD2       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_HAVAL     _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_SHA       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_SHA1      _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_ADLER     _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_CRC_ZLIB  _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_RIPEMD128 _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_RIPEMD160 _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN int TrfInit_IDEA      _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_BLOWFISH  _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_DES       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_RC4       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_RC2       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_ROT       _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN int TrfInit_RS_ECC    _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_ZIP       _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN int TrfInit_Unstack   _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_Binio     _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN int TrfInit_Transform _ANSI_ARGS_ ((Tcl_Interp* interp));


/*
 * Define general result generation, dependent on major tcl version.
 * 05/29/1997: not anymore, 8.0b1 smooths this out again. But leave
 * definitions in to avoid immediate conversion of all affected files!
 */

#define ADD_RES(interp, text) Tcl_AppendResult (interp, text, (char*) NULL);
#define RESET_RES(interp)     Tcl_ResetResult  (interp);

#endif /* TRF_INT_H */
