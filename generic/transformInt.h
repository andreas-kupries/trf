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

#ifdef TCL_STORAGE_CLASS
# undef TCL_STORAGE_CLASS
#endif
#ifdef BUILD_trf
# define TCL_STORAGE_CLASS DLLEXPORT
#else
# define TCL_STORAGE_CLASS DLLIMPORT
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
#endif


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
 * Macro to use to determine the offset of a structure member
 * in bytes from the beginning of the structure.
 */

#ifndef offsetof
#define offsetof(type, field) ((int) ((char *) &((type *) 0)->field))
#endif

#if GT81
#ifndef Tcl_ReplaceChannel
/* The core we are compiling against is not patched, so supply the
 * necesssary definitions here by ourselves. The form chosen for
 * the procedure macros (reservedXXX) will notify us if the core
 * does not have these reserved locations anymore.
 *
 * !! Synchronize the procedure indices in their definitions with
 *    the patch to tcl.decls, as they have to be the same.

 */

/* 281 */
typedef Tcl_Channel (trf_ReplaceChannel) _ANSI_ARGS_((Tcl_Interp* interp,
						      Tcl_ChannelType* typePtr,
						      ClientData instanceData,
						      int mask,
						      Tcl_Channel prevChan));
/* 282 */
typedef void (trf_UndoReplaceChannel) _ANSI_ARGS_((Tcl_Interp* interp,
						   Tcl_Channel chan));

#define Tcl_ReplaceChannel     ((trf_ReplaceChannel*) tclStubsPtr->reserved281)
#define Tcl_UndoReplaceChannel ((trf_UndoReplaceChannel*) tclStubsPtr->reserved282)

#endif /* Tcl_ReplaceChannel */
#endif /* GT81 */



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

EXTERN int TrfInit_RS_ECC    _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_ZIP       _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN int TrfInit_Unstack   _ANSI_ARGS_ ((Tcl_Interp* interp));
EXTERN int TrfInit_Binio     _ANSI_ARGS_ ((Tcl_Interp* interp));

EXTERN int TrfInit_Transform _ANSI_ARGS_ ((Tcl_Interp* interp));


#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

#endif /* TRF_INT_H */
