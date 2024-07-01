--TEST--
Check date_create()
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

$shift_interval = new \DateInterval('P3D');

$before_now = \date_create();
$before_static = \date_create('@783097200');

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

$after_now = \date_create();
$after_static = \date_create('@783097200');

if ($after_now >= $before_now || $before_static != $after_static) {
    die('failure');
}

if (
    !$before_now instanceof \DateTime || !$before_static instanceof \DateTime ||
    !$after_now instanceof \DateTime || !$after_static instanceof \DateTime
) {
    die('failure');
}

die('success');
?>
--EXPECT--
success
