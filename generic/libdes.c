/*
 * libdes.c --
 *
 *	Loader for 'libdes' encryption library.
 *
 * Copyright (c) 1996 Jan Nijtmans (nijtmans.nici.kun.nl)
 * Copyright (c) 1996 Andreas Kupries (a.kupries@westend.com)
 * All rights reserved.
 *
 * CVS: $Id$
 */

#include "transformInt.h"

#ifdef __WIN32__
#define LIBDES_LIB_NAME "libdes.dll"
#endif

#ifndef LIBDES_LIB_NAME
#define LIBDES_LIB_NAME "libdes.so"
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
  {"_des_set_key",      offsetOf (libdesFunctions, des_set_key)},
  {"_des_ecb_encrypt",  offsetOf (libdesFunctions, des_ecb_encrypt)},
  {(char *) NULL,   0}
};


/*
 * Global variable containing the vectors into the 'libdes'-library.
 */

libdesFunctions ld = {0};



int
TrfLoadLibdes (interp)
    Tcl_Interp* interp;
{
  VOID* handle = (VOID *) NULL;
  sym*  q      = symbols;
  char* p;
  char* buf;

  if (ld.handle != NULL) {
    return TCL_OK;
  }

  buf = (char *) Tcl_Alloc (strlen (LIBDES_LIB_NAME) + 3);
  strcpy (buf, LIBDES_LIB_NAME);
  strcat (buf, ".1");
  handle = dlopen (buf, RTLD_NOW);
  Tcl_Free (buf);

  if (handle == NULL) {
    dlerror ();
    handle = dlopen (LIBDES_LIB_NAME, RTLD_NOW);
  }

  if (handle == NULL) {
    if (interp != NULL) {
      Tcl_AppendResult (interp,
			"cannot open ", LIBDES_LIB_NAME,
			": ", dlerror (), (char *) NULL);
    }

    return TCL_ERROR;
  }

  /*
   * We have access to the library now.
   * Get the addresses of the relevant symbols and
   * place them into the global vector structure
   */

  while (q->sym_name) {

    p = (char *) dlsym (handle, q->sym_name + 1);

    if (p == (VOID*) NULL) {
      p = (char *) dlsym (handle, q->sym_name);

      if (p == (char*) NULL) {
	if (interp != NULL) {
	  Tcl_AppendResult (interp,
			    "cannot open ",LIBDES_LIB_NAME,
			    ": symbol \"", q->sym_name + 1,
			    "\" not found", (char *) NULL);
	}

	return TCL_ERROR;
      }
    }
    
    memcpy(((char*)(&ld))+ q->sym_offset, &p, sizeof (char*));

    q++;
  }

  ld.handle = handle;

  return TCL_OK;
}
