--TEST--
Check $_SERVER['REQUEST_TIME_FLOAT']
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

$before_request_time = $_SERVER['REQUEST_TIME_FLOAT'];

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P2Y'));

$after_request_time = $_SERVER['REQUEST_TIME_FLOAT'];

$before = new \DateTime('@' . $before_request_time);
$after = new \DateTime('@' . $after_request_time);

$interval = $after->diff($before);

if ($before == $after || $interval->y <= 0 || $interval->invert !== 0) {
    die('failed');
}

if ($before->format('u') !== $after->format('u')) {
    die('failed');
}

die('success');

?>
--EXPECT--
success
