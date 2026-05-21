--TEST--
Check createFromFormat missing fields use shifted current time
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

date_default_timezone_set('UTC');

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P365D'));

$windowStart = new \DateTimeImmutable();

$monthOnly = \DateTimeImmutable::createFromFormat('m', '5');
$timeOnly = \DateTimeImmutable::createFromFormat('H:i', '12:34');
$reset = \DateTimeImmutable::createFromFormat('!m', '5');
$absolute = \DateTimeImmutable::createFromFormat('Y-m-d H:i:s.u', '2024-02-27 09:52:55.123456');

$windowEnd = new \DateTimeImmutable();

function possible_formats(\DateTimeImmutable $start, \DateTimeImmutable $end, callable $format): array
{
    $formats = [];
    for ($timestamp = $start->getTimestamp(); $timestamp <= $end->getTimestamp() + 1; $timestamp++) {
        $current = (new \DateTimeImmutable('@' . $timestamp))->setTimezone($start->getTimezone());
        $formats[] = $format($current);
    }

    return \array_values(\array_unique($formats));
}

if (!$monthOnly || !$timeOnly || !$reset || !$absolute) {
    die('failed: parse');
}

if (!\in_array($monthOnly->format('Y-m-d H:i'), possible_formats(
    $windowStart,
    $windowEnd,
    static fn (\DateTimeImmutable $current): string => $current->format('Y-') . '05' . $current->format('-d H:i')
), true)) {
    die('failed: month');
}

if (!\in_array($timeOnly->format('Y-m-d H:i:s.u'), possible_formats(
    $windowStart,
    $windowEnd,
    static fn (\DateTimeImmutable $current): string => $current->format('Y-m-d ') . '12:34:00.000000'
), true)) {
    die('failed: time');
}

if ($reset->format('Y-m-d H:i:s.u') !== '1970-05-01 00:00:00.000000') {
    die('failed: reset');
}

if ($absolute->format('Y-m-d H:i:s.u') !== '2024-02-27 09:52:55.123456') {
    die('failed: absolute');
}

echo 'success', PHP_EOL;
?>
--EXPECT--
success
