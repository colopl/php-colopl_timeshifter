--TEST--
Check mktime()
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

\date_default_timezone_set('UTC');

$shift_interval = new \DateInterval('PT30M');

$explicit = \mktime(12, 34, 56, 1, 2, 2024);
$realBeforeHook = \time();

$expected = [];
for ($elapsed = 0; $elapsed <= 120; $elapsed++) {
    $shifted = (new \DateTimeImmutable('@' . ($realBeforeHook + $elapsed)))
        ->setTimezone(new \DateTimeZone('UTC'))
        ->sub($shift_interval);
    $expected[] = \mktime(
        12,
        (int) $shifted->format('i'),
        (int) $shifted->format('s'),
        (int) $shifted->format('n'),
        (int) $shifted->format('j'),
        (int) $shifted->format('Y'),
    );
}

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

$after = \mktime(12);
$explicitAfter = \mktime(12, 34, 56, 1, 2, 2024);

if ($explicitAfter !== $explicit) {
    die('failed explicit');
}

if (\in_array($after, $expected, true)) {
    die('success');
}

die('failed');

?>
--EXPECT--
success
