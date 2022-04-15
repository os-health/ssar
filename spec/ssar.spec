%global __os_install_post %{nil}
%define _ignore_post_scripts_errors %{nil}
%define _enable_debug_packages %{nil}
%define debug_package %{nil}
%define name          ssar
%define version       1.0.2
%define release       1
%define work_path     /var/log
Name:                 %{name}
Version:              %{version}
Release:              %{release}%{?dist}
Summary:              ssar for SRE
Group:                System Environment/Base
License:              Mulan PSL v2
ExclusiveArch:        x86_64
%description
log the system details 

%prep
install -d                             $RPM_BUILD_DIR/sresar/
install -d                             $RPM_BUILD_DIR/ssar/
install -d                             $RPM_BUILD_DIR/conf/
install -v -D $RPM_SOURCE_DIR/sresar/* $RPM_BUILD_DIR/sresar/
install -v -D $RPM_SOURCE_DIR/ssar/*   $RPM_BUILD_DIR/ssar/
install -v -D $RPM_SOURCE_DIR/conf/*   $RPM_BUILD_DIR/conf/
install -v    $RPM_SOURCE_DIR/Makefile $RPM_BUILD_DIR/Makefile

%build
make

%install
install -d                                  %{buildroot}/etc/ssar/
install $RPM_BUILD_DIR/conf/ssar.conf       %{buildroot}/etc/ssar/
install $RPM_BUILD_DIR/conf/sys.conf        %{buildroot}/etc/ssar/
install -d                                  %{buildroot}/usr/share/man/man1/
install $RPM_BUILD_DIR/conf/ssar.1.gz       %{buildroot}/usr/share/man/man1/
install -d                                  %{buildroot}/usr/src/os_health/ssar/
install $RPM_BUILD_DIR/conf/sresar.service  %{buildroot}/usr/src/os_health/ssar/
install $RPM_BUILD_DIR/conf/sresar.cron     %{buildroot}/usr/src/os_health/ssar/
install $RPM_BUILD_DIR/conf/sresard         %{buildroot}/usr/src/os_health/ssar/
install -d                                  %{buildroot}/usr/local/lib/os_health/ssar/
install $RPM_BUILD_DIR/conf/healing.sh      %{buildroot}/usr/local/lib/os_health/ssar/healing.sh
install -d                                  %{buildroot}/usr/local/bin/
install $RPM_BUILD_DIR/ssar/ssar            %{buildroot}/usr/local/bin/ssar
install $RPM_BUILD_DIR/ssar/ssar+.py        %{buildroot}/usr/local/bin/ssar+
install $RPM_BUILD_DIR/ssar/tsar2.py        %{buildroot}/usr/local/bin/tsar2
install $RPM_BUILD_DIR/sresar/sresar        %{buildroot}/usr/local/bin/sresar
install -d                                  %{buildroot}/run/lock/os_health/
touch                                       %{buildroot}/run/lock/os_health/sresar.pid

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"

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
/usr/local/bin/sresar
/usr/local/bin/ssar
/usr/local/bin/ssar+
/usr/local/bin/tsar2
/usr/local/lib/os_health/ssar/healing.sh
/run/lock/os_health/sresar.pid

%pre

%post
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
    /usr/local/lib/os_health/ssar/healing.sh
fi

%preun
if [ "$1" = "0" ]; then
    if [[ $(cat /proc/1/sched | head -n 1 | grep systemd) ]]; then
        # in host
        systemctl stop    sresar.service
        systemctl disable sresar.service
        rm -f /etc/systemd/system/sresar.service
        systemctl daemon-reload
        systemctl reset-failed
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
