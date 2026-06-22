<?php

/**
 * @generate-class-entries
 * @generate-legacy-arginfo 80100
 */

namespace Colopl\ColoplTimeShifter;

function register_hook(\DateInterval $interval): bool {}
function unregister_hook(): void {}
function is_hooked(): bool {}

final class Manager
{
    public static function isAvailable(): bool {}
    public static function isHooked(): bool {}
    public static function unhook(): bool {}
    public static function hookDateTime(\DateTimeInterface $dateTime): bool {}
    public static function hookDateInterval(\DateInterval $dateInterval): bool {}
}
