dnl initialize autoconf
AC_INIT(dtq, 0.1.1)

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR(m4)
AC_CONFIG_SRCDIR(src/dtq.c)

dnl initialize automake
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS(config.h)
AM_MAINTAINER_MODE

dnl required programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_YACC
AC_PROG_LEX
AC_PROG_MAKE_SET

AC_CHECK_LIB([fdt], [fdt_get_path],,AC_MSG_ERROR([libftd not found]))

dnl output directive
AC_OUTPUT(Makefile src/Makefile)
