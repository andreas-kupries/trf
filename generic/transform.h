/* -*- c -*-
 *
 * transform.h - externally visible facilities of data transformers
 *
 * Distributed at @mDate@.
 *
 * Copyright (c) 1995 Andreas Kupries (a.kupries@westend.com)
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

#ifndef TRF_H
#define TRF_H

#include <tcl.h>

/*
 * Definition of module version
 */

#define TRF_VERSION		"@mVersion@"
#define TRF_MAJOR_VERSION	@mMajor@
#define TRF_MINOR_VERSION	@mMinor@


/*
 * Definitions to enable the generation of a DLL under Windows.
 * Taken from 'ftp://ftp.sunlabs.com/pub/tcl/example.zip(example.c)'
 */

#if defined(__WIN32__)
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   undef WIN32_LEAN_AND_MEAN

/*
 * VC++ has an alternate entry point called DllMain, so we need to rename
 * our entry point.
 */

#   if defined(_MSC_VER)
#	define TRF_EXPORT(a,b) __declspec(dllexport) a b
#	define DllEntryPoint DllMain
#   else
#	if defined(__BORLANDC__)
#	    define TRF_EXPORT(a,b) a _export b
#	else
#	    define TRF_EXPORT(a,b) a b
#	endif
#   endif
#else
#   define TRF_EXPORT(a,b) a b
#endif

/*
 * Exported tcl level procedures.
 *
 * ATTENTION:
 * due to the fact that cpp - processing with gcc 2.5.8 removes any comments
 * in macro-arguments (even if called with option '-C') i have to use the
 * predefined macro __C2MAN__ to distinguish real compilation and manpage
 * generation, removing _ANSI_ARGS_ in the latter case.
 */

/*
 * Initialize extension in standard interpreter.
 * Extends the interpreter with extension-specific
 * structures and installs the globally visible
 * command of Tcl-TRF.  Will catch attempts for
 * multiple initialization of an interpreter.
 */

#ifdef __C2MAN__
EXTERN int
Trf_Init (Tcl_Interp* interp /* interpreter to initialize */);
#else
EXTERN TRF_EXPORT (int,Trf_Init) _ANSI_ARGS_ ((Tcl_Interp* interp));
#endif

/*
 * Initialize extension in SAFE interpreter.
 * Same as --> Trf_Init.  The only security
 * relevant operations are reading from and
 * writing to a file.  As Tcl-Handles are
 * given to these commands it is assumed that
 * they were checked and cleared beforehand.
 */

#ifdef __C2MAN__
EXTERN int
Trf_SafeInit (Tcl_Interp* interp /* interpreter to initialize */);
#else
EXTERN TRF_EXPORT (int,Trf_SafeInit) _ANSI_ARGS_ ((Tcl_Interp* interp));
#endif

/*
 * Check initialization state of specified interpreter.
 * Check, wether this extension was initialized for the
 * specified interpreter or not.
 */

#ifdef __C2MAN__
EXTERN int
Trf_IsInitialized (Tcl_Interp* interp /* interpreter to check for initialization */);
#else
EXTERN int
Trf_IsInitialized _ANSI_ARGS_ ((Tcl_Interp* interp));
#endif

/*
 * Exported C level facilities.
 */

/*
 * Interface to registry of conversion procedures.
 */

/*
 * Structure used to remember the values of fundamental option(s).
 * The values currently defined remember:
 * * Handle and access-mode of the channel specified as argument
 *   to option '-attach'.
 * * Handle of channel specified as argument to '-in'.
 * * Handle of channel specified as argument to '-out'.
 */

typedef struct _Trf_BaseOptions_ {
  Tcl_Channel attach;      /* NULL => immediate mode                */
  int         attach_mode; /* access mode of 'attach' (if not NULL) */

  /* Relevant in immediate mode only! (attach == NULL) */
  Tcl_Channel source;      /* NULL => use non option argument as input */
  Tcl_Channel destination; /* NULL => leave transformation result in interpreter result area */
} Trf_BaseOptions;


/*
 * prototypes for procedures used to specify a data transformer.
 *
 * 1) vectors for option processing
 * 2) vectors for data encode/decode.
 */

/*
 * opaque type for access to option structures.
 * mainly defined for more readability of the prototypes following.
 */

typedef ClientData   Trf_Options;

/*
 * Interface to procedures to create a container holding option values.
 * It is the responsibility of the procedure to create and
 * initialize a container to hold option values. An opaque
 * handle to the new container has to be returned.
 */

#ifdef __C2MAN__
typedef Trf_Options Trf_CreateOptions (ClientData clientData /* arbitrary information, as defined in
							      * Trf_TypeDefinition.clientData */);
#else
typedef Trf_Options Trf_CreateOptions _ANSI_ARGS_ ((ClientData clientData));
#endif


/*
 * Interface to proceduress to delete a container made with 'Trf_CreateOptions'.
 * It is the responsibility of this procedure to clear and release
 * all memory of the specified container (which must have been
 * created by the corresponding procedure of type 'Trf_CreateOptions').
 */

#ifdef __C2MAN__
typedef void Trf_DeleteOptions (Trf_Options options,   /* the container to destroy */
				ClientData  clientData /* arbitrary information, as defined in
							* Trf_TypeDefinition.clientData */);
#else
typedef void Trf_DeleteOptions _ANSI_ARGS_ ((Trf_Options options,
					     ClientData  clientData));
#endif


/*
 * Interface to procedures to check an option container.
 * The procedure has to check the contents of the specified
 * container for errors, consistency, etc. It is allowed to
 * set default values into unspecified slots. Return value
 * is a standard tcl error code. In case of failure and interp
 * not NULL an error message should be left in the result area
 * of the specified interpreter.
 */

#ifdef __C2MAN__
typedef int Trf_CheckOptions (Trf_Options            options, /* container with options to check */
			      Tcl_Interp*            interp,  /* interpreter to write error
							       * messages to (NULL possible!) */
			      CONST Trf_BaseOptions* baseOptions, /* info about common options */
			      ClientData             clientData /* arbitrary information, as defined
								 * in
								 * Trf_TypeDefinition.clientData */);
#else
typedef int Trf_CheckOptions _ANSI_ARGS_ ((Trf_Options            options,
					   Tcl_Interp*            interp,
					   CONST Trf_BaseOptions* baseOptions,
					   ClientData             clientData));
#endif

/*
 * Interface to procedures to define the value of an option.
 * The procedure takes the specified optionname (rejecting
 * illegal ones) and places the given optionvalue into the
 * container. All necessary conversions from a string to the
 * required type should be done here. Return value is a standard
 * tcl error code. In case of failure and interp not NULL an
 * error message should be left in the result area of the
 * specified interpreter.
 */

#ifdef __C2MAN__
typedef int Trf_SetOption (Trf_Options options,   /* container to place the value into */
			   Tcl_Interp* interp,    /* interpreter for error messages
						   * (NULL possible) */
			   CONST char* optname,   /* name of option to define */
			   CONST char* optvalue,  /* value to set into the container */
			   ClientData  clientData /* arbitrary information, as defined in
						   * Trf_TypeDefinition.clientData */);
#else
typedef int Trf_SetOption _ANSI_ARGS_ ((Trf_Options options,
					Tcl_Interp* interp,
					CONST char* optname,
					CONST char* optvalue,
					ClientData  clientData));
#endif

#if (TCL_MAJOR_VERSION < 8)
#define Tcl_Obj VOID /* create dummy for missing definition */
#endif

/*
 * Interface to procedures to define the value of an option.
 * The procedure takes the specified optionname (rejecting
 * illegal ones) and places the given optionvalue into the
 * container. All necessary conversions from a Tcl_Obj to the
 * required type should be done here. Return value is a standard
 * tcl error code. In case of failure and interp not NULL an
 * error message should be left in the result area of the
 * specified interpreter. This procedure makes sense for tcl
 * version 8 and above only
 */

#ifdef __C2MAN__
typedef int Trf_SetObjOption (Trf_Options    options,   /* container to place the value into */
			      Tcl_Interp*    interp,    /* interpreter for error messages
							 * (NULL possible) */
			      CONST char*    optname,   /* name of option to define */
			      CONST Tcl_Obj* optvalue,  /* value to set into the container */
			      ClientData     clientData /* arbitrary information, as defined in
							 * Trf_TypeDefinition.clientData */);
#else
typedef int Trf_SetObjOption _ANSI_ARGS_ ((Trf_Options options,
					Tcl_Interp*    interp,
					CONST char*    optname,
					CONST Tcl_Obj* optvalue,
					ClientData     clientData));
#endif




/*
 * Interface to procedures to query an option container.
 * The result value decides wether the encoder- or decoder-set of vectors
 * must be used during immediate execution of the transformer configured
 * with the container contents.
 *
 * Returns:
 * 0: use decoder.
 * 1: use encoder.
 */

#ifdef __C2MAN__
typedef int Trf_QueryOptions  (Trf_Options options,   /* option container to query */
			       ClientData  clientData /* arbitrary information, as defined in
						       * Trf_TypeDefinition.clientData */);
#else
typedef int Trf_QueryOptions  _ANSI_ARGS_ ((Trf_Options options,
					    ClientData  clientData));
#endif

/*
 * Structure to hold all vectors describing the processing of a specific option set.
 * The 5 vectors are used to create and delete containers, to check them for errors,
 * to set option values and to query them for usage of encoder or decoder vectors.
 */

typedef struct _Trf_OptionVectors_ {
  Trf_CreateOptions*  createProc; /* create container for option information */
  Trf_DeleteOptions*  deleteProc; /* delete option container */
  Trf_CheckOptions*   checkProc;  /* check defined options for consistency, errors, ... */
  Trf_SetOption*      setProc;    /* define an option value */
  Trf_SetObjOption*   setObjProc; /* define an option value via Tcl_Obj (Tcl 8.x) */
  Trf_QueryOptions*   queryProc;  /* query, wether encode (1) / decode (0) requested by options */
} Trf_OptionVectors;




/*
 * opaque type for access to the control structures of an encoder/decoder.
 * mainly defined for more readability of the following prototypes.
 */

typedef ClientData Trf_ControlBlock;

/*
 * Interface to procedures used by an encoder/decoder to write its transformation results.
 * Procedures of this type are called by an encoder/decoder to write
 * (partial) transformation results, decoupling the final destination
 * from result generation.  Return value is a standard tcl error code. In
 * case of failure and interp not NULL an error message should be left
 * in the result area of the specified interpreter.
 */

#ifdef __C2MAN__
typedef int Trf_WriteProc (ClientData     clientData /* arbitrary information, defined during
						      * controlblock creation */,
			   unsigned char* outString  /* buffer with characters to write */,
			   int            outLen     /* number of characters in buffer */,
			   Tcl_Interp*    interp     /* interpreter for error messages
						      * (NULL possible) */);
#else
typedef int Trf_WriteProc _ANSI_ARGS_ ((ClientData     clientData,
					unsigned char* outString,
					int            outLen,
					Tcl_Interp*    interp));
#endif

/*
 * Interface to procedure for creation of encoder/decoder control structures.
 * The procedure has to create a control structure for an encoder/decoder. The
 * structure must be initialized with the contents of the the option
 * container. Return value is an opaque handle aof the control structure or NULL
 * in case of failure. An error message should be left in the result area
 * of the specified interpreter then.
 */

#ifdef __C2MAN__
typedef Trf_ControlBlock Trf_CreateCtrlBlock (ClientData writeClientData /* arbitrary information
									  * given as clientdata
									  * to 'fun' */,
					      Trf_WriteProc fun     /* vector to use for writing
								     * generated results */,
					      Trf_Options   optInfo /* options to configure the
								     * control */,
					      Tcl_Interp*   interp  /* interpreter for error
								     * messages */,
					      ClientData clientData /* arbitrary information,
								     * as defined in
								     * Trf_TypeDefinition.clientData
								     */);
#else
typedef Trf_ControlBlock Trf_CreateCtrlBlock _ANSI_ARGS_ ((ClientData    writeClientData,
							   Trf_WriteProc fun,
							   Trf_Options   optInfo,
							   Tcl_Interp*   interp,
							   ClientData    clientData));
#endif

/*
 * Interface to procedure for destruction of encoder/decoder control structures.
 * It is the responsibility of the procedure to clear and release all memory
 * associated to the sspecified control structure (which must have been created
 * by the appropriate procedure of type 'Trf_CreateCtrlBlock').
 */

#ifdef __C2MAN__
typedef void Trf_DeleteCtrlBlock (Trf_ControlBlock ctrlBlock /* control structure to destroy */,
				  ClientData       clientData /* arbitrary information, as defined in
							       * Trf_TypeDefinition.clientData */);
#else
typedef void Trf_DeleteCtrlBlock _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
					       ClientData       clientData));
#endif

/*
 * Interface to procedures for transformation of a single character.
 * A procedure of this type is called to encode/decode a single
 * character. Return value is a standard tcl error code. In case of
 * failure and interp not NULL an error message should be left in the
 * result area of the specified interpreter. Only one of 'Trf_TransformCharacter'
 * and 'Trf_TransformBuffer' must be provided. This one is easier to
 * implement, the second one should be faster. If both are
 * provided, -> 'Trf_TransformBuffer' takes precedence.
 */

#ifdef __C2MAN__
typedef int Trf_TransformCharacter (Trf_ControlBlock ctrlBlock /* state of encoder/decoder */,
				    unsigned int     character /* character to transform */,
				    Tcl_Interp*      interp    /* interpreter for error messages
								* (NULL possible) */,
				    ClientData       clientData /* arbitrary information, as defined
								 * in Trf_TypeDefinition.clientData */);
#else
typedef int Trf_TransformCharacter _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						 unsigned int     character,
						 Tcl_Interp*      interp,
						 ClientData       clientData));
#endif

/*
 * Interface to procedures for transformation of character sequences.
 * A procedure of this type is called to encode/decode a complete buffer. Return
 * value is a standard tcl error code. In case of failure and interp not
 * NULL an error message should be left in the result area of the specified
 * interpreter. Only one of 'Trf_TransformCharacter' and 'Trf_TransformBuffer'
 * must be provided. The first named is easier to implement, this one should be
 * faster. If both are provided, -> 'Trf_TransformBuffer' takes precedence.
 */

#ifdef __C2MAN__
typedef int Trf_TransformBuffer (Trf_ControlBlock ctrlBlock /* state of encoder/decoder */,
				 unsigned char*   buf       /* characters to transform */,
				 int              bufLen    /* number of characters */,
				 Tcl_Interp*      interp    /* interpreter for error messages
							     * (NULL possible) */,
				 ClientData       clientData /* arbitrary information, as defined
							      * in Trf_TypeDefinition.clientData */);
#else
typedef int Trf_TransformBuffer _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
					      unsigned char*   buf,
					      int              bufLen,
					      Tcl_Interp*      interp,
					      ClientData       clientData));
#endif

/*
 * Interface to procedures used to flush buffered characters.
 * An encoder/decoder is allowed to buffer characters internally. A procedure
 * of this type is called just before destruction to invoke special processing
 * of such characters. Return value is a standard tcl error code. In case of
 * failure and interp not NULL an error message should be left in the result
 * area of the specified interpreter.
 */

#ifdef __C2MAN__
typedef int Trf_FlushTransformation (Trf_ControlBlock ctrlBlock  /* state of encoder/decoder */,
				     Tcl_Interp*      interp     /* interpreter for error messages
								  * (NULL posssible) */,
				     ClientData       clientData /* arbitrary information, as defined in
								  * Trf_TypeDefinition.clientData */);
#else
typedef int Trf_FlushTransformation _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
						  Tcl_Interp*      interp,
						  ClientData       clientData));
#endif

/*
 * Interface for procedures to reset the internal state of an encoder/decoder.
 * The generic io layer of tcl sometimes discards its input buffer. A procedure
 * of this type will be called in such a case to reset the internal state of the
 * control structure and to discard buffered characters.
 */

#ifdef __C2MAN__
typedef void Trf_ClearCtrlBlock (Trf_ControlBlock ctrlBlock /* state of encoder/decoder */,
				 ClientData       clientData /* arbitrary information, as defined in
							      * Trf_TypeDefinition.clientData */);
#else
typedef void Trf_ClearCtrlBlock _ANSI_ARGS_ ((Trf_ControlBlock ctrlBlock,
					      ClientData       clientData));
#endif

/*
 * Structure to hold all vectors describing a specific encoder/decoder.
 * The 5 vectors are ussed to create and delete the controlblock of the
 *encoder/coder, to transform a single character, to flush all internal
 * buffers and to reset the control.
 */

typedef struct _Trf_Vectors_ {
  Trf_CreateCtrlBlock*     createProc;  /* create control structure              */
  Trf_DeleteCtrlBlock*     deleteProc;  /* delete control structure              */
  Trf_TransformCharacter*  convertProc; /* process a single character            */
  Trf_TransformBuffer*     convertBufProc; /* process a buffer of characters     */
  Trf_FlushTransformation* flushProc;   /* flush possibly buffered characters    */
  Trf_ClearCtrlBlock*      clearProc;   /* reset internal control, clear buffers */
} Trf_Vectors;


/*
 * Structure describing a complete transformation.
 * Consists of option processor and vectors for encoder, decoder.
 */

typedef struct _Trf_TypeDefinition_ {
  CONST char*        name;       /* name of transformation, also name of
				  * created command */
  ClientData         clientData; /* reference to arbitrary information.
				  * This information is given to all vectors
				  * mentioned below. */
  Trf_OptionVectors* options;    /* reference to option description, can be
				  * shared between transformation descriptions */
  Trf_Vectors        encoder;    /* description of encoder */
  Trf_Vectors        decoder;    /* description of decoder */
} Trf_TypeDefinition;


/*
 * Register the specified transformation at the given interpreter.
 * Extends the given interpreter with a new command giving access
 * to the transformation described in 'type'. 'type->name' is used
 * as name of the command.
 */

#ifdef __C2MAN__
EXTERN int
Trf_Register (Tcl_Interp*               interp, /* interpreter to register at */
	      CONST Trf_TypeDefinition* type    /* transformation to register */);
#else
EXTERN int
Trf_Register _ANSI_ARGS_ ((Tcl_Interp* interp, CONST Trf_TypeDefinition* type));
#endif


/*
 * Interfaces for easier creation of certain classes of
 * transformations (message digests)
 */

/*
 * transformer class: conversions.
 *
 * There is no easier way to create a conversion transformer than
 * to create it from scratch (use template/cvt_template.c as
 * template). Additionally the option processor returned below must
 * be used.
 */

/*
 * Return the set of option processing procedures required by conversion transformers.
 */

#ifdef __C2MAN__
EXTERN Trf_OptionVectors*
Trf_ConverterOptions (void);
#else
EXTERN Trf_OptionVectors*
Trf_ConverterOptions _ANSI_ARGS_ ((void));
#endif

/*
 * Structure to hold the option information required by conversion transformers.
 * A structure of this type is created and manipulated by the set of procedures
 * returned from 'Trf_ConverterOptions'. 
 */

typedef struct _Trf_ConverterOptionBlock {
  int mode; /* converter mode */
} Trf_ConverterOptionBlock;

/*
 * Posssible modes of a conversions transformer:
 *
 * UNKNOWN: initial value, unspecified mode
 * ENCODE:  encode characters
 * DECODE:  decode characters
 */

#define TRF_UNKNOWN_MODE (0)
#define TRF_ENCODE_MODE  (1)
#define TRF_DECODE_MODE  (2)


/*
 * transformer class: message digests.
 *
 * The implementation of a message digest algorithm requires
 * 3 procedures interfacing the special MD-code with the common
 * code contained in this module (dig_opt.c, digest.c).
 */

/*
 * Interface to procedures for initialization of a MD context.
 * A procedure of this type is called to initialize the structure
 * containing the state of a special message digest algorithm. The
 * memory block was allocated by the caller, with the size as specified
 * in the 'Trf_MessageDigestDescription' structure of the algorithm.
 */

#ifdef __C2MAN__
typedef void Trf_MDStart (VOID* context /* state to initialize */);
#else
typedef void Trf_MDStart _ANSI_ARGS_ ((VOID* context));
#endif

/*
 * Interface to procedures for update of a MD context.
 * A procedure of this type is called for every character to hash
 * into the final digest.
 */

#ifdef __C2MAN__
typedef void Trf_MDUpdate (VOID* context /* state to update */,
			   unsigned int character /* character to hash into the state */);
#else
typedef void Trf_MDUpdate _ANSI_ARGS_ ((VOID* context, unsigned int character));
#endif

/*
 * Interface to procedures for update of a MD context.
 * A procedure of this type is called for character buffer to hash
 * into the final digest. This procedure is optional, its definition
 * has precedence over 'Trf_MDUpdate'.
 */

#ifdef __C2MAN__
typedef void Trf_MDUpdateBuf (VOID*          context /* state to update */,
			      unsigned char* buf     /* buffer to hash into the state */,
			      int            bufLen  /* number of characters in the buffer */);
#else
typedef void Trf_MDUpdateBuf _ANSI_ARGS_ ((VOID*          context,
					   unsigned char* buffer,
					   int            bufLen));
#endif

/*
 * Interface to procedures for generation of the final digest from a MD state.
 * A procedure of this type is called after processing the final character. It
 * is its responsibility to finalize the internal state of the MD algorithm and
 * to generate the resulting digest from this.
 */

#ifdef __C2MAN__
typedef void Trf_MDFinal (VOID* context /* state to finalize */,
			  VOID* digest  /* result area to fill */);
#else
typedef void Trf_MDFinal _ANSI_ARGS_ ((VOID* context, VOID* digest));
#endif

/*
 * Interface to procedures for check/manipulation of the environment (shared libraries, ...).
 * A procedure of this type is called before doing any sort of processing.
 */

#ifdef __C2MAN__
typedef int Trf_MDCheck (Tcl_Interp* interp /* the interpreter for error messages */);
#else
typedef int Trf_MDCheck _ANSI_ARGS_ ((Tcl_Interp* interp));
#endif

/*
 * Structure describing a message digest algorithm.
 * All information required by the common code to interface a message
 * digest algorithm with it is stored in structures of this type.
 */

typedef struct _Trf_MessageDigestDescription {
  char* name;                  /* name of message digest, also name
				* of command on tcl level */
  unsigned short context_size; /* size of the MD state structure
				* maintained by 'startProc', 'updateProc'
				* and 'finalProc' (in byte) */
  unsigned short digest_size;  /* size of the digest generated by the
				* described algorithm (in byte) */
  Trf_MDStart*     startProc;  /* initialize a MD state structure */
  Trf_MDUpdate*    updateProc; /* update the MD state for a single character */
  Trf_MDUpdateBuf* updateBufProc; /* update the MD state for a character
				     buffer */
  Trf_MDFinal*     finalProc;     /* generate digest from MD state */
  Trf_MDCheck*     checkProc;     /* check enviroment */

} Trf_MessageDigestDescription;

/*
 * Procedure to register a message digest algorithm in an interpreter.
 * The procedure registers the described MDA at the given interpreter. Return
 * value is a standard tcl error code. In case of failure an error message
 * should be left in the result area of the given interpreter.
 */

#ifdef __C2MAN__
EXTERN int
Trf_RegisterMessageDigest (Tcl_Interp* interp /* interpreter to register the MD algorithm at */,
			   CONST Trf_MessageDigestDescription* md_desc /* description of the MD
									* algorithm */);
#else
EXTERN int
Trf_RegisterMessageDigest _ANSI_ARGS_ ((Tcl_Interp* interp,
					CONST Trf_MessageDigestDescription* md_desc));
#endif


/*
 * Internal helper procedures worth exporting.
 */

/*
 * General purpose library loader functionality.
 * Used by -> TrfLoadZlib, -> TrfLoadLibdes.
 */

EXTERN int
Trf_LoadLibrary _ANSI_ARGS_ ((Tcl_Interp* interp, CONST char* libName,
			    VOID** handlePtr, char** symbols, int num));

EXTERN void
Trf_LoadFailed _ANSI_ARGS_ ((VOID** handlePtr));

/*
 * XOR the bytes in a buffer with a mask.
 * Internally used by the implementation of the
 * various stream modes available to blockciphers.
 */

#ifdef __C2MAN__
EXTERN void
Trf_XorBuffer (VOID* buffer, /* buffer to xor the mask with */
	       VOID* mask,   /* mask bytes xor'ed into the buffer */
	       int length    /* length of mask and buffer (in byte) */);
#else
EXTERN void
Trf_XorBuffer _ANSI_ARGS_ ((VOID* buffer, VOID* mask, int length));
#endif


/*
 * Shift the register.
 * The register is shifted 'shift' bytes to the left. The same
 * number of bytes from the left of the 2nd register ('in') is
 * inserted at the right of 'buffer' to replace the lost bytes.
 */

#ifdef __C2MAN__
EXTERN void
Trf_ShiftRegister (VOID* buffer,       /* data shifted to the left */
		   VOID* in,           /* 2nd register shifted into the buffer */
		   int   shift,        /* number of bytes to shift out (and in) */
		   int   buffer_length /* length of buffer and in (in byte) */);
#else
EXTERN void
Trf_ShiftRegister _ANSI_ARGS_ ((VOID* buffer, VOID* in,
				int shift, int buffer_length));
#endif


/*
 * Swap the bytes of all 2-byte words contained in the buffer.
 */

#ifdef __C2MAN__
EXTERN void
Trf_FlipRegisterShort (VOID* buffer, /* data to swap */
		       int   length  /* length of buffer (in byte) */);
#else
EXTERN void
Trf_FlipRegisterShort _ANSI_ARGS_ ((VOID* buffer, int length));
#endif

/*
 * Swap the bytes of all 4-byte words contained in the buffer.
 */

#ifdef __C2MAN__
EXTERN void
Trf_FlipRegisterLong (VOID* buffer, /* data to swap */
		      int   length  /* length of buffer (in byte) */);
#else
EXTERN void
Trf_FlipRegisterLong _ANSI_ARGS_ ((VOID* buffer, int length));
#endif

/*
 * End of exported interface
 */

#endif /* TRF_H */
