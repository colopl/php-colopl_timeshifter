--TEST--
Check GitHub PR - #7 (wrong createFromFormat)
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php

const DIFF_FORMAT = '%y-%m-%d %h:%i:%s.%F';

function expected_current_date_diff(DateInterval $interval)
{
    $now = new DateTime('now');
    $shifted = (clone $now)->sub($interval);

    return date_create($now->format('Y-m-d'))
        ->diff(date_create($shifted->format('Y-m-d')))
        ->format(DIFF_FORMAT)
    ;
}

function expected_current_day_diff(DateInterval $interval, $year_month)
{
    $now = new DateTime('now');
    $shifted = (clone $now)->sub($interval);

    return date_create($year_month . '-' . $now->format('d'))
        ->diff(date_create($year_month . '-' . $shifted->format('d')))
        ->format(DIFF_FORMAT)
    ;
}

function assert_diff($first, $second, $expected)
{
    $actual = $first->diff($second)->format(DIFF_FORMAT);

    var_dump($actual === $expected);
    if ($actual !== $expected) {
        echo 'actual: ', $actual, ', expected: ', $expected, \PHP_EOL;
    }
}

$first = date_create_from_format('Ymd', '19941026');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('PT1H'));
$second = date_create_from_format('Ymd', '19941026');
echo $first->diff($second)->format(DIFF_FORMAT), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$interval = new DateInterval('P1M');
$expected = expected_current_date_diff($interval);
$first = date_create_from_format('i', '30');
\Colopl\ColoplTimeShifter\register_hook($interval);
$second = date_create_from_format('i', '30');
assert_diff($first, $second, $expected);
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('YmdHisu', '19941026112233444444');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('PT30M'));
$second = date_create_from_format('YmdHisu', '19941026112233444444');
echo $first->diff($second)->format(DIFF_FORMAT), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('!Hisu', '112233444444');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('PT30M'));
$second = date_create_from_format('!Hisu', '112233444444');
echo $first->diff($second)->format(DIFF_FORMAT), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('dm|', '2610');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1D'));
$second = date_create_from_format('dm|', '2610');
echo $first->diff($second)->format(DIFF_FORMAT), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('P', 'Asia/Tokyo');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('PT30M'));
$second = date_create_from_format('P', 'Asia/Tokyo');
echo $first->diff($second)->format(DIFF_FORMAT), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$interval = new DateInterval('P1M');
$expected = expected_current_day_diff($interval, '1994-10');
$first = date_create_from_format('Ym\\d', '199410d');
\Colopl\ColoplTimeShifter\register_hook($interval);
$second = date_create_from_format('Ym\\d', '199410d');
assert_diff($first, $second, $expected);
\Colopl\ColoplTimeShifter\unregister_hook();

$interval = new DateInterval('P1M');
$expected = expected_current_date_diff($interval);
$first = date_create_from_format('u\\Y\\m\\d', '123456Ymd');
\Colopl\ColoplTimeShifter\register_hook($interval);
$second = date_create_from_format('u\\Y\\m\\d', '123456Ymd');
assert_diff($first, $second, $expected);
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('YmdP', '19941026Asia/Tokyo');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1M'));
$second = date_create_from_format('YmdP', '19941026Asia/Tokyo');
echo $first->diff($second)->format(DIFF_FORMAT), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('uYmdP', '12345619941026Asia/Tokyo');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1M'));
$second = date_create_from_format('uYmdP', '12345619941026Asia/Tokyo');
echo $first->diff($second)->format(DIFF_FORMAT), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

?>
--EXPECTF--
0-0-0 %d:%d:%d.000000
bool(true)
0-0-0 0:0:0.000000
0-0-0 0:0:0.000000
0-0-0 0:0:0.000000
0-0-0 0:%d:%d.%d
bool(true)
bool(true)
0-0-0 0:0:0.000000
0-0-0 0:0:0.000000
