#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(fffll, 0.1, info@xqz.ca)
AC_CONFIG_SRCDIR([src/fffll.c])
#AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
LT_INIT

# AC_PROG_YACC
# AM_PROG_LEX

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([inttypes.h libintl.h malloc.h stddef.h stdlib.h string.h unistd.h math.h])
AC_CHECK_HEADERS([pcre.h], [], [AC_MSG_ERROR("Missing pcre library")])
AC_CHECK_HEADERS([curl/curl.h], [], [AC_MSG_ERROR("Missing curl library")])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset])

AC_OUTPUT([Makefile src/Makefile])
