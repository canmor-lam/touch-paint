Name: timelink-touchwin-config
Version: 3.0.7
Release: 1%{?dist}
Group: Applications/System
Summary: The config tool for timelink-touchwin-daemon
Vendor: TimeLink, Inc.
License: GPLv2+
URL: http://www.timelink.cn
Source: %{name}-%{version}.tar.bz2
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
Requires: timelink-touchwin-daemon
Provides: timelink-touchwin-config

%description
The config tool for timelink-touchwin-daemon.

%prep
%setup -q

%build
make %{?_smp_mflags} TARGET=archive VERSION=%{version}

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot} TARGET=archive VERSION=%{version}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%doc
%{_bindir}/
%{_datadir}/

%changelog

