--TEST--
Check $_SERVER['REQUEST_TIME']
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

$before_request_time = $_SERVER['REQUEST_TIME'];

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P1Y'));

$after_request_time = $_SERVER['REQUEST_TIME'];

$before = new \DateTime('@' . $before_request_time);
$after = new \DateTime('@' . $after_request_time);

$interval = $after->diff($before);

if ($before == $after || $interval->y !== 1 || $interval->invert !== 0) {
    die('failed');
}

die('success');

?>
--EXPECT--
success
