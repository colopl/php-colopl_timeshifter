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
#ifndef HOOK_H
# define HOOK_H

# include "php.h"
# include "shared_memory.h"

bool register_hooks();
void register_pdo_hook();
bool unregister_hooks();
void apply_request_time_hook();

#endif	/* HOOK_H */
