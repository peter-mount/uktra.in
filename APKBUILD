# Maintainer: Peter Mount <peter@retep.org>

pkgname="uktrain"
pkgver="0.1"
pkgrel="2"
pkgdesc="uktra.in website"
arch="x86_64"
url="https://area51.onl/"
license="ASL 2.0"
source=""
subpackages="$pkgname-dev"
depends="libarea51 libarea51-template libarea51-rest libarea51-httpd json-c libmicrohttpd curl tzdata"
depends_dev="libarea51-dev libarea51-template-dev libarea51-rest-dev libarea51-httpd-dev json-c-dev libmicrohttpd-dev curl-dev"
#triggers="$pkgname-bin.trigger=/lib:/usr/lib:/usr/glibc-compat/lib"

builddeps() {
  sudo apk add $depends $depends_dev
}

package() {
  autoconf
  ./configure
  make clean
  make -j1
  mkdir -pv "$pkgdir/usr/share/uktrain"
  cp -rpv web/* "$pkgdir/usr/share/uktrain"
  mkdir -pv "$pkgdir/usr/bin"
  cp -rp build/package/usr/bin/* "$pkgdir/usr/bin"
  mkdir -pv "$pkgdir/usr/lib"
  cp -rp build/package/usr/lib/* "$pkgdir/usr/lib"
}

dev() {
  depends="$pkgname $depends_dev"
  mkdir -pv "$subpkgdir/usr/include"
  cp -rp build/package/usr/include/* "$subpkgdir/usr/include"
}
