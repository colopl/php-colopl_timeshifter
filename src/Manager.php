<?php

declare(strict_types=1);

namespace Colopl\ColoplTimeShifter;

final class Manager
{
    private static ?bool $isAvailable = \null;

    /**
     * Check availability of TimeShifter.
     */
    public static function isAvailable(): bool
    {
        if (self::$isAvailable === \null) {
            self::checkAvailable();
        }

        \assert(is_bool(self::$isAvailable));

        return self::$isAvailable;
    }

    /**
     * Check TimeShifter is hooked.
     */
    public static function isHooked(): bool
    {
        return self::isAvailable() && \Colopl\ColoplTimeShifter\is_hooked();
    }

    /**
     * Unset hook.
     */
    public static function unhook(): bool
    {
        if (! self::isAvailable()) {
            return \false;
        }

        \Colopl\ColoplTimeShifter\unregister_hook();

        return \true;
    }

    /**
     * Set modification time to the current time.
     */
    public static function hookDateTime(\DateTimeInterface $dateTime): bool
    {
        if (! self::isAvailable()) {
            return \false;
        }

        return self::hookDateInterval($dateTime->diff(new \DateTimeImmutable()));
    }

    /**
     * Set interval to the current time.
     */
    public static function hookDateInterval(\DateInterval $dateInterval): bool
    {
        if (! self::isAvailable()) {
            return \false;
        }

        /*
         * Calculate in days to ensure correct conversion of days
         * when specifying a period that straddles months.
         */
        $actualInterval = clone $dateInterval;
        if (is_int($actualInterval->days) && $actualInterval->days > 0 && $actualInterval->days !== $actualInterval->d) {
            $actualInterval->d = $actualInterval->days;
            $actualInterval->y = 0;
            $actualInterval->m = 0;
        }

        return \Colopl\ColoplTimeShifter\register_hook($actualInterval);
    }

    private static function checkAvailable(): void
    {
        self::$isAvailable = \extension_loaded('colopl_timeshifter');
    }
}
