--TEST--
Check failed registrations do not corrupt the active shared state
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php declare(strict_types=1);

date_default_timezone_set('UTC');

#[\AllowDynamicProperties]
class LargeDateInterval extends \DateInterval
{
}

if (!\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P1D'))) {
    die('failed: initial hook');
}

$shiftedBefore = \time();

$large = new LargeDateInterval('P2D');
$large->payload = \str_repeat('x', 4096);

if (\Colopl\ColoplTimeShifter\register_hook($large) !== false) {
    die('failed: subclass interval accepted');
}

$shiftedAfterLarge = \time();
if (!$shiftedAfterLarge || \abs($shiftedAfterLarge - $shiftedBefore) > 1 || !\Colopl\ColoplTimeShifter\is_hooked()) {
    \var_dump($shiftedBefore, $shiftedAfterLarge, \Colopl\ColoplTimeShifter\is_hooked());
    die('failed: large interval changed state');
}

$dynamic = new \DateInterval('P2D');
@$dynamic->payload = 'x';

if (\Colopl\ColoplTimeShifter\register_hook($dynamic) !== false) {
    die('failed: dynamic property interval accepted');
}

$shiftedAfterDynamic = \time();
if (!$shiftedAfterDynamic || \abs($shiftedAfterDynamic - $shiftedBefore) > 1 || !\Colopl\ColoplTimeShifter\is_hooked()) {
    \var_dump($shiftedBefore, $shiftedAfterDynamic, \Colopl\ColoplTimeShifter\is_hooked());
    die('failed: dynamic property interval changed state');
}

if (\Colopl\ColoplTimeShifter\register_hook(\DateInterval::createFromDateString('next weekday')) !== false) {
    die('failed: special relative interval accepted');
}

$shiftedAfterRelative = \time();
if (\abs($shiftedAfterRelative - $shiftedBefore) > 1 || !\Colopl\ColoplTimeShifter\is_hooked()) {
    \var_dump($shiftedBefore, $shiftedAfterRelative, \Colopl\ColoplTimeShifter\is_hooked());
    die('failed: relative interval changed state');
}

\Colopl\ColoplTimeShifter\unregister_hook();

$unshifted = \time();
if (\Colopl\ColoplTimeShifter\is_hooked() || $unshifted - $shiftedAfterRelative < 80000) {
    \var_dump($shiftedAfterRelative, $unshifted, \Colopl\ColoplTimeShifter\is_hooked());
    die('failed: unregister');
}

echo 'success', PHP_EOL;
?>
--EXPECT--
success
