# colopl_timeshifter

This extension changes the current time in PHP to a specified modified value.

> [!WARNING]
> **DO NOT USE THIS EXTENSION IN ANY PRODUCTION ENVIRONMENT!!!**

At present, this extension is effective for the following functions:

- Any built-in PHP processing that handles the current time (`ext-date`, `ext-calendar`)
- `NOW()` and many statements in MySQL or compatible DBMS via PDO
- Server environment variables for request time (e.g. `$_SERVER['REQUEST_TIME']`)

## Install

Clone the repository with submodules and build the extension.

```bash
$ git clone --recursive "https://github.com/colopl/php-colopl_timeshifter.git" "colopl_timeshifter"
$ cd "colopl_timeshifter/ext"
$ phpize
$ ./configure --with-php-config="$(which php-config)"
$ make -j"$(nproc)"
$ TEST_PHP_ARGS="--show-diff -q" make test
$ sudo make install
```

Enable the extension after installation.

```bash
$ echo "extension=colopl_timeshifter" | sudo tee "$(php-config --ini-dir)/99-colopl_timeshifter.ini"
$ php -m | grep "colopl_timeshifter"
colopl_timeshifter
```

## Build Ubuntu packages

Ubuntu packages are built with standard Debian packaging via `dpkg-buildpackage`, not `checkinstall`.
The packaging definitions live alongside each build target under `build/*/debian`.

Build packages for the official Ubuntu 22.04 PHP 8.1 stack:

```bash
$ docker build -f "build/ubuntu2204/Dockerfile" -t "colopl-timeshifter-u2204-php81" .
$ mkdir -p "artifacts"
$ docker run --rm -e VERSION="x.y.z" -v "$(pwd)/artifacts:/tmp/artifacts" "colopl-timeshifter-u2204-php81"
```

This target produces `php8.1-colopl-timeshifter_x.y.z_<arch>_ubuntu22.04_default.deb`.

Build packages for Ubuntu 22.04 with the Ondrej Sury PHP 8.4 repository:

```bash
$ docker build -f "build/ubuntu2204_sury84/Dockerfile" -t "colopl-timeshifter-u2204-sury84-php84" .
$ mkdir -p "artifacts"
$ docker run --rm -e VERSION="x.y.z" -v "$(pwd)/artifacts:/tmp/artifacts" "colopl-timeshifter-u2204-sury84-php84"
```

This target produces `php8.4-colopl-timeshifter_x.y.z_<arch>_ubuntu22.04_sury.deb`.

Build packages for the official Ubuntu 26.04 PHP 8.5 stack:

```bash
$ docker build -f "build/ubuntu2604/Dockerfile" -t "colopl-timeshifter-u2604-php85" .
$ mkdir -p "artifacts"
$ docker run --rm -e VERSION="x.y.z" -v "$(pwd)/artifacts:/tmp/artifacts" "colopl-timeshifter-u2604-php85"
```

This target produces `php8.5-colopl-timeshifter_x.y.z_<arch>_ubuntu26.04_default.deb`.

Generated `.ddeb`, `.changes`, and `.buildinfo` artifacts use the same Ubuntu version and PHP variant suffix when present.

## Migration Guide

### From Composer Package to Packagist PHP Extension Installer

As of version 2.0, this package is distributed as a PHP extension via the Packagist PHP Extension installer (PIE), not as a Composer library package. The former PHP wrapper interface (`Colopl\ColoplTimeShifter\Manager`) is now implemented by the extension itself.

#### For users previously using `Manager` class

**Before (version < 2.0):**
```php
<?php
require 'vendor/autoload.php';

use Colopl\ColoplTimeShifter\Manager;

// Shift the current time to 2021-01-01 00:00:00 UTC.
Manager::hookDateTime(new DateTimeImmutable('2021-01-01 00:00:00 UTC'));

echo date('Y-m-d H:i:s');
echo time();

Manager::unhook();
```

**After (version >= 2.0):**

1. Install the extension via the package manager (Ubuntu example):
   ```bash
   apt-get install php-colopl-timeshifter
   ```
   Or via Packagist PHP Extension installer:
   ```bash
   composer require --dev-only colopl/colopl_timeshifter --with-php-extensions
   ```

2. Verify the extension is loaded:
   ```bash
   php -m | grep colopl_timeshifter
   ```

3. Use the extension-provided `Manager` class from your test code:
   ```php
   <?php
   use Colopl\ColoplTimeShifter\Manager;

   // Shift the current time to 2021-01-01 00:00:00 UTC.
   Manager::hookDateTime(new DateTimeImmutable('2021-01-01 00:00:00 UTC'));

   echo date('Y-m-d H:i:s');
   echo time();

   Manager::unhook();
   ```

#### Key differences:

- **No Composer autoloaded wrapper**: The `Colopl\ColoplTimeShifter\Manager` class is provided by the loaded extension.
- **Compatibility API**: Existing calls to `Manager::hookDateTime()`, `Manager::hookDateInterval()`, `Manager::unhook()`, `Manager::isHooked()`, and `Manager::isAvailable()` continue to work after the extension is installed.
- **Direct functions remain available**: You can still call `register_hook($interval)`, `unregister_hook()`, and `is_hooked()` directly when you want the lower-level API.
- **Global hook state**: Once registered, time shifting applies to hooked time-related functions until `unregister_hook()` is called or the request ends with `colopl_timeshifter.is_restore_per_request=1`.

## INI directives

#### `colopl_timeshifter.is_hook_pdo_mysql`

Type: `bool`
Default: `true`
Run-time switchable: **No** (`PHP_INI_SYSTEM`)

Enables or disables the hook into `\PDO::__construct` to swap the current time in MySQL function and keywords such as `NOW()` and `CURRENT_TIMESTAMP`.

#### `colopl_timeshifter.is_hook_request_time`

Type: `bool`
Default: `true`
Run-time switchable: **No** (`PHP_INI_SYSTEM`)

Selects whether to hook the `$_SERVER` superglobals `REQUEST_TIME` and `REQUEST_TIME_FLOAT`.

#### `colopl_timeshifter.is_restore_per_request`

Type: `bool`
Default: `false`
Run-time switchable: **Yes** (`PHP_INI_ALL`)

Sets whether or not to unhook at the end of the request.

## Classes

> [!TIP]
> The extension provides both the compatibility `Manager` class and the lower-level functions below.

#### `\Colopl\ColoplTimeShifter\Manager::isAvailable(): bool`

Checks whether TimeShifter is available. When this class is provided by the extension, it returns `true`.

#### `\Colopl\ColoplTimeShifter\Manager::isHooked(): bool`

Checks whether the hook is active.

#### `\Colopl\ColoplTimeShifter\Manager::unhook(): bool`

Breaks the hook and returns `true`.

#### `\Colopl\ColoplTimeShifter\Manager::hookDateTime(\DateTimeInterface $dateTime): bool`

Sets the current time to the specified date and time.

#### `\Colopl\ColoplTimeShifter\Manager::hookDateInterval(\DateInterval $dateInterval): bool`

Sets the time difference to be subtracted from the current time.

## Functions

#### `\Colopl\ColoplTimeShifter\register_hook(\DateInterval $interval): bool`

Sets the time difference to be subtracted from the current time.

If the hook succeeds, it returns `true`; otherwise, it returns `false`.

#### `\Colopl\ColoplTimeShifter\unregister_hook(): void`

Breaks the hook.

#### `\Colopl\ColoplTimeShifter\is_hooked(): bool`

Checks whether the hook is active. Returns `true` if the hook is active, `false` otherwise.

## License

BSD-3-Clause
