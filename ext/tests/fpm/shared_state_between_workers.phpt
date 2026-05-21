--TEST--
Check FPM workers share hook state
--EXTENSIONS--
colopl_timeshifter
--SKIPIF--
<?php
if (DIRECTORY_SEPARATOR === '\\') {
    die('skip FPM is not supported on Windows');
}

function find_php_fpm(): ?string
{
    $phpBinaryName = basename(PHP_BINARY);
    $candidates = [
        getenv('TEST_PHP_FPM_EXECUTABLE') ?: null,
        getenv('PHP_FPM_BINARY') ?: null,
        dirname(PHP_BINARY) . DIRECTORY_SEPARATOR . $phpBinaryName . '-fpm',
        dirname(PHP_BINARY) . DIRECTORY_SEPARATOR . 'php-fpm',
        dirname(PHP_BINARY) . DIRECTORY_SEPARATOR . 'php-fpm' . PHP_MAJOR_VERSION . '.' . PHP_MINOR_VERSION,
        dirname(PHP_BINARY) . DIRECTORY_SEPARATOR . '..' . DIRECTORY_SEPARATOR . 'sbin' . DIRECTORY_SEPARATOR . $phpBinaryName . '-fpm',
        dirname(PHP_BINARY) . DIRECTORY_SEPARATOR . '..' . DIRECTORY_SEPARATOR . 'sbin' . DIRECTORY_SEPARATOR . 'php-fpm',
        '/usr/local/sbin/php-fpm',
        '/usr/local/bin/php-fpm',
        '/usr/sbin/php-fpm',
        '/usr/bin/php-fpm',
        'php-fpm',
        'php-fpm' . PHP_MAJOR_VERSION . '.' . PHP_MINOR_VERSION,
    ];

    foreach ($candidates as $candidate) {
        if ($candidate === null || $candidate === '') {
            continue;
        }

        if (str_contains($candidate, DIRECTORY_SEPARATOR)) {
            if (is_file($candidate) && is_executable($candidate) && php_fpm_matches_version($candidate)) {
                return $candidate;
            }
            continue;
        }

        $path = trim((string) shell_exec('command -v ' . escapeshellarg($candidate) . ' 2>/dev/null'));
        if ($path !== '' && is_executable($path) && php_fpm_matches_version($path)) {
            return $path;
        }
    }

    return null;
}

function php_fpm_matches_version(string $fpm): bool
{
    $output = (string) shell_exec(escapeshellarg($fpm) . ' -v 2>&1');
    if (!preg_match('/PHP\s+(\d+)\.(\d+)(?:\.|\s)/', $output, $matches)) {
        return false;
    }

    return (int) $matches[1] === PHP_MAJOR_VERSION && (int) $matches[2] === PHP_MINOR_VERSION;
}

if (find_php_fpm() === null) {
    die('skip matching php-fpm executable not found');
}
?>
--FILE--
<?php declare(strict_types=1);

const FCGI_VERSION_1 = 1;
const FCGI_BEGIN_REQUEST = 1;
const FCGI_END_REQUEST = 3;
const FCGI_PARAMS = 4;
const FCGI_STDIN = 5;
const FCGI_STDOUT = 6;
const FCGI_STDERR = 7;
const FCGI_RESPONDER = 1;

function find_php_fpm(): string
{
    $phpBinaryName = basename(PHP_BINARY);
    $candidates = [
        getenv('TEST_PHP_FPM_EXECUTABLE') ?: null,
        getenv('PHP_FPM_BINARY') ?: null,
        dirname(PHP_BINARY) . DIRECTORY_SEPARATOR . $phpBinaryName . '-fpm',
        dirname(PHP_BINARY) . DIRECTORY_SEPARATOR . 'php-fpm',
        dirname(PHP_BINARY) . DIRECTORY_SEPARATOR . 'php-fpm' . PHP_MAJOR_VERSION . '.' . PHP_MINOR_VERSION,
        dirname(PHP_BINARY) . DIRECTORY_SEPARATOR . '..' . DIRECTORY_SEPARATOR . 'sbin' . DIRECTORY_SEPARATOR . $phpBinaryName . '-fpm',
        dirname(PHP_BINARY) . DIRECTORY_SEPARATOR . '..' . DIRECTORY_SEPARATOR . 'sbin' . DIRECTORY_SEPARATOR . 'php-fpm',
        '/usr/local/sbin/php-fpm',
        '/usr/local/bin/php-fpm',
        '/usr/sbin/php-fpm',
        '/usr/bin/php-fpm',
        'php-fpm',
        'php-fpm' . PHP_MAJOR_VERSION . '.' . PHP_MINOR_VERSION,
    ];

    foreach ($candidates as $candidate) {
        if ($candidate === null || $candidate === '') {
            continue;
        }

        if (str_contains($candidate, DIRECTORY_SEPARATOR)) {
            if (is_file($candidate) && is_executable($candidate) && php_fpm_matches_version($candidate)) {
                return $candidate;
            }
            continue;
        }

        $path = trim((string) shell_exec('command -v ' . escapeshellarg($candidate) . ' 2>/dev/null'));
        if ($path !== '' && is_executable($path) && php_fpm_matches_version($path)) {
            return $path;
        }
    }

    throw new RuntimeException('matching php-fpm executable not found');
}

function php_fpm_matches_version(string $fpm): bool
{
    $output = (string) shell_exec(escapeshellarg($fpm) . ' -v 2>&1');
    if (!preg_match('/PHP\s+(\d+)\.(\d+)(?:\.|\s)/', $output, $matches)) {
        return false;
    }

    return (int) $matches[1] === PHP_MAJOR_VERSION && (int) $matches[2] === PHP_MINOR_VERSION;
}

function remove_tree(string $path): void
{
    if (!is_dir($path)) {
        return;
    }

    $entries = scandir($path);
    if ($entries === false) {
        return;
    }

    foreach ($entries as $entry) {
        if ($entry === '.' || $entry === '..') {
            continue;
        }

        $file = $path . DIRECTORY_SEPARATOR . $entry;
        if (is_dir($file) && !is_link($file)) {
            remove_tree($file);
        } else {
            @unlink($file);
        }
    }

    @rmdir($path);
}

function create_temp_dir(): string
{
    $base = sys_get_temp_dir() . DIRECTORY_SEPARATOR . 'cts_fpm_' . getmypid() . '_' . bin2hex(random_bytes(4));
    if (!mkdir($base, 0700)) {
        throw new RuntimeException('failed to create temp directory');
    }

    return $base;
}

function write_fpm_files(string $dir): array
{
    $socket = $dir . DIRECTORY_SEPARATOR . 'fpm.sock';
    $pidFile = $dir . DIRECTORY_SEPARATOR . 'fpm.pid';
    $errorLog = $dir . DIRECTORY_SEPARATOR . 'fpm-error.log';
    $extensionDir = ini_get('extension_dir');
    $extensionFile = $extensionDir . DIRECTORY_SEPARATOR . 'colopl_timeshifter.' . PHP_SHLIB_SUFFIX;
    if (!is_file($extensionFile)) {
        throw new RuntimeException('extension file not found: ' . $extensionFile);
    }

    $ini = $dir . DIRECTORY_SEPARATOR . 'php.ini';
    file_put_contents($ini, implode("\n", [
        'extension_dir="' . addcslashes($extensionDir, "\\\"") . '"',
        'extension=colopl_timeshifter.' . PHP_SHLIB_SUFFIX,
        'date.timezone=UTC',
        'colopl_timeshifter.is_restore_per_request=0',
        '',
    ]));

    $poolUser = '';
    if (function_exists('posix_geteuid') && posix_geteuid() === 0) {
        $poolUser = "user = root\ngroup = root\n";
    }

    $config = $dir . DIRECTORY_SEPARATOR . 'php-fpm.conf';
    file_put_contents($config, <<<CONF
[global]
pid = {$pidFile}
error_log = {$errorLog}
daemonize = no

[www]
listen = {$socket}
listen.mode = 0666
pm = static
pm.max_children = 2
pm.max_requests = 0
request_terminate_timeout = 20s
clear_env = no
catch_workers_output = yes
decorate_workers_output = no
{$poolUser}
CONF);

    $holdPid = $dir . DIRECTORY_SEPARATOR . 'hold.pid';
    $release = $dir . DIRECTORY_SEPARATOR . 'release';
    $script = $dir . DIRECTORY_SEPARATOR . 'index.php';
    file_put_contents($script, <<<'PHP'
<?php declare(strict_types=1);

date_default_timezone_set('UTC');
header('Content-Type: application/json');

$action = $_GET['action'] ?? '';
$holdPid = __DIR__ . DIRECTORY_SEPARATOR . 'hold.pid';
$release = __DIR__ . DIRECTORY_SEPARATOR . 'release';

if ($action === 'hold') {
    file_put_contents($holdPid, (string) getmypid());
    $deadline = microtime(true) + 10.0;
    while (!file_exists($release) && microtime(true) < $deadline) {
        usleep(10000);
    }

    echo json_encode([
        'action' => 'hold',
        'pid' => getmypid(),
        'hooked' => \Colopl\ColoplTimeShifter\is_hooked(),
        'time' => time(),
    ]);
    return;
}

if ($action === 'register') {
    $ok = \Colopl\ColoplTimeShifter\register_hook(new DateInterval('P1D'));
    echo json_encode([
        'action' => 'register',
        'pid' => getmypid(),
        'hooked' => \Colopl\ColoplTimeShifter\is_hooked(),
        'ok' => $ok,
        'time' => time(),
    ]);
    return;
}

http_response_code(400);
echo json_encode(['error' => 'unknown action', 'action' => $action]);
PHP);

    return [$config, $ini, $socket, $script, $holdPid, $release, $errorLog];
}

function start_fpm(string $fpm, string $config, string $ini, string $socket, string $stdout, string $stderr): mixed
{
    $command = 'PHP_INI_SCAN_DIR= ' . escapeshellarg($fpm) . ' -F -y ' . escapeshellarg($config) . ' -c ' . escapeshellarg($ini);
    if (function_exists('posix_geteuid') && posix_geteuid() === 0) {
        $command .= ' -R';
    }

    $proc = proc_open($command, [
        0 => ['pipe', 'r'],
        1 => ['file', $stdout, 'a'],
        2 => ['file', $stderr, 'a'],
    ], $pipes);
    if (!is_resource($proc)) {
        throw new RuntimeException('failed to start php-fpm');
    }

    fclose($pipes[0]);

    $deadline = microtime(true) + 10.0;
    while (microtime(true) < $deadline) {
        $status = proc_get_status($proc);
        if ($status !== false && $status['running'] === false) {
            throw new RuntimeException('php-fpm exited before creating socket');
        }

        if (is_socket_ready($socket)) {
            return $proc;
        }

        usleep(10000);
    }

    throw new RuntimeException('php-fpm did not create socket');
}

function is_socket_ready(string $socket): bool
{
    if (!file_exists($socket)) {
        return false;
    }

    $client = @stream_socket_client('unix://' . $socket, $errno, $errstr, 0.1);
    if ($client === false) {
        return false;
    }

    fclose($client);
    return true;
}

function stop_fpm(mixed $proc): void
{
    if (!is_resource($proc)) {
        return;
    }

    $status = proc_get_status($proc);
    if ($status !== false && $status['running']) {
        proc_terminate($proc);
        $deadline = microtime(true) + 5.0;
        do {
            usleep(10000);
            $status = proc_get_status($proc);
        } while ($status !== false && $status['running'] && microtime(true) < $deadline);
    }

    proc_close($proc);
}

function fcgi_record(int $type, int $requestId, string $content): string
{
    $length = strlen($content);
    $padding = (8 - ($length % 8)) % 8;

    return pack('CCnnCC', FCGI_VERSION_1, $type, $requestId, $length, $padding, 0)
        . $content
        . str_repeat("\0", $padding);
}

function fcgi_length(int $length): string
{
    if ($length < 128) {
        return chr($length);
    }

    return pack('N', $length | 0x80000000);
}

function fcgi_params(array $params): string
{
    $encoded = '';
    foreach ($params as $name => $value) {
        $name = (string) $name;
        $value = (string) $value;
        $encoded .= fcgi_length(strlen($name))
            . fcgi_length(strlen($value))
            . $name
            . $value;
    }

    return $encoded;
}

function fcgi_open_request(string $socket, string $script, string $query, int $requestId)
{
    $client = stream_socket_client('unix://' . $socket, $errno, $errstr, 2.0);
    if ($client === false) {
        throw new RuntimeException('failed to connect to php-fpm: ' . $errstr);
    }

    stream_set_timeout($client, 5);
    $params = [
        'GATEWAY_INTERFACE' => 'CGI/1.1',
        'SERVER_SOFTWARE' => 'colopl-timeshifter-test',
        'SERVER_PROTOCOL' => 'HTTP/1.1',
        'REQUEST_METHOD' => 'GET',
        'SCRIPT_FILENAME' => $script,
        'SCRIPT_NAME' => '/index.php',
        'REQUEST_URI' => '/index.php?' . $query,
        'QUERY_STRING' => $query,
        'DOCUMENT_ROOT' => dirname($script),
        'REMOTE_ADDR' => '127.0.0.1',
        'SERVER_ADDR' => '127.0.0.1',
        'SERVER_PORT' => '80',
        'CONTENT_LENGTH' => '0',
    ];

    $beginRequest = pack('nCx5', FCGI_RESPONDER, 0);
    fwrite($client, fcgi_record(FCGI_BEGIN_REQUEST, $requestId, $beginRequest));
    fwrite($client, fcgi_record(FCGI_PARAMS, $requestId, fcgi_params($params)));
    fwrite($client, fcgi_record(FCGI_PARAMS, $requestId, ''));
    fwrite($client, fcgi_record(FCGI_STDIN, $requestId, ''));

    return $client;
}

function read_exact($stream, int $length): string
{
    $data = '';
    while (strlen($data) < $length && !feof($stream)) {
        $chunk = fread($stream, $length - strlen($data));
        if ($chunk === false) {
            throw new RuntimeException('failed to read FastCGI response');
        }

        if ($chunk === '') {
            $meta = stream_get_meta_data($stream);
            if (!empty($meta['timed_out'])) {
                throw new RuntimeException('timed out while reading FastCGI response');
            }
            continue;
        }

        $data .= $chunk;
    }

    if (strlen($data) !== $length) {
        throw new RuntimeException('truncated FastCGI response');
    }

    return $data;
}

function fcgi_read_response($client): array
{
    $stdout = '';
    $stderr = '';

    while (!feof($client)) {
        $header = read_exact($client, 8);
        $record = unpack('Cversion/Ctype/nrequestId/ncontentLength/CpaddingLength/Creserved', $header);
        $content = $record['contentLength'] > 0 ? read_exact($client, $record['contentLength']) : '';
        if ($record['paddingLength'] > 0) {
            read_exact($client, $record['paddingLength']);
        }

        if ($record['type'] === FCGI_STDOUT) {
            $stdout .= $content;
        } elseif ($record['type'] === FCGI_STDERR) {
            $stderr .= $content;
        } elseif ($record['type'] === FCGI_END_REQUEST) {
            break;
        }
    }

    fclose($client);

    return [$stdout, $stderr];
}

function fcgi_body(string $stdout): string
{
    $parts = preg_split("/\r?\n\r?\n/", $stdout, 2);
    return $parts[1] ?? $stdout;
}

function fcgi_json_request(string $socket, string $script, string $query, int $requestId): array
{
    $client = fcgi_open_request($socket, $script, $query, $requestId);
    [$stdout, $stderr] = fcgi_read_response($client);
    if ($stderr !== '') {
        throw new RuntimeException('FastCGI stderr: ' . $stderr);
    }

    $decoded = json_decode(fcgi_body($stdout), true);
    if (!is_array($decoded)) {
        throw new RuntimeException('invalid JSON response: ' . $stdout);
    }

    return $decoded;
}

function wait_for_file(string $file): void
{
    $deadline = microtime(true) + 10.0;
    while (!file_exists($file) && microtime(true) < $deadline) {
        usleep(10000);
    }

    if (!file_exists($file)) {
        throw new RuntimeException('timed out waiting for ' . basename($file));
    }
}

function assert_true(bool $condition, string $message): void
{
    if (!$condition) {
        throw new RuntimeException($message);
    }
}

function assert_shifted_back_one_day(int $timestamp, int $realBefore, string $message): void
{
    if ($timestamp < $realBefore - 87000 || $timestamp > $realBefore - 86000) {
        throw new RuntimeException($message . ': got ' . $timestamp . ', real baseline ' . $realBefore);
    }
}

$dir = create_temp_dir();
$proc = null;
register_shutdown_function(static function () use (&$proc, $dir): void {
    if ($proc !== null) {
        stop_fpm($proc);
    }
    remove_tree($dir);
});

try {
    [$config, $ini, $socket, $script, $holdPid, $release, $errorLog] = write_fpm_files($dir);
    $proc = start_fpm(find_php_fpm(), $config, $ini, $socket, $dir . '/fpm.out', $dir . '/fpm.err');

    $realBefore = time();
    $holdClient = fcgi_open_request($socket, $script, 'action=hold', 1);
    wait_for_file($holdPid);

    $register = fcgi_json_request($socket, $script, 'action=register', 2);

    file_put_contents($release, '1');
    [$holdStdout, $holdStderr] = fcgi_read_response($holdClient);
    if ($holdStderr !== '') {
        throw new RuntimeException('FastCGI hold stderr: ' . $holdStderr);
    }

    $hold = json_decode(fcgi_body($holdStdout), true);
    if (!is_array($hold)) {
        throw new RuntimeException('invalid hold JSON response: ' . $holdStdout);
    }

    assert_true($register['action'] === 'register', 'register request did not run');
    assert_true($hold['action'] === 'hold', 'hold request did not run');
    assert_true($register['ok'] === true, 'register_hook() failed in second worker');
    assert_true($register['hooked'] === true, 'second worker is not hooked after register_hook()');
    assert_true($hold['hooked'] === true, 'first worker did not observe hook registered by second worker');
    assert_true($register['pid'] !== $hold['pid'], 'requests were not handled by different FPM workers');
    assert_shifted_back_one_day($register['time'], $realBefore, 'second worker time was not shifted back by about one day');
    assert_shifted_back_one_day($hold['time'], $realBefore, 'first worker time was not shifted back by about one day');

    echo "OK\n";
} catch (Throwable $e) {
    if (isset($errorLog) && is_file($errorLog)) {
        $log = trim((string) file_get_contents($errorLog));
        if ($log !== '') {
            echo "FPM log:\n", $log, "\n";
        }
    }
    echo $e::class, ': ', $e->getMessage(), "\n";
}
?>
--EXPECT--
OK
