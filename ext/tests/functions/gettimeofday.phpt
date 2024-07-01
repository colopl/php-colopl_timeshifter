--TEST--
Check gettimeofday()
--EXTENSIONS--
colopl_timeshifter
--SKIPIF--
<?php if (\function_exists('gettimeofday') === \false) { die('skip gettimeofday is missing'); } ?>
--FILE--
<?php declare(strict_types=1);

$shift_interval = new \DateInterval('P3D');

$before1 = \gettimeofday();
$before2 = \gettimeofday(\true);

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

$after1 = \gettimeofday();
$after2 = \gettimeofday(\true);

$interval1 = (new \DateTime("@{$after1['sec']}"))->diff(new \DateTime("@{$before1['sec']}"));
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
