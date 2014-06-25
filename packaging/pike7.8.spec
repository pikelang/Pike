# Notes:
#
#  This package contains the  major.minor version of Pike, in order
#  to permit the parallel installation of multiple releases at the
#  same time (for example, 7.6 and 7.8.) Documentation, including
#  the module reference may be found in  /usr/share/doc/pike-VERSION.
# 
#  the most recent stable version of pike will be available
#  as /usr/bin/pike, in addition to /usr/bin/pikeMAJOR.MINOR.
#
#  this behavior is managed by alternatives(8).
#
#  This file is based on pike.spec from repoforge, and has been fairly 
#  extensively modified. The original repoforge content appears to be
#  new BSD licensed, though this is not spelled out in individual files
#  or in any adjoining material. It would be good to clarify this.

%{?fc4:%define _without_modxorg 1}
%{?el4:%define _without_modxorg 1}
%{?fc3:%define _without_modxorg 1}
%{?fc2:%define _without_modxorg 1}
%{?fc1:%define _without_modxorg 1}
%{?el3:%define _without_modxorg 1}
%{?rh9:%define _without_modxorg 1}
%{?rh7:%define _without_modxorg 1}
%{?el2:%define _without_modxorg 1}
%{?rh6:%define _without_modxorg 1}
%{?yd3:%define _without_modxorg 1}

%{?fc1:%define _without_xorg 1}
%{?el3:%define _without_xorg 1}
%{?rh9:%define _without_xorg 1}
%{?rh8:%define _without_xorg 1}
%{?rh7:%define _without_xorg 1}
%{?el2:%define _without_xorg 1}
%{?rh6:%define _without_xorg 1}
%{?yd3:%define _without_xorg 1}

%define real_name Pike
%define real_version v7.8.866
%define relnum 7.8
%define myname pike
%define shared_docs %{_docdir}/%{myname}-%{version}

Summary: General purpose programming language
Name: pike%{relnum}
Version: 7.8.866
Release: 1.0%{?dist}
License: GPL/LGPL/MPL
Group: Development/Languages
URL: http://pike.lysator.liu.se/

Source: http://pike.lysator.liu.se/download/pub/pike/all/%{version}/Pike-v%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires: nettle-devel, gmp-devel, autoconf
BuildRequires: gdbm-devel, gettext, zlib-devel, nasm, fftw-devel
BuildRequires: mysql-devel
BuildRequires: sane-backends-devel, ffmpeg-devel
BuildRequires: freetype-devel, libjpeg-devel, libtiff-devel
BuildRequires: pcre-devel, bzip2-devel, freeglut-devel, gtk2-devel, libgnomeui-devel
BuildRequires: SDL-devel, pkgconfig, gtkglarea2-devel
BuildRequires: SDL_mixer-devel, librsvg2-devel
BuildRequires: chrpath

%if 0%{?_without_modxorg:1}
%{?_without_xorg:BuildRequires: XFree86-devel, XFree86-Mesa-libGLU}
%{!?_without_xorg:BuildRequires: xorg-x11-devel, xorg-x11-Mesa-libGLU}
%else
BuildRequires: libXt-devel, mesa-libGLU-devel
%endif

%description
Pike is a general purpose programming language, which means that you can put
it to use for almost any task. Its application domain spans anything from
the world of the Net to the world of multimedia applications, or
environments where your shell could use some spicy text processing or system
administration tools.

%package devel
Summary: Header files, libraries and development documentation for %{myname}.
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
This package contains the header files, static libraries and development
documentation for %{myname}. If you like to develop programs using %{myname},
you will need to install %{name}-devel.

%package odbc
Summary: ODBC driver module for %{myname}.
Group: Development/Languages
Requires: %{name} = %{version}-%{release}
Requires: unixODBC

%description odbc
This package contains support for ODBC database access for %{myname}.

%package freetype
Summary: Freetype module for %{myname}.
Group: Development/Languages
Requires: %{name} = %{version}-%{release}
Requires: freetype

%description freetype
This package contains support for Freetype 2 (TTF) rendering in %{myname}.

%package ffmpeg
Summary: Ffmpeg module for %{myname}.
Group: Development/Languages
Requires: %{name} = %{version}-%{release}
Requires: ffmpeg

%description ffmpeg
This package contains support for Ffmpeg in %{myname}.

%package svg
Summary: SVG Image module for %{myname}.
Group: Development/Languages
Requires: %{name} = %{version}-%{release}
Requires: librsvg2

%description svg
This package contains support for SVG rendering in %{myname}.

%package mysql
Summary: mysql driver module for %{myname}.
Group: Development/Languages
Requires: %{name} = %{version}-%{release}
Requires: mysql

%description mysql
This package contains support for mysql database access for %{myname}.

%package sdl
Summary: SDL (Simple Direct Media Layer) module for %{myname}.
Group: Development/Languages
Requires: %{name} = %{version}-%{release}
Requires: SDL SDL_mixer

%description sdl
This package contains support for using SDL in %{myname}.

%package sane
Summary: SANE (Scanner Access Now Easy) module for %{myname}.
Group: Development/Languages
Requires: %{name} = %{version}-%{release}
Requires: sane-backends

%description sane
This package contains support for using SANE scanners in %{myname}.

%package gl
Summary: OpenGL module for %{myname}.
Group: Development/Languages
Requires: %{name} = %{version}-%{release}
Requires: mesa-libGLU freeglut

%description gl
This package contains support for using OpenGL in %{myname}.

%package gtk2
Summary: GTK2 module for %{myname}.
Group: Development/Languages
Requires: %{name} = %{version}-%{release}
Requires: gtk2, gtkglarea2, libgnomeui

%description gtk2
This package contains support for using GTK2 in %{myname}.

%prep
%setup -n %{real_name}-%{real_version}

%build
STARTPWD=`pwd`
cd src
./run_autoconfig
mkdir ../build; cd ../build
${STARTPWD}/src/configure \
  --prefix=/usr 
%{__make}
%{__make} documentation
%install buildroot="%{buildroot}"
%{__rm} -rf %{buildroot}
cd build
%{__make} buildroot="%{buildroot}" install INSTALLARGS="buildroot='%{buildroot}' lib_prefix='/usr/lib/%{name}' include_prefix='/usr/include/%{name}' --traditional"

chrpath --delete %{buildroot}/usr/lib/%{name}/modules/Mysql.so
chrpath --delete %{buildroot}/usr/lib/%{name}/modules/Postgres.so
chrpath --delete %{buildroot}/usr/lib/%{name}/modules/Odbc.so

%{__mkdir_p} %{buildroot}%{_bindir}
%{__mkdir_p} %{shared_docs}
%{__rm} -rf %{buildroot}/usr/doc
%{__mv} %{buildroot}/usr/bin/pike %{buildroot}%{_bindir}/%{name}
%{__mv} %{buildroot}/usr/bin/pike.syms %{buildroot}%{_bindir}/%{name}.syms
%{__mv} %{buildroot}/usr/bin/rsif %{buildroot}%{_bindir}/rsif%{relnum}
%{__install} -d -m0755 %{buildroot}%{_mandir}/man1/

# we should make the documentation directory a macro, really.
%{__mkdir_p} %{buildroot}%{_docdir}/%{myname}-%{version}
cd ..
%{__cp} ANNOUNCE CHANGES COMMITTERS COPYING COPYRIGHT README README-CVS %{buildroot}%{shared_docs}
%{__mv} refdoc/modref %{buildroot}%{shared_docs}
%{__mv} refdoc/traditional_manual %{buildroot}%{shared_docs}

cd build
./pike "-DPRECOMPILED_SEARCH_MORE" "-m./master.pike" -x rsif -r "/usr/local/bin/pike" "%{_bindir}/%{name}" %{buildroot} 

%post
/sbin/ldconfig 2>/dev/null
alternatives --install %{_bindir}/%{myname} %{myname} %{_bindir}/%{name} 1

%postun
/sbin/ldconfig 2>/dev/null

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-, root, root, 0755)
%doc %{_mandir}/man?/*
%{shared_docs}/*
%exclude %{_libdir}/%{name}/modules/Odbc.so
%exclude %{_libdir}/%{name}/modules/SDL.so
%exclude %{_libdir}/%{name}/modules/SANE.so
%exclude %{_libdir}/%{name}/modules/Mysql.so
%exclude %{_libdir}/%{name}/modules/___GTK2.so
%exclude %{_libdir}/%{name}/modules/GL.so
%exclude %{_libdir}/%{name}/modules/GLUT.so
%exclude %{_libdir}/%{name}/modules/_Image_SVG.so
%exclude %{_libdir}/%{name}/modules/_Image_FreeType.so
%exclude %{_libdir}/%{name}/modules/_Ffmpeg.so

%{_bindir}/*
%{_libdir}/%{name}

%files devel
%defattr(-, root, root, 0755)
%{_includedir}/%{name}

%files odbc
%defattr(-, root, root, 0755)
%{_libdir}/%{name}/modules/Odbc.so

%files sdl
%defattr(-, root, root, 0755)
%{_libdir}/%{name}/modules/SDL.so

%files sane
%defattr(-, root, root, 0755)
%{_libdir}/%{name}/modules/SANE.so

%files gtk2
%defattr(-, root, root, 0755)
%{_libdir}/%{name}/modules/___GTK2.so

%files svg
%defattr(-, root, root, 0755)
%{_libdir}/%{name}/modules/_Image_SVG.so

%files ffmpeg
%defattr(-, root, root, 0755)
%{_libdir}/%{name}/modules/_Ffmpeg.so

%files mysql
%defattr(-, root, root, 0755)
%{_libdir}/%{name}/modules/Mysql.so

%files freetype
%defattr(-, root, root, 0755)
%{_libdir}/%{name}/modules/_Image_FreeType.so

%files gl
%defattr(-, root, root, 0755)
%{_libdir}/%{name}/modules/GL.so
%{_libdir}/%{name}/modules/GLUT.so

%changelog
* Wed Jun 25 2014 Bill Welliver <bill@welliver.org> - 7.8.866-1.0
* Wed Nov 7 2012 Bill Welliver <bill@welliver.org> - 7.8.700-1.0
- New stable pike release
- This RPM definition is based on pike.spec from repoforge. Please
  see repoforge for previous history.
