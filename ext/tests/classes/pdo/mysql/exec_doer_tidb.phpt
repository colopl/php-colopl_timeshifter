--TEST--
Check PDO MySQL (doer, TiDB)
--EXTENSIONS--
colopl_timeshifter
pdo
pdo_mysql
mysqlnd
--FILE--
<?php declare(strict_types=1);

$shift_interval = new \DateInterval('P3D');

$before = new \DateTimeImmutable();

\Colopl\ColoplTimeShifter\register_hook($shift_interval);

$pdo = new \PDO('mysql:dbname=mysql;host=tidb;port=4000', 'root', '');

$pdo->exec('CREATE DATABASE IF NOT EXISTS testing;');
$pdo->exec('USE testing;');
$pdo->exec('DROP TABLE IF EXISTS testing;');
$pdo->exec('CREATE TABLE IF NOT EXISTS testing (
    id INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
    date DATETIME NOT NULL
);');
$pdo->exec('INSERT INTO testing (date) VALUES (NOW());');

$after = new \DateTimeImmutable(
    $pdo->query('SELECT date FROM testing ORDER BY id DESC LIMIT 1;')->fetch(\PDO::FETCH_NUM)[0]
);

$interval = $after->diff($before);

if ($after < $before && 4 > $interval->days && $interval->days >= 2 && $interval->invert === 0) {
    die('success');
}

die('failed');
?>
--EXPECT--
success
