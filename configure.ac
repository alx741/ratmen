AC_PREREQ([2.69])
AC_INIT([vmenu], [1.0], [alx741@riseup.net])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_FILES([Makefile src/Makefile])
CFLAGS="$CFLAGS -Wall -std=c11 -pedantic"

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([X11], [XGetDefault])

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h getopt.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strcasecmp strrchr strstr])

AC_OUTPUT
