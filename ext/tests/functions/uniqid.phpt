--TEST--
Check uniqid() is based on shifted current time
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

function uniqid_timestamp(string $id, string $prefix = ''): int
{
    if (!\str_starts_with($id, $prefix)) {
        \var_dump($id, $prefix);
        die('failed: prefix');
    }

    return (int) \hexdec(\substr($id, \strlen($prefix), 8));
}

function assert_shifted(int $after, int $before, string $label): void
{
    $afterDate = new \DateTimeImmutable("@{$after}");
    $beforeDate = new \DateTimeImmutable("@{$before}");
    $interval = $afterDate->diff($beforeDate);

    if (!($afterDate < $beforeDate && 4 > $interval->days && $interval->days >= 2 && $interval->invert === 0)) {
        \var_dump($label, $afterDate, $beforeDate, $interval);
        die('failed');
    }
}

$before = \time();

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P3D'));

$plain = \uniqid();
$prefixed = \uniqid('php_', true);

if (!\preg_match('/^[0-9a-f]{13}$/', $plain)) {
    \var_dump($plain);
    die('failed: plain format');
}

if (!\preg_match('/^php_[0-9a-f]{13,14}\.[0-9]{8}$/', $prefixed)) {
    \var_dump($prefixed);
    die('failed: prefixed format');
}

assert_shifted(uniqid_timestamp($plain), $before, 'plain');
assert_shifted(uniqid_timestamp($prefixed, 'php_'), $before, 'prefixed');

echo 'success', PHP_EOL;
?>
--EXPECT--
success
