#!/bin/bash

function usage
{
   cat << EOF
Usage:  ${0##*/} <local|rpm|source|logging|debug|clean|distclean|svnclean

where:
	local:	    builds and installs a local copy of GeneTorrent
	rpm:	    builds a binary RPM of GeneTorrent (also does 'local')
	source:	    builds a source tarball suitable for distribution (also
		    does 'local' and 'distclean')
	logging:    builds a verbose logging version of GeneTorrent and 
		    installs locally
	debug:      builds a debugging and verbose logging version of 
		    GeneTorrent and installs locally
	clean:	    perform a standard make clean
	distclean:  prepares environment for building a source release (also
		    does 'local')
	svnclean:   restores the environment to SVN checkout state (except
		    for edits to svn controlled files)
EOF
   exit 1
}

function bailout
{
   echo
   echo "============================================================================="
   echo "top level build script failure detected in in function $1" 
   echo "============================================================================="
   echo
   exit 1;
}


function build_libtorrent
{
   saveDir=${PWD}
   cd libtorrent
   ./autotool.sh || bailout $FUNCNAME
   ./configure ${*} --disable-geoip --disable-dht --prefix=/usr --with-boost-libdir=/usr/lib64 --libdir=/usr/lib64 CFLAGS="-g -O2" CXXFLAGS="-g -O2" || bailout $FUNCNAME
   make clean || bailout $FUNCNAME
   make -j 14 || bailout $FUNCNAME
   sudo make install || bailout $FUNCNAME
   cd ${saveDir}
}

function build_GeneTorrent
{
   saveDir=${PWD}
   cd GeneTorrent
   ./autogen.sh || bailout $FUNCNAME
   ./configure --prefix=/usr CFLAGS="-g -O2 -Wall" CXXFLAGS="-g -O2 -Wall" || bailout $FUNCNAME
   make clean || bailout $FUNCNAME
   make || bailout $FUNCNAME
   sudo make install || bailout $FUNCNAME
   cd ${saveDir}
}

function build_scripts
{
   saveDir=${PWD}
   cd scripts
   ./autogen.sh || bailout $FUNCNAME
   ./configure --prefix=/usr || bailout $FUNCNAME
   sudo make install || bailout $FUNCNAME
   cd ${saveDir}
}

function configureRPMBuild
{
   cd rpmbuild/SPECS

   sed -i "s/Version:.*/Version:        ${geneTorrentVer}/" GeneTorrent.spec

   cd - > /dev/null
   cd rpmbuild/SOURCES

   # dummy source hack
   if [[ -e GeneTorrent-${geneTorrentVer} || -e GeneTorrent-${geneTorrentVer}.tgz ]]
   then
      rm -f GeneTorrent-${geneTorrentVer} GeneTorrent-${geneTorrentVer}.tgz
   fi
   cp -ar GeneTorrent-template GeneTorrent-${geneTorrentVer}
   tar czf GeneTorrent-${geneTorrentVer}.tgz GeneTorrent-${geneTorrentVer}

   cd - > /dev/null
}

function buildRPMS
{
   if [[ -e ~/rpmbuild ]]
   then
      rm -rf ~/rpmbuild
   fi

   cp -ar rpmbuild ~/.
   cd ~/rpmbuild/SPECS

   rpmbuild -bb GeneTorrent.spec

   cd - > /dev/null
}

function collectRPMS
{
   rm -rf ~/GeneTorrent-${geneTorrentVer}
   mkdir ~/GeneTorrent-${geneTorrentVer}

   cd ~/rpmbuild/RPMS/x86_64
   cp GeneTorrent-${geneTorrentVer}-1.el6.CP.x86_64.rpm ~/GeneTorrent-${geneTorrentVer}/.

   cd - > /dev/null

   cp ${startDir}/release.notes.txt ~/GeneTorrent-${geneTorrentVer}/.

   cd

   tar czf GeneTorrent-${geneTorrentVer}.tgz GeneTorrent-${geneTorrentVer}

   echo "All done!  GeneTorrent-${geneTorrentVer}.tgz is available in `pwd` for testing and delivery to the client."
}

function build_common
{
      build_GeneTorrent
      build_scripts
}

function build_standard
{
   build_libtorrent --enable-callbacklogger --enable-logging=minimal 
   build_common
}

function build_rpm
{
      geneTorrentVer="`grep AC_INIT GeneTorrent/configure.ac|cut -d, -f2|tr -d ')'|awk '{print $1}'`"
      configureRPMBuild
      buildRPMS
      collectRPMS
}

[[ $#1 -lt 1 ]] && usage

case $1 in 
   local)
      build_standard
      ;;

   rpm)
      build_standard
      build_rpm
      ;;
      
   source)
      ;;

   logging)
      build_libtorrent --enable-logging=verbose 
      build_common
      ;;
      
   debug)
      build_libtorrent --enable-debug --enable-logging=verbose 
      build_common
      ;;

   clean)

      ;;
      
   distclean)

      ;;

   svnclean)

      ;;
      
   *)
      usage
      ;;
esac

