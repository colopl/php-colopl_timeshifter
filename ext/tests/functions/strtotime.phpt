--TEST--
Check strtotime()
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

$shift_interval = new \DateInterval('P3D');

$now = \time();

$before = \date('Y-m-d H:i:s.u', strtotime("@{$now} +1 year"));

$before_fixed = strtotime("10 September 2000");

$before_now = strtotime("now");

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

$now = \time();

$after = \date('Y-m-d H:i:s.u', strtotime("@{$now} +1 year"));

$after_fixed = strtotime("10 September 2000");

$after_now = strtotime("now");

$interval = (new \DateTime("{$after}"))->diff(new \DateTime("{$before}"));

if ($before_now != $after_now && $before_fixed === $after_fixed && 4 > $interval->days && $interval->days >= 2 && $interval->invert === 0) {
    die('success');
}

die('failed');
?>
--EXPECT--
success
