/*
 * loadman.c --
 *
 *	Loader for various crypto libraries.
 *
 * Copyright (c) 1997 Andreas Kupries (a.kupries@westend.com)
 * All rights reserved.
 *
 * CVS: $Id$
 */

#include "loadman.h"

#ifdef __WIN32__
#define LIBDES_LIB_NAME "libdes.dll"
#endif
#ifndef LIBDES_LIB_NAME
#define LIBDES_LIB_NAME "libdes.so"
#endif

#ifdef __WIN32__
#define SSL_LIB_NAME "crypto32.dll"
#endif
#ifndef SSL_LIB_NAME
#define SSL_LIB_NAME "libcrypto.so"
#endif


typedef struct SslLibFunctions {
  void* handle;
  /* DES */
  void (* des_set_key)     _ANSI_ARGS_ ((des_cblock *key, des_key_schedule schedule));
  void (* des_ecb_encrypt) _ANSI_ARGS_ ((des_cblock *input,des_cblock *output,
					 des_key_schedule schedule, int enc));
  /* MD2 */
  void (* md2_init)        _ANSI_ARGS_ ((MD2_CTX* c));
  void (* md2_update)      _ANSI_ARGS_ ((MD2_CTX* c, unsigned char* data, unsigned long length));
  void (* md2_final)       _ANSI_ARGS_ ((unsigned char* digest, MD2_CTX* c));
  /* RC2 */
  void (* rc2_set_key)     _ANSI_ARGS_ ((RC2_KEY *ks, int len, unsigned char* key, int bits));
  void (* rc2_ecb_encrypt) _ANSI_ARGS_ ((unsigned char* in, unsigned char* out,
					 RC2_KEY* schedule, int enc));
  /* SHA1 */
  void (* sha1_init)        _ANSI_ARGS_ ((SHA_CTX* c));
  void (* sha1_update)      _ANSI_ARGS_ ((SHA_CTX* c, unsigned char* data, unsigned long length));
  void (* sha1_final)       _ANSI_ARGS_ ((unsigned char* digest, SHA_CTX* c));
} sslLibFunctions;


typedef struct LibDesFunctions {
  void* handle;
  void (* des_set_key)     _ANSI_ARGS_ ((des_cblock *key, des_key_schedule schedule));
  void (* des_ecb_encrypt) _ANSI_ARGS_ ((des_cblock *input,des_cblock *output,
					 des_key_schedule schedule, int enc));
} libDesFunctions;



static char* ssl_symbols [] = {
  /* des */
  "des_set_key",
  "des_ecb_encrypt",
  /* md2 */
  "MD2_Init",
  "MD2_Update",
  "MD2_Final",
  /* rc2 */
  "RC2_set_key",
  "RC2_ecb_encrypt",
  /* sha1 */
  "SHA1_Init",
  "SHA1_Update",
  "SHA1_Final",
  /* -- */
  (char *) NULL,
};


static char* ld_symbols [] = {
  "des_set_key",
  "des_ecb_encrypt",
  (char *) NULL,
};


/*
 * Global variables containing the vectors to DES, MD2, ...
 */

desFunctions  desf  = {0};
md2Functions  md2f  = {0};
rc2Functions  rc2f  = {0};
sha1Functions sha1f = {0};


/*
 * Internal global variables, contains all vectors loaded from SSL's 'cryptlib'.
 *                            contains all vectors loaded from 'libdes' library.
 */

static sslLibFunctions ssl;
static libDesFunctions ld;

/*
 *------------------------------------------------------*
 *
 *	TrfLoadDes --
 *
 *	------------------------------------------------*
 *	Makes DES functionality available.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Loads the required shared library and
 *		makes the addresses of DES functionality
 *		available. In case of failure an error
 *		message is left in the result area of
 *		the specified interpreter.
 *
 *	Result:
 *		A standard tcl error code.
 *
 *------------------------------------------------------*
 */

int
TrfLoadDes (interp)
    Tcl_Interp* interp;
{
  int res;

  if (desf.loaded)
    return TCL_OK;

  res = TrfLoadLibrary (interp, SSL_LIB_NAME, (VOID**) &ssl, ssl_symbols, 0);

  if ((res == TCL_OK) &&
      (ssl.des_set_key != NULL) &&
      (ssl.des_ecb_encrypt != NULL)) {
    desf.set_key     = ssl.des_set_key;
    desf.ecb_encrypt = ssl.des_ecb_encrypt;
    desf.loaded      = 1;
    return TCL_OK;
  }

  RESET_RES (interp);
  res = TrfLoadLibrary (interp, LIBDES_LIB_NAME, (VOID**) &ld, ld_symbols, 2);

  if ((res == TCL_OK) &&
      (ld.des_set_key != NULL) &&
      (ld.des_ecb_encrypt != NULL)) {
    desf.set_key     = ld.des_set_key;
    desf.ecb_encrypt = ld.des_ecb_encrypt;
    desf.loaded      = 1;
    return TCL_OK;
  }

  return TCL_ERROR;
}

/*
 *------------------------------------------------------*
 *
 *	TrfLoadMD2 --
 *
 *	------------------------------------------------*
 *	Makes MD2 functionality available.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Loads the required shared library and
 *		makes the addresses of MD2 functionality
 *		available. In case of failure an error
 *		message is left in the result area of
 *		the specified interpreter.
 *
 *	Result:
 *		A standard tcl error code.
 *
 *------------------------------------------------------*
 */

int
TrfLoadMD2 (interp)
    Tcl_Interp* interp;
{
  int res;

  if (md2f.loaded)
    return TCL_OK;

  res = TrfLoadLibrary (interp, SSL_LIB_NAME, (VOID**) &ssl, ssl_symbols, 0);

  if ((res == TCL_OK) &&
      (ssl.md2_init   != NULL) &&
      (ssl.md2_update != NULL) &&
      (ssl.md2_final  != NULL)) {
    md2f.init   = ssl.md2_init;
    md2f.update = ssl.md2_update;
    md2f.final  = ssl.md2_final;
    md2f.loaded      = 1;
    return TCL_OK;
  }

  return TCL_ERROR;
}

/*
 *------------------------------------------------------*
 *
 *	TrfLoadSHA1 --
 *
 *	------------------------------------------------*
 *	Makes SHA-1 functionality available.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Loads the required shared library and
 *		makes the addresses of SHA-1 functionality
 *		available. In case of failure an error
 *		message is left in the result area of
 *		the specified interpreter.
 *		
 *
 *	Result:
 *		A standard tcl error code.
 *
 *------------------------------------------------------*
 */

int
TrfLoadSHA1 (interp)
    Tcl_Interp* interp;
{
  int res;

  if (sha1f.loaded)
    return TCL_OK;

  res = TrfLoadLibrary (interp, SSL_LIB_NAME, (VOID**) &ssl, ssl_symbols, 0);

  if ((res == TCL_OK) &&
      (ssl.sha1_init   != NULL) &&
      (ssl.sha1_update != NULL) &&
      (ssl.sha1_final  != NULL)) {
    sha1f.init   = ssl.sha1_init;
    sha1f.update = ssl.sha1_update;
    sha1f.final  = ssl.sha1_final;
    sha1f.loaded = 1;
    return TCL_OK;
  }

  return TCL_ERROR;
}

/*
 *------------------------------------------------------*
 *
 *	TrfLoadRC2 --
 *
 *	------------------------------------------------*
 *	Makes RC2 functionality available.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		Loads the required shared library and
 *		makes the addresses of RC2 functionality
 *		available. In case of failure an error
 *		message is left in the result area of
 *		the specified interpreter.
 *
 *	Result:
 *		A standard tcl error code.
 *
 *------------------------------------------------------*
 */

int
TrfLoadRC2 (interp)
    Tcl_Interp* interp;
{
  int res;

  if (rc2f.loaded)
    return TCL_OK;

  res = TrfLoadLibrary (interp, SSL_LIB_NAME, (VOID**) &ssl, ssl_symbols, 0);

  if ((res == TCL_OK) &&
      (ssl.rc2_set_key     != NULL) &&
      (ssl.rc2_ecb_encrypt != NULL)) {
    rc2f.set_key     = ssl.rc2_set_key;
    rc2f.ecb_encrypt = ssl.rc2_ecb_encrypt;
    rc2f.loaded      = 1;
    return TCL_OK;
  }

  return TCL_ERROR;
}
