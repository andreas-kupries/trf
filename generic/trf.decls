# trf.decls --
#
#	This file contains the declarations for all supported public
#	functions that are exported by the Trf library via the stubs table.
#	This file is used to generate the trfDecls.h file.
#	

library trf

# Define the tcl interface with several sub interfaces:
#     tclPlat	 - platform specific public
#     tclInt	 - generic private
#     tclPlatInt - platform specific private

interface trf
hooks {trfInt}

# Declare each of the functions in the public Trf interface.  Note that
# every index should never be reused for a different function in order
# to preserve backwards compatibility.

declare 0 generic {
    int Trf_IsInitialized(Tcl_Interp *interp)
}
declare 1 generic {
    int Trf_Register(Tcl_Interp *interp, CONST Trf_TypeDefinition *type)
}
declare 2 generic {
    Trf_OptionVectors* Trf_ConverterOptions(void)
}
declare 3 generic {
    int Trf_LoadLibrary(Tcl_Interp *interp, CONST char *libName,
	    VOID **handlePtr, char **symbols, int num)
}
declare 4 generic {
    void Trf_LoadFailed(VOID** handlePtr)
}
