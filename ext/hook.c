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
#include "hook.h"

#include "php.h"
#include "php_colopl_timeshifter.h"
#include "ext/date/php_date.h"
#include "ext/pdo/php_pdo.h"
#include "ext/pdo/php_pdo_driver.h"

#include "third_party/timelib/timelib.h"

#ifdef PHP_WIN32
# include "win32/time.h"
#else
# include <sys/time.h>
#endif

typedef struct _format_flags_t {
	bool y, m, d, h, i, s, us;
} format_flags_t;

static inline void parse_format(char *format, format_flags_t *flags) {
	memset(flags, 0, sizeof(format_flags_t));
	bool skip_next = false;

	for (char *c = format; *c != '\0'; c++) {
		if (skip_next) {
			skip_next = false;
			continue;
		}
		switch (*c) {
			case '\\':
				skip_next = true;
				continue;
			case 'X':
			case 'x':
			case 'Y':
			case 'y':
				flags->y = true;
				continue;
			case 'F':
			case 'M':
			case 'm':
			case 'n':
				flags->m = true;
				continue;
			case 'd':
			case 'j':
			case 'D':
			/* case 'l': */
			/* case 'S': */
			case 'z':
				flags->d = true;
				continue;
			/* case 'a': */
			/* case 'A': */
			case 'g':
			case 'h':
			case 'G':
			case 'H':
				flags->h = true;
				continue;
			case 'i':
				flags->i = true;
				continue;
			case 's':
				flags->s = true;
				continue;
			case 'v':
			case 'u':
				flags->us = true;
				continue;
			case '!':
			case '|':
			case 'U':
				flags->y = true;
				flags->m = true;
				flags->d = true;
				flags->h = true;
				flags->i = true;
				flags->s = true;
				flags->us = true;
				continue;
			default:
				continue;
		}
	}
}

static inline void apply_interval(timelib_time **time, timelib_rel_time *interval)
{
	timelib_time *new_time = timelib_sub(*time, interval);
	timelib_update_ts(new_time, NULL);
	timelib_time_dtor(*time);
	*time = new_time;
}

#define CALL_ORIGINAL_FUNCTION_WITH_PARAMS(_name, _params, _param_count) \
	do { \
		zend_fcall_info *fci = ecalloc(1, sizeof(zend_fcall_info)); \
		zend_fcall_info_cache *fcc = ecalloc(1, sizeof(zend_fcall_info_cache)); \
		fci->size = sizeof(zend_fcall_info); \
		fci->object = NULL; \
		fci->retval = return_value; \
		fci->param_count = _param_count; \
		fci->params = _params; \
		fci->named_params = NULL; \
		fcc->function_handler = zend_hash_str_find_ptr(CG(function_table), #_name, strlen(#_name)); \
		zif_handler *hook_handler = fcc->function_handler->internal_function.handler; \
		fcc->function_handler->internal_function.handler = COLOPL_TS_G(orig_##_name); \
		fcc->called_scope = NULL; \
		fcc->object = NULL; \
		zend_call_function(fci, fcc); \
		fcc->function_handler->internal_function.handler = hook_handler; \
		efree(fci); \
		efree(fcc); \
	} while (0);

#define CALL_ORIGINAL_FUNCTION(name) \
	do { \
		COLOPL_TS_G(orig_##name)(INTERNAL_FUNCTION_PARAM_PASSTHRU); \
	} while (0);

#define CHECK_STATE(name) \
	do { \
		if (!get_is_hooked()) { \
			CALL_ORIGINAL_FUNCTION(name); \
			return; \
		} \
	} while (0);

#define DEFINE_DT_HOOK_CONSTRUCTOR(name) \
	static void hook_##name##_con(INTERNAL_FUNCTION_PARAMETERS) \
	{ \
		CHECK_STATE(name##_con); \
		\
		CALL_ORIGINAL_FUNCTION(name##_con); \
		\
		zend_string *datetime = NULL; \
		zval *timezone = NULL; \
		php_date_obj *date = NULL; \
		timelib_rel_time interval; \
		\
		ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_QUIET, 0, 2) \
			Z_PARAM_OPTIONAL; \
			Z_PARAM_STR_OR_NULL(datetime); \
			Z_PARAM_OBJECT_OF_CLASS_OR_NULL(timezone, php_date_get_timezone_ce()); \
		ZEND_PARSE_PARAMETERS_END(); \
		\
		date = Z_PHPDATE_P(ZEND_THIS); \
		\
		/* Early return if construction failed. */ \
		if (!date || !date->time) { \
			return; \
		} \
		\
		if (datetime && is_fixed_time_str(datetime, timezone) == 1) { \
			return; \
		} \
		\
		get_shift_interval(&interval); \
		apply_interval(&date->time, &interval); \
	}

#define DEFINE_CREATE_FROM_FORMAT_EX(fname, name) \
	static void hook_##fname(INTERNAL_FUNCTION_PARAMETERS) { \
		CHECK_STATE(name); \
		\
		zend_string *format, *_datetime; \
		zval *_timezone_object; \
		format_flags_t flags; \
		timelib_time *orig, *shifted; \
		\
		CALL_ORIGINAL_FUNCTION(name); \
		if (!return_value || Z_TYPE_P(return_value) == IS_FALSE || !Z_PHPDATE_P(return_value)->time) { \
			return; \
		} \
		\
		ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_QUIET, 2, 3); \
			Z_PARAM_STR(format) \
			Z_PARAM_STR(_datetime) \
			Z_PARAM_OPTIONAL \
			Z_PARAM_OBJECT_OF_CLASS_OR_NULL(_timezone_object, php_date_get_timezone_ce()) \
		ZEND_PARSE_PARAMETERS_END(); \
		\
		parse_format(ZSTR_VAL(format), &flags); \
		\
		/* backup original method result */ \
		orig = timelib_time_clone(Z_PHPDATE_P(return_value)->time); \
		shifted = get_shifted_timelib_time(orig->tz_info); \
		\
		/* overwrite current shifted datetime */ \
		Z_PHPDATE_P(return_value)->time->y = shifted->y; \
		Z_PHPDATE_P(return_value)->time->m = shifted->m; \
		Z_PHPDATE_P(return_value)->time->d = shifted->d; \
		Z_PHPDATE_P(return_value)->time->h = shifted->h; \
		Z_PHPDATE_P(return_value)->time->i = shifted->i; \
		Z_PHPDATE_P(return_value)->time->s = shifted->s; \
		Z_PHPDATE_P(return_value)->time->us = shifted->us; \
		\
		/* restore original method result if required */ \
		if (flags.h || flags.i || flags.s || flags.us) { \
			Z_PHPDATE_P(return_value)->time->h = 0; \
			Z_PHPDATE_P(return_value)->time->i = 0; \
			Z_PHPDATE_P(return_value)->time->s = 0; \
			Z_PHPDATE_P(return_value)->time->us = 0; \
		} \
		if (flags.y) { Z_PHPDATE_P(return_value)->time->y = orig->y; } \
		if (flags.m) { Z_PHPDATE_P(return_value)->time->m = orig->m; } \
		if (flags.d) { Z_PHPDATE_P(return_value)->time->d = orig->d; } \
		if (flags.h) { Z_PHPDATE_P(return_value)->time->h = orig->h; } \
		if (flags.i) { Z_PHPDATE_P(return_value)->time->i = orig->i; } \
		if (flags.s) { Z_PHPDATE_P(return_value)->time->s = orig->s; } \
		if (flags.us) { Z_PHPDATE_P(return_value)->time->us = orig->us; } \
		\
		/* release shifted time */ \
		timelib_time_dtor(orig); \
		timelib_time_dtor(shifted); \
		timelib_update_ts(Z_PHPDATE_P(return_value)->time, NULL); \
	}

#define DEFINE_CREATE_FROM_FORMAT(name) \
	DEFINE_CREATE_FROM_FORMAT_EX(name, name);

#define HOOK_CONSTRUCTOR(ce, name) \
	do { \
		COLOPL_TS_G(orig_##name##_con) = ce->constructor->internal_function.handler; \
		ce->constructor->internal_function.handler = hook_##name##_con; \
	} while (0);

#define HOOK_METHOD(ce, name, method) \
	do { \
		zend_function *php_function_entry = zend_hash_str_find_ptr(&ce->function_table, #method, strlen(#method)); \
		ZEND_ASSERT(php_function_entry); \
		COLOPL_TS_G(orig_##name##_##method) = php_function_entry->internal_function.handler; \
		php_function_entry->internal_function.handler = hook_##name##_##method; \
	} while (0);

#define HOOK_FUNCTION(name) \
	do { \
		zend_function *php_function_entry = zend_hash_str_find_ptr(CG(function_table), #name, strlen(#name)); \
		ZEND_ASSERT(php_function_entry); \
		COLOPL_TS_G(orig_##name) = php_function_entry->internal_function.handler; \
		php_function_entry->internal_function.handler = hook_##name; \
	} while (0);

#define RESTORE_CONSTRUCTOR(ce, name) \
	do { \
		ZEND_ASSERT(COLOPL_TS_G(orig_##name##_con)); \
		ce->constructor->internal_function.handler = COLOPL_TS_G(orig_##name##_con); \
		COLOPL_TS_G(orig_##name##_con) = NULL; \
	} while (0);

#define RESTORE_METHOD(ce, name, method) \
	do { \
		zend_function *php_function_entry = zend_hash_str_find_ptr(&ce->function_table, #method, strlen(#method)); \
		ZEND_ASSERT(php_function_entry); \
		ZEND_ASSERT(COLOPL_TS_G(orig_##name##_##method)); \
		php_function_entry->internal_function.handler = COLOPL_TS_G(orig_##name##_##method); \
		COLOPL_TS_G(orig_##name##_##method) = NULL; \
	} while (0);

#define RESTORE_FUNCTION(name) \
	do { \
		zend_function *php_function_entry = zend_hash_str_find_ptr(CG(function_table), #name, strlen(#name)); \
		ZEND_ASSERT(php_function_entry); \
		ZEND_ASSERT(COLOPL_TS_G(orig_##name)); \
		php_function_entry->internal_function.handler = COLOPL_TS_G(orig_##name); \
		COLOPL_TS_G(orig_##name) = NULL; \
	} while (0);

static inline int is_fixed_time_str(zend_string *datetime, zval *timezone)
{
	zend_string *datetime_lower;
	zval before_zv, after_zv;
	php_date_obj *before, *after;
	zend_class_entry *ce = php_date_get_immutable_ce();
	bool is_fixed_time_str;

	datetime_lower = zend_string_tolower(datetime);
	if (strncmp(ZSTR_VAL(datetime_lower), "now", 3) == 0 ||
		strncmp(ZSTR_VAL(datetime_lower), "yesterday", 9) == 0 ||
		strncmp(ZSTR_VAL(datetime_lower), "today", 5) == 0 ||
		strncmp(ZSTR_VAL(datetime_lower), "tomorrow", 8) == 0
	) {
		zend_string_release(datetime_lower);
		return 2;
	}

	zend_string_release(datetime_lower);

	php_date_instantiate(ce, &before_zv);
	before = Z_PHPDATE_P(&before_zv);
	if (!php_date_initialize(before, ZSTR_VAL(datetime), ZSTR_LEN(datetime), NULL, timezone, 0)) {
		zval_ptr_dtor(&before_zv);
		return FAILURE;
	}

	usleep(((uint32_t) COLOPL_TS_G(usleep_sec)) > 0 ? (uint32_t) COLOPL_TS_G(usleep_sec) : 1);

	php_date_instantiate(ce, &after_zv);
	after = Z_PHPDATE_P(&after_zv);
	if (!php_date_initialize(after, ZSTR_VAL(datetime), ZSTR_LEN(datetime), NULL, timezone, 0)) {
		zval_ptr_dtor(&before_zv);
		zval_ptr_dtor(&after_zv);
		return FAILURE;
	}

	is_fixed_time_str = before->time->y == after->time->y
		&& before->time->m == after->time->m
		&& before->time->d == after->time->d
		&& before->time->h == after->time->h
		&& before->time->i == after->time->i
		&& before->time->s == after->time->s
		&& before->time->us == after->time->us
	;

	zval_ptr_dtor(&before_zv);
	zval_ptr_dtor(&after_zv);

	return (int) is_fixed_time_str;
}

static inline timelib_time *get_current_timelib_time(timelib_tzinfo *tzi)
{
	timelib_time *t = timelib_time_ctor();

	if (tzi != NULL) {
		timelib_set_timezone(t, tzi);
		timelib_unixtime2local(t, (timelib_sll) php_time());
	} else {
		timelib_unixtime2gmt(t, php_time());
	}

	return t;
}

static inline timelib_time *get_shifted_timelib_time(timelib_tzinfo *tzi)
{
	timelib_time *t = get_current_timelib_time(tzi);
	timelib_rel_time interval;

	get_shift_interval(&interval);
	apply_interval(&t, &interval);

	return t;
}

static inline time_t get_shifted_time(timelib_tzinfo *tzi)
{
	time_t timestamp;
	timelib_time *t = get_shifted_timelib_time(tzi);

	timestamp = t->sse;

	timelib_time_dtor(t);

	return timestamp;
}

static inline bool pdo_time_apply(pdo_dbh_t *dbh)
{
	zend_string *sql;
	char buf[1024];

	if (!COLOPL_TS_G(pdo_mysql_orig_methods) || !COLOPL_TS_G(pdo_mysql_orig_methods)->doer) {
		return false;
	}

	zend_sprintf(buf, "SET @@session.timestamp = %ld;", get_shifted_time(NULL));
	sql = zend_string_init_fast(buf, strlen(buf));
	COLOPL_TS_G(pdo_mysql_orig_methods)->doer(dbh, sql);
	zend_string_release(sql);

	return true;
}

static bool hook_pdo_driver_preparer(pdo_dbh_t *dbh, zend_string *sql, pdo_stmt_t *stmt, zval *driver_options)
{
	bool retval;

	if (get_is_hooked()) {
		pdo_time_apply(dbh);
	}

	retval = COLOPL_TS_G(pdo_mysql_orig_methods)->preparer(dbh, sql, stmt, driver_options);

	return retval;
}

static zend_long hook_pdo_driver_doer(pdo_dbh_t *dbh, const zend_string *sql)
{
	if (get_is_hooked()) {
		pdo_time_apply(dbh);
	}

	return COLOPL_TS_G(pdo_mysql_orig_methods)->doer(dbh, sql);
}

static void hook_pdo_con(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(pdo_con);

	pdo_dbh_t *dbh = Z_PDO_DBH_P(ZEND_THIS);

	CALL_ORIGINAL_FUNCTION(pdo_con);

	if (!dbh->driver ||
		strncmp(dbh->driver->driver_name, "mysql", 5) == 0 ||
		dbh->methods != &COLOPL_TS_G(hooked_mysql_driver_methods)
	) {
		if (!COLOPL_TS_G(pdo_mysql_orig_methods)) {
			/* Check pdo_mysql driver. */
			if (!dbh->methods) {
				return;
			}

			/* Copy original methods struct. */
			COLOPL_TS_G(pdo_mysql_orig_methods) = dbh->methods;
			memcpy(&COLOPL_TS_G(hooked_mysql_driver_methods), dbh->methods, sizeof(struct pdo_dbh_methods));

			/* Override function pointer. */
			COLOPL_TS_G(hooked_mysql_driver_methods).preparer = hook_pdo_driver_preparer;
			COLOPL_TS_G(hooked_mysql_driver_methods).doer = hook_pdo_driver_doer;
		}

		/* Override MySQL specific driver methods pointer. */
		dbh->methods = &COLOPL_TS_G(hooked_mysql_driver_methods);
	}
}

static inline void mktime_common(INTERNAL_FUNCTION_PARAMETERS, zend_long timestamp)
{
	zend_long hou, min, sec, mon, day, yea;
	bool min_is_null = true, sec_is_null = true, mon_is_null = true, day_is_null = true, yea_is_null = true;
	timelib_time *t = timelib_time_ctor();
	timelib_rel_time interval;

	timelib_unixtime2gmt(t, timestamp);
	get_shift_interval(&interval);
	apply_interval(&t, &interval);

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_QUIET, 1, 6)
		Z_PARAM_LONG(hou)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG_OR_NULL(min, min_is_null)
		Z_PARAM_LONG_OR_NULL(sec, sec_is_null)
		Z_PARAM_LONG_OR_NULL(mon, mon_is_null)
		Z_PARAM_LONG_OR_NULL(day, day_is_null)
		Z_PARAM_LONG_OR_NULL(yea, yea_is_null)
	ZEND_PARSE_PARAMETERS_END();

	if (!min_is_null) {
		t->i = min;
	}

	if (!sec_is_null) {
		t->s = sec;
	}

	if (!mon_is_null) {
		t->m = mon;
	}

	if (!day_is_null) {
		t->d = day;
	}

	if (!yea_is_null) {
		if (yea >= 0 && yea < 70) {
			yea += 2000;
		} else if (yea >= 70 && yea <= 100) {
			yea += 1900;
		}
		t->y = yea;
	}

	RETVAL_LONG(t->sse);
	timelib_time_dtor(t);
}

static inline void date_common(INTERNAL_FUNCTION_PARAMETERS, int localtime)
{
	zend_string *format;
	zend_long ts;
	bool ts_is_null = true;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_QUIET, 1, 2)
		Z_PARAM_STR(format)
		Z_PARAM_OPTIONAL;
		Z_PARAM_LONG_OR_NULL(ts, ts_is_null)
	ZEND_PARSE_PARAMETERS_END();

	if (ts_is_null) {
		ts = get_shifted_time(NULL);
	}

	RETVAL_STR(php_format_date(ZSTR_VAL(format), ZSTR_LEN(format), ts, localtime));
}

static inline void date_create_common(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce)
{
	zval *timezone_object = NULL;
	zend_string *time_str = NULL;
	php_date_obj *date = NULL;
	timelib_rel_time interval;

	ZEND_PARSE_PARAMETERS_START(0, 2)
		Z_PARAM_OPTIONAL;
		Z_PARAM_STR(time_str)
		Z_PARAM_OBJECT_OF_CLASS_OR_NULL(timezone_object, php_date_get_timezone_ce())
	ZEND_PARSE_PARAMETERS_END();

	php_date_instantiate(ce, return_value);
	if (!php_date_initialize(
		Z_PHPDATE_P(return_value),
		(!time_str ? NULL : ZSTR_VAL(time_str)),
		(!time_str ? 0 : ZSTR_LEN(time_str)),
		NULL,
		timezone_object,
		0
	)) {
		zval_ptr_dtor(return_value);
		RETVAL_FALSE;
	}

	if (time_str && is_fixed_time_str(time_str, timezone_object) == 1) {
		return;
	}

	get_shift_interval(&interval);
	apply_interval(&Z_PHPDATE_P(return_value)->time, &interval);
}

DEFINE_DT_HOOK_CONSTRUCTOR(dt);

DEFINE_DT_HOOK_CONSTRUCTOR(dti);

static void hook_time(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(time);

	CALL_ORIGINAL_FUNCTION(time);
	RETURN_LONG(get_shifted_time(NULL));
}

static void hook_mktime(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(mktime);

	CALL_ORIGINAL_FUNCTION(mktime);
	mktime_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, Z_LVAL_P(return_value));
}

static void hook_gmmktime(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(gmmktime);

	CALL_ORIGINAL_FUNCTION(gmmktime);
	mktime_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, Z_LVAL_P(return_value));
}

static void hook_date_create(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(date_create);

	date_create_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, php_date_get_date_ce());
}

static void hook_date_create_immutable(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(date_create_immutable);

	date_create_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, php_date_get_immutable_ce());
}

DEFINE_CREATE_FROM_FORMAT(date_create_from_format);

DEFINE_CREATE_FROM_FORMAT(date_create_immutable_from_format);

DEFINE_CREATE_FROM_FORMAT_EX(dt_createfromformat, date_create_from_format);

DEFINE_CREATE_FROM_FORMAT_EX(dti_createfromformat, date_create_immutable_from_format);

static void hook_date(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(date);

	date_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}

static void hook_gmdate(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(gmdate);

	date_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}

static void hook_idate(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(idate);

	zend_string *format;
	zend_long ts;
	bool ts_is_null = 1;

	if (Z_TYPE_P(return_value) == IS_FALSE) {
		return;
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_QUIET,1, 2)
		Z_PARAM_STR(format)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG_OR_NULL(ts, ts_is_null)
	ZEND_PARSE_PARAMETERS_END();

	if (ts_is_null) {
		ts = get_shifted_time(NULL);
	}

	RETURN_LONG(php_idate(ZSTR_VAL(format)[0], ts, 0));
}

static void hook_getdate(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(getdate);

	zend_long timestamp;
	bool timestamp_is_null = true;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_QUIET, 0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG_OR_NULL(timestamp, timestamp_is_null)
	ZEND_PARSE_PARAMETERS_END();

	if (!timestamp_is_null) {
		return;
	}

	/* Call original function with timestamp params. */
	zval params[1];
	ZVAL_LONG(&params[0], get_shifted_time(NULL));
	CALL_ORIGINAL_FUNCTION_WITH_PARAMS(getdate, params, 1);
}

static void hook_localtime(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(localtime);

	zend_long timestamp;
	bool timestamp_is_null = true, associative = false;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_QUIET, 0, 2)
		Z_PARAM_OPTIONAL;
		Z_PARAM_LONG_OR_NULL(timestamp, timestamp_is_null);
		Z_PARAM_BOOL(associative);
	ZEND_PARSE_PARAMETERS_END();

	/* Call original function with params. */
	zval params[2];
	ZVAL_LONG(&params[0], get_shifted_time(NULL));
	ZVAL_BOOL(&params[1], associative);
	CALL_ORIGINAL_FUNCTION_WITH_PARAMS(localtime, params, 2);
}

static void hook_strtotime(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(strtotime);

	zend_string *times, *times_lower;
	zend_long preset_ts;
	bool preset_ts_is_null = true;
	int is_fixed_ret;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_QUIET, 1, 2)
		Z_PARAM_STR(times);
		Z_PARAM_OPTIONAL;
		Z_PARAM_LONG_OR_NULL(preset_ts, preset_ts_is_null);
	ZEND_PARSE_PARAMETERS_END();

	is_fixed_ret = is_fixed_time_str(times, NULL);

	if (!preset_ts_is_null || is_fixed_ret == 1 || is_fixed_ret == FAILURE) {
		//php_error_docref(NULL, E_NOTICE, "hook_strtotime is calling original function (%s, %ld)", ZSTR_VAL(times), preset_ts);
		CALL_ORIGINAL_FUNCTION(strtotime);
		return;
	}

	/* Call original function based on shifted time */
	zval params[2];
	ZVAL_STR(&params[0], times);
	ZVAL_LONG(&params[1], get_shifted_time(NULL));
	//php_error_docref(NULL, E_NOTICE, "hook_strtotime is calling original function with params (%s, %ld)", Z_STRVAL_P(&params[0]), Z_LVAL_P(&params[1]));
	CALL_ORIGINAL_FUNCTION_WITH_PARAMS(strtotime, params, 2);
}

#if HAVE_GETTIMEOFDAY
static inline void gettimeofday_common(INTERNAL_FUNCTION_PARAMETERS, int mode)
{
	bool get_as_float = false;
	struct timeval tp = {0};
	timelib_time *tm = timelib_time_ctor();
	timelib_rel_time interval;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL;
		Z_PARAM_BOOL(get_as_float);
	ZEND_PARSE_PARAMETERS_END();

	if (gettimeofday(&tp, NULL)) {
		ZEND_ASSERT(0 && "gettimeofday() can't fail");
	}

	timelib_unixtime2gmt(tm, tp.tv_sec);
	tm->us = tp.tv_usec;
	get_shift_interval(&interval);
	apply_interval(&tm, &interval);

	if (get_as_float) {
		RETVAL_DOUBLE((double)(tm->sse + tm->us / 1000000.00));
	} else {
		if (mode) {
			timelib_time_offset *offset;

			offset = timelib_get_time_zone_info(tm->sse, get_timezone_info());

			array_init(return_value);
			add_assoc_long(return_value, "sec", tm->sse);
			add_assoc_long(return_value, "usec", tm->us);

			add_assoc_long(return_value, "minuteswest", -offset->offset / 60);
			add_assoc_long(return_value, "dsttime", -offset->is_dst);

			timelib_time_offset_dtor(offset);
		} else {
			RETVAL_NEW_STR(zend_strpprintf(0, "%.8F %ld", tm->us / 1000000.00, (long) tm->sse));
		}
	}

	timelib_time_dtor(tm);
}

static void hook_microtime(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(microtime);

	gettimeofday_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}

static void hook_gettimeofday(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(gettimeofday);

	gettimeofday_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
#endif

bool register_hooks()
{
	/* \DateTime::__construct */
	HOOK_CONSTRUCTOR(php_date_get_date_ce(), dt);

	/* \DateTimeImmutabel::__construct */
	HOOK_CONSTRUCTOR(php_date_get_immutable_ce(), dti);

	/* \DateTime::createFromFormat */
	HOOK_METHOD(php_date_get_date_ce(), dt, createfromformat);

	/* \DateTimeImmutable::createFromFormat */
	HOOK_METHOD(php_date_get_immutable_ce(), dti, createfromformat);

	HOOK_FUNCTION(time);
	HOOK_FUNCTION(mktime);
	HOOK_FUNCTION(gmmktime);
	HOOK_FUNCTION(date_create);
	HOOK_FUNCTION(date_create_immutable);
	HOOK_FUNCTION(date_create_from_format);
	HOOK_FUNCTION(date_create_immutable_from_format);
	HOOK_FUNCTION(date);
	HOOK_FUNCTION(gmdate);
	HOOK_FUNCTION(idate);
	HOOK_FUNCTION(getdate);
	HOOK_FUNCTION(localtime);
	HOOK_FUNCTION(strtotime);

#if HAVE_GETTIMEOFDAY
	HOOK_FUNCTION(microtime);
	HOOK_FUNCTION(gettimeofday);
#endif

	return true;
}

void register_pdo_hook()
{
	/* \PDO::__construct */
	HOOK_CONSTRUCTOR(php_pdo_get_dbh_ce(), pdo);
}

bool unregister_hooks()
{
	/* \DateTime::__construct */
	RESTORE_CONSTRUCTOR(php_date_get_date_ce(), dt);

	/* \DateTimeImmutabel::__construct */
	RESTORE_CONSTRUCTOR(php_date_get_immutable_ce(), dti);

	/* \DateTime::createFromFormat */
	RESTORE_METHOD(php_date_get_date_ce(), dt, createfromformat);

	/* \DateTimeImmutable::createFromFormat */
	RESTORE_METHOD(php_date_get_immutable_ce(), dti, createfromformat);

	RESTORE_FUNCTION(time);
	RESTORE_FUNCTION(mktime);
	RESTORE_FUNCTION(gmmktime);
	RESTORE_FUNCTION(date_create);
	RESTORE_FUNCTION(date_create_immutable);
	RESTORE_FUNCTION(date_create_from_format);
	RESTORE_FUNCTION(date_create_immutable_from_format);
	RESTORE_FUNCTION(date);
	RESTORE_FUNCTION(gmdate);
	RESTORE_FUNCTION(idate);
	RESTORE_FUNCTION(getdate);
	RESTORE_FUNCTION(localtime);
	RESTORE_FUNCTION(strtotime);

#if HAVE_GETTIMEOFDAY
	RESTORE_FUNCTION(microtime);
	RESTORE_FUNCTION(gettimeofday);
#endif

	return true;
}

void apply_request_time_hook()
{
	zval *globals_server, *request_time, *request_time_float;
	timelib_time *t;
	timelib_rel_time interval;

	globals_server = zend_hash_str_find(&EG(symbol_table), "_SERVER", strlen("_SERVER"));

	if (!globals_server || Z_TYPE_P(globals_server) != IS_ARRAY) {
		/* $_SERVER not defined */
		return;
	}

	request_time = zend_hash_str_find(Z_ARR_P(globals_server), "REQUEST_TIME", strlen("REQUEST_TIME"));
	request_time_float = zend_hash_str_find(Z_ARR_P(globals_server), "REQUEST_TIME_FLOAT", strlen("REQUEST_TIME_FLOAT"));

	/* Get original request time at once */
	if (COLOPL_TS_G(orig_request_time) == 0 && COLOPL_TS_G(orig_request_time_float) == 0) {
		if (request_time_float) {
			COLOPL_TS_G(orig_request_time_float) = Z_DVAL_P(request_time_float);
		} else if (request_time) {
			COLOPL_TS_G(orig_request_time) = Z_LVAL_P(request_time);
		} else {
			/* Missing REQUEST_TIME or REQUEST_TIME_FLOAT */
			return;
		}
	}

	if (COLOPL_TS_G(orig_request_time_float) != 0) {
		timelib_sll ts = (timelib_sll) COLOPL_TS_G(orig_request_time_float);
		timelib_sll tus = (timelib_sll) ((COLOPL_TS_G(orig_request_time_float) - ts) * 1e6);

		t = timelib_time_ctor();
		timelib_unixtime2gmt(t, ts);
		t->us = tus;
		timelib_update_ts(t, NULL);
	} else if (COLOPL_TS_G(orig_request_time) != 0) {
		t = timelib_time_ctor();
		timelib_unixtime2gmt(t, (timelib_sll) COLOPL_TS_G(orig_request_time));
	} else {
		/* REQUEST_TIME or REQUEST_TIME_FLOAT not found */
		return;
	}

	/* Apply interval. */
	get_shift_interval(&interval);
	apply_interval(&t, &interval);

	if (request_time) {
		ZVAL_LONG(request_time, (zend_long) t->sse);
	}

	if (request_time_float) {
		ZVAL_DOUBLE(request_time_float, ((double) t->sse + ((double) t->us / 1000000.0)));
	}

	timelib_time_dtor(t);
}
