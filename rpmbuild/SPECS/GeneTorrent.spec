Name:           GeneTorrent
Version:        0.9.0.4
Release:        1%{?dist}.CP
Summary:        GeneTorrent

Group:          Development/Libraries
License:        Unknown
URL:            http://annaisystems.com/
Source0:        GeneTorrent-%{version}.tgz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

#BuildRequires:  libtorrent-cp-devel >= 0.15.7.7
#Requires:  xqilla tclap libcurl log4cpp
#BuildRequires:  xqilla-devel tclap libcurl-devel log4cpp-devel
#BuildRequires:  boost >= 1.48.0
#BuildRequires:  boost-regex >= 1.48.0

%description
GeneTorrent

%prep
%setup -q

%build
make %{?_smp_mflags}

%install
rm -rf %{buildroot}

mkdir -p %{buildroot}/usr/bin
mkdir -p %{buildroot}/usr/share/man/man1

make install DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_bindir}/gtoinfo
%{_bindir}/GTLoadBalancer
%{_mandir}/man1/%{name}*
%{_datadir}/GeneTorrent/*

%changelog
* Mon Sep 26 2011 donavan nelson <dnelson at cardinalpeak dot com> 0.4.2-1
- bump version to 0.4.2, new deployment system

* Fri Sep 21 2011 donavan nelson <dnelson at cardinalpeak dot com> 0.4.1-1
- bump version to 0.4.1
- fixed path issue is server mode
- bit more debug to UUID fetching
- misc cleanup here and there

* Fri Sep 21 2011 donavan nelson <dnelson at cardinalpeak dot com> 0.4.0-1
- bump version to 0.4.0
- primitive UUID downloader added

* Tue Sep 20 2011 donavan nelson <dnelson at cardinalpeak dot com> 0.3.1-1
- bump version to 0.3.1
- onscreen debug code removed
- server mode partial implemented
- new version of libtorrent-cp

* Tue Sep 20 2011 donavan nelson <dnelson at cardinalpeak dot com> 0.3.0-1
- bump version to 0.3.0
- numerous new features

* Wed Sep 07 2011 donavan nelson <dnelson at cardinalpeak dot com> 0.2.4-1
- Initial creation
