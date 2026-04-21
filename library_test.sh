#!/bin/sh -eu

export USE_ZEND_ALLOC=1
export USE_TRACKED_ALLOC=0
export ZEND_DONT_UNLOAD_MODULES=1

cd "/project"
  cd "ext"
    phpize
    ./configure --with-php-config="$(which php-config)"
    make clean
    make -j"$(nproc)"
    TEST_PHP_ARGS="--show-diff -q" make test
    make install
    docker-php-ext-enable "colopl_timeshifter"
  cd -
  composer install
  composer exec -- phpunit "tests"
  composer exec -- phpstan --memory-limit=-1
cd -
