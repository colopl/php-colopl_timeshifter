#!/bin/sh -eu

cd "/project"
  cd "ext"
    phpize
    ./configure
    make -j"$(nproc)"
    TEST_PHP_ARGS="--show-diff -q" make test
    make install
    docker-php-ext-enable "colopl_timeshifter"
  cd -
  composer install
  composer exec -- phpunit "tests"
  composer exec -- phpstan
cd -
