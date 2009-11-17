#! /bin/sh

usage() {
    echo "\
Usage: build-tau [-h|--help] [options to be passed to build]

Build the tau source code into rpms.  All arguments are simply passed
to the build command, which should exist in \$PATH.  Make certain
to set and export any variables expected by the build command.

To build using build.rpm (/usr/bin/build):

	export BUILD_ROOT=/tmp/tau
	./build-tau --rpms <path to pkg repo 1>:<path to pkg repo 2>:<...>

To build using autobuild's build command (/opt/SuSE/bin/build):

	export BUILD_ROOT=/tmp/tau
	export BUILD_DIST=sles10-oes2-sp-i386
	./build-tau

In both of the above examples, the built rpms will be placed in
/tmp/tau/usr/src/packages/RPMS.
"
    exit $1
}

case "$1" in
-h|--help)
    usage 0
    ;;
esac

# Create tarball from the svn tau source
rm -rf ./tau.pj ./tau.pj.tar.bz2
cp -rf ../tau.pj .
find ./tau.pj -name ".svn" -exec rm -rf {} \;
tar -cvjf ./tau.pj.tar.bz2 ./tau.pj
rm -rf ./tau.pj

# Build the rpms
build $@
