/*
  +----------------------------------------------------------------------+
  | COLOPL PHP TimeShifter.                                              |
  +----------------------------------------------------------------------+
  | Copyright (c) COLOPL, Inc.                                           |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | info@colopl.co.jp so we can mail you a copy immediately.             |
  +----------------------------------------------------------------------+
  | Author: Go Kudo <g-kudo@colopl.co.jp>                                |
  +----------------------------------------------------------------------+
*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "ext/date/php_date.h"
#include "ext/standard/info.h"
#include "php_colopl_timeshifter.h"
#include "colopl_timeshifter_arginfo.h"

#include "shared_memory.h"
#include "hook.h"

/* True global */
typedef struct {
	bool is_hooked;
	timelib_rel_time shift_interval;
} timeshifter_global_t;
sm_t timeshifter_global;

/* Module global */
ZEND_DECLARE_MODULE_GLOBALS(colopl_timeshifter);

PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("colopl_timeshifter.is_hook_pdo_mysql", "1", PHP_INI_SYSTEM, OnUpdateBool, is_hook_pdo_mysql, zend_colopl_timeshifter_globals, colopl_timeshifter_globals)
	STD_PHP_INI_ENTRY("colopl_timeshifter.is_hook_request_time", "1", PHP_INI_SYSTEM, OnUpdateBool, is_hook_request_time, zend_colopl_timeshifter_globals, colopl_timeshifter_globals)
	STD_PHP_INI_ENTRY("colopl_timeshifter.usleep_sec", "1", PHP_INI_ALL, OnUpdateLong, usleep_sec, zend_colopl_timeshifter_globals, colopl_timeshifter_globals)
	STD_PHP_INI_ENTRY("colopl_timeshifter.is_restore_per_request", "0", PHP_INI_ALL, OnUpdateBool, is_restore_per_request, zend_colopl_timeshifter_globals, colopl_timeshifter_globals)
PHP_INI_END()

void get_shift_interval(timelib_rel_time *time) {
	timeshifter_global_t tg;

	sm_read(&timeshifter_global, &tg);
	if (tg.is_hooked) {
		memcpy(time, &tg.shift_interval, sizeof(timelib_rel_time));
	}
}

void set_is_hooked(bool flag) {
	timeshifter_global_t tg;

	sm_read(&timeshifter_global, &tg);
	if (tg.is_hooked != flag) {
		tg.is_hooked = flag;
		sm_write(&timeshifter_global, &tg);
	}
}

bool get_is_hooked() {
	timeshifter_global_t tg;

	sm_read(&timeshifter_global, &tg);
	return tg.is_hooked;
}

ZEND_FUNCTION(Colopl_ColoplTimeShifter_register_hook)
{
	zval *intern;
	timeshifter_global_t tg;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(intern, php_date_get_interval_ce())
	ZEND_PARSE_PARAMETERS_END();

	/* Copy interval. */
	sm_read(&timeshifter_global, &tg);
	memcpy(&tg.shift_interval, Z_PHPINTERVAL_P(intern)->diff, sizeof(timelib_rel_time));
	tg.is_hooked = true;
	if (!sm_write(&timeshifter_global, &tg)) {
		RETURN_FALSE;
	}

	if (COLOPL_TS_G(is_hook_request_time)) {
		apply_request_time_hook();
	}

	RETURN_TRUE;
}

ZEND_FUNCTION(Colopl_ColoplTimeShifter_unregister_hook)
{
	set_is_hooked(false);
}

ZEND_FUNCTION(Colopl_ColoplTimeShifter_is_hooked)
{
	RETURN_BOOL(get_is_hooked());
}

PHP_MINIT_FUNCTION(colopl_timeshifter)
{
	REGISTER_INI_ENTRIES();

	if (COLOPL_TS_G(is_hook_pdo_mysql) == true) {
		register_pdo_hook();
	}

	if (!register_hooks()) {
		return FAILURE;
	}

	if (get_is_hooked() && COLOPL_TS_G(is_hook_request_time)) {
		apply_request_time_hook();
	}

	COLOPL_TS_G(pdo_mysql_orig_methods) = NULL;

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(colopl_timeshifter)
{
	UNREGISTER_INI_ENTRIES();

	if (!unregister_hooks()) {
		return FAILURE;
	}

	return SUCCESS;
}

PHP_RINIT_FUNCTION(colopl_timeshifter)
{
# if defined(ZTS) && defined(COMPILE_DL_COLOPL_TIMESHIFTER)
	ZEND_TSRMLS_CACHE_UPDATE();
# endif

	COLOPL_TS_G(orig_request_time) = 0;
	COLOPL_TS_G(orig_request_time_float) = 0.0;

	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(colopl_timeshifter)
{
	if (COLOPL_TS_G(is_restore_per_request) && get_is_hooked()) {
		set_is_hooked(false);
		if (!unregister_hooks()) {
			return FAILURE;
		}
	}

	return SUCCESS;
}

PHP_MINFO_FUNCTION(colopl_timeshifter)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "colopl_timeshifter support", "enabled");
	php_info_print_table_end();
}

PHP_GINIT_FUNCTION(colopl_timeshifter)
{
	timeshifter_global_t tg;

# if defined(ZTS) && defined(COMPILE_DL_COLOPL_TIMESHIFTER)
	ZEND_TSRMLS_CACHE_UPDATE();
# endif

	sm_init(&timeshifter_global, sizeof(timeshifter_global_t));
	sm_read(&timeshifter_global, &tg);
	tg.is_hooked = false;
	sm_write(&timeshifter_global, &tg);
}

PHP_GSHUTDOWN_FUNCTION(colopl_timeshifter)
{
	sm_free(&timeshifter_global);
}

zend_module_entry colopl_timeshifter_module_entry = {
	STANDARD_MODULE_HEADER,
	"colopl_timeshifter",
	ext_functions,
	PHP_MINIT(colopl_timeshifter),
	PHP_MSHUTDOWN(colopl_timeshifter),
	PHP_RINIT(colopl_timeshifter),
	PHP_RSHUTDOWN(colopl_timeshifter),
	PHP_MINFO(colopl_timeshifter),
	PHP_COLOPL_TIMESHIFTER_VERSION,
	PHP_MODULE_GLOBALS(colopl_timeshifter),
	PHP_GINIT(colopl_timeshifter),
	PHP_GSHUTDOWN(colopl_timeshifter),
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_COLOPL_TIMESHIFTER
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(colopl_timeshifter)
#endif
