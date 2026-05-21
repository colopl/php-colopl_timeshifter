--TEST--
Check PDO SQLite is not affected by the PDO MySQL hook
--EXTENSIONS--
colopl_timeshifter
pdo
pdo_sqlite
--SKIPIF--
<?php
if (stripos(PHP_BINARY, 'msan') !== false) {
    die('skip system libsqlite3 is not MemorySanitizer-instrumented');
}
?>
--INI--
colopl_timeshifter.is_hook_pdo_mysql=1
--FILE--
<?php declare(strict_types=1);

\Colopl\ColoplTimeShifter\register_hook(new \DateInterval('P3D'));

$pdo = new \PDO('sqlite::memory:');
$pdo->setAttribute(\PDO::ATTR_ERRMODE, \PDO::ERRMODE_EXCEPTION);
$pdo->exec('CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT)');
$pdo->exec("INSERT INTO t (name) VALUES ('ok')");

$value = $pdo->query('SELECT name FROM t')->fetchColumn();
$error = $pdo->errorInfo();

if ($value !== 'ok' || $error[0] !== '00000') {
    \var_dump($value, $error);
    die('failed');
}

echo 'success', PHP_EOL;
?>
--EXPECT--
success
