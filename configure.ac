#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([fibril], [0.0.2], [chaoran@rice.edu])
AM_INIT_AUTOMAKE([-Wall -Wno-extra-portability -Werror foreign])
LT_PREREQ([2.2])
LT_INIT
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/fibril.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for command-line.
FIBRIL_IF_ENABLED([debug], [Build fibril in debugging mode],
  [
   case "${enable_debug}" in 1|2|3) ;; *) enable_debug=0 ;; esac
   AC_DEFINE_UNQUOTED([FIBRIL_DEBUG], [${enable_debug}], [Fibril debug enabled])
  ])
FIBRIL_IF_ENABLED([stats], [Enable statistics collection],
  [ AC_DEFINE([FIBRIL_STATS], [1], [Enable statistics collection.]) ])
FIBRIL_IF_ENABLED([log], [Enable logging collection],
  [ AC_DEFINE([FIBRIL_LOG], [1], [Enable logging collection.]) ])

# Check for pthreads
ACX_PTHREAD([LIBS="$PTHREAD_LIBS $LIBS"
             CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
             CC="$PTHREAD_CC"])

# Checks for libraries.

FIBRIL_IF_ENABLED([hwloc], [Enable hwloc],
  [
    AC_DEFINE([FIBRIL_USE_HWLOC], [1], [Enable hwloc.])
    LIBS="`pkg-config --cflags hwloc` $LIBS"
    CFLAGS="$CFLAGS `pkg-config --libs hwloc`"
  ])

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdint.h stdlib.h unistd.h pthread.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MMAP
AC_CHECK_FUNCS([mmap, madvise])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
                 benchmark/Makefile
                 benchmark/cilkplus/Makefile
                 benchmark/tbb/Makefile
                 benchmark/serial/Makefile])
AC_OUTPUT
