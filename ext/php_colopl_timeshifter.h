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
#ifndef PHP_COLOPL_TIMESHIFTER_H
# define PHP_COLOPL_TIMESHIFTER_H

# include "ext/date/php_date.h"
# include "ext/pdo/php_pdo_driver.h"

# include "shared_memory.h"

typedef struct _colopl_timeshifter_interval_t {
	bool initialized;
	int civil_or_wall;
	timelib_rel_time diff;
} colopl_timeshifter_interval_t;

bool get_shift_interval(colopl_timeshifter_interval_t *interval);
bool get_is_hooked();
bool validate_shift_interval(zval *interval);

extern zend_module_entry colopl_timeshifter_module_entry;
# define phpext_colopl_timeshifter_ptr &colopl_timeshifter_module_entry

# define PHP_COLOPL_TIMESHIFTER_VERSION "2.0.0"

ZEND_BEGIN_MODULE_GLOBALS(colopl_timeshifter)
	struct pdo_dbh_methods hooked_mysql_driver_methods;
	const struct pdo_dbh_methods *pdo_mysql_orig_methods;
	zif_handler orig_pdo_con; /* \PDO::__construct */
	zif_handler orig_dt_con; /* \DateTime::__construct() */
	zif_handler orig_dt_createfromformat; /* \DateTime::createFromFormat() */
	zif_handler orig_dti_con; /* \DateTimeImmutable::__construct() */
	zif_handler orig_dti_createfromformat; /* \DateTimeImmutable::createFromFormat() */
	zif_handler orig_time;
	zif_handler orig_mktime;
	zif_handler orig_gmmktime;
	zif_handler orig_date_create;
	zif_handler orig_date_create_immutable;
	zif_handler orig_date_create_from_format;
	zif_handler orig_date_create_immutable_from_format;
	zif_handler orig_date_parse_from_format;
	zif_handler orig_date;
	zif_handler orig_gmdate;
	zif_handler orig_idate;
	zif_handler orig_getdate;
	zif_handler orig_localtime;
	zif_handler orig_strtotime;
	zif_handler orig_strftime;
	zif_handler orig_gmstrftime;
	zif_handler orig_uniqid;
	zif_handler orig_easter_date;
	zif_handler orig_easter_days;
# if HAVE_GETTIMEOFDAY
	zif_handler orig_microtime;
	zif_handler orig_gettimeofday;
# endif
	zend_long orig_request_time;
	double orig_request_time_float;
	bool in_internal_call;
	bool is_restore_per_request;
	bool is_hook_pdo_mysql;
	bool is_hook_request_time;
ZEND_END_MODULE_GLOBALS(colopl_timeshifter)

ZEND_EXTERN_MODULE_GLOBALS(colopl_timeshifter)

# define COLOPL_TS_G(v)  ZEND_MODULE_GLOBALS_ACCESSOR(colopl_timeshifter, v)

PHP_MINIT_FUNCTION(colopl_timeshifter);
PHP_MSHUTDOWN_FUNCTION(colopl_timeshifter);
PHP_RINIT_FUNCTION(colopl_timeshifter);
PHP_RSHUTDOWN_FUNCTION(colopl_timeshifter);
PHP_MINFO_FUNCTION(colopl_timeshifter);
/* PHP_GINIT_FUNCTION(colopl_timeshifter); */

# if defined(ZTS) && defined(COMPILE_DL_COLOPL_TIMESHIFTER)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

#endif	/* PHP_COLOPL_TIMESHIFTER_H */
