--TEST--
Check GitHub PR - #9 (wrong createFromFormat of start of month)
--EXTENSIONS--
colopl_timeshifter
--SKIPIF--
<?php
if (posix_getuid() !== 0) die('skip require root');
if (!is_string(($result = shell_exec('date')))) die ('skip cannot set current date');
?>
--FILE--
<?php

$current_date = trim(shell_exec('date "+%Y-%m-%d %H:%M:%S"'));

shell_exec('date -s "2024-09-01"');

$interval = new DateInterval('P1D');

$first = new \DateTime();
echo $first->format('Y-m-d H:i:s.u'), \PHP_EOL;
\Colopl\ColoplTimeShifter\register_hook($interval);
echo (new \DateTime())->format('Y-m-d H:i:s.u'), \PHP_EOL;
$second = date_create_from_format('d', '10');
echo $second->format('Y-m-d H:i:s.u'), \PHP_EOL;

shell_exec("date -s \"{$current_date}\"");

?>
--EXPECTF--
2024-09-01 %d:%d:%d.%d
2024-08-31 %d:%d:%d.%d
2024-08-10 00:00:%d.000000
