%global __os_install_post %{nil}
%define _ignore_post_scripts_errors %{nil}
%define _enable_debug_packages %{nil}
%define debug_package %{nil}
%define anolis_release 1
%define work_path      /var/log

Name:                 ssar
Version:              1.0.4
Release:              %{?anolis_release}%{?dist}
Url:                  https://gitee.com/anolis/ssar
Summary:              ssar for SRE
Group:                System Environment/Base
License:              Mulan PSL v2
Source0:              %{name}-%{version}.tar.gz

BuildRequires:        zlib-devel

Vendor:               Alibaba

%description
log the system details 

%prep
cd $RPM_BUILD_DIR
rm -rf %{name}-%{version}
gzip -dc $RPM_SOURCE_DIR/%{name}-%{version}.tar.gz | tar -xvvf -
if [ $? -ne 0 ]; then
    exit $?
fi
main_dir=$(tar -tzvf $RPM_SOURCE_DIR/%{name}-%{version}.tar.gz| head -n 1 | awk '{print $NF}' | awk -F/ '{print $1}')
if [ "${main_dir}" == %{name} ];then
    mv %{name} %{name}-%{version}
fi
cd %{name}-%{version}
chmod -R a+rX,u+w,g-w,o-w .

%build
cd %{name}-%{version}
make

%install
rm -rf $RPM_BUILD_ROOT

BuildDir=$RPM_BUILD_DIR/%{name}-%{version}

install -d                                %{buildroot}/etc/ssar/
install -p $BuildDir/conf/ssar.conf       %{buildroot}/etc/ssar/
install -p $BuildDir/conf/sys.conf        %{buildroot}/etc/ssar/
install -d                                %{buildroot}/usr/share/man/man1/
install -p $BuildDir/conf/ssar.1.gz       %{buildroot}/usr/share/man/man1/
install -d                                %{buildroot}/usr/src/os_health/ssar/
install -p $BuildDir/conf/sresar.service  %{buildroot}/usr/src/os_health/ssar/
install -p $BuildDir/conf/sresar.cron     %{buildroot}/usr/src/os_health/ssar/
install -p $BuildDir/conf/sresard         %{buildroot}/usr/src/os_health/ssar/
install -d                                %{buildroot}/usr/lib/os_health/ssar/
install -p $BuildDir/conf/healing.sh      %{buildroot}/usr/lib/os_health/ssar/healing.sh
install -d                                %{buildroot}/usr/bin/
install -p $BuildDir/ssar/ssar            %{buildroot}/usr/bin/ssar
install -p $BuildDir/ssar/ssar+.py        %{buildroot}/usr/bin/ssar+
install -p $BuildDir/ssar/tsar2.py        %{buildroot}/usr/bin/tsar2
install -p $BuildDir/sresar/sresar        %{buildroot}/usr/bin/sresar
install -d                                %{buildroot}/run/lock/os_health/
touch                                     %{buildroot}/run/lock/os_health/sresar.pid

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"
rm -rf $RPM_BUILD_DIR/%{name}-%{version}

%files
%defattr(-,root,root,-)
%dir    %attr(755, root, root) /etc/ssar/
%config %attr(644, root, root) /etc/ssar/ssar.conf
%config %attr(644, root, root) /etc/ssar/sys.conf
%dir    %attr(755, root, root) /usr/src/os_health/ssar/
%config %attr(644, root, root) /usr/src/os_health/ssar/sresar.service
%config %attr(644, root, root) /usr/src/os_health/ssar/sresar.cron
%config %attr(644, root, root) /usr/src/os_health/ssar/sresard
%doc    %attr(644, root, root) /usr/share/man/man1/ssar.1.gz
/usr/bin/sresar
/usr/bin/ssar
/usr/bin/ssar+
/usr/bin/tsar2
/usr/lib/os_health/ssar/healing.sh
/run/lock/os_health/sresar.pid

%pre

%post
/usr/bin/env python --version >/dev/null 2>&1
if [ $? -ne 0 ];then
    sed -i 's:/usr/bin/env python:/usr/bin/env python3:' /usr/bin/tsar2
fi

if [[ $(cat /proc/1/sched | head -n 1 | grep systemd) ]]; then
    # in host
    cp -f /usr/src/os_health/ssar/sresar.service /etc/systemd/system/sresar.service
    chown root:root /etc/systemd/system/sresar.service
    systemctl daemon-reload
    systemctl is-enabled sresar.service && systemctl disable sresar.service
    systemctl enable sresar.service
    systemctl is-active sresar.service && systemctl stop sresar.service
    systemctl start sresar.service
else
    # in docker
    cp -f /usr/src/os_health/ssar/sresar.cron /etc/cron.d/sresar.cron
    chown root:root /etc/cron.d/sresar.cron
    cp -f /usr/src/os_health/ssar/sresard /etc/init.d/sresard
    chown root:root /etc/init.d/sresard
    chmod a+x /etc/init.d/sresard
    chkconfig --add sresard 
    /usr/lib/os_health/ssar/healing.sh
fi

%preun
if [ "$1" = "0" ]; then
    if [[ $(cat /proc/1/sched | head -n 1 | grep systemd) ]]; then
        # in host
        systemctl stop    sresar.service
        systemctl disable sresar.service
        rm -f /etc/systemd/system/sresar.service
    else
        # in docker
        chkconfig --del sresard
        rm -f /etc/cron.d/sresar.cron
        rm -f /etc/init.d/sresard
    fi

    if [[ -e %{work_path}"/sre_proc/" ]];then
        for i1 in $(ls %{work_path}"/sre_proc/")
        do
            rm -fr %{work_path}"/sre_proc/"${i1}
            sleep 0.01
        done
        rm -fr %{work_path}"/sre_proc/"
    fi

    if [[ -e "/var/log/sre_proc/" ]];then
        for i2 in $(ls "/var/log/sre_proc/")
        do
            rm -fr "/var/log/sre_proc/"${i2}
            sleep 0.01
        done
        rm -fr "/var/log/sre_proc/"
    fi

    customized_work_path=$(cat /etc/ssar/ssar.conf | grep work_path | awk -F= '{print $2}')
    if [[ -e ${customized_work_path}"/sre_proc/" ]];then
        for i3 in $(ls ${customized_work_path}"/sre_proc/")
        do
            rm -fr ${customized_work_path}"/sre_proc/"${i3}
            sleep 0.01
        done
        rm -fr ${customized_work_path}"/sre_proc/"
    fi
fi

%postun

%changelog
* Mon Mar 13 2023 MilesWen <mileswen@linux.alibaba.com> - 1.0.4-1
- Fix some segfault.
* Wed Jul 13 2022 MilesWen <mileswen@linux.alibaba.com> - 1.0.3-1
- Release ssar RPM package
--end
