--TEST--
Check strftime()/gmstrftime() use shifted current time when timestamp is omitted
--EXTENSIONS--
colopl_timeshifter
--SKIPIF--
<?php
if (!\function_exists('strftime') || !\function_exists('gmstrftime')) {
    die('skip strftime/gmstrftime are missing');
}
?>
--FILE--
<?php declare(strict_types=1);

\error_reporting(\E_ALL & ~\E_DEPRECATED);
\date_default_timezone_set('UTC');

$shift_interval = new \DateInterval('P3D');
$format = '%Y-%m-%d %H:%M:%S';
$timestamp = 1700000000;

$beforeLocal = new \DateTimeImmutable(\strftime($format));
$beforeGm = new \DateTimeImmutable(\gmstrftime($format), new \DateTimeZone('UTC'));
$explicitLocal = \strftime($format, $timestamp);
$explicitGm = \gmstrftime($format, $timestamp);

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

$afterLocal = new \DateTimeImmutable(\strftime($format));
$afterGm = new \DateTimeImmutable(\gmstrftime($format), new \DateTimeZone('UTC'));

if (\strftime($format, $timestamp) !== $explicitLocal) {
    die('failed: explicit strftime');
}

if (\gmstrftime($format, $timestamp) !== $explicitGm) {
    die('failed: explicit gmstrftime');
}

foreach ([[$afterLocal, $beforeLocal, 'strftime'], [$afterGm, $beforeGm, 'gmstrftime']] as [$after, $before, $label]) {
    $interval = $after->diff($before);
    if (!($after < $before && 4 > $interval->days && $interval->days >= 2 && $interval->invert === 0)) {
        \var_dump($label, $after, $before, $interval);
        die('failed');
    }
}

echo 'success', PHP_EOL;
?>
--EXPECT--
success
