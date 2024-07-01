--TEST--
Check DateTime::__construct()
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

$shift_interval = new \DateInterval('P3D');

$before_now = new \DateTime();
$before_static = new \DateTime('@783097200');

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

$after_now = new \DateTime();
$after_static = new \DateTime('@783097200');

if ($after_now >= $before_now || $before_static != $after_static) {
    die('failure');
}

die('success');
?>
--EXPECT--
success
