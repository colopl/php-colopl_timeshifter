dnl config.m4 for colopl_timeshifter extension

dnl Check for headers needed by timelib
AC_CHECK_HEADERS([io.h])

PHP_COLOPL_TIMESHIFTER_CFLAGS="-Wno-implicit-fallthrough -I@ext_builddir@/third_party/timelib -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -DHAVE_TIMELIB_CONFIG_H=1"
timelib_sources="third_party/timelib/dow.c third_party/timelib/timelib.c third_party/timelib/tm2unixtime.c third_party/timelib/unixtime2tm.c third_party/timelib/parse_posix.c third_party/timelib/parse_tz.c third_party/timelib/interval.c"
timeshifter_sources="hook.c shared_memory.c"

PHP_ARG_ENABLE(colopl_timeshifter, whether to enable colopl_timeshifter support,
[  --enable-colopl_timeshifter           Enable colopl_timeshifter support])

if test "$PHP_COLOPL_TIMESHIFTER" != "no"; then
  PHP_NEW_EXTENSION(colopl_timeshifter, colopl_timeshifter.c $timeshifter_sources $timelib_sources, $ext_shared,, $PHP_COLOPL_TIMESHIFTER_CFLAGS)
fi

PHP_ADD_BUILD_DIR([$ext_builddir/third_party/timelib], 1)
PHP_ADD_INCLUDE([$ext_builddir/third_party/timelib])
PHP_ADD_INCLUDE([$ext_srcdir/third_party/timelib])

AC_DEFINE([HAVE_TIMELIB_CONFIG_H], [1], [Have timelib_config.h])

dnl Use Zend Memory Manager in timelib
cat > $ext_builddir/third_party/timelib/timelib_config.h <<EOF
#ifdef PHP_WIN32
# include "config.w32.h"
#else
# include <php_config.h>
#endif
#include <inttypes.h>
#include <stdint.h>

#include "zend.h"

#define timelib_malloc  emalloc
#define timelib_realloc erealloc
#define timelib_calloc  ecalloc
#define timelib_strdup  estrdup
#define timelib_strndup estrndup
#define timelib_free    efree
EOF
