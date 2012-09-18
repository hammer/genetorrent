#!/bin/bash

# wrapper script to call libtorrent configure

prefix="/usr/local"
libdir=
srcdir="."
prefix_regex="^--prefix=(.*)$"
libdir_regex="^--libdir=(.*)$"
srcdir_regex="^--srcdir=(.*)$"

for arg in "${@}"
do
   if [[ "${arg}" =~ $prefix_regex ]]; then
      prefix=${BASH_REMATCH[1]}
      echo "found prefix: ${prefix}"
   fi
   if [[ "${arg}" =~ $libdir_regex ]]; then
      libdir=${BASH_REMATCH[1]}
      echo "found libdir: ${libdir}"
   fi
   if [[ "${arg}" =~ $srcdir_regex ]]; then
      srcdir=${BASH_REMATCH[1]}
      echo "found srcdir: ${srcdir}"
   fi
   echo $arg
done

if [[ -z "${libdir}" ]]; then
   libdir=${prefix}/lib
fi

${srcdir}/configure "${@}" --with-boost-system=boost_system --disable-geoip \
   --disable-dht --enable-callbacklogger --enable-logging=minimal \
   --libdir=${libdir}/GeneTorrent --includedir=${prefix}/include/GeneTorrent \
   --enable-shared --disable-static

