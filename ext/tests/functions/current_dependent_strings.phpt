--TEST--
Check current-dependent date strings use shifted base time
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

date_default_timezone_set('UTC');

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P365D'));

$strings = [
    'May 5',
    '10 September',
    '10:30',
    'noon',
    'next Monday',
    'first day of this month',
    'last day of next month',
];

function possible_timestamps(int $start, int $end, callable $timestamp): array
{
    $timestamps = [];
    for ($base = $start; $base <= $end + 1; $base++) {
        $value = $timestamp($base);
        if ($value !== false) {
            $timestamps[] = $value;
        }
    }

    return \array_values(\array_unique($timestamps));
}

foreach ($strings as $string) {
    $baseStart = time();
    $actual = (new \DateTimeImmutable($string))->getTimestamp();
    $actualByStrtotime = strtotime($string);
    $baseEnd = time();

    $expected = possible_timestamps(
        $baseStart,
        $baseEnd,
        static fn (int $base): int|false => strtotime($string, $base)
    );
    if (!\in_array($actual, $expected, true) || !\in_array($actualByStrtotime, $expected, true)) {
        echo "failed: {$string}\n";
        var_dump($baseStart, $baseEnd, $expected, $actual, $actualByStrtotime);
        exit;
    }
}

$tokyo = new \DateTimeZone('Asia/Tokyo');
$baseStart = time();
$actualTokyo = new \DateTimeImmutable('10:30', $tokyo);
$baseEnd = time();
$expectedTokyo = possible_timestamps(
    $baseStart,
    $baseEnd,
    static fn (int $base): int => (new \DateTimeImmutable("@{$base}"))->setTimezone($tokyo)->modify('10:30')->getTimestamp()
);
if (!\in_array($actualTokyo->getTimestamp(), $expectedTokyo, true) || $actualTokyo->getTimezone()->getName() !== 'Asia/Tokyo') {
    echo "failed: explicit timezone\n";
    var_dump($baseStart, $baseEnd, $expectedTokyo, $actualTokyo);
    exit;
}

$baseStart = time();
$timezoneDate = new \DateTimeImmutable('Asia/Tokyo');
$timezoneStrtotime = strtotime('Asia/Tokyo');
$baseEnd = time();
$expectedTimezoneDate = \range($baseStart, $baseEnd + 1);
$expectedStrtotime = possible_timestamps(
    $baseStart,
    $baseEnd,
    static fn (int $base): int|false => strtotime('Asia/Tokyo', $base)
);
if (!\in_array($timezoneDate->getTimestamp(), $expectedTimezoneDate, true) ||
    $timezoneDate->getTimezone()->getName() !== 'Asia/Tokyo' ||
    !\in_array($timezoneStrtotime, $expectedStrtotime, true)) {
    echo "failed: Asia/Tokyo\n";
    var_dump(
        $baseStart,
        $baseEnd,
        $timezoneDate->getTimestamp(),
        $timezoneDate->getTimezone()->getName(),
        $timezoneStrtotime,
        $expectedStrtotime
    );
    exit;
}

echo 'success', PHP_EOL;
?>
--EXPECT--
success
