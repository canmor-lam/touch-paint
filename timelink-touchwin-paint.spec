Name: timelink-touchwin-paint
Version: 
Release: 1%{?dist}
Group: Applications/System
Summary: The test tool for timelink-touchwin
Vendor: TimeLink, Inc.
License: GPLv2+
URL: http://www.timelink.cn
Source: %{name}-%{version}.tar.bz2
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%description
The test tool for timelink-touchwin.

%prep
%setup -q

%build
cmake -G'Unix Makefiles' -DCMAKE_INSTALL_PREFIX=/usr/
make %{?_smp_mflags} VERSION=%{version}

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot} VERSION=%{version}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%doc
%{_bindir}/

%changelog

