--TEST--
Check localtime()
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

$shift_interval = new \DateInterval('P3D');

$before1 = \localtime(); $before1[5] += 1900;
$before2 = \localtime(associative: \true); $before2['tm_year'] += 1900;

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

$after1 = \localtime(); $after1[5] += 1900;
$after2 = \localtime(associative: \true); $after2['tm_year'] += 1900;

$interval1 = (new \DateTime("{$after1[5]}-{$after1[4]}-{$after1[3]} {$after1[2]}:{$after1[1]}:{$after1[0]}"))->diff(new \DateTime("{$before1[5]}-{$before1[4]}-{$before1[3]} {$before1[2]}:{$before1[1]}:{$before1[0]}"));
$interval2 = (new \DateTime("{$after2['tm_year']}-{$after2['tm_mon']}-{$after2['tm_mday']} {$after2['tm_hour']}:{$after2['tm_min']}:{$after2['tm_sec']}"))->diff(new \DateTime("{$before2['tm_year']}-{$before2['tm_mon']}-{$before2['tm_mday']} {$before2['tm_hour']}:{$before2['tm_min']}:{$before2['tm_sec']}"));

if ($interval1->days >= 2 && $interval1->invert === 0) {
    die('success');
}

die('failed');
?>
--EXPECT--
success
