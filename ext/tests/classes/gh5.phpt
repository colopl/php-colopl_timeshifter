--TEST--
Check GitHub Issue - #5 (createFromFormat returns incorrect result during hook)
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php

$interval = @unserialize('O:12:"DateInterval":16:{s:1:"y";i:0;s:1:"m";i:0;s:1:"d";i:1;s:1:"h";i:1;s:1:"i";i:2;s:1:"s";i:1;s:1:"f";d:0;s:7:"weekday";i:0;s:16:"weekday_behavior";i:0;s:17:"first_last_day_of";i:0;s:6:"invert";i:1;s:4:"days";i:1;s:12:"special_type";i:0;s:14:"special_amount";i:0;s:21:"have_weekday_relative";i:0;s:21:"have_special_relative";i:0;}');

echo DateTime::createFromFormat('YmdHi', 202211091600)->format('Y-m-d H:i:s'), \PHP_EOL;

\Colopl\ColoplTimeShifter\register_hook($interval);

echo DateTime::createFromFormat('YmdHi', 202211091600)->format('Y-m-d H:i:s'), \PHP_EOL;

\Colopl\ColoplTimeShifter\unregister_hook();

echo DateTime::createFromFormat('YmdHi', 202211091600)->format('Y-m-d H:i:s'), \PHP_EOL;

\Colopl\ColoplTimeShifter\register_hook($interval);

echo DateTime::createFromFormat('YmdHi', 202211091600)->format('Y-m-d H:i:s'), \PHP_EOL;

?>
--EXPECT--
2022-11-09 16:00:00
2022-11-09 16:00:00
2022-11-09 16:00:00
2022-11-09 16:00:00
