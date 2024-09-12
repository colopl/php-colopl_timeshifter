--TEST--
Check GitHub PR - #9 (wrong createFromFormat of start of month)
--EXTENSIONS--
colopl_timeshifter
--SKIPIF--
<?php
if (posix_getuid() !== 0) die('skip require root');
if (! is_string(($result = shell_exec('date -s "2024-09-01"')))) die ('skip cannot set current date');
try {
    $dt = new \DateTimeImmutable($result);
    if (!($dt instanceof \DateTimeImmutable)) {
        throw new Exception("construction failed: {$result}");
    }
} catch (\Exception $e) {
    die("skip {$e->getmessage()}");
}
if ($dt->format('d') !== '01') die('skip not start of month');
?>
--FILE--
<?php

$interval = new DateInterval('P1D');

$first = new \DateTime();
echo $first->format('Y-m-d H:i:s.u'), \PHP_EOL;
\Colopl\ColoplTimeShifter\register_hook($interval);
echo (new \DateTime())->format('Y-m-d H:i:s.u'), \PHP_EOL;
$second = date_create_from_format('d', '10');
echo $second->format('Y-m-d H:i:s.u'), \PHP_EOL;

?>
--EXPECTF--
2024-09-01 %d:%d:%d.%d
2024-08-31 %d:%d:%d.%d
2024-08-10 00:00:%d.000000
