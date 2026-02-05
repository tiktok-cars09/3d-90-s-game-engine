Fedora packaging and install instructions

Quick local install (recommended for testing)

1. Install dependencies:

   sudo dnf install -y gcc make SDL2-devel

2. Build and install (system-wide):

   sudo ./install_fedora.sh

This will install `/usr/bin/game90`, the desktop shortcut at `/usr/share/applications/game90.desktop`, and the icon at `/usr/share/icons/hicolor/scalable/apps/game90.svg`.

Building an RPM with `rpmbuild`

1. Create a source tarball from the repo root:

   tar czf game90-0.1.tar.gz --transform 's,^,game90-0.1/,' README.md Makefile src assets game90.desktop

2. Prepare rpm build tree and copy files:

   mkdir -p ~/rpmbuild/SOURCES ~/rpmbuild/SPECS
   mv game90-0.1.tar.gz ~/rpmbuild/SOURCES/
   cp game90.spec ~/rpmbuild/SPECS/

3. Build the RPM:

   sudo dnf builddep -y game90.spec
   rpmbuild -ba ~/rpmbuild/SPECS/game90.spec

Resulting RPMs will be in `~/rpmbuild/RPMS/`.

Notes

- The spec expects a `LICENSE` file; if none is present the `%license` line in the spec can be removed or a license file added.
- The simple installer script uses `/usr/bin` and system-wide locations; run it with `sudo` for convenience when testing.
