<?php

declare(strict_types=1);

namespace Colopl\ColoplTimeShifter\Tests;

use Colopl\ColoplTimeShifter\Manager;
use PHPUnit\Framework\TestCase;

final class ManagerTest extends TestCase
{
    public function testIsAvailable(): void
    {
        self::assertTrue(Manager::isAvailable());
    }

    public function testIsHooked(): void
    {
        self::assertFalse(Manager::isHooked());
        Manager::hookDateTime(new \DateTimeImmutable('1994-10-26 12:00:00'));
        self::assertTrue(Manager::isHooked());

        self::assertEquals('1994-10-26', (new \DateTimeImmutable())->format('Y-m-d'));
    }

    public function testUnhook(): void
    {
        $now = new \DateTimeImmutable();

        Manager::hookDateTime(new \DateTimeImmutable('1994-10-26 12:00:00'));

        self::assertEquals('1994-10-26', (new \DateTimeImmutable())->format('Y-m-d'));

        Manager::unhook();

        self::assertEquals($now->format('Y-m-d'), (new \DateTimeImmutable())->format('Y-m-d'));
    }

    public function testHookInterval(): void
    {
        $now = new \DateTimeImmutable();
        self::assertTrue(Manager::hookDateInterval(new \DateInterval('P1D')));
        self::assertEquals($now->sub(new \DateInterval('P1D'))->format('Y-m-d'), (new \DateTimeImmutable())->format('Y-m-d'));
    }

    public function testHookDateTime(): void
    {
        $now = new \DateTimeImmutable();
        $diff = (new \DateTimeImmutable('1994-10-26 12:00:00'))->diff($now);

        self::assertTrue(Manager::hookDateTime(new \DateTimeImmutable('1994-10-26 12:00:00')));

        self::assertEquals($diff->format('%d'), (new \DateTimeImmutable())->diff($now)->format('%d'));
    }

    protected function tearDown(): void
    {
        Manager::unhook();
        \Colopl\ColoplTimeShifter\unregister_hook();
    }
}
