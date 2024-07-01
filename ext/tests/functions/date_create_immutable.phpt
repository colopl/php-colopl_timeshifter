--TEST--
Check date_create_immutable()
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

$shift_interval = new \DateInterval('P3D');

$before_now = \date_create_immutable();
$before_static = \date_create_immutable('@783097200');

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

$after_now = \date_create_immutable();
$after_static = \date_create_immutable('@783097200');

if ($after_now >= $before_now || $before_static != $after_static) {
    die('failure');
}

if (
    !$before_now instanceof \DateTimeImmutable || !$before_static instanceof \DateTimeImmutable ||
    !$after_now instanceof \DateTimeImmutable || !$after_static instanceof \DateTimeImmutable
) {
    die('failure');
}

die('success');
?>
--EXPECT--
success
