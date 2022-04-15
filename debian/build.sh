#!/bin/bash

deb_workspace=$(cd $(dirname $0);pwd)
echo $deb_workspace

sourcedir=${deb_workspace}"/../"
builddir=${deb_workspace}"/.deb_create/BUILD/"
buildrootdir=${deb_workspace}"/.deb_create/BUILDROOT/"
debdir=${deb_workspace}"/.deb_create/DEBS/"

rm -fr .deb_create
mkdir -p $builddir
mkdir -p $buildrootdir
mkdir -p $debdir

# prep
install -d                        $builddir/sresar/
install -d                        $builddir/ssar/
install -d                        $builddir/conf/
install -v -D $sourcedir/sresar/* $builddir/sresar/
install -v -D $sourcedir/ssar/*   $builddir/ssar/
install -v -D $sourcedir/conf/*   $builddir/conf/
install -v    $sourcedir/Makefile $builddir/Makefile
install -d                        $builddir/debian/
install -v -D $sourcedir/debian/* $builddir/debian/

# make 
make -C $builddir

# install
install -d                             ${buildrootdir}/etc/ssar/
install $builddir/conf/ssar.conf       ${buildrootdir}/etc/ssar/
install $builddir/conf/sys.conf        ${buildrootdir}/etc/ssar/
install -d                             ${buildrootdir}/usr/share/man/man1/
install $builddir/conf/ssar.1.gz       ${buildrootdir}/usr/share/man/man1/
install -d                             ${buildrootdir}/usr/src/os_health/ssar/
install $builddir/conf/sresar.service  ${buildrootdir}/usr/src/os_health/ssar/
install $builddir/conf/sresar.cron     ${buildrootdir}/usr/src/os_health/ssar/
install $builddir/conf/sresard         ${buildrootdir}/usr/src/os_health/ssar/
install -d                             ${buildrootdir}/usr/local/lib/os_health/ssar/
install $builddir/conf/healing.sh      ${buildrootdir}/usr/local/lib/os_health/ssar/healing.sh
install -d                             ${buildrootdir}/usr/local/bin/
install $builddir/ssar/ssar            ${buildrootdir}/usr/local/bin/ssar
install $builddir/ssar/ssar+.py        ${buildrootdir}/usr/local/bin/ssar+
install $builddir/ssar/tsar2.py        ${buildrootdir}/usr/local/bin/tsar2
install $builddir/sresar/sresar        ${buildrootdir}/usr/local/bin/sresar
install -d                             ${buildrootdir}/run/lock/os_health/
touch                                  ${buildrootdir}/run/lock/os_health/sresar.pid

# install plus
install -d                             ${buildrootdir}/DEBIAN/
install $builddir/debian/control       ${buildrootdir}/DEBIAN/
install $builddir/debian/postinst      ${buildrootdir}/DEBIAN/
install $builddir/debian/prerm         ${buildrootdir}/DEBIAN/

# build
package_name=$(grep -F Package ${buildrootdir}/DEBIAN/control | head -n 1 | awk -F: '{print $2}')
version_name=$(grep -F Version ${buildrootdir}/DEBIAN/control | head -n 1 | awk -F: '{print $2}')
architecture_name=$(grep -F Architecture ${buildrootdir}/DEBIAN/control | head -n 1 | awk -F: '{print $2}')

package_name=$(echo $package_name)
version_name=$(echo $version_name)
architecture_name=$(echo $architecture_name)

dpkg-deb --build ${buildrootdir} ${debdir}/${package_name}"_"${version_name}"_"${architecture_name}".deb"

# bakup
cp ${debdir}/${package_name}"_"${version_name}"_"${architecture_name}".deb" ${deb_workspace}/
