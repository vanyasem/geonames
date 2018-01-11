AC_INIT(geonames, 0.2)

AM_INIT_AUTOMAKE([foreign])
AM_SILENT_RULES([yes])
AC_CONFIG_MACRO_DIR([m4])

LT_INIT([disable-static])
AC_PROG_CC
AM_PROG_CC_C_O

GTK_DOC_CHECK([1.21], [--flavour no-tmpl])

PKG_CHECK_MODULES(GIO, gio-2.0)
AC_SUBST(GLIB_COMPILE_RESOURCES, `$PKG_CONFIG --variable glib_compile_resources gio-2.0`)

AC_ARG_ENABLE([demo], [AS_HELP_STRING([--enable-demo], [build demo application (requires gtk)])], [], [enable_demo=no])
AM_CONDITIONAL([ENABLE_DEMO], [test x$enable_demo != xno])
AS_IF([test "x$enable_demo" != "xno"], [PKG_CHECK_MODULES(GTK, gtk+-3.0)])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
    Makefile
    data/Makefile
    src/Makefile
    src/geonames.pc
    tests/Makefile
    demo/Makefile
    doc/Makefile
    doc/reference/Makefile
    doc/reference/geonames-docs.xml
])
AC_OUTPUT
