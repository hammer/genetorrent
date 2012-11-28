#!/bin/bash

# wrapper script to call libtorrent configure

prefix="/usr/local"
libdir=
srcdir="."
logging="minimal"

prefix_regex="^--prefix=(.*)$"
libdir_regex="^--libdir=(.*)$"
srcdir_regex="^--srcdir=(.*)$"
logging_regex="^--enable-logging=(.*)$"

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
   if [[ "${arg}" =~ $logging_regex ]]; then
      logging=${BASH_REMATCH[1]}
      echo "found logging option: ${logging}"
   fi
   echo $arg
done

if [[ -z "${libdir}" ]]; then
   libdir=${prefix}/lib
fi

${srcdir}/configure "${@}" --disable-geoip \
   --disable-dht --enable-callbacklogger --enable-logging=${logging} \
   --libdir=${libdir}/GeneTorrent --includedir=${prefix}/include/GeneTorrent \
   --enable-shared --disable-static

