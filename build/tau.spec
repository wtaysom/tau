# norootforbuild

Name:		tau
BuildRequires:	kernel-source kernel-syms readline-devel e2fsprogs-devel novell-nss-devel novell-zapi-devel qt-devel
License:	GPL
Group:		System/Kernel
Summary:	tau File System
Version:	1.0
Release:	0
Source0: 	tau.pj.tar.bz2
BuildRoot:	%{_tmppath}/%{name}-%{version}-build

%suse_kernel_module_package -x default

%description
This package includes the user-space portion of the tau File System.

%package KMP
Summary: tau File System kernel modules
Group: System/Kernel

%description KMP
This package includes the tau File System kernel modules.

%prep
%setup -n tau.pj
set -- *
mkdir source
mv "$@" source/
mkdir obj

%build
for flavor in %flavors_to_build; do
    rm -rf obj/$flavor
    cp -r source obj/$flavor

# Tweak the makefiles - this section encompasses all the changes necessary
# to transform the developer's build structure into a KMPM-compliant
# environment for autobuild.
    cp obj/$flavor/mk/multi_lkm.mk obj/$flavor/mk/multi_lkm.mk.orig
    cat obj/$flavor/mk/multi_lkm.mk \
      | sed -e "s/kdir.*:=.*/kdir := \/usr\/src\/linux-obj\/%_target_cpu\/$flavor/" \
            -e "s/^.*depmod.*//" \
      > obj/$flavor/mk/tmpfile
    mv obj/$flavor/mk/tmpfile obj/$flavor/mk/multi_lkm.mk
    cp obj/$flavor/mk/kernel.mk obj/$flavor/mk/kernel.mk.orig
    cat obj/$flavor/mk/kernel.mk \
      | sed -e "s/kdir.*:=.*/kdir := \/usr\/src\/linux-obj\/%_target_cpu\/$flavor/" \
            -e "s/^.*depmod.*//" \
      > obj/$flavor/mk/tmpfile
    mv obj/$flavor/mk/tmpfile obj/$flavor/mk/kernel.mk
    cp obj/$flavor/mk/default.mk obj/$flavor/mk/default.mk.orig
    cat obj/$flavor/mk/default.mk \
      | sed -e "s/bin.*?=.*/bin ?= \$(BUILD_ROOT)\/opt\/novell\/tau\/sbin/" \
	    -e "s/.*cp \$(opus).*//" \
	    -e "/install:/a\\\tcp \$(opus) \$(bin)" \
      > obj/$flavor/mk/tmpfile
    mv obj/$flavor/mk/tmpfile obj/$flavor/mk/default.mk
    cp obj/$flavor/mk/multi.mk obj/$flavor/mk/multi.mk.orig
    cat obj/$flavor/mk/multi.mk \
      | sed -e "s/bin.*?=.*/bin ?= \$(BUILD_ROOT)\/opt\/novell\/tau\/sbin/" \
	    -e "s/.*cp \$@.*//" \
	    -e "s/.*cp \$(opuses).*//" \
	    -e "/install:/a\\\tcd \$(objdir); cp \$(opuses) \$(bin)" \
      > obj/$flavor/mk/tmpfile
    mv obj/$flavor/mk/tmpfile obj/$flavor/mk/multi.mk
# End of Makefile modifications for KMPM-compliant build

# Build the libraries
    pushd $PWD/obj/$flavor/libmsg.b
    make
    popd
    pushd $PWD/obj/$flavor/libq.b
    make
    popd

# Build kernel modules and binaries
    pushd $PWD/obj/$flavor/tau.t
    make
    popd
    pushd $PWD/obj/$flavor/alg.t
    for i in `ls .`; do
        cd $i
        make
        cd ..
    done
    popd
    pushd $PWD/obj/$flavor/tests.t
    for i in `ls -I sha.d .`; do
        cd $i
        make
        cd ..
    done
    popd
    pushd $PWD/obj/$flavor/tools.t
    for i in `ls -I adam.d -I scripts.d .`; do
        cd $i
        make
        cd ..
    done
    popd
done

%install
export INSTALL_PATH=$RPM_BUILD_ROOT
export INSTALL_MOD_PATH=$RPM_BUILD_ROOT
export INSTALL_MOD_DIR=updates

# Make the directories
install -m755 -d $RPM_BUILD_ROOT/opt/novell/tau
install -m755 -d $RPM_BUILD_ROOT/opt/novell/tau/sbin
install -m755 -d $RPM_BUILD_ROOT/opt/novell/tau/lib
install -m755 -d $RPM_BUILD_ROOT/opt/novell/tau/scripts
install -m755 -d $RPM_BUILD_ROOT/opt/novell/tau/tests
install -m755 -d $RPM_BUILD_ROOT/opt/novell/tau/tests/sbin
install -m755 -d $RPM_BUILD_ROOT/opt/novell/tau/tools

for flavor in %flavors_to_build; do
# Install the libraries - this should be moved into the library makefile
    install -m644    $PWD/obj/$flavor/libmsg.b/.%_target_cpu/libmsg \
                     $RPM_BUILD_ROOT/opt/novell/tau/lib/libmsg
    install -m644    $PWD/obj/$flavor/libq.b/.%_target_cpu/libq \
                     $RPM_BUILD_ROOT/opt/novell/tau/lib/libq

# Install the kernel modules
    pushd $PWD/obj/$flavor/tau.t/kernel.m
    BUILD_ROOT=$RPM_BUILD_ROOT make install
    popd
    pushd $PWD/obj/$flavor/tests.t/perf.k
    BUILD_ROOT=$RPM_BUILD_ROOT make install
    popd
    pushd $PWD/obj/$flavor/tests.t/yang.k
    BUILD_ROOT=$RPM_BUILD_ROOT make install
    popd
    pushd $PWD/obj/$flavor/tools.t/kmdb.k
    BUILD_ROOT=$RPM_BUILD_ROOT make install
    popd

# Install the binaries
    pushd $PWD/obj/$flavor/alg.t
    subdirs="blog.d btree.d dirkache.d fs.d lfs.d log.d mtree.d"
    for i in $subdirs; do
	cd $i
	BUILD_ROOT=$RPM_BUILD_ROOT make install
	cd ..
    done
    popd
    pushd $PWD/obj/$flavor/tau.t
    subdirs="cmd.d dirfs.d mkfs.tau.d sage.d sw.d"
    for i in $subdirs; do
	cd $i
        BUILD_ROOT=$RPM_BUILD_ROOT make install
	cd ..
    done
    popd

# Install the test binaries
    pushd $PWD/obj/$flavor/tests.t
    subdirs="ants.d cntsha.d file.m mkfs.perf.d play.m rgb.d yang.d yin.d zapi.m"
    for i in $subdirs; do
        cd $i
        BUILD_ROOT=$RPM_BUILD_ROOT bin=$BUILD_ROOT/opt/novell/tau/tests/sbin make install
        cd ..
    done
    popd
    pushd $PWD/obj/$flavor/tau.t/tests.m
    BUILD_ROOT=$RPM_BUILD_ROOT bin=$BUILD_ROOT/opt/novell/tau/tests/sbin make install
    popd

# Install the tools
    pushd $PWD/obj/$flavor/tools.t
    subdirs="calc.d kplot.d mdb.d qalc.d"
    for i in $subdirs; do
        cd $i
        BUILD_ROOT=$RPM_BUILD_ROOT bin=$BUILD_ROOT/opt/novell/tau/tools make install
        cd ..
    done
    popd

# Install the scripts - this really shouldn't have to be done per flavor
# but that's the structure of this build system
# Perhaps create a script makefile?
    install -m644    $PWD/obj/$flavor/tau.t/scripts.m/* \
		     $RPM_BUILD_ROOT/opt/novell/tau/scripts/
    install -m644    $PWD/obj/$flavor/tools.t/scripts.d/* \
		     $RPM_BUILD_ROOT/opt/novell/tau/scripts/
done

%files
%defattr(-,root,root)
# list all the user-space files put in place by make installs
/opt/novell/tau/

%changelog
Tue Dec 23 2008 - andavis@novell.com
- Initial package.
