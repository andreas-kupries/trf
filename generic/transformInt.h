#ifndef TRF_INT_H
#define TRF_INT_H

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

#ifdef __cplusplus
extern "C" {
#endif

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
#include "compat:bzlib.h"
#else
#ifdef HAVE_DLFCN_H
#   include <dlfcn.h>
#else
#   include "../compat/dlfcn.h"
#endif
#ifdef HAVE_ZLIB_H
#   include <zlib.h>
#else
#   include "../compat/zlib.h"
#endif
#endif
#ifdef HAVE_BZLIB_H
#   include <bzlib.h>
#else
#   include "../compat/bzlib.h"
#endif
#ifdef HAVE_STDLIB_H
#   include <stdlib.h>
#else
#   include "../compat/stdlib.h"
#endif
#endif

#ifdef TCL_STORAGE_CLASS
# undef TCL_STORAGE_CLASS
#endif
#ifdef BUILD_trf
# define TCL_STORAGE_CLASS DLLEXPORT
#else
# define TCL_STORAGE_CLASS DLLIMPORT
#endif

/* Debugging definitions.
 */

#ifdef _WIN32
#undef TRF_DEBUG
#endif

#ifdef TRF_DEBUG
static int n = 0;
#define BLNKS {int i; for (i=0;i<n;i++) printf (" "); }
#define IN n+=4
#define OT n-=4 ; if (n<0) {n=0;}
#define FL       fflush (stdout)
#define START(p) BLNKS; printf ("Start %s...\n",#p); FL; IN
#define DONE(p)  OT ; BLNKS; printf ("....Done %s\n",#p); FL;
#define PRINT    BLNKS; printf
#else
#define BLNKS
#define IN
#define OT
#define FL
#define START(p)
#define DONE(p)
#define PRINT if (0) printf
#endif


/* Define macro which is TRUE for tcl versions >= 8.1
 * Required as there are incompatibilities between 8.0 and 8.1
 */

#define GT81 ((TCL_MAJOR_VERSION > 8) || \
	      ((TCL_MAJOR_VERSION == 8) && \
	       (TCL_MINOR_VERSION >= 1)))


#if ! (GT81)
/* enable use of procedure internal to tcl */
EXTERN void
panic _ANSI_ARGS_ ((char* format, ...));

#undef  Tcl_Panic
#define Tcl_Panic panic
#endif

/*
 * A single structure of the type below is created and maintained for
 * every interpreter. Beyond the table of registered transformers it
 * contains version information about the interpreter used to switch
 * runtime behaviour.
 */

typedef struct _Trf_Registry_ {
  Tcl_HashTable* registry;        /* Table containing all registered
				   * transformers. */
  int            patchIntegrated; /* Boolean flag, set to one if the patch
				   * is integrated into the core. If yes
				   * switch some runtime behaviour, as the
				   * integrated patch has some semantic
				   * differences. This information is
				   * propagated into the state of running
				   * transformers as well. */
} Trf_Registry;

/*
 * A structure of the type below is created and maintained
 * for every registered transformer (and every interpreter).
 */

typedef struct _Trf_RegistryEntry_ {
  Trf_Registry*       registry;   /* Backpointer to the registry */

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

EXTERN Trf_Registry*
TrfGetRegistry  _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN Trf_Registry*
TrfPeekForRegistry _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN int
Trf_Unregister _ANSI_ARGS_ ((Tcl_Interp*        interp,
			     Trf_RegistryEntry* entry));


/*
 * Procedures used by 3->4 encoders (uu, base64).
 */

EXTERN void TrfSplit3to4 _ANSI_ARGS_ ((CONST unsigned char* in,
				       unsigned char* out, int length));

EXTERN void TrfMerge4to3 _ANSI_ARGS_ ((CONST unsigned char* in,
				       unsigned char* out));

EXTERN void TrfApplyEncoding   _ANSI_ARGS_ ((unsigned char* buf, int length,
					     CONST char* map));

EXTERN int  TrfReverseEncoding _ANSI_ARGS_ ((unsigned char* buf, int length,
					     CONST char* reverseMap,
					     unsigned int padChar,
					     int* hasPadding));

/*
 * Definition of option information for message digests and accessor
 * to set of vectors processing these.
 */


typedef struct _TrfMDOptionBlock {
  int         behaviour; /* IMMEDIATE vs. ATTACH, got from checkProc */
  int         mode;      /* what to to with the generated hashvalue */

  char*       readDestination;	/* Name of channel (or global variable)
				 * to write the hash of read data to
				 * (mode = TRF_WRITE_HASH / ..TRANSPARENT) */
  char*       writeDestination;	/* Name of channel (or global variable)
				 * to write the hash of written data to
				 * (mode = TRF_WRITE_HASH / ..TRANSPARENT) */

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
#define TRF_TRANSPARENT (3)

EXTERN Trf_OptionVectors*
TrfMDOptions _ANSI_ARGS_ ((void));





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

/*
 * Definition of option information for BZ2 compressor
 * + accessor to set of vectors processing them
 */

typedef struct _TrfBz2OptionBlock {
  int mode;   /* compressor mode: compress/decompress */
  int level;  /* compression level (1..9, 9 = default) */
} TrfBz2OptionBlock;

EXTERN Trf_OptionVectors*
TrfBZ2Options _ANSI_ARGS_ ((void));

#define TRF_COMPRESS   (1)
#define TRF_DECOMPRESS (2)

#define TRF_MIN_LEVEL      (1)
#define TRF_MAX_LEVEL      (9)
#define TRF_DEFAULT_LEVEL (-1)

#define TRF_MIN_LEVEL_STR "1"
#define TRF_MAX_LEVEL_STR "9"


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
  int (* deflateInit_)      _ANSI_ARGS_ ((z_streamp strm, int level,
					  CONST char *version,
					  int stream_size));
  int (* deflateReset)      _ANSI_ARGS_ ((z_streamp strm));
  int (* inflate)           _ANSI_ARGS_ ((z_streamp strm, int flush));
  int (* inflateEnd)        _ANSI_ARGS_ ((z_streamp strm));
  int (* inflateInit_)      _ANSI_ARGS_ ((z_streamp strm, CONST char *version,
					  int stream_size));
  int (* inflateReset)      _ANSI_ARGS_ ((z_streamp strm));
  unsigned long (* adler32) _ANSI_ARGS_ ((unsigned long adler,
					  CONST unsigned char *buf,
					  unsigned int len));
  unsigned long (* crc32)   _ANSI_ARGS_ ((unsigned long crc,
					  CONST unsigned char *buf,
					  unsigned int len));
} zFunctions;


EXTERN zFunctions zf; /* THREADING: serialize initialization */

EXTERN int
TrfLoadZlib _ANSI_ARGS_ ((Tcl_Interp *interp));

/*
 * 'libbz2' will be dynamically loaded. Following a structure to
 * contain the addresses of all functions required by this extension.
 *
 * Affected commands are: bzip.
 * They will fail, if the library could not be loaded.
 */

#ifndef WINAPI
#define WINAPI
#endif

typedef struct BZFunctions {
  VOID *handle;
  int (WINAPI * compress)           _ANSI_ARGS_ ((bz_stream* strm,
						  int action));
  int (WINAPI * compressEnd)        _ANSI_ARGS_ ((bz_stream* strm));
  int (WINAPI * compressInit)       _ANSI_ARGS_ ((bz_stream* strm,
						  int blockSize100k,
						  int verbosity,
						  int workFactor));
  int (WINAPI * decompress)         _ANSI_ARGS_ ((bz_stream* strm));
  int (WINAPI * decompressEnd)      _ANSI_ARGS_ ((bz_stream* strm));
  int (WINAPI * decompressInit)     _ANSI_ARGS_ ((bz_stream* strm,
						  int verbosity, int small));
} bzFunctions;


EXTERN bzFunctions bz; /* THREADING: serialize initialization */

EXTERN int
TrfLoadBZ2lib _ANSI_ARGS_ ((Tcl_Interp *interp));

/*
 * The following definitions have to be usable for 7.6, 8.0.x, 8.1.x and 8.2
 * and beyond. The differences between these versions:
 *
 * 7.6, 8.0.x: Trf usable only if core is patched, to check at compile time
 *             (Check = Fails to compile, for now).
 * 8.1:        Trf usable with unpatched core, but restricted, check at
 *             compile time for missing definitions, check at runtime to
 *             disable the missing features.
 * 8.2:        Changed semantics for Tcl_StackChannel (Tcl_ReplaceChannel).
 *             Check at runtime to switch the behaviour. The patch is part
 *             of the core from now on.
 */

#ifdef USE_TCL_STUBS
#ifndef Tcl_StackChannel
/* The core we are compiling against is not patched, so supply the
 * necesssary definitions here by ourselves. The form chosen for
 * the procedure macros (reservedXXX) will notify us if the core
 * does not have these reserved locations anymore.
 *
 * !! Synchronize the procedure indices in their definitions with
 *    the patch to tcl.decls, as they have to be the same.
 */

/* 281 */
typedef Tcl_Channel (trf_StackChannel) _ANSI_ARGS_((Tcl_Interp* interp,
						    Tcl_ChannelType* typePtr,
						    ClientData instanceData,
						    int mask,
						    Tcl_Channel prevChan));
/* 282 */
typedef void (trf_UnstackChannel) _ANSI_ARGS_((Tcl_Interp* interp,
					       Tcl_Channel chan));

#define Tcl_StackChannel     ((trf_StackChannel*) tclStubsPtr->reserved281)
#define Tcl_UnstackChannel ((trf_UnstackChannel*) tclStubsPtr->reserved282)

#endif /* Tcl_StackChannel */

#ifndef Tcl_GetStackedChannel
/*
 * Separate definition, available in 8.2, but not 8.1 and before !
 */

/* 283 */
typedef Tcl_Channel (trf_GetStackedChannel) _ANSI_ARGS_((Tcl_Channel chan));

#define Tcl_GetStackedChannel ((trf_GetStackedChannel*) tclStubsPtr->reserved283)

#endif /* Tcl_GetStackedChannel */
#endif /* USE_TCL_STUBS */


/*
 * Internal initialization procedures for all transformers implemented here.
 */

EXTERN int TrfInit_Bin       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_Oct       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_Hex       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_UU        _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_B64       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_Ascii85   _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_OTP_WORDS _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_QP        _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN int TrfInit_CRC       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_MD5       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_MD2       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_HAVAL     _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_SHA       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_SHA1      _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_OTP_SHA1  _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_ADLER     _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_CRC_ZLIB  _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_RIPEMD128 _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_RIPEMD160 _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_OTP_MD5   _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN int TrfInit_RS_ECC    _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_ZIP       _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_BZ2       _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN int TrfInit_Unstack   _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_Binio     _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN int TrfInit_Transform _ANSI_ARGS_ ((Tcl_Interp* interp));


#if GT81 && defined (TCL_THREADS) /* THREADING: Lock procedures */

EXTERN void TrfLockIt   _ANSI_ARGS_ ((void));
EXTERN void TrfUnlockIt _ANSI_ARGS_ ((void));

#define TrfLock   TrfLockIt ()
#define TrfUnlock TrfUnlockIt ()
#else
/* Either older version of Tcl, or non-threaded 8.1.x.
 * Whatever, locking is not required, undefine the calls.
 */

#define TrfLock
#define TrfUnlock
#endif

#include "trfIntDecls.h"

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

#ifdef __cplusplus
}
#endif /* C++ */
#endif /* TRF_INT_H */
