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
  docker-php-ext-install "opcache"
  echo "opcache.enable_cli=1"        >> "$(php-config --ini-dir)/opcache.ini"
  echo "opcache.jit=tracing"         >> "$(php-config --ini-dir)/opcache.ini"
  echo "opcache.jit_buffer_size=64M" >  "$(php-config --ini-dir)/opcache.ini"
  composer install
  composer exec -- phpunit "tests"
  composer exec -- phpstan
  composer exec -- psalm
cd -
