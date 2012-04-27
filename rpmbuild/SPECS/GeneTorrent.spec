Name:           GeneTorrent
Version:        3.1.0
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

# This prevents binaries from being stripped of their debug bits by rpmbuild
%define __spec_install_post /usr/lib/rpm/brp-compress

%define debug_package %{nil}

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
* Sat Jan  6 2012 donavan nelson <dnelson at cardinalpeak dot com> 1.0.x.y-1
- Prevent binaires from being stripped

