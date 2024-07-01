# colopl_timeshifter

This extension changes the current time in PHP to a specified modified value.

> [!WARNING]
> **DO NOT USE THIS EXTENSION IN ANY PRODUCTION ENVIRONMENT!!!**

At present, this extension is effective for the following functions:

- Any built-in PHP processing that handles the current time (`ext-date`)
- `NOW()` and many statements in MySQL or compatible DBMS via PDO
- Server environment variables for request time (e.g. `S_SERVER['REQUEST_TIME']`)

## Setup

```bash
$ git clone --recursive "https://github.com/colopl/php-colopl_timesifter.git" "colopl_timeshifter"
$ cd "colopl_timeshifter/ext"
$ phpize
$ ./configure --with-php-config="$(which php-config)"
$ make -j$(nproc)
$ TEST_PHP_ARGS="-q --show-diff" make test
$ sudo make install
```

And enable extension.

```
$ sudo echo "extension=colopl_timesfhiter" > "$(php-config --ini-dir)/99-colopl_timeshifter.ini"
$ php -m | grep colopl_timeshifter
colopl_timeshifter
```

### PHP Library (recommended)

```bash
$ composer require --dev "colopl/colopl_timeshifter"
```

And use `Colopl\ColoplTimeShifter\Manager` class.

## INI directives

#### `colopl_timeshifter.is_hook_pdo_mysql`

Type: `bool`
Default: `true`
Run-time switchable: **No** (`PHP_INI_SYSTEM`)

Enables or disables the hook into `\PDO::__construct` to swap the current time in MySQL function and keywords (e.g. `NOW()`, `CURRENT_TIMESTAMP`)

#### `colopl_timeshifter.is_hook_request_time`

Type: `bool`
Default: `true`
Run-time switchable: **No** (`PHP_INI_SYSTEM`)

Selects whether to hook the $_SERVER superglobals `REQUEST_TIME` and `REQUEST_TIME_FLOAT`.

#### `colopl_timeshifter.usleep_sec`

Type: `int` (`int<1, max>`)
Defalt: `1`
Run-time switchable: **Yes** (`PHP_INI_ALL`)

For a string representing time, set the number of wait microseconds to check whether it is absolute or relative time.

#### `colopl_timeshifter.is_restore_per_request`

Type: `bool`
Default: `false`
Run-time switchable: **Yes** (`PHP_INI_ALL`)

Sets whether or not to unhook at the end of the request.

## Functions

> [!TIP]
> Install `colopl/colopl_timeshifter` **Composer** package and use `Colopl\ColoplTimeShifter\Manager` support class instead.

#### `\Colopl\ColoplTimeShifter\register_hook(\DateInterval $interval): bool`

Sets the time difference to be subtracted from the current time.

If the hook succeeds, it returns `true`; otherwise, it returns `false`.

#### `\Colopl\ColoplTimeShifter\unregister_hook(): void`

Breaks the hook.

#### `\Colopl\ColoplTimeShifter\is_hooked(): bool`

Check to see if the hook is done. Returns `true` if the hook is done, `false` otherwise.

## License

PHP License 3.01
