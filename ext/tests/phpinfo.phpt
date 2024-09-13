--TEST--
Check phpinfo()
--EXTENSIONS--
colopl_timeshifter
--FILE--
<?php
ob_start(
    static fn (string $phpinfo): string
      => str_contains($phpinfo, 'colopl_timeshifter support')
        ? ('Success: ' . \phpversion('colopl_timeshifter'))
        : 'Failure'
);
phpinfo();
ob_end_flush();
?>
--EXPECTF--
Success: %d.%d.%d%s
