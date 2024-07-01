--TEST--
Check getdate()
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

$shift_interval = new \DateInterval('P3D');

$before = \getdate();

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

$after = \getdate();

$interval = (new \DateTime("@{$after[0]}"))->diff(new \DateTime("@{$before[0]}"));

if (4 > $interval->days && $interval->days >= 2 && $interval->invert === 0) {
    die('success');
}

die('failed');
?>
--EXPECT--
success
