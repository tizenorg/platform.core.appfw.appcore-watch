Name:       appcore-watch
Summary:    Watch Application
Version:    1.2.3.1
Release:    3
Group:      Application Framework/Libraries
License:    Apache-2.0
Source0:    appcore-watch-%{version}.tar.gz
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(dbus-glib-1)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(vconf-internal-keys)
BuildRequires:  pkgconfig(alarm-service)
BuildRequires:  pkgconfig(capi-appfw-app-control)
BuildRequires:  pkgconfig(capi-appfw-app-common)
BuildRequires:	pkgconfig(capi-appfw-widget-application)
BuildRequires:	pkgconfig(libtzplatform-config)
BuildRequires:	pkgconfig(icu-uc)
BuildRequires:  cmake


%description
Watch application


%package devel
Summary:    appcore watch
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
%description devel
appcore watch (developement files)

%package -n capi-appfw-watch-application-devel
Summary:    watch appliation
Group:      Development/Libraries
Requires:    appcore-watch-devel = %{version}-%{release}
%description -n capi-appfw-watch-application-devel
watch application (developement files)

%prep
%setup -q


%build
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif
export CFLAGS="$CFLAGS -Wall -Werror -Wno-unused-function -Wno-unused-but-set-variable"
CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" cmake . -DCMAKE_INSTALL_PREFIX=/usr

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}/usr/lib/pkgconfig
cp capi-appfw-watch-application.pc %{buildroot}/usr/lib/pkgconfig
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest appcore-watch.manifest
%defattr(-,root,root,-)
/usr/lib/libappcore-watch.so
/usr/lib/libappcore-watch.so.1
/usr/lib/libappcore-watch.so.1.1
/usr/share/license/%{name}

%files devel
%defattr(-,root,root,-)
/usr/lib/pkgconfig/appcore-watch.pc
/usr/include/appcore-watch/appcore-watch.h

%files -n capi-appfw-watch-application-devel
/usr/include/appcore-watch/watch_app.h
/usr/include/appcore-watch/watch_app_efl.h
/usr/lib/pkgconfig/capi-appfw-watch-application.pc

