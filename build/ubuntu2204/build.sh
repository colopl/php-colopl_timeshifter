#!/bin/sh -eux

cd "/tmp/ext"
  echo "COLOPL PHP timeshifter extension" > "description-pak"
  phpize
  ./configure --with-php-config="$(which "php-config")"
  make -j$(nproc)
  checkinstall \
        --pkgname="php-colopl-timeshifter" \
        --pkglicense="PHP-3.01" \
        --pkgversion="${VERSION}" \
        --pkggroup="php" \
        --maintainer="g-kudo@colopl.co.jp" \
        --requires="php" \
        --stripso="yes" \
        --pakdir="/tmp/artifacts" \
        --nodoc
cd -
