--TEST--
Check hooked date functions preserve invalid argument handling
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

date_default_timezone_set('UTC');

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P1D'));

foreach ([
    static fn () => \date([]),
    static fn () => \gmdate([]),
    static fn () => \getdate('x'),
    static fn () => \localtime('x'),
    static fn () => \strtotime([]),
    static fn () => \date_parse_from_format([], ''),
] as $callable) {
    try {
        $callable();
        echo "missing TypeError\n";
    } catch (\TypeError) {
        echo "TypeError\n";
    }
}

\set_error_handler(static function (int $severity, string $message): bool {
    echo $message, PHP_EOL;
    return true;
});

\var_dump(\idate(''));
\var_dump(\idate('Ym'));

\restore_error_handler();

echo 'success', PHP_EOL;
?>
--EXPECT--
TypeError
TypeError
TypeError
TypeError
TypeError
TypeError
idate(): idate format is one char
bool(false)
idate(): idate format is one char
bool(false)
success
