AC_INIT([interflop-stdlib],[0.0.1],[interflop.project@gmail.com])
AM_SILENT_RULES([yes])
AC_CONFIG_AUX_DIR(autoconf)
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign])
AC_CONFIG_MACRO_DIRS([m4])
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AR
AC_CONFIG_HEADERS([config.h])
LT_INIT
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_SUBST(INTERFLOP_DIRNAME, interflop-${PACKAGE_VERSION})

AX_WARNINGS()
AX_LTO()

# Checks if the libquadmath library is available
AC_CHECK_LIB([quadmath], [fmaq], 
    [QUADMATH_LIBS="-lquadmath" HAVE_QUADMATH=yes],
    [QUADMATH_LIBS="" HAVE_QUADMATH=no])
AC_SUBST(QUADMATH_LIBS)
AM_CONDITIONAL([HAVE_QUADMATH], [test "x$HAVE_QUADMATH" = "xyes"])

AC_CONFIG_FILES([
 Makefile
 rng/Makefile
 fma/Makefile
 hashmap/Makefile
 iostream/Makefile
])
AC_OUTPUT