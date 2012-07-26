Name:           GeneTorrent
Version:        3.1.1
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
Requires(pre): /usr/sbin/useradd, /usr/bin/getent

# This prevents binaries from being stripped of their debug bits by rpmbuild
%define __spec_install_post /usr/lib/rpm/brp-compress

%define debug_package %{nil}

%description
GeneTorrent

%package Server
Obsoletes: GeneTorrent
Conflicts: GeneTorrent-Client
Summary: GeneTorrent Server process (and related files).
%description Server
GeneTorrent Server.  This package contains GeneTorrent Server components.

%package Client
Obsoletes: GeneTorrent
Conflicts: GeneTorrent-Server
Summary: GeneTorrent Client process (and related files).
%description Client
GeneTorrent Client.  This package contains GeneTorrent Download and Upload client components.

%pre Server
/usr/bin/getent group gtorrent >/dev/null || /usr/sbin/groupadd -r gtorrent >/dev/null 
/usr/bin/getent passwd gtorrent >/dev/null || /usr/sbin/useradd -r -g gtorrent -d /etc/gnos.d/ -s /bin/nologin gtorrent >/dev/null

%post Server
/sbin/chkconfig --add GeneTorrent
/sbin/chkconfig --add GTLoadBalancer

%preun Server
/sbin/service GeneTorrent stop > /dev/null
/sbin/service GTLoadBalancer stop > /dev/null
/sbin/chkconfig --del GeneTorrent
/sbin/chkconfig --del GTLoadBalancer

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

%files Client
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_bindir}/gtoinfo
%{_bindir}/gtocheck
%{_mandir}/man1/%{name}*
%{_datadir}/GeneTorrent/*
/usr/lib/python2.6/site-packages/*

%files Server
%{_bindir}/%{name}
%{_bindir}/gtoinfo
%{_bindir}/gtocheck
%{_bindir}/GTLoadBalancer
%{_mandir}/man1/%{name}*
%{_datadir}/GeneTorrent/*
/usr/lib/python2.6/site-packages/*
%config(noreplace) %{_initddir}/*
%config(noreplace) %{_sysconfdir}/gnos.d/GeneTorrent.conf
%config(noreplace) %{_sysconfdir}/gnos.d/GTLoadBalancer.conf

%files
%defattr(-,root,root,-)
%{nil}

%changelog
* Sat Jan  6 2012 donavan nelson <dnelson at cardinalpeak dot com> 1.0.x.y-1
- Prevent binaires from being stripped

