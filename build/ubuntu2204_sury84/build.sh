#!/bin/sh -eux

: "${VERSION:?VERSION is required}"

SOURCE_DIR="/tmp/src"
PACKAGING_DIR="/tmp/build/ubuntu2204_sury84/debian"
ARTIFACTS_DIR="/tmp/artifacts"
UBUNTU_VERSION="22.04"
PHP_VARIANT="sury"

rm -rf "${SOURCE_DIR}/debian"
install -d "${SOURCE_DIR}/debian" "${ARTIFACTS_DIR}"
find "${ARTIFACTS_DIR}" -mindepth 1 -maxdepth 1 -delete

cp -a "${PACKAGING_DIR}/." "${SOURCE_DIR}/debian/"
sed "s/@VERSION@/${VERSION}/g" "${PACKAGING_DIR}/changelog.in" > "${SOURCE_DIR}/debian/changelog"
rm -f "${SOURCE_DIR}/debian/changelog.in"

cd "${SOURCE_DIR}"
dpkg-buildpackage -us -uc -B

for DEB in /tmp/*.deb /tmp/*.ddeb; do
  test -e "${DEB}" || continue
  DEB_BASENAME="$(basename "${DEB}" .deb)"
  DEB_BASENAME="$(basename "${DEB_BASENAME}" .ddeb)"
  DEB_EXT="${DEB##*.}"
  cp "${DEB}" "${ARTIFACTS_DIR}/${DEB_BASENAME}_ubuntu${UBUNTU_VERSION}_${PHP_VARIANT}.${DEB_EXT}"
done

for ARTIFACT in /tmp/*.buildinfo /tmp/*.changes; do
  test -e "${ARTIFACT}" || continue
  ARTIFACT_BASENAME="$(basename "${ARTIFACT}")"
  ARTIFACT_STEM="${ARTIFACT_BASENAME%.*}"
  ARTIFACT_EXT="${ARTIFACT_BASENAME##*.}"
  cp "${ARTIFACT}" "${ARTIFACTS_DIR}/${ARTIFACT_STEM}_ubuntu${UBUNTU_VERSION}_${PHP_VARIANT}.${ARTIFACT_EXT}"
done
