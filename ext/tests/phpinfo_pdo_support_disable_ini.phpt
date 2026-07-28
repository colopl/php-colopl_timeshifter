--TEST--
Check phpinfo() reports "PDO Support => Disable" when the hook INI is off
--EXTENSIONS--
colopl_timeshifter
--INI--
colopl_timeshifter.is_hook_pdo_mysql=0
--FILE--
<?php declare(strict_types=1);

ob_start();
phpinfo(INFO_MODULES);
$info = ob_get_clean();

var_dump(str_contains($info, 'PDO Support => Disable'));
var_dump(str_contains($info, 'PDO Support => Enable'));
?>
--EXPECT--
bool(true)
bool(false)
