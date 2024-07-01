--TEST--
Check DateTime::createFromFormat()
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

$shift_interval = new \DateInterval('P3Y');

$before_now = \DateTime::createFromFormat('m', '5');
$before_static = \DateTime::createFromFormat('Y-m-d H:i:s.u', '2024-02-27 09:52:55.12345');

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

usleep(5);

$after_now = \DateTime::createFromFormat('m', '5');
$after_static = \DateTime::createFromFormat('Y-m-d H:i:s.u', '2024-02-27 09:52:55.12345');

$interval = $after_now->diff($before_now);

if (!$before_now instanceof \DateTime || !$before_static instanceof \DateTime || !$after_now instanceof \DateTime || !$after_static instanceof \DateTime) {
    die('failed');
}

if ($after_now != $before_now && $interval->y === 3 && $interval->invert === 0) {
    die('success');
}

die('failed');
?>
--EXPECT--
success
