--TEST--
Check DateTimeImmutable::__construct()
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

$shift_interval = new \DateInterval('P3D');

$before_now = new \DateTimeImmutable();
$before_static = new \DateTimeImmutable('@783097200');

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

$after_now = new \DateTimeImmutable();
$after_static = new \DateTimeImmutable('@783097200');

if ($after_now >= $before_now || $before_static != $after_static) {
    die('failure');
}

die('success');
?>
--EXPECT--
success
