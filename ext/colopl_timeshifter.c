/*
  +----------------------------------------------------------------------+
  | COLOPL PHP TimeShifter.                                              |
  +----------------------------------------------------------------------+
  | Copyright (c) COLOPL, Inc.                                           |
  +----------------------------------------------------------------------+
  | This source file is subject to the BSD-3-Clause license that is      |
  | bundled with this package in the file LICENSE.                       |
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
	colopl_timeshifter_interval_t interval;
} timeshifter_global_t;
sm_t timeshifter_global;

/* Module global */
ZEND_DECLARE_MODULE_GLOBALS(colopl_timeshifter);

PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("colopl_timeshifter.is_hook_pdo_mysql", "1", PHP_INI_SYSTEM, OnUpdateBool, is_hook_pdo_mysql, zend_colopl_timeshifter_globals, colopl_timeshifter_globals)
	STD_PHP_INI_ENTRY("colopl_timeshifter.is_hook_request_time", "1", PHP_INI_SYSTEM, OnUpdateBool, is_hook_request_time, zend_colopl_timeshifter_globals, colopl_timeshifter_globals)
	STD_PHP_INI_ENTRY("colopl_timeshifter.is_restore_per_request", "0", PHP_INI_ALL, OnUpdateBool, is_restore_per_request, zend_colopl_timeshifter_globals, colopl_timeshifter_globals)
PHP_INI_END()

static bool is_supported_interval_object(zval *interval)
{
	if (Z_OBJCE_P(interval) != php_date_get_interval_ce()) {
		return false;
	}

	if (Z_OBJ_P(interval)->properties && zend_hash_num_elements(Z_OBJ_P(interval)->properties) > 0) {
		return false;
	}

	return true;
}

static bool is_supported_interval_value(php_interval_obj *interval_obj)
{
	timelib_rel_time *diff = interval_obj->diff;

	if (!interval_obj->initialized || !diff) {
		return false;
	}

	return !diff->have_weekday_relative &&
		!diff->have_special_relative &&
		diff->first_last_day_of == 0 &&
		diff->special.type == 0;
}

static bool copy_interval_from_php(colopl_timeshifter_interval_t *dest, zval *src)
{
	php_interval_obj *interval_obj;

	memset(dest, 0, sizeof(colopl_timeshifter_interval_t));

	if (!is_supported_interval_object(src)) {
		return false;
	}

	interval_obj = Z_PHPINTERVAL_P(src);
	if (!is_supported_interval_value(interval_obj)) {
		return false;
	}

	dest->initialized = true;
	dest->civil_or_wall = interval_obj->civil_or_wall;
	memcpy(&dest->diff, interval_obj->diff, sizeof(timelib_rel_time));

	return true;
}

bool get_shift_interval(colopl_timeshifter_interval_t *interval) {
	timeshifter_global_t tg;

	sm_read(&timeshifter_global, &tg);
	if (tg.is_hooked) {
		memcpy(interval, &tg.interval, sizeof(colopl_timeshifter_interval_t));
		return true;
	}

	memset(interval, 0, sizeof(colopl_timeshifter_interval_t));

	return false;
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
	timeshifter_global_t tg;
	zval *intern;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(intern, php_date_get_interval_ce())
	ZEND_PARSE_PARAMETERS_END();

	if (!is_supported_interval_object(intern)) {
		RETURN_FALSE;
	}

	if (!is_supported_interval_value(Z_PHPINTERVAL_P(intern))) {
		RETURN_FALSE;
	}

	if (!validate_shift_interval(intern)) {
		RETURN_FALSE;
	}

	sm_read(&timeshifter_global, &tg);
	if (!copy_interval_from_php(&tg.interval, intern)) {
		RETURN_FALSE;
	}

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
	COLOPL_TS_G(in_internal_call) = false;

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
	php_info_print_table_row(2, "timeshifter version", PHP_COLOPL_TIMESHIFTER_VERSION);
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
	memset(&tg.interval, 0, sizeof(tg.interval));
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
