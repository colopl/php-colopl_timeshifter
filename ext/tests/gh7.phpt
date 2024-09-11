--TEST--
Check GitHub PR - #7 (wrong createFromFormat)
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php

$first = date_create_from_format('Ymd', '19941026');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('PT1H'));
$second = date_create_from_format('Ymd', '19941026');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s.%F'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('i', '30');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1MT30S'));
$second = date_create_from_format('i', '30');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s.%F'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('YmdHisu', '19941026112233444444');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('PT30M'));
$second = date_create_from_format('YmdHisu', '19941026112233444444');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s.%F'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('!Hisu', '112233444444');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('PT30M'));
$second = date_create_from_format('!Hisu', '112233444444');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s.%F'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('dm|', '2610');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1D'));
$second = date_create_from_format('dm|', '2610');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s.%F'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('P', 'Asia/Tokyo');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('PT30M'));
$second = date_create_from_format('P', 'Asia/Tokyo');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s.%F'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('Ym\\d', '199410d');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1M'));
$second = date_create_from_format('Ym\\d', '199410d');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s.%F'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('u\\Y\\m\\d', '123456Ymd');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1M'));
$second = date_create_from_format('u\\Y\\m\\d', '123456Ymd');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s.%F'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('YmdP', '19941026Asia/Tokyo');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1M'));
$second = date_create_from_format('YmdP', '19941026Asia/Tokyo');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s.%F'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('uYmdP', '12345619941026Asia/Tokyo');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1M'));
$second = date_create_from_format('uYmdP', '12345619941026Asia/Tokyo');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s.%F'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

?>
--EXPECTF--
0-0-0 1:0:0.000000
0-1-0 0:0:0.000000
0-0-0 0:0:0.000000
0-0-0 0:0:0.000000
0-0-0 0:0:0.000000
0-0-0 0:%d:%d.%d
0-0-0 0:0:0.000000
0-1-0 0:0:0.000000
0-0-0 0:0:0.000000
0-0-0 0:0:0.000000
