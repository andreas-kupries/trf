/*
 * zlib.c --
 *
 *	Loader for 'zlib' compression library.
 *
 * Copyright (c) 1996 Jan Nijtmans (nijtmans.nici.kun.nl)
 * All rights reserved.
 *
 * CVS: $Id$
 */

#include "transformInt.h"

#ifdef __WIN32__
#define Z_LIB_NAME "zlib.dll"
#endif

#ifndef Z_LIB_NAME
#define Z_LIB_NAME "libz.so"
#endif


static char* symbols [] = {
  "deflate",
  "deflateEnd",
  "deflateInit_",
  "deflateReset",
  "inflate",
  "inflateEnd",
  "inflateInit_",
  "inflateReset",
  "adler32",
  "crc32",
  (char *) NULL
};


/*
 * Global variable containing the vectors into the 'zlib'-library.
 */

zFunctions z = {0}; /* THREADING: serialize initialization */


int
TrfLoadZlib (interp)
    Tcl_Interp* interp;
{
  int res;

  TrfLock; /* THREADING: serialize initialization */

  res = Trf_LoadLibrary (interp, Z_LIB_NAME, (VOID**) &z, symbols, 10);
  TrfUnlock;

  return res;
}
