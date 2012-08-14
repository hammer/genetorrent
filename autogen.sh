#!/bin/bash

set -e # bail out on any error

echo "Running aclocal --force -I m4"
aclocal --force -I m4
echo "Running autoheader -f"
autoheader -f
echo "Running autoconf -f"
autoconf -f
echo "Running libtoolize"
libtoolize
echo "Running automake --foreign"
automake --foreign --add-missing
echo "Running libtorrent/autotool.sh"
pushd libtorrent > /dev/null
./autotool.sh
popd > /dev/null

