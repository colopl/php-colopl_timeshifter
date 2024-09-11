--TEST--
Check GitHub PR - #7 (wrong createFromFormat)
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php

$first = date_create_from_format('Ymd', '19941026');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('PT1H'));
$second = date_create_from_format('Ymd', '19941026');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('i', '30');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1DT30S'));
$second = date_create_from_format('i', '30');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('YmdHisu', '19941026112233444444');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('PT30M'));
$second = date_create_from_format('YmdHisu', '19941026112233444444');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('!Hisu', '112233444444');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('PT30M'));
$second = date_create_from_format('!Hisu', '112233444444');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('dm|', '2610');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1D'));
$second = date_create_from_format('dm|', '2610');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('P', '+0230');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('PT1H'));
$second = date_create_from_format('P', '+0230');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('Y\\md', '1994m26');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('PT1H'));
$second = date_create_from_format('Y\\md', '1994m26');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('Ym\\d', '199410d');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1M'));
$second = date_create_from_format('Ym\\d', '199410d');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

$first = date_create_from_format('u\\Y\\m\\d', '123456Ymd');
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1M'));
$second = date_create_from_format('u\\Y\\m\\d', '123456Ymd');
echo $first->diff($second)->format('%y-%m-%d %h:%i:%s'), \PHP_EOL;
\Colopl\ColoplTimeShifter\unregister_hook();

?>
--EXPECT--
0-0-0 1:0:0
0-0-1 0:0:0
0-0-0 0:0:0
0-0-0 0:0:0
0-0-0 0:0:0
0-0-0 0:0:0
0-0-0 1:0:0
0-0-0 0:0:0
0-1-0 0:0:0
