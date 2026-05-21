--TEST--
Check date_parse_from_format() missing fields use shifted current time
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

date_default_timezone_set('UTC');

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P365D'));

$windowStart = new \DateTimeImmutable();

$monthOnly = \date_parse_from_format('m', '5');
$timeOnly = \date_parse_from_format('H:i', '12:34');
$reset = \date_parse_from_format('!m', '5');
$absolute = \date_parse_from_format('Y-m-d H:i:s.u', '2024-02-27 09:52:55.123456');

$windowEnd = new \DateTimeImmutable();

function possible_parts(\DateTimeImmutable $start, \DateTimeImmutable $end, callable $parts): array
{
    $possible = [];
    for ($timestamp = $start->getTimestamp(); $timestamp <= $end->getTimestamp() + 1; $timestamp++) {
        $current = (new \DateTimeImmutable('@' . $timestamp))->setTimezone($start->getTimezone());
        $possible[] = $parts($current);
    }

    return $possible;
}

function parts_match(array $actual, array $expected): bool
{
    foreach ($expected as $key => $value) {
        $matched = \is_float($value)
            ? \abs($actual[$key] - $value) < 0.000001
            : $actual[$key] === $value;

        if (!$matched) {
            return false;
        }
    }

    return true;
}

function assert_any_parts(array $actual, array $expectedParts, string $label): void
{
    foreach ($expectedParts as $expected) {
        if (parts_match($actual, $expected)) {
            return;
        }
    }

    \var_dump($label, $actual, $expectedParts);
    die('failed');
}

assert_any_parts($monthOnly, possible_parts(
    $windowStart,
    $windowEnd,
    static fn (\DateTimeImmutable $current): array => [
        'year' => (int) $current->format('Y'),
        'month' => 5,
        'day' => (int) $current->format('d'),
        'hour' => (int) $current->format('H'),
        'minute' => (int) $current->format('i'),
    ]
), 'month');

assert_any_parts($timeOnly, possible_parts(
    $windowStart,
    $windowEnd,
    static fn (\DateTimeImmutable $current): array => [
        'year' => (int) $current->format('Y'),
        'month' => (int) $current->format('m'),
        'day' => (int) $current->format('d'),
        'hour' => 12,
        'minute' => 34,
        'second' => 0,
        'fraction' => 0.0,
    ]
), 'time');

assert_any_parts($reset, [[
    'year' => 1970,
    'month' => 5,
    'day' => 1,
    'hour' => 0,
    'minute' => 0,
    'second' => 0,
    'fraction' => 0.0,
]], 'reset');

assert_any_parts($absolute, [[
    'year' => 2024,
    'month' => 2,
    'day' => 27,
    'hour' => 9,
    'minute' => 52,
    'second' => 55,
    'fraction' => 0.123456,
]], 'absolute');

echo 'success', PHP_EOL;
?>
--EXPECT--
success
