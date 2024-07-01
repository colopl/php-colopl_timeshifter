--TEST--
Check gmmktime()
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

$shift_interval = new \DateInterval('PT30M');

$before = \gmmktime(12);

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

$after = \gmmktime(12);

$interval = (new \DateTime("@{$after}"))->diff(new \DateTime("@{$before}"));

if ($interval->i >= 29 && 32 > $interval->i && $interval->invert === 0) {
    die('success');
}

die('failed');

?>
--EXPECT--
success
