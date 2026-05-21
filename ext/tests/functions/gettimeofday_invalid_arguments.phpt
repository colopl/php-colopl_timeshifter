--TEST--
Check hooked microtime/gettimeofday preserve invalid argument handling
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

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P1D'));

foreach ([
    static fn () => \microtime([]),
    static fn () => \gettimeofday([]),
] as $callable) {
    try {
        $callable();
        echo "missing TypeError\n";
    } catch (\TypeError) {
        echo "TypeError\n";
    }
}

echo 'success', PHP_EOL;
?>
--EXPECT--
TypeError
TypeError
success
