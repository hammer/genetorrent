#!/bin/sh

# wrapper script to call libtorrent configure

prefix=/usr/local
libdir=
srcdir=.
logging=minimal

for arg in "${@}"
do
   case ${arg} in
      --prefix=*)
         prefix=${arg##*=}
         ;;
      --libdir=*)
         libdir=${arg##*=}
         ;;
      --srcdir=*)
         srcdir=${arg##*=}
         ;;
      --enable-logging=*)
         logging=${arg##*=}
         ;;
   esac
done

if test -z "${libdir}"; then
   libdir=${prefix}/lib
fi

${srcdir}/configure "${@}" --disable-geoip \
   --prefix=${prefix} --srcdir=${srcdir} \
   --disable-dht --enable-callbacklogger --enable-logging=${logging} \
   --libdir=${libdir}/GeneTorrent --includedir=${prefix}/include/GeneTorrent \
   --enable-shared --disable-static

