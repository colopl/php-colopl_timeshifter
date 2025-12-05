--TEST--
Check PDO MySQL (preparer, TiDB)
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

$pdo = (static function () {
    $attempt = 0;
    $delay = 100000;
    while (true) {
        try {
            return new \PDO('mysql:dbname=mysql;host=tidb;port=4000', 'root', '');
        } catch (\PDOException $exception) {
            if (++$attempt >= 5) {
                throw $exception;
            }
        }
        usleep($delay);
        $delay *= 2;
    }
})();
$pdo->exec('CREATE DATABASE IF NOT EXISTS testing;');
$pdo->exec('USE testing;');

/* NOW(), CURRENT_TIMESTAMP, CURRENT_TIMESTAMP(), UTC_TIMESTAMP() */
foreach ($pdo->query('
    SELECT NOW() AS "now",
    CURRENT_TIMESTAMP AS "current_timestamp",
    CURRENT_TIMESTAMP() AS "current_timestamp_fun",
    UTC_TIMESTAMP() AS "utc_timestamp";
', \PDO::FETCH_ASSOC)->fetch() as $result) {
    $after = new \DateTimeImmutable($result);
    
    if ($after->getTimestamp() >= $before->getTimestamp()) {
        die('failure');
    }
    
    $interval = $after->diff($before);
    if ($after < $before && 4 > $interval->days && $interval->days >= 2 && $interval->invert === 0) {
    } else {
        die('failure');
    }
}

/* UNIX_TIMESTAMP() */
$after = new \DateTimeImmutable('@' . $pdo->query('SELECT UNIX_TIMESTAMP() AS "unix_timestamp";')->fetch()[0]);
$interval = $after->diff($before);
if ($after < $before && 4 > $interval->days && $interval->days >= 2 && $interval->invert === 0) {
} else {
    die('failure');
}

die('success');

?>
--EXPECT--
success
