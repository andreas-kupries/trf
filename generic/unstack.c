/*
 * unstack.c --
 *
 *	Implements the 'unstack' command to remove a conversion.
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

#include	"transformInt.h"

static int
TrfUnstackCmd _ANSI_ARGS_ ((ClientData notUsed, Tcl_Interp* interp, int argc, char** argv));

/*
 *----------------------------------------------------------------------
 *
 * TrfUnstackCmd --
 *
 *	This procedure is invoked to process the "unstack" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Unstacks the channel, thereby restoring its parent.
 *
 *----------------------------------------------------------------------
 */

static int
TrfUnstackCmd(notUsed, interp, argc, argv)
    ClientData  notUsed;		/* Not used. */
    Tcl_Interp* interp;			/* Current interpreter. */
    int         argc;			/* Number of arguments. */
    char**      argv;			/* Argument strings. */
{
  /*
   * unstack <channel>
   */

  Tcl_Channel chan;
  int         mode;

  if ((argc < 1) || (argc > 2)) {
    Tcl_AppendResult (interp,
		      "wrong # args: should be \"unstack channel\"",
		      (char*) NULL);
    return TCL_ERROR;
  }

  chan = Tcl_GetChannel (interp, argv[1], &mode);
  if (chan == (Tcl_Channel) NULL) {
    return TCL_ERROR;
  }

  Tcl_UndoReplaceChannel (interp, chan);
  return TCL_OK;
}

/*
 *------------------------------------------------------*
 *
 *	TrfInit_Unstack --
 *
 *	------------------------------------------------*
 *	Register the 'unstack' command.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		As of 'Tcl_CreateCommand'.
 *
 *	Result:
 *		A standard Tcl error code.
 *
 *------------------------------------------------------*
 */

int
TrfInit_Unstack (interp)
Tcl_Interp* interp;
{
  Tcl_CreateCommand (interp, "unstack", TrfUnstackCmd,
		     (ClientData) NULL,
		     (Tcl_CmdDeleteProc *) NULL);

  return TCL_OK;
}

