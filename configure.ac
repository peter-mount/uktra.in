AC_INIT([uktrain], [0.1], [peter@retep.org])

# Check for gcc
AC_PROG_CC
AC_LANG(C)

# Math library
#AC_CHECK_LIB(m, cos)

# The command's to build
APPS="uktrain"

# Project base for include paths
LDFLAGS="$LDFLAGS -L../build/package/usr/lib"

# header files
CPPFLAGS="$CPPFLAGS -I../include"

# This is a shared library so compile position independent code
CCFLAGS="$CCFLAGS -fPIC"

# Final list of libraries required in the correct sequence.

LDFLAGS="$LDFLAGS -larea51-template -larea51-rest -larea51-httpd -larea51 -ljson-c -lmicrohttpd -lcurl"

# Our core library (i.e. under lib)
LDFLAGS="-luktrain $LDFLAGS"

# Core C libraries
LDFLAGS="$LDFLAGS -lpthread -lm"

DIRS="lib $APPS"

# Add web content first
DIRS="web $DIRS"

AC_SUBST([DIRECTORIES],[$DIRS])

AC_SUBST([CPPFLAGS],[$CPPFLAGS])
AC_SUBST([CCFLAGS],[$CCFLAGS])
AC_SUBST([LDFLAGS],[$LDFLAGS])

# Extract some details out of our APKBuild file, mainly architecture, version etc
head APKBUILD >/tmp/apkbuild
. /tmp/apkbuild
AC_SUBST([PKGVER],["$pkgver"])
AC_SUBST([PKGREL],["$pkgrel"])
date=$(date -u "+%FT%TZ")
AC_SUBST([PKGDATE],["$date"])
AC_SUBST([PKGBUILD],["$pkgname ${pkgver}r${pkgrel} $arch $date"])

# Build make files
MAKEFILES="SubMakefile $(find . -type f -name Makefile.in|sed s/\\.in//g)"
MAKEFILES="$MAKEFILES $(find . -type f -name '*.h.in'|sed s/\\.in//g)"
AC_CONFIG_FILES([$MAKEFILES])

AC_OUTPUT()

AC_MSG_NOTICE([Now run make clean;make])
