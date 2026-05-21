--TEST--
Check negative shifted timestamps are represented consistently
--EXTENSIONS--
colopl_timeshifter
--SKIPIF--
<?php
if (!\function_exists('microtime')) {
    die('skip microtime is missing');
}
if (!\function_exists('gettimeofday')) {
    die('skip gettimeofday is missing');
}
?>
--FILE--
<?php declare(strict_types=1);

date_default_timezone_set('UTC');

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P200Y'));

$time = \time();
$microtimeFloat = \microtime(true);
$microtimeParts = \explode(' ', \microtime());
$timeofday = \gettimeofday();

if ($time >= 0 ||
    $microtimeFloat >= 0 ||
    (int) $microtimeParts[1] >= 0 ||
    (float) $microtimeParts[0] < 0.0 ||
    (float) $microtimeParts[0] >= 1.0 ||
    $timeofday['sec'] >= 0 ||
    $timeofday['usec'] < 0 ||
    $timeofday['usec'] >= 1000000
) {
    \var_dump($time, $microtimeFloat, $microtimeParts, $timeofday);
    die('failed');
}

echo 'success', PHP_EOL;
?>
--EXPECT--
success
