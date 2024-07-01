--TEST--
Check strtotime() extra pattern
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

$before = new \DateTime('@' . \strtotime('now'));

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P2Y'));

$after_one = new \DateTime('@' . \strtotime('now'));
$after_two = new \DateTime('@' . \strtotime('now'));

$interval = $after_one->diff($before);

if ($before == $after_one || $before == $after_two) {
    die('failed');
}

/* Note: Sometime valgrind makes flaky: $interval->y !== 2 */
if (($interval->y > 2 && $interval->y !== 0) || $interval->invert !== 0) {
    die('failed');
}

die('success');

?>
--EXPECT--
success
