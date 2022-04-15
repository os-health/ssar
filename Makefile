default:
	make -C conf
	make -C ssar
	make -C sresar
clean:
	make -C conf clean
	make -C ssar clean
	make -C sresar clean
install:
	install -d                           /etc/ssar/
	install conf/ssar.conf               /etc/ssar/
	install conf/sys.conf                /etc/ssar/
	install -d                           /usr/src/os_health/ssar/
	install conf/sresar.service          /usr/src/os_health/ssar/
	install -d                           /usr/local/bin/
	install ssar/ssar                    /usr/local/bin/ssar
	install ssar/ssar+.py                /usr/local/bin/ssar+
	install ssar/tsar2.py                /usr/local/bin/tsar2
	install sresar/sresar                /usr/local/bin/sresar
	install -d                           /run/lock/os_health/
	touch                                /run/lock/os_health/sresar.pid
	cp -f /usr/src/os_health/ssar/sresar.service /etc/systemd/system/sresar.service
	chown root:root /etc/systemd/system/sresar.service
	systemctl daemon-reload
	if [ systemctl is-enabled sresar.service ]; then \
	    systemctl disable sresar.service;  \
	fi
	systemctl enable sresar.service
	if systemctl is-active sresar.service; then \
	    systemctl stop sresar.service; \
	fi
	systemctl start sresar.service
uninstall:
	systemctl stop    sresar.service
	systemctl disable sresar.service
	rm -f /etc/systemd/system/sresar.service
	systemctl daemon-reload
	systemctl reset-failed
	if [[ -e "/var/log/sre_proc/" ]];then \
	    for i2 in $(ls "/var/log/sre_proc/"); do \
                rm -fr "/var/log/sre_proc/"${i2}; \
                sleep 0.01; \
            done; \
            rm -fr "/var/log/sre_proc/"; \
	fi
	customized_work_path=$(cat /etc/ssar/ssar.conf | grep work_path | awk -F= '{print $2}')
	if [[ -e ${customized_work_path}"/sre_proc/" ]];then \
            for i3 in $(ls ${customized_work_path}"/sre_proc/"); do \
	        rm -fr ${customized_work_path}"/sre_proc/"${i3}; \
                sleep 0.01; \
            done; \
            rm -fr ${customized_work_path}"/sre_proc/"; \
	fi
	rm -fr /etc/ssar/
	rm -fr /usr/src/os_health/ssar/
	rm -f  /usr/local/bin/ssar
	rm -f  /usr/local/bin/ssar+
	rm -f  /usr/local/bin/tsar2
	rm -f  /usr/local/bin/sresar
	rm -fr /run/lock/os_health/
