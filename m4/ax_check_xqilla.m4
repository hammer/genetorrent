# SYNOPSIS
#
#   AX_CHECK_XQILLA([action-if-found[, action-if-not-found]])
#
# DESCRIPTION
#
#   Look for XQilla in a number of default spots, or in a user-selected
#   spot (via --with-xqilla).  Sets
#
#     XQILLA_INCLUDES to the include directives required
#     XQILLA_LIBS to the -l directives required
#     XQILLA_LDFLAGS to the -L or -R flags required
#
#   and calls ACTION-IF-FOUND or ACTION-IF-NOT-FOUND appropriately
#
#   This macro sets XQILLA_INCLUDES such that source files should use the
#   xqilla/ directory in include directives:
#
#     #include <xqilla/xqilla-simple.hpp>
#
# LICENSE
#
#   Adapted from ax_check_openssl.m4 by:
#   Copyright (c) 2009,2010 Zmanda Inc. <http://www.zmanda.com/>
#   Copyright (c) 2009,2010 Dustin J. Mitchell <dustin@zmanda.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

AU_ALIAS([CHECK_SSL], [AX_CHECK_XQILLA])
AC_DEFUN([AX_CHECK_XQILLA], [
    found=false
    AC_ARG_WITH([xqilla],
        [AS_HELP_STRING([--with-xqilla=DIR],
            [root of the XQilla directory])],
        [
            case "$withval" in
            "" | y | ye | yes | n | no)
            AC_MSG_ERROR([Invalid --with-xqilla value])
              ;;
            *) xqilladirs="$withval"
              ;;
            esac
        ], [
            # if pkg-config is installed and xqilla has installed a .pc file,
            # then use that information and don't search xqilladirs
            AC_PATH_PROG([PKG_CONFIG], [pkg-config])
            if test x"$PKG_CONFIG" != x""; then
                XQILLA_LDFLAGS=`$PKG_CONFIG xqilla --libs-only-L 2>/dev/null`
                if test $? = 0; then
                    XQILLA_LIBS=`$PKG_CONFIG xqilla --libs-only-l 2>/dev/null`
                    XQILLA_INCLUDES=`$PKG_CONFIG xqilla --cflags-only-I 2>/dev/null`
                    found=true
                fi
            fi

            # no such luck; use some default xqilladirs
            if ! $found; then
                xqilladirs="/usr/pkg /usr/local /usr"
            fi
        ]
        )


    # note that we #include <xqilla/foo.h>, so the XQilla headers have to be in
    # an 'xqilla' subdirectory

    if ! $found; then
        XQILLA_INCLUDES=
        for xqilladir in $xqilladirs; do
            AC_MSG_CHECKING([for xqilla/xqilla-simple.hpp in $xqilladir])
            if test -f "$xqilladir/include/xqilla/xqilla-simple.hpp"; then
                XQILLA_INCLUDES="-I$xqilladir/include"
                XQILLA_LDFLAGS="-L$xqilladir/lib"
                XQILLA_LIBS="-lxqilla"
                found=true
                AC_MSG_RESULT([yes])
                break
            else
                AC_MSG_RESULT([no])
            fi
        done

        # if the file wasn't found, well, go ahead and try the link anyway -- maybe
        # it will just work!
    fi

    # try the preprocessor and linker with our new flags,
    # being careful not to pollute the global LIBS, LDFLAGS, and CPPFLAGS

    AC_MSG_CHECKING([whether compiling and linking against XQilla works])
    echo "Trying link with XQILLA_LDFLAGS=$XQILLA_LDFLAGS;" \
        "XQILLA_LIBS=$XQILLA_LIBS; XQILLA_INCLUDES=$XQILLA_INCLUDES" >&AS_MESSAGE_LOG_FD

    save_LIBS="$LIBS"
    save_LDFLAGS="$LDFLAGS"
    save_CPPFLAGS="$CPPFLAGS"
    LDFLAGS="$LDFLAGS $XQILLA_LDFLAGS $XERCES_LDFLAGS"
    LIBS="$XQILLA_LIBS $LIBS $XERCES_LIBS"
    CPPFLAGS="$XQILLA_INCLUDES $CPPFLAGS $XERCES_CPPFLAGS"
    AC_LINK_IFELSE(
        [AC_LANG_PROGRAM([#include <cstddef>
            #include <xqilla/xqilla-simple.hpp>;], [ XQilla xqilla; ])],
        [
            AC_MSG_RESULT([yes])
            $1
        ], [
            AC_MSG_RESULT([no])
            $2
        ])
    CPPFLAGS="$save_CPPFLAGS"
    LDFLAGS="$save_LDFLAGS"
    LIBS="$save_LIBS"

    AC_SUBST([XQILLA_INCLUDES])
    AC_SUBST([XQILLA_LIBS])
    AC_SUBST([XQILLA_LDFLAGS])
])
