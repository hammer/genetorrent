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
         shift 1
         ;;
      --libdir=*)
         libdir=${arg##*=}
         shift 1
         ;;
      --srcdir=*)
         srcdir=${arg##*=}
         shift 1
         ;;
      --enable-logging=*)
         logging=${arg##*=}
         shift 1
         ;;
   esac
done

if test -z "${libdir}"; then
   libdir=${prefix}/lib
fi

${srcdir}/configure "${@}" --disable-geoip \
   --prefix=${prefix} --libdir=${libdir} --srcdir=${srcdir} \
   --disable-dht --enable-callbacklogger --enable-logging=${logging} \
   --libdir=${libdir}/GeneTorrent --includedir=${prefix}/include/GeneTorrent \
   --enable-shared --disable-static

