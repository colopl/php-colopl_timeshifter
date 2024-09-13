--TEST--
Check GitHub PR - #10 (if invalid argument passed to strtotime to SEGV)
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php

echo 'before:', strtotime('invalid'), \PHP_EOL;
\Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1D'));
echo 'after:', strtotime('invalid'), \PHP_EOL;
die('Success');

?>
--EXPECT--
before:
after:
Success
