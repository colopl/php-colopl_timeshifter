# colopl_timeshifter

This extension changes the current time in PHP to a specified modified value.

> [!WARNING]
> **DO NOT USE THIS EXTENSION IN ANY PRODUCTION ENVIRONMENT!!!**

At present, this extension is effective for the following functions:

- Any built-in PHP processing that handles the current time (`ext-date`)
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

## Build Ubuntu 22.04 packages

Ubuntu 22.04 packages are built with standard Debian packaging via `dpkg-buildpackage`, not `checkinstall`.
The packaging definitions live under `build/ubuntu2204/debian` and `build/ubuntu2204_sury84/debian`.

Build packages for the official Ubuntu 22.04 PHP 8.1 stack:

```bash
$ docker build -f "build/ubuntu2204/Dockerfile" -t "colopl-timeshifter-u2204-php81" .
$ mkdir -p "artifacts"
$ docker run --rm -e VERSION="x.y.z" -v "$(pwd)/artifacts:/tmp/artifacts" "colopl-timeshifter-u2204-php81"
```

This target produces `php-colopl-timeshifter` and `php8.1-colopl-timeshifter` together with the corresponding `.changes` and `.buildinfo` files.

Build packages for Ubuntu 22.04 with the Ondrej Sury PHP 8.4 repository:

```bash
$ docker build -f "build/ubuntu2204_sury84/Dockerfile" -t "colopl-timeshifter-u2204-php84" .
$ mkdir -p "artifacts"
$ docker run --rm -e VERSION="x.y.z" -v "$(pwd)/artifacts:/tmp/artifacts" "colopl-timeshifter-u2204-php84"
```

This target produces `php8.4-colopl-timeshifter` together with the corresponding `.changes` and `.buildinfo` files.

## PHP Library

The Composer package is the recommended interface from application code.

```bash
$ composer require --dev "colopl/colopl_timeshifter"
```

Use `Colopl\ColoplTimeShifter\Manager` as a support class.

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

#### `colopl_timeshifter.usleep_sec`

Type: `int` (`int<1, max>`)
Default: `1`
Run-time switchable: **Yes** (`PHP_INI_ALL`)

For a string representing time, set the number of wait microseconds used to check whether it is absolute or relative time.

#### `colopl_timeshifter.is_restore_per_request`

Type: `bool`
Default: `false`
Run-time switchable: **Yes** (`PHP_INI_ALL`)

Sets whether or not to unhook at the end of the request.

## Functions

> [!TIP]
> Install `colopl/colopl_timeshifter` via Composer and use `Colopl\ColoplTimeShifter\Manager` instead when possible.

#### `\Colopl\ColoplTimeShifter\register_hook(\DateInterval $interval): bool`

Sets the time difference to be subtracted from the current time.

If the hook succeeds, it returns `true`; otherwise, it returns `false`.

#### `\Colopl\ColoplTimeShifter\unregister_hook(): void`

Breaks the hook.

#### `\Colopl\ColoplTimeShifter\is_hooked(): bool`

Checks whether the hook is active. Returns `true` if the hook is active, `false` otherwise.

## License

BSD-3-Clause

This repository also vendors timelib under [ext/third_party/timelib](ext/third_party/timelib).
timelib remains available under the MIT License, and
[ext/third_party/timelib/parse_posix.c](ext/third_party/timelib/parse_posix.c)
includes an additional note for code adapted from IANA tzcode that is marked as
public domain. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for the
bundled third-party notice summary.
