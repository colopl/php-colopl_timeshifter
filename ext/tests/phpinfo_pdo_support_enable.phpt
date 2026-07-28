--TEST--
Check phpinfo() reports "PDO Support => Enable" when the PDO hook is active
--EXTENSIONS--
colopl_timeshifter
pdo
--SKIPIF--
<?php
if (getenv('COLOPL_TS_NO_PDO_HOOK_BUILD')) {
    die('skip colopl_timeshifter built without PDO hook support');
}
?>
--INI--
colopl_timeshifter.is_hook_pdo_mysql=1
--FILE--
<?php declare(strict_types=1);

ob_start();
phpinfo(INFO_MODULES);
$info = ob_get_clean();

var_dump(str_contains($info, 'PDO Support => Enable'));
var_dump(str_contains($info, 'PDO Support => Disable'));
?>
--EXPECT--
bool(true)
bool(false)
