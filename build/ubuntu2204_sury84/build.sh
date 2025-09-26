#!/bin/sh -eux

cd "/tmp/ext"
  echo "COLOPL PHP timeshifter extension for sury PHP 8.4" > "description-pak"
  phpize8.4
  ./configure --with-php-config="$(which "php-config8.4")"
  make -j"$(nproc)"
  checkinstall \
        --pkgname="php8.4-colopl-timeshifter" \
        --pkglicense="PHP-3.01" \
        --pkgversion="${VERSION}" \
        --pkggroup="php8.4" \
        --maintainer="g-kudo@colopl.co.jp" \
        --requires="php8.4-common" \
        --stripso="yes" \
        --pakdir="/tmp/artifacts" \
        --nodoc
cd -
