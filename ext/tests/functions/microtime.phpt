--TEST--
Check microtime()
--EXTENSIONS--
colopl_timeshifter
--SKIPIF--
<?php if (\function_exists('microtime') === \false) { die('skip microtime is missing'); } ?>
--FILE--
<?php declare(strict_types=1);

$shift_interval = new \DateInterval('P3D');

$before1 = \explode(' ', \microtime());
$before2 = \microtime(\true);

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

$after1 = \explode(' ', \microtime());
$after2 = \microtime(\true);

$interval1 = (new \DateTime("@{$after1[1]}"))->diff(new \DateTime("@{$before1[1]}"));
$interval2 = (new \DateTime("@{$after2}"))->diff(new \DateTime("@{$before2}"));

if (4 > $interval1->days && $interval1->days >= 2 && $interval1->invert === 0 &&
    4 > $interval2->days && $interval2->days >= 2 && $interval2->invert === 0
) {
    die('success');
}

die('failed');
?>
--EXPECT--
success
