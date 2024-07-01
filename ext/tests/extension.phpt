--TEST--
Check if colopl_timeshifter is loaded
--FILE--
<?php declare(strict_types=1);

if (!\extension_loaded('colopl_timeshifter')) {
    die('failure extension');
}

if (\Colopl\ColoplTimeShifter\is_hooked() === \true) {
    die('failure state');
}

$before = new \DateTime('now');

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P3D'));

if (\Colopl\ColoplTimeShifter\is_hooked() === \false) {
    die('failure state');
}

$hooked = new \DateTime('now');

\Colopl\ColoplTimeShifter\unregister_hook();

if (\Colopl\ColoplTimeShifter\is_hooked() === \true) {
    die('failure state');
}

$after = new \DateTime('now');

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P2D'));

if (\Colopl\ColoplTimeShifter\is_hooked() === \false) {
    die('failure state');
}

$seconde_hooked = new \DateTime('now');

if ($before >= $after || $hooked >= $before || $seconde_hooked >= $before) {
    die('failure behavior');
}

die('success');
?>
--EXPECT--
success
