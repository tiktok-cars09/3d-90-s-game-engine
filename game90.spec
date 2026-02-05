Name:           game90
Version:        0.1
Release:        1%{?dist}
Summary:        Minimal 90s style raycast shooter

License:        custom
URL:            
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc, make, SDL2-devel
Requires:       SDL2

%description
A minimal 90s-style raycast shooter using SDL2.

%prep
%setup -q

%build
%{__make}

%install
rm -rf %{buildroot}
%{__make} DESTDIR=%{buildroot} install || true
install -Dm755 game90 %{buildroot}/usr/bin/game90
if [ -f map_editor ]; then
  install -Dm755 map_editor %{buildroot}/usr/bin/map_editor
fi
install -Dm644 README.md %{buildroot}/usr/share/doc/%{name}/README.md
install -Dm644 game90.desktop %{buildroot}/usr/share/applications/game90.desktop
install -Dm644 assets/icon.svg %{buildroot}/usr/share/icons/hicolor/scalable/apps/game90.svg

%files
%license LICENSE
%doc README.md
/usr/bin/game90
/usr/bin/map_editor
/usr/share/applications/game90.desktop
/usr/share/icons/hicolor/scalable/apps/game90.svg

%changelog
* Thu Feb 04 2026 Packager <packager@example.com> - 0.1-1
- Initial package
