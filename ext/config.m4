dnl config.m4 for colopl_timeshifter extension

PHP_COLOPL_TIMESHIFTER_CFLAGS="-Wno-implicit-fallthrough -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1"
timeshifter_sources="hook.c shared_memory.c"

PHP_ARG_ENABLE(colopl_timeshifter, whether to enable colopl_timeshifter support,
[  --enable-colopl_timeshifter           Enable colopl_timeshifter support])

if test "$PHP_COLOPL_TIMESHIFTER" != "no"; then
  PHP_NEW_EXTENSION(colopl_timeshifter, colopl_timeshifter.c $timeshifter_sources, $ext_shared,, $PHP_COLOPL_TIMESHIFTER_CFLAGS)
fi
