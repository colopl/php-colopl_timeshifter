--TEST--
Check unsupported special relative intervals are rejected without throwing
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

$result = \Colopl\ColoplTimeShifter\register_hook(\DateInterval::createFromDateString('next weekday'));

if ($result !== false || \Colopl\ColoplTimeShifter\is_hooked()) {
    die('failed');
}

echo 'success', PHP_EOL;
?>
--EXPECT--
success
