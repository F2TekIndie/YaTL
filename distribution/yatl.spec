Name:           yatl
Version:        0.9.0
Release:        1%{?dist}
Summary:        Local project and todo manager
License:        MIT
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  libglvnd-devel
Requires:       qt6-qtbase
Requires:       qt6-qtdeclarative
Requires:       libnotify

%description
YaTL is a local-first project and todo manager with a Qt Quick desktop client,
CLI, SQLite storage, and optional niri and DankMaterialShell integrations.

%prep
%autosetup

%build
qmake6 YaTL.pro
%{make_build}

%install
install -Dpm0755 build/bin/yatl %{buildroot}%{_bindir}/yatl
install -Dpm0755 build/bin/yatlctl %{buildroot}%{_bindir}/yatlctl
install -Dpm0644 integrations/niri/org.yatl.YaTL.desktop %{buildroot}%{_datadir}/applications/org.yatl.YaTL.desktop
install -Dpm0644 integrations/niri/org.yatl.YaTL.QuickCapture.desktop %{buildroot}%{_datadir}/applications/org.yatl.YaTL.QuickCapture.desktop
mkdir -p %{buildroot}%{_datadir}/yatl/dms %{buildroot}%{_datadir}/yatl/niri
cp -a integrations/dms/YaTL %{buildroot}%{_datadir}/yatl/dms/
install -Dpm0644 integrations/niri/yatl.kdl %{buildroot}%{_datadir}/yatl/niri/yatl.kdl
install -Dpm0644 integrations/niri/README.md %{buildroot}%{_datadir}/yatl/niri/README.md

%files
%{_bindir}/yatl
%{_bindir}/yatlctl
%{_datadir}/applications/org.yatl.YaTL.desktop
%{_datadir}/applications/org.yatl.YaTL.QuickCapture.desktop
%{_datadir}/yatl/

%changelog
* Sun Sep 13 2026 YaTL maintainers - 0.9.0-1
- Add Fedora packaging metadata for the Qt 6 desktop client and CLI.
