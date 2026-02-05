pkgname=game90
pkgver=0.1
pkgrel=1
pkgdesc="Minimal 90s style raycast shooter"
arch=('x86_64')
url=""
license=('custom')
depends=('sdl2')
source=("src/main.c" "Makefile" "README.md" "game90.desktop" "assets/icon.svg")

build() {
  make
}

package() {
  install -Dm755 game90 "$pkgdir/usr/bin/game90"
  install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
  install -Dm644 game90.desktop "$pkgdir/usr/share/applications/game90.desktop"
  install -Dm644 assets/icon.svg "$pkgdir/usr/share/icons/hicolor/scalable/apps/game90.svg"
}
