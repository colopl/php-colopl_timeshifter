--TEST--
Check easter_date()/easter_days() use shifted current year
--EXTENSIONS--
colopl_timeshifter
--SKIPIF--
<?php
if (!\function_exists('easter_date') || !\function_exists('easter_days')) {
    die('skip calendar extension missing');
}
?>
--FILE--
<?php declare(strict_types=1);

\date_default_timezone_set('UTC');
\putenv('TZ=UTC');

$explicitDate = \easter_date(2024);
$explicitDays = \easter_days(2024);

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P370D'));

$localtime = \localtime(null, true);
$shiftedYear = $localtime['tm_year'] + 1900;

if (\easter_date(2024) !== $explicitDate || \easter_days(2024) !== $explicitDays) {
    die('failed explicit');
}

if (\easter_date() !== \easter_date($shiftedYear)) {
    die('failed date default');
}

if (\easter_days() !== \easter_days($shiftedYear)) {
    die('failed days default');
}

echo 'success', PHP_EOL;
?>
--EXPECT--
success
