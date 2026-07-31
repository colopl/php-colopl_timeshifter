<?php

function colopl_ts_load_ffi(): bool
{
    if (extension_loaded('FFI')) {
        return true;
    }

    $module = 'ffi.' . PHP_SHLIB_SUFFIX;

    if (@dl($module)) {
        return true;
    }

    $source = PHP_EXTENSION_DIR . DIRECTORY_SEPARATOR . $module;
    $target = ini_get('extension_dir') . DIRECTORY_SEPARATOR . $module;

    if (!is_readable($source)) {
        return false;
    }

    if (!file_exists($target) && !@copy($source, $target)) {
        return false;
    }

    return (bool) @dl($module);
}
