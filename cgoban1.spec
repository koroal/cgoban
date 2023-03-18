%define name cgoban
%define version 1.9.14
%define release 0.3
%define realname cgoban1
# This is for debug-flavor. Do not remove. Package is stripped conditionally.
# %define __os_install_post       %{nil}
# %{expand:%%define optflags %{optflags} %([ $DEBUG ] && echo '-g3')}

Summary:	cgoban is an X board for playing go
Name:		%{name}
Version:	%{version}
Release:	%{release}
Group:		Graphical desktop/Other
License:	GPL
URL:		http://cgoban1.sourceforge.net/
Obsoletes:  %{name}-1.9.13

BuildRequires:	XFree86-devel
BuildRoot:	%{_tmppath}/%{name}-buildroot
Prefix:		%{_prefix}

Source:		http://prdownloads.sourceforge.net/cgoban1/%{name}-%{version}.tar.gz

%description
   Cgoban (Complete Goban) is for Unix systems with X11.  It has the ability
to be a computerized go board, view and edit smart-go files, and connect to
go servers on the Internet.

%prep
rm -rf $RPM_BUILD_ROOT
%setup -q

%configure

%build
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir $RPM_BUILD_ROOT
chmod 777 $RPM_BUILD_ROOT
make install prefix=$RPM_BUILD_ROOT/usr mandir=$RPM_BUILD_ROOT/usr/share/man bindir=$RPM_BUILD_ROOT/usr/bin 
mv $RPM_BUILD_ROOT/usr/bin/%{name} $RPM_BUILD_ROOT/usr/bin/%{realname}
# mkdir $RPM_BUILD_ROOT/usr/share
mkdir $RPM_BUILD_ROOT/usr/share/pixmaps
cp cgoban_icon.png $RPM_BUILD_ROOT/usr/share/pixmaps
mkdir $RPM_BUILD_ROOT/etc
mkdir $RPM_BUILD_ROOT/etc/X11
mkdir $RPM_BUILD_ROOT/etc/X11/applnk
mkdir $RPM_BUILD_ROOT/etc/X11/applnk/Games
cat >$RPM_BUILD_ROOT/etc/X11/applnk/Games/%{realname}.desktop <<EOF
[Desktop Entry]
Name=%{name} %{version}
Comment=cgoban v1 Board viewer and gnugo interface
Exec=%{_prefix}/bin/%{name}
Icon=cgoban_icon.png
Terminal=0
Type=Application
EOF
mv $RPM_BUILD_ROOT/usr/share/man/man6/%{name}.6 $RPM_BUILD_ROOT/usr/share/man/man6/%{realname}.6

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,755)
%doc COPYING README TODO
/usr/bin
/usr/share/man/man6/*
/usr/share/pixmaps/cgoban_icon.png
/etc/X11/applnk/Games/%{realname}.desktop

%changelog
* Thu Oct 25 2002 Kevin Sonney <ksonney@redhat.com> 1.9.14-0.2
- latest dev build
- tested rpm spec on RHL 7.x

* Thu Oct 25 2002 Kevin Sonney <ksonney@redhat.com> 1.9.14-0.1
- version jump
- remove patches
- modify .spec to be included in mainline cgoban1 distribution

* Mon Oct 22 2002 Kevin Sonney <ksonney@redhat.com> 1.9.12-0.4
- change desktop/menu entry description

* Mon Oct 21 2002 Kevin Sonney <ksonney@redhat.com> 1.9.12-0.3
- rename binary to cgoban1
- added desktop entry for gnome

* Thu Aug 22 2002 Kevin Sonney <ksonney@redhat.com> 1.9.12-0.2
- Better patch to Makefile.in
- still needs porting to the new autoconf
- can now use %configure macro
- clean out fluxbox comments form my template .spec file
- bad hacks for man pages & doc lcoations

* Thu Aug 22 2002 Kevin Sonney <ksonney@redhat.com> 1.9.12-0.1
- Initial Spec File
- lots of fixes to the spec to get it to build right as an RPM
- patched the Makefile.in to correct autoconf error
- needs porting to the new autoconf

# end of file
