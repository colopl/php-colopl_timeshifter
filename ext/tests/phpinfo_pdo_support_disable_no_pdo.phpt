--TEST--
Check phpinfo() reports "PDO Support => Disable" when the PDO extension is not loaded
--EXTENSIONS--
colopl_timeshifter
--SKIPIF--
<?php
if (extension_loaded('pdo')) die('skip PDO extension is loaded');
?>
--INI--
colopl_timeshifter.is_hook_pdo_mysql=1
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
