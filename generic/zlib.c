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
#define Z_LIB_NAME "z.dll"
#endif

#ifndef Z_LIB_NAME
#define Z_LIB_NAME "libz.so"
#endif

/*
 * In some systems, like SunOS 4.1.3, the RTLD_NOW flag isn't defined
 * and this argument to dlopen must always be 1.
 */

#ifndef RTLD_NOW
#   define RTLD_NOW 1
#endif

#ifndef offsetOf
#define offsetOf(type,field) ((unsigned long) (((char *) &((type *) 0)->field)))
#endif

/*#define FN_DIST ((int ) offsetOf(struct ZFunctions, deflateEnd) - \ */
/*	(int) offsetOf(struct ZFunctions, deflate))/sizeof(char *) */

typedef struct _sym_ {
  char*         sym_name;
  unsigned long sym_offset;
} sym;

static sym symbols[] = {
  {"_deflate",      offsetOf (zFunctions, deflate)},
  {"_deflateEnd",   offsetOf (zFunctions, deflateEnd)},
  {"_deflateInit_", offsetOf (zFunctions, deflateInit_)},
  {"_deflateReset", offsetOf (zFunctions, deflateReset)},
  {"_inflate",      offsetOf (zFunctions, inflate)},
  {"_inflateEnd",   offsetOf (zFunctions, inflateEnd)},
  {"_inflateInit_", offsetOf (zFunctions, inflateInit_)},
  {"_inflateReset", offsetOf (zFunctions, inflateReset)},
  {"_adler32",      offsetOf (zFunctions, adler32)},
  {"_crc32",        offsetOf (zFunctions, crc32)},
  {(char *) NULL,   0}
};


/*
 * Global variable containing the vectors into the 'zlib'-library.
 */

zFunctions z = {0};



int
TrfLoadZlib (interp)
    Tcl_Interp* interp;
{
  VOID* handle = (VOID *) NULL;
  sym*  q      = symbols;
  char* p;
  char* buf;

  if (z.handle != NULL) {
    return TCL_OK;
  }

  /* DEBUG */
  printf ("TrfLoadZlib (interp=%p)\n", interp);
  /* DEBUG */

  buf = (char *) Tcl_Alloc (strlen (Z_LIB_NAME) + 3);
  strcpy (buf, Z_LIB_NAME);
  strcat (buf, ".1");
  handle = dlopen (buf, RTLD_NOW);
  Tcl_Free (buf);

  if (handle == NULL) {
    dlerror ();
    handle = dlopen (Z_LIB_NAME, RTLD_NOW);
  }

  if (handle == NULL) {
    /* DEBUG */
    printf ("\terror: %s\n", dlerror ());
    /* DEBUG */
    if (interp != NULL) {
      Tcl_AppendResult (interp,
			"cannot open ", Z_LIB_NAME,
			": ", dlerror (), (char *) NULL);
    }

    return TCL_ERROR;
  }

  /*
   * We have access to the library now.
   * Get the addresses of the relevant symbols and
   * place them into the global vector structure
   */

  /* DEBUG */
  for (q=symbols; q->sym_name; q++) {
    printf ("\tsymbol '%s':\t%d\n", q->sym_name, q->sym_offset);
  }
  q = symbols;
  /* DEBUG */


  while (q->sym_name) {

    p = (char *) dlsym (handle, q->sym_name + 1);

    if (p == (VOID*) NULL) {
      p = (char *) dlsym (handle, q->sym_name);

      if (p == (char*) NULL) {
	if (interp != NULL) {
	  Tcl_AppendResult (interp,
			    "cannot open ",Z_LIB_NAME,
			    ": symbol \"", q->sym_name + 1,
			    "\" not found", (char *) NULL);
	}

	return TCL_ERROR;
      }
    }
    
    memcpy(((char*)(&z))+ q->sym_offset, &p, sizeof (char*));

    q++;
  }

  z.handle = handle;

  return TCL_OK;
}
