# SRESAR

SRESAR (Site Reliability Engineering System Activity Reporter) is a new addition to the family of sar tools and provides the overall specifications, process-level metrics and featured Load metrics.

## Table of Contents

* [Introduction](#introduction)
* [Advantage](#advantage)
* [Installation](#installation)
  * [AnolisOS CentOS](#anolisos-centos)
  * [Ubuntu](#other-parsers)  
  * [Source Code](#source-code)
* [Architecture](#architecture)
* [Beginner Guide](#beginner-guide)
* [Reference](#feference)
* [LICENSE](#license)
* [Dependencies](#dependencies)
* [Get Support](#get-support)
* [Contributor](#contributor)

## Introduction

SRESAR (Site Reliability Engineering System Activity Reporter) is a new addition to the family of sar tools. It covers most of the main functions of traditional sar tools, but also extends the overall specifications. Added process-level metrics and featured Load metrics.

## Advantage

Compared to other sar family tools, SRESAR has several features:

* Traditional sar tools can only collect some major system indicators on a fixed basis. ssar tools can extend almost any indicator of the system by simply modifying the configuration file within a few minutes without modifying the code.
* Traditional sar tools may not be able to secondary development, or secondary development threshold is high, ssar tools support the use of Python language secondary development, secondary development threshold is low;
* Traditional sar tools record process-level indicators more preliminarily, while ssar tools record key indicators of all processes in the system.
* For Linux load metrics, the ssar tool also provides detailed load metrics. Load5s is an industry-unique indicator now.

Of course, collecting more data requires more disk storage. In the last 20 years, with the development of storage technology, disk space has increased 1000 times with the same cost structure. In this context, it is cost-effective to collect more data metrics for the appropriate storage space.

## Installation

To use, there are several methods:

### Provide packages

The project provides several packages for direct use.

```bash
$ git clone https://gitee.com/anolis/tracing-ssar.git
$ ls ssar/package/
ssar-1.0.2-1.an8.x86_64.rpm ssar-1.0.2-1.el7.x86_64.rpm ssar_1.0.2-1_amd64.deb
```

### AnolisOS CentOS

Method under AnolisOS and centos.

```bash
$ yum install zlib-devel gcc-c++
$ yum install rpm-build rpmdevtools git
$ rpmdev-setuptree
$ cd ~/
$ git clone https://gitee.com/anolis/tracing-ssar.git
$ cp -fr ~/ssar/* ~/rpmbuild/SOURCES/
$ cd ~/rpmbuild/
$ cp SOURCES/spec/ssar.spec SPECS/
$ rpmbuild -bb SPECS/ssar.spec 
$ cd RPMS/x86_64/
$ sudo rpm -ivh ssar-1.0.2-1.an8.x86_64.rpm
$ sudo rpm -e ssar                                             # remove package
```

### Ubuntu

Method under Ubuntu.

```bash
$ apt-get update
$ apt install zlib1g-dev git
$ cd ~/
$ git clone https://gitee.com/anolis/tracing-ssar.git
$ cd ssar/debian
$ ./build.sh
$ dpkg -i ssar_1.0.2-1_amd64.deb
$ dpkg -r ssar                                                 # remove package
```

### Source Code

Source code installation method (not recommended).

```bash
$ yum install zlib-devel                                       # ubuntu need zlib1g-dev
$ cd ~/
$ git clone https://gitee.com/anolis/tracing-ssar.git
$ make 
$ sudo make install
$ sudo make uninstall                                          # remove                                   
```

## Architecture

A brief introduction to the SRESAR project architecture:

* The sresar resident process is the data collector for SRESAR project, and the data is stored under /var/log/sre_proc.
* The ssar command is the generic data parser from the disk data collected by the sresar process. The ssar command can directly display results to the consumer.
* The tsar2 command is just a wrapper of the ssar command, providing consumers with more comprehensive results.
* ssar+ command is the future of tsar2, various option parameters and implementations of ssar+ are currently being planned.

## Beginner Guide

Here's a quick start with some common commands

```bash
$ ssar --help                          # Display help Information
$ ssar                                 # the historical data output of the default indicator is displayed
$ ssar --cpu                           # Display the output historical data of THE CPU category.
$ ssar -f 2020-09-29T18:34:00          # Specifies the end time of a time interval
$ ssar -r 60                           # The specified duration is 60 minutes. The default value is 300 minutes
$ ssar -i 1                            # In the specified time range, the display accuracy is 1 minute. The default value is 10 minutes
$ ssar --api                           # If selected, the information is output in JSON format
$ ssar -f +10 -i 1s                    # with real-time mode, the acquisition output accuracy is 1 second, the default value is 5 seconds.
$ ssar -o user,shmem,memfree           # Only user, shmem, and memfree indicators are output
$ ssar -o "dev|eth0:|2|d;snmp:8:11:d"                                            # Indicators can be separated by semicolons (;)
$ ssar -o 'metric=c|cfile=meminfo|line_begin=MemFree:|column=2|alias=free'       # Take the value of MemFree from memInfo and name it free
$ ssar -o 'metric=d:cfile=snmp:line=8:column=13:alias=retranssegs'               # Take the difference between the values in line 8 and column 13 in snmp
$ ssar -o 'metric=c|cfile=loadavg|line=1|column=1|dtype=float|alias=load1'       # Get the load1 data of type float
$ ssar -o 'metric=c|cfile=loadavg|line=1|column=4|dtype=string|alias=runq_plit'  # Get string information like 2/1251
$ ssar -o 'metric=d|cfile=stat|line=2|column=2-11|alias=cpu0_{column};'          # Displays data for columns 2 through 11 of CPU0
$ ssar -o 'metric=d|cfile=stat|line=2-17|column=5|alias=idle_{line};'            # Display idle data for cpu0 through CPU15
$ ssar procs                          # Display the historical information about the process indicator
$ ssar procs --mem                    # A combination of process memory indicators
$ ssar procs -k -ppid,+sid,pid        # Specifies sort fields in descending order by ppid, ascending order by sid, and ascending order by pid (built-in)
$ ssar procs -l 100                   # If more than 100 messages are displayed, only 100 results are displayed
$ ssar proc -p 1                      # Display the historical information about the process whose pid is 1.
$ ssar load5s                         # Display historical information about load of the server
$ ssar load5s -z                      # Only load values that trigger details collection are displayed
$ ssar load2p -c 2020-10-07T07:45:25  # The load details are displayed
$ tsar2 --help                        # The help information about the tsar2 wrapper is displayed
$ tsar2 --io -I sda,sda3,sdb1         # You can display indicators of both the sda disk and the sda3 partition
$ tsar2 --cpu -I cpu,cpu1,cpu4        # It can display the performance indicators of both the entire CPU and a single CPU
$ tsar2 --traffic -I eth0,lo          # You can display indicators for both eth0 and lo
$ tsar2 --retran                      # Display detailed information about TCP retransmission
$ tsar2 --tcpofo                      # Display detailed information about Tcp Out-Of-Order
$ tsar2 --tcpdrop                     # Display detailed information about Tcp Drop
$ tsar2 --tcperr                      # Display detailed information about Tcp Err
$ tsar2 irqtop -C 7,30-32             # Displays interrupt details for cpus 7 and 30 through 32
$ tsar2 cputop -l -i 1 -S sirq        # Displays the top 4 softirq cpu usage of all cpu
```

## Reference

for more usage with ssar tools，visit reference，[查看参考手册](./Reference_zh-CN.md)

## LICENSE

It is distributed under the Mulan Permissive Software License，Version 2 - see the accompanying [LICENSE](./LICENSE) file for more details.  Some  component  dependencies and their copyright are listed below.

## Dependencies

Each Dependency component and it's licensed:

* [tomlc99][tomlc99] project is licensed under [MIT License](https://github.com/cktan/tomlc99/blob/master/LICENSE).

* [cpptoml](https://github.com/skystrife/cpptoml.git) project is licensed under [MIT License](https://github.com/skystrife/cpptoml/blob/master/LICENSE).

* [nlohmann json](https://github.com/nlohmann/json.git) project is licensed under [MIT License](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT).

* [CLI11](https://github.com/CLIUtils/CLI11.git) project is licensed under [3-clause BSD License](https://github.com/CLIUtils/CLI11/blob/master/LICENSE).

## Get Support 

DingDing group number 33304007

## Contributor

This project was created by Wen Maoquan (English Name Miles Wen) who working for Alibaba Cloud-Computing Platform. Special thanks to all the [contributors](./CONTRIBUTOR).

Ccheng   <ccheng@linux.alibaba.com>

Dust.Li  <dust.li@linux.alibaba.com>

MilesWen <mileswen@linux.alibaba.com>

TonyLu   <tonylu@linux.alibaba.com>

Xlpang   <xlpang@linux.alibaba.com>

[tomlc99]: https://github.com/cktan/tomlc99.git
