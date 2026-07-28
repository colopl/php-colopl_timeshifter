--TEST--
Extension works when the PDO extension is not loaded
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

use Colopl\ColoplTimeShifter\Manager;

date_default_timezone_set('UTC');

var_dump(Manager::isAvailable());
var_dump(Manager::hookDateInterval(new \DateInterval('PT10S')));
var_dump(Manager::isHooked());
Manager::unhook();
var_dump(Manager::isHooked());

echo 'success', PHP_EOL;
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(false)
success
