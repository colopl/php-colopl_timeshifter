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

#include <stdint.h>
#include <time.h>

#include "hook.h"
#include "php_colopl_timeshifter.h"

#include "php.h"

#include "Zend/zend_exceptions.h"
#include "Zend/zend_interfaces.h"
#include "ext/date/php_date.h"
#include "ext/pdo/php_pdo.h"
#include "ext/pdo/php_pdo_driver.h"

#ifdef PHP_WIN32
# include <win32/time.h>
#else
# include <sys/time.h>
#endif

typedef struct _format_flags_t {
	bool y, m, d, h, i, s, us;
} format_flags_t;

typedef struct _datetime_parts_t {
	zend_long y, m, d, h, i, s, us;
} datetime_parts_t;

static inline void parse_format(const char *format, format_flags_t *flags) {
	const char *c;
	bool skip_next = false;

	memset(flags, 0, sizeof(format_flags_t));

	for (c = format; *c != '\0'; c++) {
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

static inline zend_long sec_from_usec(int64_t usec)
{
	if (usec >= 0) {
		return (zend_long) (usec / 1000000);
	}

	return (zend_long) -(((-usec) + 999999) / 1000000);
}

static inline zend_long usec_remainder(int64_t usec)
{
	zend_long sec = sec_from_usec(usec);
	return (zend_long) (usec - ((int64_t) sec * 1000000));
}

static inline int64_t get_real_time_usec(void)
{
#if HAVE_GETTIMEOFDAY
	struct timeval tp = {0};

	if (gettimeofday(&tp, NULL)) {
		ZEND_ASSERT(0 && "gettimeofday() can't fail");
	}

	return ((int64_t) tp.tv_sec * 1000000) + tp.tv_usec;
#else
	return (int64_t) time(NULL) * 1000000;
#endif
}

static inline void format_timestamp_usec(int64_t usec, char *buf, size_t buf_len)
{
	zend_long sec, rem;

	sec = sec_from_usec(usec);
	rem = usec_remainder(usec);

	if (rem < 0) {
		rem = -rem;
	}

	slprintf(buf, buf_len, ZEND_LONG_FMT ".%06" ZEND_LONG_FMT_SPEC, sec, rem);
}

static bool parse_timestamp_usec(zend_string *value, int64_t *usec)
{
	const char *p, *end;
	int64_t sec = 0, fraction = 0;
	bool negative = false;
	int digits = 0;

	p = ZSTR_VAL(value);
	end = p + ZSTR_LEN(value);

	if (p < end && *p == '-') {
		negative = true;
		p++;
	}

	if (p >= end || *p < '0' || *p > '9') {
		return false;
	}

	while (p < end && *p >= '0' && *p <= '9') {
		int digit = *p - '0';

		if (sec > (INT64_MAX - digit) / 10) {
			return false;
		}
		sec = sec * 10 + digit;
		p++;
	}

	if (p < end && *p == '.') {
		p++;
		while (p < end && *p >= '0' && *p <= '9' && digits < 6) {
			fraction = fraction * 10 + (*p - '0');
			p++;
			digits++;
		}
	}

	while (digits < 6) {
		fraction *= 10;
		digits++;
	}

	if (sec > (INT64_MAX - fraction) / 1000000) {
		return false;
	}

	*usec = (sec * 1000000) + fraction;
	if (negative) {
		*usec = -*usec;
	}

	return true;
}

#define CALL_ORIGINAL_FUNCTION_WITH_PARAMS_RET(_name, _retval, _params, _param_count) \
	do { \
		zend_fcall_info fci = { \
			.size = sizeof(zend_fcall_info), \
			.retval = _retval, \
			.param_count = _param_count, \
			.params = _params, \
		}; \
		zend_function fnc = { \
			.type = ZEND_INTERNAL_FUNCTION, \
		}; \
		zend_fcall_info_cache fcc = { \
			.function_handler = &fnc, \
		}; \
		zend_function *src = (zend_function *) zend_hash_str_find_ptr(CG(function_table), #_name, strlen(#_name)); \
		ZEND_ASSERT(src); \
		memcpy(&fnc.internal_function, &src->internal_function, sizeof(zend_internal_function)); \
		fnc.internal_function.handler = COLOPL_TS_G(orig_##_name); \
		zend_call_function(&fci, &fcc); \
	} while (0);

#define CALL_ORIGINAL_FUNCTION_WITH_PARAMS(_name, _params, _param_count) \
	CALL_ORIGINAL_FUNCTION_WITH_PARAMS_RET(_name, return_value, _params, _param_count)

#define CALL_ORIGINAL_FUNCTION(name) \
	do { \
		COLOPL_TS_G(orig_##name)(INTERNAL_FUNCTION_PARAM_PASSTHRU); \
	} while (0);

#define CHECK_STATE(name) \
	do { \
		if (!get_is_hooked() || COLOPL_TS_G(in_internal_call)) { \
			CALL_ORIGINAL_FUNCTION(name); \
			return; \
		} \
	} while (0);

#define DEFINE_DT_HOOK_CONSTRUCTOR(name) \
	static void ZEND_FASTCALL hook_##name##_con(INTERNAL_FUNCTION_PARAMETERS) \
	{ \
		CHECK_STATE(name##_con); \
		\
		CALL_ORIGINAL_FUNCTION(name##_con); \
		if (EG(exception)) { \
			return; \
		} \
		\
		zend_string *datetime = NULL; \
		zval *timezone = NULL, object_timezone; \
		php_date_obj *date = NULL; \
		int64_t timestamp_usec; \
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
		if (!time_string_depends_on_current(datetime)) { \
			return; \
		} \
		\
		ZVAL_UNDEF(&object_timezone); \
		if (!get_date_timezone(ZEND_THIS, &object_timezone)) { \
			return; \
		} \
		\
		if (!resolve_time_string_to_timestamp_usec(datetime, &object_timezone, &timestamp_usec)) { \
			zval_ptr_dtor(&object_timezone); \
			return; \
		} \
		\
		initialize_date_from_timestamp_usec(ZEND_THIS, timestamp_usec, &object_timezone); \
		zval_ptr_dtor(&object_timezone); \
	}

#define DEFINE_CREATE_FROM_FORMAT_EX(fname, name) \
	static void ZEND_FASTCALL hook_##fname(INTERNAL_FUNCTION_PARAMETERS) { \
		CHECK_STATE(name); \
		\
		zend_string *format, *_datetime; \
		zval *_timezone_object; \
		format_flags_t flags; \
		\
		CALL_ORIGINAL_FUNCTION(name); \
		if (EG(exception) || !return_value || Z_TYPE_P(return_value) == IS_FALSE || !Z_PHPDATE_P(return_value)->time) { \
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
		adjust_create_from_format_result(return_value, &flags); \
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

#define HOOK_FUNCTION_OPTIONAL(name) \
	do { \
		zend_function *php_function_entry = zend_hash_str_find_ptr(CG(function_table), #name, strlen(#name)); \
		if (php_function_entry) { \
			COLOPL_TS_G(orig_##name) = php_function_entry->internal_function.handler; \
			php_function_entry->internal_function.handler = hook_##name; \
		} \
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

#define RESTORE_FUNCTION_OPTIONAL(name) \
	do { \
		if (COLOPL_TS_G(orig_##name)) { \
			zend_function *php_function_entry = zend_hash_str_find_ptr(CG(function_table), #name, strlen(#name)); \
			if (php_function_entry) { \
				php_function_entry->internal_function.handler = COLOPL_TS_G(orig_##name); \
			} \
			COLOPL_TS_G(orig_##name) = NULL; \
		} \
	} while (0);

static inline bool create_datetime_from_timestamp_usec(int64_t timestamp_usec, zval *datetime)
{
	char timestamp[64];

	php_date_instantiate(php_date_get_immutable_ce(), datetime);
	format_timestamp_usec(timestamp_usec, timestamp, sizeof(timestamp));

	if (!php_date_initialize(Z_PHPDATE_P(datetime), timestamp, strlen(timestamp), "U.u", NULL, PHP_DATE_INIT_FORMAT)) {
		zval_ptr_dtor(datetime);
		ZVAL_UNDEF(datetime);
		return false;
	}

	return true;
}

static inline bool call_date_format(zval *datetime, const char *format, zval *formatted)
{
	zval format_zv;

	ZVAL_STRING(&format_zv, format);
	ZVAL_UNDEF(formatted);
	zend_call_method_with_1_params(Z_OBJ_P(datetime), Z_OBJCE_P(datetime), NULL, "format", formatted, &format_zv);
	zval_ptr_dtor(&format_zv);

	return Z_TYPE_P(formatted) == IS_STRING;
}

static inline bool get_date_timezone(zval *datetime, zval *timezone)
{
	ZVAL_UNDEF(timezone);
	zend_call_method_with_0_params(Z_OBJ_P(datetime), Z_OBJCE_P(datetime), NULL, "gettimezone", timezone);

	return Z_TYPE_P(timezone) == IS_OBJECT && instanceof_function(Z_OBJCE_P(timezone), php_date_get_timezone_ce());
}

static inline bool localize_datetime(zval *datetime, zval *timezone, zval *localized)
{
	ZVAL_UNDEF(localized);

	if (timezone && Z_TYPE_P(timezone) == IS_OBJECT) {
		zend_call_method_with_1_params(Z_OBJ_P(datetime), Z_OBJCE_P(datetime), NULL, "settimezone", localized, timezone);
		return Z_TYPE_P(localized) == IS_OBJECT;
	}

	ZVAL_COPY(localized, datetime);
	return true;
}

static bool create_interval_from_snapshot(const colopl_timeshifter_interval_t *src, zval *interval)
{
	php_interval_obj *interval_obj;

	if (!src->initialized) {
		return false;
	}

	php_date_instantiate(php_date_get_interval_ce(), interval);

	interval_obj = Z_PHPINTERVAL_P(interval);
	interval_obj->diff = ecalloc(1, sizeof(timelib_rel_time));
	if (!interval_obj->diff) {
		zval_ptr_dtor(interval);
		ZVAL_UNDEF(interval);
		return false;
	}

	memcpy(interval_obj->diff, &src->diff, sizeof(timelib_rel_time));
	interval_obj->civil_or_wall = src->civil_or_wall;
#if PHP_VERSION_ID >= 80200
	interval_obj->from_string = false;
	interval_obj->date_string = NULL;
#endif
	interval_obj->initialized = true;

	return true;
}

static inline bool get_datetime_timestamp_usec(zval *datetime, int64_t *timestamp_usec)
{
	zval formatted;
	bool result;

	if (!call_date_format(datetime, "U.u", &formatted)) {
		return false;
	}

	result = parse_timestamp_usec(Z_STR(formatted), timestamp_usec);
	zval_ptr_dtor(&formatted);

	return result;
}

bool validate_shift_interval(zval *interval)
{
	zval datetime, shifted;
	bool result = false, previous_internal_call;

	if (!create_datetime_from_timestamp_usec(get_real_time_usec(), &datetime)) {
		return false;
	}

	previous_internal_call = COLOPL_TS_G(in_internal_call);
	COLOPL_TS_G(in_internal_call) = true;
	ZVAL_UNDEF(&shifted);
	zend_call_method_with_1_params(Z_OBJ_P(&datetime), Z_OBJCE_P(&datetime), NULL, "sub", &shifted, interval);
	COLOPL_TS_G(in_internal_call) = previous_internal_call;

	if (EG(exception)) {
		zend_clear_exception();
	} else if (Z_TYPE(shifted) == IS_OBJECT) {
		result = true;
	}

	if (!Z_ISUNDEF(shifted)) {
		zval_ptr_dtor(&shifted);
	}
	zval_ptr_dtor(&datetime);

	return result;
}

static bool calculate_shifted_time_usec(int64_t real_usec, zval *timezone, int64_t *shifted_usec)
{
	colopl_timeshifter_interval_t interval_snapshot;
	zval datetime, localized, interval, shifted;
	bool result = false, previous_internal_call;

	if (!get_shift_interval(&interval_snapshot)) {
		*shifted_usec = real_usec;
		return true;
	}

	if (!create_datetime_from_timestamp_usec(real_usec, &datetime)) {
		return false;
	}

	if (!localize_datetime(&datetime, timezone, &localized)) {
		zval_ptr_dtor(&datetime);
		return false;
	}

	if (!create_interval_from_snapshot(&interval_snapshot, &interval)) {
		zval_ptr_dtor(&localized);
		zval_ptr_dtor(&datetime);
		return false;
	}

	previous_internal_call = COLOPL_TS_G(in_internal_call);
	COLOPL_TS_G(in_internal_call) = true;
	ZVAL_UNDEF(&shifted);
	zend_call_method_with_1_params(Z_OBJ_P(&localized), Z_OBJCE_P(&localized), NULL, "sub", &shifted, &interval);
	COLOPL_TS_G(in_internal_call) = previous_internal_call;

	if (EG(exception)) {
		zend_clear_exception();
	} else if (Z_TYPE(shifted) == IS_OBJECT) {
		result = get_datetime_timestamp_usec(&shifted, shifted_usec);
	}
	if (!Z_ISUNDEF(shifted)) {
		zval_ptr_dtor(&shifted);
	}

	zval_ptr_dtor(&interval);
	zval_ptr_dtor(&localized);
	zval_ptr_dtor(&datetime);

	return result;
}

static inline int64_t get_shifted_time_usec_for_timezone(zval *timezone)
{
	int64_t real_usec = get_real_time_usec(), shifted_usec;

	if (!calculate_shifted_time_usec(real_usec, timezone, &shifted_usec)) {
		return real_usec;
	}

	return shifted_usec;
}

static inline int64_t get_shifted_time_usec(void)
{
	return get_shifted_time_usec_for_timezone(NULL);
}

static inline zend_long get_shifted_time(void)
{
	return sec_from_usec(get_shifted_time_usec());
}

static bool get_timestamp_parts(int64_t timestamp_usec, zval *timezone, datetime_parts_t *parts)
{
	zval datetime, localized, formatted;
	int parsed;

	if (!create_datetime_from_timestamp_usec(timestamp_usec, &datetime)) {
		return false;
	}

	if (!localize_datetime(&datetime, timezone, &localized)) {
		zval_ptr_dtor(&datetime);
		return false;
	}

	if (!call_date_format(&localized, "Y n j G i s u", &formatted)) {
		zval_ptr_dtor(&localized);
		zval_ptr_dtor(&datetime);
		return false;
	}

	parsed = sscanf(
		Z_STRVAL(formatted),
		ZEND_LONG_FMT " " ZEND_LONG_FMT " " ZEND_LONG_FMT " " ZEND_LONG_FMT " " ZEND_LONG_FMT " " ZEND_LONG_FMT " " ZEND_LONG_FMT,
		&parts->y,
		&parts->m,
		&parts->d,
		&parts->h,
		&parts->i,
		&parts->s,
		&parts->us
	);

	zval_ptr_dtor(&formatted);
	zval_ptr_dtor(&localized);
	zval_ptr_dtor(&datetime);

	return parsed == 7;
}

static bool get_date_parts(zval *datetime, datetime_parts_t *parts)
{
	zval formatted;
	int parsed;

	if (!call_date_format(datetime, "Y n j G i s u", &formatted)) {
		return false;
	}

	parsed = sscanf(
		Z_STRVAL(formatted),
		ZEND_LONG_FMT " " ZEND_LONG_FMT " " ZEND_LONG_FMT " " ZEND_LONG_FMT " " ZEND_LONG_FMT " " ZEND_LONG_FMT " " ZEND_LONG_FMT,
		&parts->y,
		&parts->m,
		&parts->d,
		&parts->h,
		&parts->i,
		&parts->s,
		&parts->us
	);

	zval_ptr_dtor(&formatted);

	return parsed == 7;
}

static bool initialize_date_from_parts(zval *datetime, datetime_parts_t *parts, zval *timezone)
{
	char date[128];

	slprintf(
		date,
		sizeof(date),
		ZEND_LONG_FMT "-%02" ZEND_LONG_FMT_SPEC "-%02" ZEND_LONG_FMT_SPEC " %02" ZEND_LONG_FMT_SPEC ":%02" ZEND_LONG_FMT_SPEC ":%02" ZEND_LONG_FMT_SPEC ".%06" ZEND_LONG_FMT_SPEC,
		parts->y,
		parts->m,
		parts->d,
		parts->h,
		parts->i,
		parts->s,
		parts->us
	);

	return php_date_initialize(Z_PHPDATE_P(datetime), date, strlen(date), "Y-m-d H:i:s.u", timezone, PHP_DATE_INIT_FORMAT);
}

static inline bool initialize_date_from_timestamp_usec(zval *datetime, int64_t timestamp_usec, zval *timezone)
{
	datetime_parts_t parts;

	if (!get_timestamp_parts(timestamp_usec, timezone, &parts)) {
		return false;
	}

	return initialize_date_from_parts(datetime, &parts, timezone);
}

static inline bool call_original_strtotime(zend_string *times, zend_long base, zval *retval)
{
	zval params[2];

	ZVAL_STR(&params[0], times);
	ZVAL_LONG(&params[1], base);
	ZVAL_UNDEF(retval);
	CALL_ORIGINAL_FUNCTION_WITH_PARAMS_RET(strtotime, retval, params, 2);

	return Z_TYPE_P(retval) == IS_LONG;
}

static bool call_named_function(const char *name, zval *retval, zval *params, uint32_t param_count)
{
	zend_fcall_info fci = {
		.size = sizeof(zend_fcall_info),
		.retval = retval,
		.param_count = param_count,
		.params = params,
	};
	bool result;

	ZVAL_STRING(&fci.function_name, name);
	ZVAL_UNDEF(retval);
	result = zend_call_function(&fci, NULL) == SUCCESS && !Z_ISUNDEF_P(retval);
	zval_ptr_dtor(&fci.function_name);

	return result;
}

static inline bool get_timezone_name(zval *timezone, zval *name)
{
	ZVAL_UNDEF(name);
	zend_call_method_with_0_params(Z_OBJ_P(timezone), Z_OBJCE_P(timezone), NULL, "getname", name);

	return Z_TYPE_P(name) == IS_STRING;
}

static inline bool get_default_timezone_name(zval *name)
{
	return call_named_function("date_default_timezone_get", name, NULL, 0) && Z_TYPE_P(name) == IS_STRING;
}

static bool create_timezone_from_name(zval *name, zval *timezone)
{
	zval params[1];
	bool result;

	ZVAL_COPY(&params[0], name);
	result = call_named_function("timezone_open", timezone, params, 1) &&
		Z_TYPE_P(timezone) == IS_OBJECT &&
		instanceof_function(Z_OBJCE_P(timezone), php_date_get_timezone_ce());
	zval_ptr_dtor(&params[0]);

	return result;
}

static bool get_default_timezone(zval *timezone)
{
	zval name;
	bool result;

	ZVAL_UNDEF(&name);
	if (!get_default_timezone_name(&name)) {
		return false;
	}

	result = create_timezone_from_name(&name, timezone);
	zval_ptr_dtor(&name);

	return result;
}

static bool get_utc_timezone(zval *timezone)
{
	zval name;
	bool result;

	ZVAL_STRING(&name, "UTC");
	result = create_timezone_from_name(&name, timezone);
	zval_ptr_dtor(&name);

	return result;
}

static inline bool set_default_timezone(zval *name)
{
	zval retval, params[1];
	bool result;

	ZVAL_COPY(&params[0], name);
	result = call_named_function("date_default_timezone_set", &retval, params, 1) && Z_TYPE(retval) == IS_TRUE;
	zval_ptr_dtor(&params[0]);
	if (!Z_ISUNDEF(retval)) {
		zval_ptr_dtor(&retval);
	}

	return result;
}

static bool call_original_strtotime_with_timezone(zend_string *times, zend_long base, zval *timezone, zval *retval)
{
	zval timezone_name, original_timezone;
	bool switched_timezone = false, result;

	ZVAL_UNDEF(&timezone_name);
	ZVAL_UNDEF(&original_timezone);

	if (timezone && Z_TYPE_P(timezone) == IS_OBJECT &&
		get_timezone_name(timezone, &timezone_name) &&
		get_default_timezone_name(&original_timezone) &&
		set_default_timezone(&timezone_name)) {
		switched_timezone = true;
	}

	result = call_original_strtotime(times, base, retval);

	if (switched_timezone) {
		set_default_timezone(&original_timezone);
	}

	if (!Z_ISUNDEF(timezone_name)) {
		zval_ptr_dtor(&timezone_name);
	}
	if (!Z_ISUNDEF(original_timezone)) {
		zval_ptr_dtor(&original_timezone);
	}

	return result;
}

static inline bool time_string_is_now(zend_string *datetime)
{
	zend_string *datetime_lower;
	bool result;

	if (!datetime || ZSTR_LEN(datetime) == 0) {
		return true;
	}

	datetime_lower = zend_string_tolower(datetime);
	result = ZSTR_LEN(datetime_lower) == sizeof("now") - 1
		&& memcmp(ZSTR_VAL(datetime_lower), "now", sizeof("now") - 1) == 0;
	zend_string_release(datetime_lower);

	return result;
}

static bool time_string_depends_on_current(zend_string *datetime)
{
	zval first, second;
	bool result;

	if (!datetime || ZSTR_LEN(datetime) == 0 || time_string_is_now(datetime)) {
		return true;
	}

	if (ZSTR_VAL(datetime)[0] == '@') {
		return false;
	}

	if (!call_original_strtotime(datetime, 946684800, &first)) {
		if (!Z_ISUNDEF(first)) {
			zval_ptr_dtor(&first);
		}

		return false;
	}

	if (!call_original_strtotime(datetime, 978307200, &second)) {
		zval_ptr_dtor(&first);
		if (!Z_ISUNDEF(second)) {
			zval_ptr_dtor(&second);
		}

		return false;
	}

	result = Z_LVAL(first) != Z_LVAL(second);
	zval_ptr_dtor(&first);
	zval_ptr_dtor(&second);

	return result;
}

static bool resolve_time_string_to_timestamp_usec(zend_string *datetime, zval *timezone, int64_t *timestamp_usec)
{
	zend_long base;
	zval parsed;

	if (time_string_is_now(datetime)) {
		*timestamp_usec = get_shifted_time_usec_for_timezone(timezone);
		return true;
	}

	if (!datetime) {
		*timestamp_usec = get_shifted_time_usec_for_timezone(timezone);
		return true;
	}

	base = sec_from_usec(get_shifted_time_usec_for_timezone(timezone));
	if (!call_original_strtotime_with_timezone(datetime, base, timezone, &parsed)) {
		if (!Z_ISUNDEF(parsed)) {
			zval_ptr_dtor(&parsed);
		}
		return false;
	}

	*timestamp_usec = (int64_t) Z_LVAL(parsed) * 1000000;

	zval_ptr_dtor(&parsed);

	return true;
}

static void adjust_create_from_format_result(zval *datetime, format_flags_t *flags)
{
	zval timezone;
	datetime_parts_t original, shifted, result;
	bool has_time = flags->h || flags->i || flags->s || flags->us;

	ZVAL_UNDEF(&timezone);
	if (!get_date_timezone(datetime, &timezone)) {
		return;
	}

	if (!get_date_parts(datetime, &original) ||
		!get_timestamp_parts(get_shifted_time_usec_for_timezone(&timezone), &timezone, &shifted)) {
		zval_ptr_dtor(&timezone);

		return;
	}
	shifted.us = 0;

	result = shifted;
	if (has_time) {
		result.h = 0;
		result.i = 0;
		result.s = 0;
		result.us = 0;
	}

	if (flags->y) { result.y = original.y; }
	if (flags->m) { result.m = original.m; }
	if (flags->d) { result.d = original.d; }
	if (flags->h) { result.h = original.h; }
	if (flags->i) { result.i = original.i; }
	if (flags->s) { result.s = original.s; }
	if (flags->us) { result.us = original.us; }

	initialize_date_from_parts(datetime, &result, &timezone);
	zval_ptr_dtor(&timezone);
}

static void adjust_date_parse_from_format_result(zval *parsed, zend_string *format, zend_string *datetime)
{
	datetime_parts_t parts;
	format_flags_t flags;
	zval params[2], created;

	if (Z_TYPE_P(parsed) != IS_ARRAY) {
		return;
	}

	parse_format(ZSTR_VAL(format), &flags);

	ZVAL_STR(&params[0], format);
	ZVAL_STR(&params[1], datetime);
	ZVAL_UNDEF(&created);
	CALL_ORIGINAL_FUNCTION_WITH_PARAMS_RET(date_create_immutable_from_format, &created, params, 2);

	if (EG(exception) || Z_TYPE(created) != IS_OBJECT || !Z_PHPDATE_P(&created)->time) {
		if (!Z_ISUNDEF(created)) {
			zval_ptr_dtor(&created);
		}

		return;
	}

	adjust_create_from_format_result(&created, &flags);
	if (get_date_parts(&created, &parts)) {
		add_assoc_long(parsed, "year", parts.y);
		add_assoc_long(parsed, "month", parts.m);
		add_assoc_long(parsed, "day", parts.d);
		add_assoc_long(parsed, "hour", parts.h);
		add_assoc_long(parsed, "minute", parts.i);
		add_assoc_long(parsed, "second", parts.s);
		add_assoc_double(parsed, "fraction", (double) parts.us / 1000000.0);
	}

	zval_ptr_dtor(&created);
}

static inline bool pdo_time_apply(pdo_dbh_t *dbh)
{
	zend_string *sql;
	char buf[1024];

	if (!COLOPL_TS_G(pdo_mysql_orig_methods) || !COLOPL_TS_G(pdo_mysql_orig_methods)->doer) {
		return false;
	}

	slprintf(buf, sizeof(buf), "SET @@session.timestamp = " ZEND_LONG_FMT ";", get_shifted_time());
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

static void ZEND_FASTCALL hook_pdo_con(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(pdo_con);

	pdo_dbh_t *dbh = Z_PDO_DBH_P(ZEND_THIS);

	CALL_ORIGINAL_FUNCTION(pdo_con);
	if (EG(exception)) {
		return;
	}

	if (!dbh->driver || strcmp(dbh->driver->driver_name, "mysql") != 0 ||
		dbh->methods == &COLOPL_TS_G(hooked_mysql_driver_methods)) {
		return;
	}

	if (!COLOPL_TS_G(pdo_mysql_orig_methods)) {
		if (!dbh->methods || !dbh->methods->preparer || !dbh->methods->doer) {
			return;
		}

		COLOPL_TS_G(pdo_mysql_orig_methods) = dbh->methods;
		memcpy(&COLOPL_TS_G(hooked_mysql_driver_methods), dbh->methods, sizeof(struct pdo_dbh_methods));

		COLOPL_TS_G(hooked_mysql_driver_methods).preparer = hook_pdo_driver_preparer;
		COLOPL_TS_G(hooked_mysql_driver_methods).doer = hook_pdo_driver_doer;
	}

	dbh->methods = &COLOPL_TS_G(hooked_mysql_driver_methods);
}

static inline void date_common(INTERNAL_FUNCTION_PARAMETERS, int localtime)
{
	zend_string *format;
	zend_long ts;
	bool ts_is_null = true;

	ZEND_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_STR(format)
		Z_PARAM_OPTIONAL;
		Z_PARAM_LONG_OR_NULL(ts, ts_is_null)
	ZEND_PARSE_PARAMETERS_END();

	if (ts_is_null) {
		ts = get_shifted_time();
	}

	RETVAL_STR(php_format_date(ZSTR_VAL(format), ZSTR_LEN(format), ts, localtime));
}

static void adjust_date_create_result(zval *datetime, zend_string *time_str)
{
	zval timezone;
	int64_t timestamp_usec;

	if (Z_TYPE_P(datetime) != IS_OBJECT || !Z_PHPDATE_P(datetime)->time) {
		return;
	}

	if (!time_string_depends_on_current(time_str)) {
		return;
	}

	ZVAL_UNDEF(&timezone);
	if (!get_date_timezone(datetime, &timezone)) {
		return;
	}

	if (!resolve_time_string_to_timestamp_usec(time_str, &timezone, &timestamp_usec)) {
		zval_ptr_dtor(&timezone);

		return;
	}

	initialize_date_from_timestamp_usec(datetime, timestamp_usec, &timezone);
	zval_ptr_dtor(&timezone);
}

DEFINE_DT_HOOK_CONSTRUCTOR(dt);

DEFINE_DT_HOOK_CONSTRUCTOR(dti);

static void ZEND_FASTCALL hook_time(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(time);

	CALL_ORIGINAL_FUNCTION(time);
	RETURN_LONG(get_shifted_time());
}

static bool get_shifted_mktime_parts(bool localtime, datetime_parts_t *parts)
{
	zval timezone;
	bool result;

	ZVAL_UNDEF(&timezone);
	if (localtime) {
		if (!get_default_timezone(&timezone)) {
			return false;
		}
	} else if (!get_utc_timezone(&timezone)) {
		return false;
	}

	result = get_timestamp_parts(get_shifted_time_usec_for_timezone(&timezone), &timezone, parts);
	zval_ptr_dtor(&timezone);

	return result;
}

static void mktime_common(INTERNAL_FUNCTION_PARAMETERS, bool localtime)
{
	datetime_parts_t current;
	zend_long hour = 0, minute = 0, second = 0, month = 0, day = 0, year = 0;
	zval params[6];
	bool hour_is_null = true, minute_is_null = true, second_is_null = true,
		month_is_null = true, day_is_null = true, year_is_null = true;

	if (localtime) {
		CALL_ORIGINAL_FUNCTION(mktime);
	} else {
		CALL_ORIGINAL_FUNCTION(gmmktime);
	}

	if (EG(exception)) {
		return;
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_QUIET, 1, 6)
		Z_PARAM_LONG_OR_NULL(hour, hour_is_null)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG_OR_NULL(minute, minute_is_null)
		Z_PARAM_LONG_OR_NULL(second, second_is_null)
		Z_PARAM_LONG_OR_NULL(month, month_is_null)
		Z_PARAM_LONG_OR_NULL(day, day_is_null)
		Z_PARAM_LONG_OR_NULL(year, year_is_null)
	ZEND_PARSE_PARAMETERS_END();

	if (!hour_is_null && !minute_is_null && !second_is_null &&
		!month_is_null && !day_is_null && !year_is_null) {
		return;
	}

	if (!get_shifted_mktime_parts(localtime, &current)) {
		return;
	}

	ZVAL_LONG(&params[0], hour_is_null ? current.h : hour);
	ZVAL_LONG(&params[1], minute_is_null ? current.i : minute);
	ZVAL_LONG(&params[2], second_is_null ? current.s : second);
	ZVAL_LONG(&params[3], month_is_null ? current.m : month);
	ZVAL_LONG(&params[4], day_is_null ? current.d : day);
	ZVAL_LONG(&params[5], year_is_null ? current.y : year);

	if (localtime) {
		CALL_ORIGINAL_FUNCTION_WITH_PARAMS(mktime, params, 6);
	} else {
		CALL_ORIGINAL_FUNCTION_WITH_PARAMS(gmmktime, params, 6);
	}
}

static void ZEND_FASTCALL hook_mktime(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(mktime);

	mktime_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, true);
}

static void ZEND_FASTCALL hook_gmmktime(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(gmmktime);

	mktime_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, false);
}

static void ZEND_FASTCALL hook_date_create(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(date_create);

	zend_string *time_str = NULL;
	zval *timezone_object = NULL;

	CALL_ORIGINAL_FUNCTION(date_create);
	if (EG(exception) || !return_value || Z_TYPE_P(return_value) == IS_FALSE) {
		return;
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_QUIET, 0, 2)
		Z_PARAM_OPTIONAL
		Z_PARAM_STR_OR_NULL(time_str)
		Z_PARAM_OBJECT_OF_CLASS_OR_NULL(timezone_object, php_date_get_timezone_ce())
	ZEND_PARSE_PARAMETERS_END();

	adjust_date_create_result(return_value, time_str);
}

static void ZEND_FASTCALL hook_date_create_immutable(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(date_create_immutable);

	zend_string *time_str = NULL;
	zval *timezone_object = NULL;

	CALL_ORIGINAL_FUNCTION(date_create_immutable);
	if (EG(exception) || !return_value || Z_TYPE_P(return_value) == IS_FALSE) {
		return;
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_QUIET, 0, 2)
		Z_PARAM_OPTIONAL
		Z_PARAM_STR_OR_NULL(time_str)
		Z_PARAM_OBJECT_OF_CLASS_OR_NULL(timezone_object, php_date_get_timezone_ce())
	ZEND_PARSE_PARAMETERS_END();

	adjust_date_create_result(return_value, time_str);
}

DEFINE_CREATE_FROM_FORMAT(date_create_from_format);

DEFINE_CREATE_FROM_FORMAT(date_create_immutable_from_format);

DEFINE_CREATE_FROM_FORMAT_EX(dt_createfromformat, date_create_from_format);

DEFINE_CREATE_FROM_FORMAT_EX(dti_createfromformat, date_create_immutable_from_format);

static void ZEND_FASTCALL hook_date_parse_from_format(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(date_parse_from_format);

	zend_string *format, *datetime;

	CALL_ORIGINAL_FUNCTION(date_parse_from_format);
	if (EG(exception) || !return_value || Z_TYPE_P(return_value) != IS_ARRAY) {
		return;
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_QUIET, 2, 2)
		Z_PARAM_STR(format)
		Z_PARAM_STR(datetime)
	ZEND_PARSE_PARAMETERS_END();

	adjust_date_parse_from_format_result(return_value, format, datetime);
}

static void ZEND_FASTCALL hook_date(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(date);

	date_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}

static void ZEND_FASTCALL hook_gmdate(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(gmdate);

	date_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}

static void ZEND_FASTCALL hook_idate(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(idate);

	zend_long ts;
	zend_string *format;
	bool ts_is_null = 1;

	ZEND_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_STR(format)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG_OR_NULL(ts, ts_is_null)
	ZEND_PARSE_PARAMETERS_END();

	if (!ts_is_null || ZSTR_LEN(format) != 1) {
		CALL_ORIGINAL_FUNCTION(idate);
		return;
	}

	if (ts_is_null) {
		ts = get_shifted_time();
	}

	RETURN_LONG(php_idate(ZSTR_VAL(format)[0], ts, 0));
}

static void ZEND_FASTCALL hook_getdate(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(getdate);

	zend_long timestamp;
	zval params[1];
	bool timestamp_is_null = true;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG_OR_NULL(timestamp, timestamp_is_null)
	ZEND_PARSE_PARAMETERS_END();

	if (!timestamp_is_null) {
		CALL_ORIGINAL_FUNCTION(getdate);
		return;
	}

	/* Call original function with timestamp params. */
	ZVAL_LONG(&params[0], get_shifted_time());
	CALL_ORIGINAL_FUNCTION_WITH_PARAMS(getdate, params, 1);
}

static void ZEND_FASTCALL hook_localtime(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(localtime);

	zend_long timestamp;
	zval params[2];
	bool timestamp_is_null = true, associative = false;

	ZEND_PARSE_PARAMETERS_START(0, 2)
		Z_PARAM_OPTIONAL;
		Z_PARAM_LONG_OR_NULL(timestamp, timestamp_is_null);
		Z_PARAM_BOOL(associative);
	ZEND_PARSE_PARAMETERS_END();

	if (!timestamp_is_null) {
		CALL_ORIGINAL_FUNCTION(localtime);
		return;
	}

	/* Call original function with params. */
	ZVAL_LONG(&params[0], get_shifted_time());
	ZVAL_BOOL(&params[1], associative);
	CALL_ORIGINAL_FUNCTION_WITH_PARAMS(localtime, params, 2);
}

static void ZEND_FASTCALL hook_strtotime(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(strtotime);

	zend_long preset_ts;
	zend_string *times;
	zval params[2];
	bool preset_ts_is_null = true;

	ZEND_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_STR(times);
		Z_PARAM_OPTIONAL;
		Z_PARAM_LONG_OR_NULL(preset_ts, preset_ts_is_null);
	ZEND_PARSE_PARAMETERS_END();

	if (!preset_ts_is_null) {
		CALL_ORIGINAL_FUNCTION(strtotime);
		return;
	}

	/* Call original function based on shifted time */
	ZVAL_STR(&params[0], times);
	ZVAL_LONG(&params[1], get_shifted_time());
	CALL_ORIGINAL_FUNCTION_WITH_PARAMS(strtotime, params, 2);
}

static void ZEND_FASTCALL hook_strftime(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(strftime);

	zend_long timestamp;
	zend_string *format;
	zval params[2];
	bool timestamp_is_null = true;

	ZEND_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_STR(format)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG_OR_NULL(timestamp, timestamp_is_null)
	ZEND_PARSE_PARAMETERS_END();

	if (!timestamp_is_null) {
		CALL_ORIGINAL_FUNCTION(strftime);
		return;
	}

	ZVAL_STR(&params[0], format);
	ZVAL_LONG(&params[1], get_shifted_time());
	CALL_ORIGINAL_FUNCTION_WITH_PARAMS(strftime, params, 2);
}

static void ZEND_FASTCALL hook_gmstrftime(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(gmstrftime);

	zend_long timestamp;
	zend_string *format;
	zval params[2];
	bool timestamp_is_null = true;

	ZEND_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_STR(format)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG_OR_NULL(timestamp, timestamp_is_null)
	ZEND_PARSE_PARAMETERS_END();

	if (!timestamp_is_null) {
		CALL_ORIGINAL_FUNCTION(gmstrftime);
		return;
	}

	ZVAL_STR(&params[0], format);
	ZVAL_LONG(&params[1], get_shifted_time());
	CALL_ORIGINAL_FUNCTION_WITH_PARAMS(gmstrftime, params, 2);
}

static void ZEND_FASTCALL hook_uniqid(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(uniqid);

	zend_string *prefix = NULL, *original, *replacement;
	zend_long sec, usec;
	int64_t shifted_usec;
	zval *more_entropy = NULL;
	char id[32], *suffix_start;
	size_t prefix_len, timestamp_len, suffix_len;
	int fraction_width;

	CALL_ORIGINAL_FUNCTION(uniqid);
	if (EG(exception) || !return_value || Z_TYPE_P(return_value) != IS_STRING) {
		return;
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_QUIET, 0, 2)
		Z_PARAM_OPTIONAL
		Z_PARAM_STR_OR_NULL(prefix)
		Z_PARAM_ZVAL(more_entropy)
	ZEND_PARSE_PARAMETERS_END();
	(void) more_entropy;

	original = Z_STR_P(return_value);
	prefix_len = prefix ? ZSTR_LEN(prefix) : 0;
	if (ZSTR_LEN(original) <= prefix_len + 8) {
		return;
	}

	suffix_start = memchr(
		ZSTR_VAL(original) + prefix_len,
		'.',
		ZSTR_LEN(original) - prefix_len
	);
	timestamp_len = suffix_start
		? (size_t) (suffix_start - ZSTR_VAL(original) - prefix_len)
		: ZSTR_LEN(original) - prefix_len
	;

	if (timestamp_len <= 8 || timestamp_len >= sizeof(id)) {
		return;
	}

	shifted_usec = get_shifted_time_usec();
	sec = sec_from_usec(shifted_usec);
	usec = usec_remainder(shifted_usec);
	fraction_width = (int) timestamp_len - 8;
	slprintf(id, sizeof(id), "%08x%0*x", (unsigned int) sec, fraction_width, (unsigned int) usec);

	suffix_len = ZSTR_LEN(original) - prefix_len - timestamp_len;
	replacement = zend_string_alloc(prefix_len + timestamp_len + suffix_len, 0);
	if (prefix_len > 0) {
		memcpy(ZSTR_VAL(replacement), ZSTR_VAL(prefix), prefix_len);
	}

	memcpy(ZSTR_VAL(replacement) + prefix_len, id, timestamp_len);

	if (suffix_len > 0) {
		memcpy(
			ZSTR_VAL(replacement) + prefix_len + timestamp_len,
			ZSTR_VAL(original) + prefix_len + timestamp_len,
			suffix_len
		);
	}

	ZSTR_VAL(replacement)[prefix_len + timestamp_len + suffix_len] = '\0';

	zval_ptr_dtor(return_value);
	ZVAL_STR(return_value, replacement);
}

static bool get_shifted_local_year(zend_long *year)
{
	zval params[2], parts, *tm_year;
	bool result = false;

	ZVAL_LONG(&params[0], get_shifted_time());
	ZVAL_TRUE(&params[1]);
	ZVAL_UNDEF(&parts);
	CALL_ORIGINAL_FUNCTION_WITH_PARAMS_RET(localtime, &parts, params, 2);

	if (Z_TYPE(parts) == IS_ARRAY &&
		(tm_year = zend_hash_str_find(Z_ARRVAL(parts), "tm_year", strlen("tm_year"))) &&
		Z_TYPE_P(tm_year) == IS_LONG) {
		*year = Z_LVAL_P(tm_year) + 1900;
		result = true;
	}

	if (!Z_ISUNDEF(parts)) {
		zval_ptr_dtor(&parts);
	}

	return result;
}

static void easter_common(INTERNAL_FUNCTION_PARAMETERS, bool easter_date)
{
	zend_long year = 0, mode = 0, shifted_year;
	zval params[2];
	bool year_is_null = true;

	if (easter_date) {
		CALL_ORIGINAL_FUNCTION(easter_date);
	} else {
		CALL_ORIGINAL_FUNCTION(easter_days);
	}

	if (EG(exception)) {
		return;
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_QUIET, 0, 2)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG_OR_NULL(year, year_is_null)
		Z_PARAM_LONG(mode)
	ZEND_PARSE_PARAMETERS_END();

	if (!year_is_null) {
		return;
	}

	if (!get_shifted_local_year(&shifted_year)) {
		return;
	}

	ZVAL_LONG(&params[0], shifted_year);
	ZVAL_LONG(&params[1], mode);
	if (easter_date) {
		CALL_ORIGINAL_FUNCTION_WITH_PARAMS(easter_date, params, 2);
	} else {
		CALL_ORIGINAL_FUNCTION_WITH_PARAMS(easter_days, params, 2);
	}
}

static void ZEND_FASTCALL hook_easter_date(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(easter_date);

	easter_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, true);
}

static void ZEND_FASTCALL hook_easter_days(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(easter_days);

	easter_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, false);
}

#if HAVE_GETTIMEOFDAY
static inline void gettimeofday_common(INTERNAL_FUNCTION_PARAMETERS, int mode)
{
	zend_long sec, usec, offset, is_dst;
	zend_string *timezone_info;
	int64_t shifted_usec;
	bool get_as_float = false;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL;
		Z_PARAM_BOOL(get_as_float);
	ZEND_PARSE_PARAMETERS_END();

	shifted_usec = get_shifted_time_usec();
	sec = sec_from_usec(shifted_usec);
	usec = usec_remainder(shifted_usec);

	if (get_as_float) {
		RETVAL_DOUBLE((double) sec + ((double) usec / 1000000.00));
	} else {
		if (mode) {
			offset = 0;
			is_dst = 0;

			array_init(return_value);
			add_assoc_long(return_value, "sec", sec);
			add_assoc_long(return_value, "usec", usec);

			timezone_info = php_format_date("Z I", strlen("Z I"), sec, 1);
			sscanf(ZSTR_VAL(timezone_info), ZEND_LONG_FMT " " ZEND_LONG_FMT, &offset, &is_dst);
			zend_string_release(timezone_info);

			add_assoc_long(return_value, "minuteswest", -offset / 60);
			add_assoc_long(return_value, "dsttime", is_dst);
		} else {
			RETVAL_NEW_STR(zend_strpprintf(0, "%.8F %ld", usec / 1000000.00, (long) sec));
		}
	}
}

static void ZEND_FASTCALL hook_microtime(INTERNAL_FUNCTION_PARAMETERS)
{
	CHECK_STATE(microtime);

	gettimeofday_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}

static void ZEND_FASTCALL hook_gettimeofday(INTERNAL_FUNCTION_PARAMETERS)
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
	HOOK_FUNCTION(date_parse_from_format);
	HOOK_FUNCTION(date);
	HOOK_FUNCTION(gmdate);
	HOOK_FUNCTION(idate);
	HOOK_FUNCTION(getdate);
	HOOK_FUNCTION(localtime);
	HOOK_FUNCTION(strtotime);
	HOOK_FUNCTION_OPTIONAL(strftime);
	HOOK_FUNCTION_OPTIONAL(gmstrftime);
	HOOK_FUNCTION(uniqid);
	HOOK_FUNCTION_OPTIONAL(easter_date);
	HOOK_FUNCTION_OPTIONAL(easter_days);

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
	RESTORE_FUNCTION(date_parse_from_format);
	RESTORE_FUNCTION(date);
	RESTORE_FUNCTION(gmdate);
	RESTORE_FUNCTION(idate);
	RESTORE_FUNCTION(getdate);
	RESTORE_FUNCTION(localtime);
	RESTORE_FUNCTION(strtotime);
	RESTORE_FUNCTION_OPTIONAL(strftime);
	RESTORE_FUNCTION_OPTIONAL(gmstrftime);
	RESTORE_FUNCTION(uniqid);
	RESTORE_FUNCTION_OPTIONAL(easter_date);
	RESTORE_FUNCTION_OPTIONAL(easter_days);

#if HAVE_GETTIMEOFDAY
	RESTORE_FUNCTION(microtime);
	RESTORE_FUNCTION(gettimeofday);
#endif

	return true;
}

void apply_request_time_hook()
{
	zval *globals_server, *request_time, *request_time_float;
	int64_t request_usec, shifted_request_usec;
	double request_time_value;

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

	if (request_time) {
		if (COLOPL_TS_G(orig_request_time_float) != 0) {
			request_usec = (int64_t) (COLOPL_TS_G(orig_request_time_float) * 1000000.0);
		} else if (COLOPL_TS_G(orig_request_time) != 0) {
			request_usec = (int64_t) COLOPL_TS_G(orig_request_time) * 1000000;
		} else {
			return;
		}

		if (!calculate_shifted_time_usec(request_usec, NULL, &shifted_request_usec)) {
			return;
		}

		ZVAL_LONG(request_time, sec_from_usec(shifted_request_usec));
	}

	if (request_time_float) {
		if (COLOPL_TS_G(orig_request_time_float) != 0) {
			request_time_value = COLOPL_TS_G(orig_request_time_float);
		} else if (COLOPL_TS_G(orig_request_time) != 0) {
			request_time_value = (double) COLOPL_TS_G(orig_request_time);
		} else {
			return;
		}

		if (!calculate_shifted_time_usec((int64_t) (request_time_value * 1000000.0), NULL, &shifted_request_usec)) {
			return;
		}

		ZVAL_DOUBLE(request_time_float, (double) shifted_request_usec / 1000000.0);
	}
}
