--TEST--
Check idate()
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

$shift_interval = new \DateInterval('P3D');

$before = \idate('U');

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

$after = \idate('U');

$interval = (new \DateTime("@{$after}"))->diff(new \DateTime("@{$before}"));

if (4 > $interval->days && $interval->days >= 2 && $interval->invert === 0) {
    die('success');
}

die('failed');
?>
--EXPECT--
success
