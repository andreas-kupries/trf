/*
 * bz2lib.c --
 *
 *	Loader for 'bz2' compression library.
 *
 * Copyright (c) 1996 Jan Nijtmans (Jan.Nijtmans.wxs.nl)
 * All rights reserved.
 *
 * CVS: $Id$
 */

#include "transformInt.h"

#ifdef __WIN32__
#define BZ2_LIB_NAME "bz2.dll"
#endif

#ifndef BZ2_LIB_NAME
#define BZ2_LIB_NAME "libbz2.so"
#endif


static char* symbols [] = {
  "bzCompress",
  "bzCompressEnd",
  "bzCompressInit",
  "bzDecompress",
  "bzDecompressEnd",
  "bzDecompressInit",
  (char *) NULL
};


/*
 * Global variable containing the vectors into the 'bz2'-library.
 */

bzFunctions bz = {0}; /* THREADING: serialize initialization */


int
TrfLoadBZ2lib (interp)
    Tcl_Interp* interp;
{
  int res;

  TrfLock; /* THREADING: serialize initialization */

  res = Trf_LoadLibrary (interp, BZ2_LIB_NAME, (VOID**) &bz, symbols, 6);
  TrfUnlock;

  return res;
}
