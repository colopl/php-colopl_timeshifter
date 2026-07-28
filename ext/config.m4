dnl config.m4 for colopl_timeshifter extension

PHP_COLOPL_TIMESHIFTER_CFLAGS="-Wno-implicit-fallthrough -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1"
timeshifter_sources="hook.c shared_memory.c"

PHP_ARG_ENABLE(colopl_timeshifter, whether to enable colopl_timeshifter support,
[  --enable-colopl_timeshifter           Enable colopl_timeshifter support])

PHP_ARG_ENABLE(colopl_timeshifter_pdo, whether to enable the PDO (MySQL) hook,
[  --disable-colopl_timeshifter_pdo      Disable PDO (MySQL) hook support], yes, no)

if test "$PHP_COLOPL_TIMESHIFTER" != "no"; then
  dnl PDO is an optional dependency: the PDO (MySQL) hook is compiled in only
  dnl when the PDO headers are available and the hook is not explicitly
  dnl disabled. Whether PDO is actually loaded is checked again at runtime.
  if test "$PHP_COLOPL_TIMESHIFTER_PDO" != "no"; then
    AC_MSG_CHECKING([for PDO includes])
    if test -f "$abs_srcdir/include/php/ext/pdo/php_pdo_driver.h"; then
      colopl_timeshifter_have_pdo=yes
    elif test -f "$abs_srcdir/ext/pdo/php_pdo_driver.h"; then
      colopl_timeshifter_have_pdo=yes
    elif test -f "$phpincludedir/ext/pdo/php_pdo_driver.h"; then
      colopl_timeshifter_have_pdo=yes
    else
      colopl_timeshifter_have_pdo=no
    fi

    if test "$colopl_timeshifter_have_pdo" = "yes"; then
      AC_MSG_RESULT([yes])
      AC_DEFINE([COLOPL_TIMESHIFTER_HAVE_PDO], [1], [Define to 1 when the PDO headers are available.])
    else
      AC_MSG_RESULT([no, building without PDO hook support])
    fi
  fi

  PHP_NEW_EXTENSION(colopl_timeshifter, colopl_timeshifter.c $timeshifter_sources, $ext_shared,, $PHP_COLOPL_TIMESHIFTER_CFLAGS)
fi
