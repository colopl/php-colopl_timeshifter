--TEST--
Check hooked DateTime APIs preserve invalid argument handling
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

date_default_timezone_set('UTC');

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P1D'));

foreach ([
    static fn () => new \DateTime([]),
    static fn () => new \DateTimeImmutable([]),
    static fn () => \DateTime::createFromFormat([], 'x'),
    static fn () => \DateTimeImmutable::createFromFormat([], 'x'),
    static fn () => \date_create_from_format([], 'x'),
    static fn () => \date_create_immutable_from_format([], 'x'),
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
TypeError
TypeError
TypeError
TypeError
success
