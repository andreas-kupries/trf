# Tcl package index file, version 1.0
#
# Window package index for Trf @mFullVersion@ (as of @mDate@)
#

proc trfifneeded dir {
    rename trfifneeded {}
    if {[package vcompare [info tclversion] 8.1] >= 0} {
	set version {}
    } else {
	regsub {\.} [info tclversion] {} version
    }
    package ifneeded Trf @mFullVersion@ "load [list [file join $dir trf@mShortDosVersion@$version.dll]] Trf"
}
trfifneeded $dir
