/*
 * init.c --
 *
 *	Implements the C level procedures handling the initialization of this package
 *
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
 * IN NO EVENT SHALL I LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL,
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

#include "transformInt.h"

/*
 *----------------------------------------------------------------------
 *
 * DllEntryPoint --
 *
 *	This wrapper function is used by Windows to invoke the
 *	initialization code for the DLL.  If we are compiling
 *	with Visual C++, this routine will be renamed to DllMain.
 *	routine.
 *
 * Results:
 *	Returns TRUE;
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef __WIN32__
BOOL APIENTRY
DllEntryPoint(hInst, reason, reserved)
    HINSTANCE hInst;		/* Library instance handle. */
    DWORD reason;		/* Reason this function is being called. */
    LPVOID reserved;		/* Not used. */
{
    return TRUE;
}
#endif

/*
 *------------------------------------------------------*
 *
 *	Trf_Init --
 *
 *	------------------------------------------------*
 *	Standard procedure required by 'load'. 
 *	Initializes this extension.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of 'TrfGetRegistry'.
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

int
Trf_Init (interp)
Tcl_Interp* interp;
{
  Tcl_HashTable* hTablePtr;

  if (Trf_IsInitialized (interp)) {
      /*
       * catch multiple initialization of an interpreter
       */
      return TCL_OK;
    }

  hTablePtr = TrfGetRegistry (interp);

  if (hTablePtr) {
    int res;

    /* register extension as now available package */
    Tcl_PkgProvide (interp, "Trf", TRF_VERSION);

    res = TrfInit_Unstack (interp);

    if (res != TCL_OK)
      return res;

    res = TrfInit_Binio (interp);

    if (res != TCL_OK)
      return res;

    /*
     * Register error correction algorithms.
     */

    res = TrfInit_RS_ECC (interp);

    if (res != TCL_OK)
      return res;

    /*
     * Register compressors.
     */

    res = TrfInit_ZIP (interp);

    if (res != TCL_OK)
      return res;

    /*
     * Register message digests
     */

    res = TrfInit_CRC (interp);

    if (res != TCL_OK)
      return res;

    res = TrfInit_ADLER (interp);

    if (res != TCL_OK)
      return res;

    res = TrfInit_CRC_ZLIB (interp);

    if (res != TCL_OK)
      return res;

    res = TrfInit_MD5 (interp);

    if (res != TCL_OK)
      return res;

    res = TrfInit_MD2 (interp);

    if (res != TCL_OK)
      return res;

    res = TrfInit_HAVAL (interp);

    if (res != TCL_OK)
      return res;

    res = TrfInit_SHA (interp);

    if (res != TCL_OK)
      return res;

    res = TrfInit_SHA1 (interp);

    if (res != TCL_OK)
      return res;

    res = TrfInit_RIPEMD160 (interp);

    if (res != TCL_OK)
      return res;

    res = TrfInit_RIPEMD128 (interp);

    if (res != TCL_OK)
      return res;

    /*
     * Register freeform transformation, reflector into tcl level
     */

    res = TrfInit_Transform (interp);

    if (res != TCL_OK)
      return res;

    /*
     * Register standard encodings.
     */

    res = TrfInit_Ascii85 (interp);

    if (res != TCL_OK)
      return res;

    res = TrfInit_UU (interp);

    if (res != TCL_OK)
      return res;

    res = TrfInit_B64 (interp);

    if (res != TCL_OK)
      return res;

    res = TrfInit_Bin (interp);

    if (res != TCL_OK)
      return res;

    res = TrfInit_Oct (interp);

    if (res != TCL_OK)
      return res;

    return TrfInit_Hex (interp);
  } else {
    return TCL_ERROR;
  }
}

/*
 *------------------------------------------------------*
 *
 *	Trf_SafeInit --
 *
 *	------------------------------------------------*
 *	Standard procedure required by 'load'. 
 *	Initializes this extension for a safe interpreter.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of 'TrfGetRegistry'
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

int
Trf_SafeInit (interp)
Tcl_Interp* interp;
{
  return Trf_Init (interp);
}

/*
 *------------------------------------------------------*
 *
 *	Trf_IsInitialized --
 *
 *	------------------------------------------------*
 *	Checks, wether the extension is initialized in
 *	the specified interpreter.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		None.
 *
 *	Result:
 *		1 if and onlly if the extension is already
 *		initialized in the specified interpreter,
 *		0 else.
 *
 *------------------------------------------------------*
 */

int
Trf_IsInitialized (interp)
Tcl_Interp* interp;
{
  Tcl_HashTable* hTablePtr;

  hTablePtr = TrfPeekForRegistry (interp);

  return hTablePtr != (Tcl_HashTable*) NULL;
}
