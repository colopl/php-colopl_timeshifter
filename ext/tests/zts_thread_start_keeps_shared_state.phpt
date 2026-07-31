--TEST--
ZTS: Check starting a new thread does not reset the process wide hook state
--EXTENSIONS--
colopl_timeshifter
--SKIPIF--
<?php
if (!PHP_ZTS) {
    die('skip ZTS build required');
}

if (!extension_loaded('FFI') && !@dl('ffi.' . PHP_SHLIB_SUFFIX)) {
    die('skip FFI extension required to simulate a thread start');
}

ob_start();
phpinfo(INFO_GENERAL);
if (str_contains((string) ob_get_clean(), '--enable-memory-sanitizer')) {
    die('skip FFI links an uninstrumented libffi under MemorySanitizer');
}

try {
    FFI::cdef('void zm_globals_ctor_colopl_timeshifter(void *globals);');
} catch (Throwable $e) {
    die('skip cannot resolve zm_globals_ctor_colopl_timeshifter(): ' . $e->getMessage());
}
?>
--INI--
ffi.enable=1
date.timezone=UTC
colopl_timeshifter.is_restore_per_request=0
--FILE--
<?php declare(strict_types=1);

const GLOBALS_BUFFER_SIZE = 65536;
const SHIFT_DAY = 86400;
const TOLERANCE = 5;

if (!extension_loaded('FFI')) {
    dl('ffi.' . PHP_SHLIB_SUFFIX);
}

$ffi = FFI::cdef('void zm_globals_ctor_colopl_timeshifter(void *globals);');

function simulate_thread_start(FFI $ffi): void
{
    $globals = $ffi->new('char[' . GLOBALS_BUFFER_SIZE . ']');
    $ffi->zm_globals_ctor_colopl_timeshifter(FFI::addr($globals[0]));
}

function assert_shifted_by(int $expected, int $baseline, string $message): void
{
    $actual = $baseline - \time();
    if (\abs($actual - $expected) > TOLERANCE) {
        die($message . ': expected a shift of ' . $expected . ' seconds, got ' . $actual);
    }
}

$real = \time();

if (!\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P1D'))) {
    die('failed: register_hook()');
}

assert_shifted_by(SHIFT_DAY, $real, 'failed: hook did not shift the time');

simulate_thread_start($ffi);

if (!\Colopl\ColoplTimeShifter\is_hooked()) {
    die('failed: a new thread reset the hook state');
}

assert_shifted_by(SHIFT_DAY, $real, 'failed: a new thread reset the registered interval');

if (!\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P2D'))) {
    die('failed: register_hook() after a new thread started');
}

assert_shifted_by(2 * SHIFT_DAY, $real, 'failed: re-registering after a new thread started');

\Colopl\ColoplTimeShifter\unregister_hook();

if (\Colopl\ColoplTimeShifter\is_hooked()) {
    die('failed: unregister_hook() after a new thread started');
}

assert_shifted_by(0, $real, 'failed: unregister_hook() did not restore the time');

echo 'success', PHP_EOL;
?>
--EXPECT--
success

