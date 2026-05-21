--TEST--
Check createFromFormat escaped/reset/microsecond edge cases
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

date_default_timezone_set('UTC');

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P365D'));

$windowStart = new \DateTimeImmutable();

$escaped = \DateTimeImmutable::createFromFormat('\Y-m-d H:i', 'Y-05-06 07:08');
$pipeReset = \DateTimeImmutable::createFromFormat('m|', '5');
$microsecondsOnly = \DateTimeImmutable::createFromFormat('u', '123456');

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

if (!$escaped || !$pipeReset || !$microsecondsOnly) {
    die('failed: parse');
}

if (!\in_array($escaped->format('Y-m-d H:i:s.u'), possible_formats(
    $windowStart,
    $windowEnd,
    static fn (\DateTimeImmutable $current): string => $current->format('Y-') . '05-06 07:08:00.000000'
), true)) {
    \var_dump($escaped->format('Y-m-d H:i:s.u'));
    die('failed: escaped');
}

if ($pipeReset->format('Y-m-d H:i:s.u') !== '1970-05-01 00:00:00.000000') {
    \var_dump($pipeReset->format('Y-m-d H:i:s.u'));
    die('failed: pipe');
}

if (!\in_array($microsecondsOnly->format('Y-m-d H:i:s.u'), possible_formats(
    $windowStart,
    $windowEnd,
    static fn (\DateTimeImmutable $current): string => $current->format('Y-m-d ') . '00:00:00.123456'
), true)) {
    \var_dump($microsecondsOnly->format('Y-m-d H:i:s.u'));
    die('failed: microseconds');
}

echo 'success', PHP_EOL;
?>
--EXPECT--
success
