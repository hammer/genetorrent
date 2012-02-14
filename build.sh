#!/bin/bash

buildThreads="4"

function usage
{
   cat << EOF
Usage:  ${0##*/} <local|rpm|source|logging|debug|clean|distclean|svnclean

where:
	local:	    builds and installs a local copy of GeneTorrent
	full:       Runs autoconf tools, builds and installs a local copy
                    of GeneTorrent
	rpm:	    builds a binary RPM of GeneTorrent (also does 'local')
	source:	    builds a source tarball suitable for distribution (also
		    does 'local' and 'distclean')
	logging:    builds a verbose logging version of GeneTorrent and 
		    installs locally
	debug:      builds a debugging and verbose logging version of 
		    GeneTorrent and installs locally
	make:	    perform a standard make 
	install:    perform a standard make install
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
   [[ -e ../.fullbuild ]] && { ./autotool.sh || bailout $FUNCNAME 
                             }
   ./configure ${*} --disable-geoip --disable-dht --prefix=/usr --enable-static --disable-shared --with-boost-libdir=/usr/lib64 --libdir=/usr/lib64 CFLAGS="-g -O2" CXXFLAGS="-g -O2" || bailout $FUNCNAME
   make clean || bailout $FUNCNAME
   make -j ${buildThreads} || bailout $FUNCNAME
   sudo make install || bailout $FUNCNAME
   cd ${saveDir}
}

function build_GeneTorrent
{
   saveDir=${PWD}
   cd GeneTorrent
   [[ -e ../.fullbuild ]] && { ./autogen.sh || bailout $FUNCNAME 
                             }
   ./configure --prefix=/usr CFLAGS="-g -O2 -Wall" CXXFLAGS="-g -O2 -Wall" || bailout $FUNCNAME
   make clean || bailout $FUNCNAME
   make -j ${buildThreads} || bailout $FUNCNAME
   sudo make install || bailout $FUNCNAME
   cd ${saveDir}
}

function build_scripts
{
   saveDir=${PWD}
   cd scripts
   [[ -e ../.fullbuild ]] && { ./autogen.sh || bailout $FUNCNAME 
                             }
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
   cp GeneTorrent-${geneTorrentVer}-1.el6.CP.x86_64.rpm ~/.

   cd - > /dev/null

   ## cp ${startDir}/release.notes.txt ~/GeneTorrent-${geneTorrentVer}/.

   cd

   echo "All done!  GeneTorrent-${geneTorrentVer} is available in ~ for testing and delivery to the client."
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

function build_libtorrent_maker
{
   make $1 || bailout $FUNCNAME
}

function build_GeneTorrent_maker
{
   make $1 || bailout $FUNCNAME
}

function build_scripts_maker
{
   make $1 || bailout $FUNCNAME
}

function maker
{
   for dir in libtorrent GeneTorrent scripts
   do
      saveDir=${PWD}
      cd ${dir}
      build_${dir}_maker $1
      cd ${saveDir}
   done
}

function svn_clean
{
   cd libtorrent
   [[ -e Makefile ]] && make maintainer-clean
   rm -f config.log config.report configure
   rm -f m4/libtool.m4 m4/lt~obsolete.m4 m4/ltsugar.m4 m4/ltversion.m4 m4/ltoptions.m4 aclocal.m4 
   rm -fr autom4te.cache build-aux
   rm -f Makefile Makefile.in
   rm -f src/Makefile src/Makefile.in
   rm -f include/libtorrent/Makefile include/libtorrent/Makefile.in
   rm -f examples/Makefile examples/Makefile.in
   rm -f test/Makefile test/Makefile.in
   rm -f bindings/Makefile bindings/Makefile.in
   rm -f bindings/python/Makefile bindings/python/Makefile.in
   chmod a-x docs/*.rst docs/*.htm* src/*.cpp include/libtorrent/*.hpp
   cd - > /dev/null

   cd GeneTorrent
   [[ -e Makefile ]] && make maintainer-clean
   rm -fr autom4te.cache
   rm -f configure
   rm -f Makefile.in
   rm -f aclocal.m4
   rm -f src/Makefile.in
   rm -f src/config.h.in
   cd - > /dev/null


   cd scripts
   [[ -e Makefile ]] && make maintainer-clean
   rm -fr autom4te.cache
   rm -f configure
   rm -f Makefile.in
   rm -f missing install-sh
   rm -f aclocal.m4
   cd - > /dev/null

   cd rpmbuild/SOURCES
   rm -fr GeneTorrent-[0-9].[0-9].[0-9].[0-9]*
   cd - > /dev/null
}

function build_source
{
   cd ${bDir}
   maker distclean
   cd ..
   tar czvf GeneTorrent-${geneTorrentVer}.src.tgz --exclude-vcs --exclude="\.*" --exclude="rpmbuild" $1
   cp GeneTorrent-${geneTorrentVer}.src.tgz ~/.
   cd - >/dev/null
}

[[ $#1 -lt 1 ]] && usage

bDir=${PWD}

if [[ `hostname -s` = "radon" || `hostname -s` = "xenon" ]]
then
   buildThreads="16"
fi

case $1 in 
   local)
      [[ ! -e libtorrent/configure ]] && touch .fullbuild
      build_standard
      rm -f .fullbuild
      ;;

   full)
      touch .fullbuild
      build_standard
      rm -f .fullbuild
      ;;

   rpm)
      [[ ! -e libtorrent/configure ]] && touch .fullbuild
      build_standard
      build_rpm
      rm -f .fullbuild
      ;;
      
   source)
      build_source ${bDir##*/}
      ;;

   logging)
      [[ ! -e libtorrent/configure ]] && touch .fullbuild
      build_libtorrent --enable-logging=verbose 
      build_common
      ;;
      
   debug)
      build_libtorrent --enable-debug --enable-logging=verbose 
      build_common
      ;;

   clean)
      maker clean
      ;;
      
   distclean)
      maker distclean
      ;;

   svnclean)
      svn_clean
      ;;

   make)   # hidden option
      maker "-j4"
      ;;

   install)   # hidden option
      maker install
      ;;

   release)   # hidden option
      touch .fullbuild
      build_standard
      rm -f .fullbuild
      build_rpm
      build_source ${bDir##*/}
      ;;

   *)
      usage
      ;;
esac
