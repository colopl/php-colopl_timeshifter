--TEST--
Check GitHub PR - #12 (wrong strtotime)
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

shell_exec('date -s "2024-09-17 17:00:00"');

\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1D'));

echo (new \DateTime('@' . strtotime('now')))->format('Y-m-d H:i:s.u'), \PHP_EOL;
echo (new \DateTime('@' . strtotime('today')))->format('Y-m-d H:i:s.u'), \PHP_EOL;
echo (new \DateTime('@' . strtotime('tomorrow')))->format('Y-m-d H:i:s.u'), \PHP_EOL;
echo (new \DateTime('@' . strtotime('yesterday')))->format('Y-m-d H:i:s.u'), \PHP_EOL;

echo (new \DateTime('now'))->format('Y-m-d H:i:s.u'), \PHP_EOL;
echo (new \DateTime('today'))->format('Y-m-d H:i:s.u'), \PHP_EOL;
echo (new \DateTime('tomorrow'))->format('Y-m-d H:i:s.u'), \PHP_EOL;
echo (new \DateTime('yesterday'))->format('Y-m-d H:i:s.u'), \PHP_EOL;

shell_exec("date -s \"{$current_date}\"");

?>
--EXPECTF--
2024-09-16 17:00:00.%d
2024-09-16 00:00:00.%d
2024-09-17 00:00:00.%d
2024-09-15 00:00:00.%d
2024-09-16 17:00:00.%d
2024-09-16 00:00:00.%d
2024-09-17 00:00:00.%d
2024-09-15 00:00:00.%d
