--TEST--
Check GitHub Issue - #5 (createFromFormat returns incorrect result during hook)
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php

$interval = new DateInterval('P1DT1H2M1S');
$interval->invert = 1;

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
