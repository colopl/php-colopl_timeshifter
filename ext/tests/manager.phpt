--TEST--
Check Manager compatibility API
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

use Colopl\ColoplTimeShifter\Manager;

date_default_timezone_set('UTC');

if (!\class_exists(Manager::class, false)) {
    die('failed: missing class');
}

$reflection = new \ReflectionClass(Manager::class);
if (! $reflection->isFinal()) {
    die('failed: manager is not final');
}

if (! Manager::isAvailable()) {
    die('failed: unavailable');
}

if (Manager::isHooked()) {
    die('failed: initially hooked');
}

$start = new \DateTimeImmutable('2020-02-01 00:00:00 UTC');
$end = new \DateTimeImmutable('2020-03-05 00:00:00 UTC');
$monthInterval = $start->diff($end);

$before = \time();
if (! Manager::hookDateInterval($monthInterval)) {
    die('failed: hookDateInterval');
}

if (! Manager::isHooked()) {
    die('failed: not hooked after interval');
}

$after = \time();
$diff = $before - $after;
if ($diff < 32 * 86400 || $diff > 34 * 86400) {
    \var_dump($diff, $monthInterval);
    die('failed: interval days normalization');
}

if (! Manager::unhook() || Manager::isHooked()) {
    die('failed: unhook');
}

$target = new \DateTimeImmutable('2001-02-03 04:05:06 UTC');
if (! Manager::hookDateTime($target)) {
    die('failed: hookDateTime');
}

$actual = new \DateTimeImmutable('now', new \DateTimeZone('UTC'));
if (\abs($actual->getTimestamp() - $target->getTimestamp()) > 1) {
    \var_dump($actual->format('Y-m-d H:i:s'), $target->format('Y-m-d H:i:s'));
    die('failed: hookDateTime target');
}

Manager::unhook();

echo 'success', PHP_EOL;
?>
--EXPECT--
success
