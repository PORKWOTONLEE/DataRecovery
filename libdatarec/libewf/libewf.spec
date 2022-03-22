Name: libewf
Version: 20140813
Release: 1
Summary: Library to access the Expert Witness Compression Format (EWF)
Group: System Environment/Libraries
License: LGPLv3+
Source: %{name}-%{version}.tar.gz
URL: https://github.com/libyal/libewf-legacy
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires:                zlib
BuildRequires: gcc gcc-c++                zlib-devel

%description
libewf is a library to access the Expert Witness Compression Format (EWF).
libewf allows you to read media information of EWF files in the SMART (EWF-S01)
format and the EnCase (EWF-E01, EWF-L01, EWF2-Ex01 and EWF2-Lx01) formats.
Supports files created by EnCase 1 to 7, linen 5 to 7 and FTK Imager.

%package -n libewf-static
Summary: Library to access the Expert Witness Compression Format (EWF)
Group: Development/Libraries
Requires:                zlib-static

%description -n libewf-static
Static library version of libewf

%package -n libewf-devel
Summary: Header files and libraries for developing applications for libewf
Group: Development/Libraries
Requires: libewf = %{version}-%{release}

%description -n libewf-devel
Header files and libraries for developing applications for libewf.

%package -n libewf-python2
Obsoletes: libewf-python < %{version}
Provides: libewf-python = %{version}
Summary: Python 2 bindings for libewf
Group: System Environment/Libraries
Requires: libewf = %{version}-%{release} python2
BuildRequires: python2-devel

%description -n libewf-python2
Python 2 bindings for libewf

%package -n libewf-python3
Summary: Python 3 bindings for libewf
Group: System Environment/Libraries
Requires: libewf = %{version}-%{release} python3
BuildRequires: python3-devel

%description -n libewf-python3
Python 3 bindings for libewf

%package -n libewf-tools
Summary: Several tools for reading and writing EWF files
Group: Applications/System
Requires: libewf = %{version}-%{release}       
 byacc flex       

%description -n libewf-tools
Several tools for reading and writing EWF files

%prep
%setup -q

%build
%configure --prefix=/usr --libdir=%{_libdir} --mandir=%{_mandir} --enable-python2 --enable-python3
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%make_install

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files -n libewf
%defattr(644,root,root,755)
%license COPYING COPYING.LESSER
%doc AUTHORS README
%attr(755,root,root) %{_libdir}/*.so.*

%files -n libewf-static
%defattr(644,root,root,755)
%license COPYING COPYING.LESSER
%doc AUTHORS README
%attr(755,root,root) %{_libdir}/*.a

%files -n libewf-devel
%defattr(644,root,root,755)
%license COPYING COPYING.LESSER
%doc AUTHORS README
%{_libdir}/*.la
%{_libdir}/*.so
%{_libdir}/pkgconfig/libewf.pc
%{_includedir}/*
%{_mandir}/man3/*

%files -n libewf-python2
%defattr(644,root,root,755)
%license COPYING COPYING.LESSER
%doc AUTHORS README
%{_libdir}/python2*/site-packages/*.a
%{_libdir}/python2*/site-packages/*.la
%{_libdir}/python2*/site-packages/*.so

%files -n libewf-python3
%defattr(644,root,root,755)
%license COPYING COPYING.LESSER
%doc AUTHORS README
%{_libdir}/python3*/site-packages/*.a
%{_libdir}/python3*/site-packages/*.la
%{_libdir}/python3*/site-packages/*.so

%files -n libewf-tools
%defattr(644,root,root,755)
%license COPYING COPYING.LESSER
%doc AUTHORS README
%attr(755,root,root) %{_bindir}/*
%{_mandir}/man1/*

# Exclude ewfdebug tool
%exclude %{_bindir}/ewfdebug

%changelog
* Fri Mar 11 2022 Joachim Metz <joachim.metz@gmail.com> 20140813-1
- Auto-generated
