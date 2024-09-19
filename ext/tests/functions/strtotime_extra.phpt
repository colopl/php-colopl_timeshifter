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

if ($before->getTimestamp() == $after_one->getTimestamp()) {
    die('failed, $before == $after_one, ' . $before->format('Y-m-d H:i:s.u') . ' / ' . $after_one->format('Y-m-d H:i:s.u') . ' / ' . $after_two->format('Y-m-d H:i:s.u'));
}
if ($before->getTimestamp() == $after_two->getTimestamp()) {
    die('failed, $before == $after_two, ' . $before->format('Y-m-d H:i:s.u') . ' / ' . $after_one->format('Y-m-d H:i:s.u') . ' / ' . $after_two->format('Y-m-d H:i:s.u'));
}

/* Note: Sometime valgrind makes flaky: $interval->y !== 2 */
if (($interval->y > 2 && $interval->y !== 0) || $interval->invert !== 0) {
    die('failed, $interval->y = ' . $interval->y . ', $interval->invert = ' . $interval->invert);
}

die('success');

?>
--EXPECT--
success
