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
  "_deflate",
  "_deflateEnd",
  "_deflateInit_",
  "_deflateReset",
  "_inflate",
  "_inflateEnd",
  "_inflateInit_",
  "_inflateReset",
  "_adler32",
  "_crc32",
  (char *) NULL
};


/*
 * Global variable containing the vectors into the 'zlib'-library.
 */

zFunctions z = {0};


int
TrfLoadZlib (interp)
    Tcl_Interp* interp;
{
  return TrfLoadLibrary (interp, Z_LIB_NAME, (VOID**) &z, symbols, 10);
}
