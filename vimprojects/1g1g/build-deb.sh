VERSION='0.9-2'
ARCH='i386'
make
mv linux1g1g debian/usr/bin/
cp bindkeys.sh debian/usr/bin/
sed -e 's/Version: .*/Version: '${VERSION}'/' -e 's/Architecture: .*/Architecture: '${ARCH}'/' < debian/control >debian/DEBIAN/control
dpkg-deb --build debian
mv debian.deb linux1g1g-${VERSION}-${ARCH}.deb
