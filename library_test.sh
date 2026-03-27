#!/bin/sh -eu

cd "/project"
  cd "ext"
    phpize
    ./configure
    make -j"$(nproc)"
    TEST_PHP_ARGS="--show-diff -q" make test
    make install
  cd -
  composer install
  docker-php-ext-enable "colopl_timeshifter"
  composer exec -- phpunit "tests"
  composer exec -- phpstan
cd -
