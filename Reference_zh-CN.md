# SRESAR使用手册

# 目录

* [一、工具介绍](#工具介绍)
* [二、工具特点](#工具特点)
* [三、工具安装](#工具安装)
    * [3.1、预提供安装包](#预提供安装包)
    * [3.2、rpm包打包方法](#rpm包打包方法)
    * [3.3、deb包打包方法](#deb包打包方法)
    * [3.4、源代码编译安装](#源代码编译安装)
* [四、整机指标](#整机指标)
    * [4.1、整机参数说明](#整机参数说明) 
    * [4.2、整机视图说明](#整机视图说明)
    * [4.3、整机预定义指标说明](#整机预定义指标说明)
    * [4.4、整机自定义指标说明](#整机自定义指标说明)
    * [4.5、整机实时模式说明](#整机实时模式说明)
* [五、多进程指标](#多进程指标)
    * [5.1、多进程参数说明](#多进程参数说明) 
    * [5.2、多进程视图说明](#多进程视图说明)
    * [5.3、多进程指标组合](#多进程指标组合)
    * [5.4、多进程指标说明](#多进程指标说明)  
* [六、单进程指标](#单进程指标)
    * [6.1、单进程参数说明](#单进程参数说明)
    * [6.2、单进程视图说明](#单进程视图说明)
    * [6.3、单进程指标组合](#单进程指标组合)
    * [6.4、单进程指标说明](#单进程指标说明)  
* [七、整机load指标](#整机load指标)
    * [7.1、整机load参数说明](#整机load参数说明) 
    * [7.2、整机load指标说明](#整机load指标说明) 
    * [7.3、特异指标load5s深入说明](#特异指标load5s深入说明)   
* [八、Load详情指标](#Load详情指标)
    * [8.1、Load详情参数说明](#Load详情参数说明) 
    * [8.2、Load详情聚合指标说明](#Load详情聚合指标说明) 
    * [8.3、Load详情详细指标说明](#Load详情详细指标说明) 
* [九、数据采集器sresar](#数据采集器sresar)
    * [9.1、ssar工具简介](#ssar工具简介)
    * [9.2、ssar工作目录](#ssar工作目录)
    * [9.3、数据采集器通用选项配置](#数据采集器通用选项配置)
    * [9.4、数据采集器load选项配置](#数据采集器load选项配置)  
    * [9.5、数据采集器整机选项配置](#数据采集器整机选项配置)
* [十、通用查询器ssar](#通用查询器ssar)
    * [10.1、通用查询器整机指标配置](#通用查询器整机指标配置)
    * [10.2、通用查询器整机视图配置](#通用查询器整机视图配置)
    * [10.3、通用查询器整机指标组配置](#通用查询器整机指标组配置)
    * [10.4、通用查询器整机指标组作用优先级](#通用查询器整机指标组作用优先级)  
* [十一、古典查询器tsar2](#古典查询器tsar2)
    * [11.1、时间范围类选项](#时间范围类选项)
        *  [11.1.1、date选项](#date选项)
        *  [11.1.2、ndays选项](#ndays选项)
        *  [11.1.3、watch选项](#watch选项)
        *  [11.1.4、live选项](#live选项)
    * [11.2、时间间隔选项interval](#时间间隔选项interval)
    * [11.3、数据指标大类选项](#数据指标大类选项)
        *  [11.3.1、cpu选项](#cpu选项)
        *  [11.3.2、mem选项](#mem选项)
        *  [11.3.3、swap选项](#swap选项)
        *  [11.3.4、tcp选项](#tcp选项)
        *  [11.3.5、udp选项](#udp选项)
        *  [11.3.6、traffic选项](#traffic选项)
        *  [11.3.7、io选项](#io选项)
        *  [11.3.8、pcsw选项](#pcsw选项)
        *  [11.3.9、tcpx选项](#tcpx选项)
        *  [11.3.10、load选项](#load选项)
    * [11.4、设备列表选项](#设备列表选项)
        *  [11.4.1、item选项](#item选项)
        *  [11.4.2、merge选项](#merge选项)
    * [11.5、指标类选项](#指标类选项)
        *  [11.5.1、spec选项](#spec选项)
        *  [11.5.2、spec选项](#spec选项)
* [十二、tsar2创新指标](#tsar2创新指标)
    * [12.1、网络tcp扩展指标](#网络tcp扩展指标)
* [十三、tsar2中断子命令irqtop](#tsar2中断子命令irqtop)
    * [13.1、输出指标项说明](#输出指标项说明)
    * [13.2、输入选项说明](#输入选项说明)
    * [13.3、大C选项参数特殊说明](#大C选项参数特殊说明)
    * [13.4、interrupts数据采集](#interrupts数据采集)  
* [十四、tsar2的CPU子命令cputop](#tsar2的CPU子命令cputop)
* [十五、增强查询器ssar+](#增强查询器ssar+)
* [十六、技术交流](#技术交流)

<a name="工具介绍"/>
# 一、工具介绍

　　ssar(Site Reliability Engineers System Activity Reporter)是系统活动报告sar工具家族中崭新的一个。在几乎涵盖了传统sar工具的大部分主要功能之外，它还扩展了更多的整机指标；新增了进程级指标和特色的load指标。

<a name="工具特点"/>
# 二、工具特点

　　与其他sar家族工具相比，ssar有如下几个特色的地方：

1. 传统sar工具只能固定采集一些主要的系统指标，ssar工具无需修改代码，只需几分钟简单修改配置文件，几乎可以扩展采集系统的任意指标；

2. 传统sar工具或者无法二次开发，或二次开发门槛较高，ssar工具支持使用python语言二次开发，二次开发入门的门槛低；

3. 传统的sar工具对进程级指标的记录较为初级，ssar工具完整的记录了系统所有进程的关键指标；

4. 针对linux load指标，ssar工具还提供了详细的load指标信息，其中的load5s指标是国内外全行业独创。

　　当然了，采集更多的数据，必然占用更多的磁盘存储空间。近20年来，随着存储技术的发展，在同样成本结构的前提下，磁盘空间增长了1000倍。基于这样背景，适当占用一定的存储空间，采集更多数据指标是划算的。

<a name="工具安装"/>
# 三、工具安装

　　工具的安装，有如下几种方法:

<a name="预提供安装包"/>
## 3.1、预提供安装包

　　提供了几个常用安装包供直接使用。

```bash
$ git clone https://gitee.com/anolis/tracing-ssar.git
$ ls ssar/package/
ssar-1.0.2-1.an8.x86_64.rpm                 # 适用于 anolisos 和 centos8
ssar-1.0.2-1.el7.x86_64.rpm                 # 适用于 centos7
ssar_1.0.2-1_amd64.deb                      # 适用于 ubuntu
```

<a name="rpm包打包方法"/>
## 3.2、rpm包打包方法

　　rpm包的打包方法如下，本方法适用于AnolisOS and CentOS等操作系统环境。

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

<a name="deb包打包方法"/>
## 3.3、deb包打包方法

　　deb包的打包方法如下，本方法适用于Ubuntu等操作系统环境。

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

<a name="源代码编译安装"/>
## 3.4、源代码编译安装

　　源代码安装方法。

```bash
$ yum install zlib-devel                                       # ubuntu need zlib1g-dev
$ cd ~/
$ git clone https://gitee.com/anolis/tracing-ssar.git
$ make 
$ sudo make install
$ sudo make uninstall                                          # remove                                   
```

<a name="整机指标"/>
# 四、整机指标

<a name="整机参数说明"/>
## 4.1、整机参数说明

　　运行ssar --help即可获取整机指标帮助信息，下面以实例形式分别介绍。

```bash
$ ssar --help                   # 获取整机指标帮助信息
$ ssar -V                       # 查询版本号信息
$ ssar -f 2020-09-29T18:34:00   # 指定时间区间的结束时间点
$ ssar -f 20210731155500        # 指定时间区间的结束时间点
$ ssar -f 202107311555          # 指定时间区间的结束时间点
$ ssar -f 2021073115            # 指定时间区间的结束时间点
$ ssar -f 20210731              # 指定时间区间的结束时间点
$ ssar -f -10                   # 指定时间区间的结束时间点为当前时刻之前10分钟
$ ssar -f -5.5h                 # 指定时间区间的结束时间点为当前时刻之前5.5小时
$ ssar -f -1.8d                 # 指定时间区间的结束时间点为当前时刻之前1.8天
$ ssar -r 60                    # 指定时间区间长度为60分钟，默认值为300分钟
$ ssar -b 2020-09-29T17:34:00   # 指定时间区间的开始时间点，默认为结束时间点减去时间长度
$ ssar -i 1                     # 指定时间区间长度范围内展示精度为1分钟，默认为10分钟
$ ssar -H                       # 选中则不显示指标的标题信息
$ ssar -P                       # 选中则不显示- * < > =等非数字指标
$ ssar --api                    # 选中则以json格式输出信息
```
<a name="整机视图说明"/>
## 4.2、整机视图说明

　　与传统sar工具类似，ssar工具也针对用户使用的一组指标提炼成视图（view）。目前已经提供的视图有如下几个。

```bash
$ ssar --cpu                    # CPU类指标组合
$ ssar --mem                    # 内存类指标组合
$ ssar --vm                     # 内存活动类指标组合
$ ssar --tcp                    # TCP类指标组合
$ ssar --udp                    # UDP类指标组合
$ ssar --retrans                # tcp重传类指标组合
$ ssar --softirq                # 软中断类指标组合
$ ssar --node0                  # 伙伴算法内存类指标组合
$ ssar --extfrag                # 伙伴算法内存类指标组合
$ ssar --unusable               # 伙伴算法内存类指标组合
$ ssar --cpu --mem              # 支持同时显示2个视图
```

　　具体的视图的配置，请参考《通用查询器整机视图配置》小节。后续还会不断新增更多的视图。而且已经提炼的视图的指标组合还可能会在后续版本中进行调整。

```bash
$ ssar  -i 1

collect_datetime        user/s   system/s   iowait/s  softirq/s  memfree  anonpages      shmem      dirty 
2021-08-21T10:33:00       2.28       3.73       0.02       0.00  2525893     193540       1552        412 
2021-08-21T10:34:00       1.65       3.12       0.02       0.00  2525884     193668       1552        396 
2021-08-21T10:35:00       2.43       4.07       0.00       0.00  2525884     193640       1552        420 
2021-08-21T10:36:00          -          -          -          -        -          -          -          - 
2021-08-21T10:37:00          -          -          -          -        -          -          -          - 
2021-08-21T10:38:00          *          *          *          *        *          *          *          * 
2021-08-21T10:39:00       4.27       6.32       0.42       0.00  2539824     172572       1228       1056 
2021-08-21T10:40:00       2.28       3.75       0.07       0.00  2539819     164508       1244        784 
2021-08-21T10:41:00       2.67       3.78       0.82       0.00  2538176     175248       1292        620 
```

　　这里对输出结果中的关键点做一些说明：

* 第一列是指标采集的时间点信息，精确到秒。

* 标题信息中带有/s结尾的是增量数据，不论数据采集间隔是1分钟，还是10分钟，增量值都统一为每秒值。没有/s结尾的是采集时间发生时刻的刻度值。

* 如果某个采集时间点未采集到数据，则显示为“-”，对于增量数据，如果其前一个时间点未采集到数据，则不论当前时间点是否采集到数据，也显示"-"。

* 如果显示*，则表示当前采集时间和前一个采集时间的周期内，操作系统发生了重启行为。

<a name="整机预定义指标说明"/>
## 4.3、整机预定义指标说明

　　如果已有的视图不能很好的满足需求，我们可以自己组合既有指标(indicator)进行输出。

```bash
$ ssar -o user,shmem,memfree    # 只输出user、shmem和memfree三个指标
$ ssar --cpu -O shmem,memfree   # 在cpu视图的指标基础上，追加输出shmem和memfree指标
```

　　这里要区分小o和大O，小o表示单独输出，大O表示追加输出。具体的指标的配置，请参考《通用查询器整机指标配置》小节。

<a name="整机自定义指标说明"/>
## 4.4、整机自定义指标说明

　　如果有些指标本工具没有定义也没有关系，我们可以直接在命令行中获取指标。自定义指标同样适用小o和大O选项输出。

```bash
$ ssar -O shmem,meminfo:2:2            # 预定义指标和自定义指标可以同时存在；
$ ssar -o 'meminfo:2:2,snmp|8|11|d'    # 指标和指标之间可以用逗号","做分隔符；
$ ssar -o "dev|eth0:|2|d;snmp:8:11:d"  # 指标和指标之间可以用分号";"做分隔符；
```

　　指标间的分隔符可以是分号“;”，也可以是逗号","。指标内的分隔符可以是冒号“:”，也可以是竖线"|"。指标说明：

* 指标“meminfo:2:2”表示meminfo伪文件中第2行的第2列的数据；

* 指标“snmp|8|11|d”表示snmp伪文件中第8行的第11列的数据的增量值，这里d是delta的含义；

* 指标“dev|eth0:|2|d”表示dev伪文件中“eth0:”开头行的第2列的数据的增量值；

　　另外一种用法，可以用key-value对的形式来制定参数值：

```bash
$ ssar -o 'metric=c|cfile=meminfo|line_begin=MemFree:|column=2|alias=free'       # 取meminfo中MemFree值命名free
$ ssar -o 'metric=d:cfile=snmp:line=8:column=13:alias=retranssegs'               # 取snmp中第8行第13列值的差值
$ ssar -o 'metric=c|cfile=loadavg|line=1|column=1|dtype=float|alias=load1'       # 获取load1这种实型数据
$ ssar -o 'metric=c|cfile=loadavg|line=1|column=4|dtype=string|alias=runq_plit'  # 获取2/1251这种字符串信息
$ ssar -o 'metric=d|cfile=stat|line=2|column=2-11|alias=cpu0_{column};'          # 显示cpu0的第2到11列数据 
$ ssar -o 'metric=d|cfile=stat|line=2-17|column=5|alias=idle_{line};'            # 显示cpu0到cpu15的idle数据  
```

　　各个参数key的详细说明：

* cfile=meminfo，这里cfile用来指定日志文件中的后缀名，需要同sys.conf配置文件中的[file]部分的cfile值保持一致；

* line_begin=MemFree:，这里用来指定指标所在行的行首匹配关键字符串，需要保证在整个文件中独一无二；

* line=8，直接指定指标所在的行数，line与line_begin不能同时指定；

* column=13，指定指标在特定行所处的列数，列以空格为分隔符计数；

* metric=c，用于指定按以上规则获取到值后，是直接输出，还是取相邻时间差值输出，可能的值只有c和d，c表示直接输出，d表示取差值；

* dtype=float，用于指定按以上规则获取的值的类型，可以是int整型、float实型和string字符串型，不指定默认是int类型，其中string类型不支持metric=d的情况；

* alias=free，用于指定输出中本指标的标题名或json格式中的key名；

* width=13，用于指定本指标列的格式宽度，不指定默认为10字节宽度；

* position=a，有些情况下line_begin指定的行首关键字所在行数，在不同时间频繁变化，需要使用position来动态判断行数，此时值设置为a（auto），默认不指定值为f，将只判断一次所在行数。

<a name="整机实时模式说明"/>
## 4.5、整机实时模式说明

　　与其他sar工具一样，ssar也支持实时模式。实时模式的开始时刻永远是当前时刻，我们用-f选项值前面的加号表示开启实时模式。

```bash
$ ssar -f +10                   # 表示实时模式的结束时刻是10分钟之后，因此会持续输出10分钟后结束；
$ ssar -f +10 --api             # --api表示实时模式情况下，以json格式输出；
$ ssar -f +10 -i 1s             # -i 1s表示实时模式情况下，采集输出精度是1秒，默认值是5秒；
```

　　有些场景不方便直接部署和安装ssar工具，可以将ssar命令直接分发到相关机器上，此时可以直接在live模式下运行ssar命令：

```bash
$ which ssar                    # 获取到ssar命令所处路径/usr/local/bin/ssar，拷贝到目标机
$ ./ssar -f +10 -i 1s -o 'metric=d|src_path=/proc/net/snmp|line=8|column=13|alias=retranssegs'  # 在目标机执行
```

　　此时需要使用src_path=/proc/net/snmp替代cfile直接指定数据源所处的绝对路径即可。

<a name="多进程指标"/>
# 五、多进程指标

<a name="多进程参数说明"/>
## 5.1、多进程参数说明

　　多进程指标以procs为子命令，运行ssar procs --help即可获取帮助信息，下面以实例形式分别介绍。

```bash
$ ssar procs --help                   # 获取整机指标帮助信息
$ ssar procs -f 2020-09-29T18:34:00   # 指定时间区间的结束时间点，默认为当前时刻
$ ssar procs -f -10                   # 指定时间区间的结束时间点为当前时刻之前10分钟
$ ssar procs -f -5.5h                 # 指定时间区间的结束时间点为当前时刻之前5.5小时
$ ssar procs -f -1.8d                 # 指定时间区间的结束时间点为当前时刻之前1.8天
$ ssar procs -r 60                    # 指定时间区间跨度为60分钟，默认值为5分钟
$ ssar procs -b 2020-09-29T17:34:00   # 指定时间区间的开始时间点，默认为结束时间点减时间长度
$ ssar procs -H                       # 选中则不显示指标的标题信息
$ ssar procs -k -ppid,+sid,pid        # 指定排序字段，优先以ppid降序，其次以sid升序，最后以pid升序(内建)
$ ssar procs -l 100                   # 超过100条信息，仅显示100条结果
$ ssar procs --api                    # 选中则以json格式输出信息
```

<a name="多进程视图说明"/>
## 5.2、多进程视图说明

　　ssar工具多进程也针对用户使用提供了几个常用的视图（view）。目前已经提供的视图有如下几个。

```bash
$ ssar procs                          # 进程默认指标组合
$ ssar procs --cpu                    # 进程CPU类指标组合
$ ssar procs --mem                    # 进程内存类指标组合
$ ssar procs --job                    # 进程组类指标组合
$ ssar procs --sched                  # 进程优先级类指标组合
```

　　以cpu视图为例，默认包含pid、ppid、cputime、cpu_utime、cpu_stime、pcpu、pucpu、pscpu、vcswchs、ncswchs、s和cmd指标信息。

```bash
$ ssar procs --cpu

    pid    ppid       cputime     cpu_utime     cpu_stime    pcpu   pucpu   pscpu vcswchs ncswchs s cmd             
 167667       1  102T09:05:39   95T04:57:49    7T04:07:50    61.3    61.3     0.0     948       0 S pserver 
 250077       1   17T21:59:03   12T08:09:56    5T13:49:07     7.3     6.7     0.7      10       0 S dun       
  20710       1    5T21:12:44    4T11:45:09    1T09:27:35     3.0     2.7     0.3       0       0 S sensor     
 215730       1      13:43:14       8:06:53       5:36:21     2.7     1.7     1.0       0       0 S agent      
  52713       1    2T12:14:47    1T08:44:22    1T03:30:25     1.7     1.0     0.7       1       0 S service 
```

<a name="多进程指标组合"/>
## 5.3、多进程指标组合

　　如果已有的视图不能很好的满足需求，我们也可以自己组合既有指标(indicator)进行输出。

```bash
$ ssar procs -o pid,pcpu,cmd          # 只输出pid、pcpu和cmd三个指标
$ ssar procs --cpu -O cmdline         # 在cpu视图的指标基础上，追加输出cmdline指标
```

　　这里同样要区分小o和大O，小o表示单独输出，大O表示追加输出。

<a name="多进程指标说明"/>
## 5.4、多进程指标说明

　　多进程指标目前有如下50个，具体如下表：

<table>
    <thead>
        <tr>
            <th width="12%">指标名</th>
            <th width="8%">属性</th>
            <th width="5%">排序</th>
            <th width="15%">示例</th>
            <th width="60%">含义说明</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td>pid</td>
            <td>数值</td>
            <td>增序</td>
            <td>96540</td>
            <td>进程ID</td>
        </tr>
        <tr>
            <td>tgid</td>
            <td>数值</td>
            <td>增序</td>
            <td>96540</td>
            <td>同pid</td>
        </tr>
        <tr>
            <td>ppid</td>
            <td>数值</td>
            <td>增序</td>
            <td>1</td>
            <td>父进程ID</td>
        </tr>
        <tr>
            <td>pgid</td>
            <td>数值</td>
            <td>增序</td>
            <td>91631</td>
            <td>进程组ID</td>
        </tr>
        <tr>
            <td>sid</td>
            <td>数值</td>
            <td>增序</td>
            <td>91631</td>
            <td>进程会话ID</td>
        </tr>
        <tr>
            <td>tpgid</td>
            <td>数值</td>
            <td>增序</td>
            <td>-1</td>
            <td>tty pgid</td>
        </tr>
        <tr>
            <td>nlwp</td>
            <td>数值</td>
            <td>降序</td>
            <td>1</td>
            <td>进程的线程数</td>
        </tr>
        <tr>
            <td>flags</td>
            <td>字符串</td>
            <td>降序</td>
            <td>0</td>
            <td>进程Flag</td>
        </tr>
        <tr>
            <td>s</td>
            <td>字符串</td>
            <td>降序</td>
            <td>S</td>
            <td>进程状态</td>
        </tr>
        <tr>
            <td>sched</td>
            <td>字符串</td>
            <td>降序</td>
            <td>0</td>
            <td>进程调度</td>
        </tr>
        <tr>
            <td>cls</td>
            <td>字符串</td>
            <td>增序</td>
            <td>TS</td>
            <td>进程调度算法</td>
        </tr>
        <tr>
            <td>nice</td>
            <td>数值</td>
            <td>降序</td>
            <td>0</td>
            <td>NICE值</td>
        </tr>
        <tr>
            <td>rtprio</td>
            <td>数值</td>
            <td>降序</td>
            <td>0</td>
            <td>实时进程优先级</td>
        </tr>
        <tr>
            <td>prio</td>
            <td>数值</td>
            <td>降序</td>
            <td>120</td>
            <td>进程优先级</td>
        </tr>
        <tr>
            <td>vcswch</td>
            <td>数值</td>
            <td>降序</td>
            <td>530758</td>
            <td>主动上下文切换次数累计值</td>
        </tr>
        <tr>
            <td>ncswch</td>
            <td>数值</td>
            <td>降序</td>
            <td>25427</td>
            <td>被动上下文切换次数累计值</td>
        </tr>
        <tr>
            <td>vcswchs</td>
            <td>数值</td>
            <td>降序</td>
            <td>0</td>
            <td>主动上下文切换次数增量值</td>
        </tr>
        <tr>
            <td>ncswchs</td>
            <td>数值</td>
            <td>降序</td>
            <td>0</td>
            <td>被动上下文切换次数增量值</td>
        </tr>
        <tr>
            <td>etimes</td>
            <td>数值</td>
            <td>降序</td>
            <td>769579</td>
            <td>进程持续自然时间，单位秒</td>
        </tr>
        <tr>
            <td>etime</td>
            <td>字符串</td>
            <td>降序</td>
            <td>8T21:46:19</td>
            <td>进程持续自然时间，人类可读格式</td>
        </tr>
        <tr>
            <td>start_time</td>
            <td>数值</td>
            <td>增序</td>
            <td>1600445804</td>
            <td>进程启动时间戳</td>
        </tr>
        <tr>
            <td>start_datetime</td>
            <td>字符串</td>
            <td>增序</td>
            <td>2020-09-19T00:16:44</td>
            <td>进程启动时间，datetime格式</td>
        </tr>
        <tr>
            <td>time</td>
            <td>数值</td>
            <td>降序</td>
            <td>5104</td>
            <td>当前的累计CPU时间，单位秒</td>
        </tr>
        <tr>
            <td>utime</td>
            <td>数值</td>
            <td>降序</td>
            <td>4095</td>
            <td>当前时刻的用户空间累计CPU时间，单位秒</td>
        </tr>
        <tr>
            <td>stime</td>
            <td>数值</td>
            <td>降序</td>
            <td>1009</td>
            <td>当前时刻的内核空间累计CPU时间，单位秒</td>
        </tr>
        <tr>
            <td>time_dlt</td>
            <td>数值</td>
            <td>降序</td>
            <td>2</td>
            <td>当前的增量CPU时间，单位秒</td>
        </tr>
        <tr>
            <td>utime_dlt</td>
            <td>数值</td>
            <td>降序</td>
            <td>2</td>
            <td>当前时刻的用户空间增量CPU时间，单位秒</td>
        </tr>
        <tr>
            <td>stime_dlt</td>
            <td>数值</td>
            <td>降序</td>
            <td>0</td>
            <td>当前时刻的内核空间增量CPU时间，单位秒</td>
        </tr>
        <tr>
            <td>cputime</td>
            <td>字符串</td>
            <td>降序</td>
            <td>1:25:04</td>
            <td>当前时刻的累计CPU时间，人类可读格式</td>
        </tr>
        <tr>
            <td>cpu_utime</td>
            <td>字符串</td>
            <td>降序</td>
            <td>1:08:15</td>
            <td>当前时刻的用户空间累计CPU时间，人类可读格式</td>
        </tr>
        <tr>
            <td>cpu_stime</td>
            <td>字符串</td>
            <td>降序</td>
            <td>0:16:49</td>
            <td>当前时刻的内核空间累计CPU时间，人类可读格式</td>
        </tr>
        <tr>
            <td>pcpu</td>
            <td>数值</td>
            <td>降序</td>
            <td>0.7</td>
            <td>对照时刻到当前时刻的CPU利用率</td>
        </tr>
        <tr>
            <td>pucpu</td>
            <td>数值</td>
            <td>降序</td>
            <td>0.7</td>
            <td>对照时刻到当前时刻的用户空间CPU利用率</td>
        </tr>
        <tr>
            <td>pscpu</td>
            <td>数值</td>
            <td>降序</td>
            <td>0.0</td>
            <td>对照时刻到当前时刻的内核空间CPU利用率</td>
        </tr>
        <tr>
            <td>size</td>
            <td>数值</td>
            <td>降序</td>
            <td>212712</td>
            <td>进程虚拟内存，单位kb</td>
        </tr>
        <tr>
            <td>rss</td>
            <td>数值</td>
            <td>降序</td>
            <td>21508</td>
            <td>进程物理内存，单位kb</td>
        </tr>
        <tr>
            <td>rss_dlt</td>
            <td>数值</td>
            <td>降序</td>
            <td>0</td>
            <td>进程物理内存增量，单位kb</td>
        </tr>
        <tr>
            <td>maj_flt</td>
            <td>数值</td>
            <td>降序</td>
            <td>28934512</td>
            <td>主缺页中断次数</td>
        </tr>
        <tr>
            <td>min_flt</td>
            <td>数值</td>
            <td>降序</td>
            <td>0</td>
            <td>次缺页中断次数</td>
        </tr>
        <tr>
            <td>maj_dlt</td>
            <td>数值</td>
            <td>降序</td>
            <td>0</td>
            <td>主缺页中断增量次数</td>
        </tr>
        <tr>
            <td>min_dlt</td>
            <td>数值</td>
            <td>降序</td>
            <td>9180</td>
            <td>次缺页中断增量次数</td>
        </tr>
        <tr>
            <td>shr</td>
            <td>数值</td>
            <td>降序</td>
            <td>7384</td>
            <td>进程物理共享内存，单位kb</td>
        </tr>
        <tr>
            <td>shr_dlt</td>
            <td>数值</td>
            <td>降序</td>
            <td>0</td>
            <td>进程物理共享内存增量，单位kb</td>
        </tr>
        <tr>
            <td>cmd</td>
            <td>字符串</td>
            <td>增序</td>
            <td>monitor</td>
            <td>命令名称</td>
        </tr>
        <tr>
            <td>cmdline</td>
            <td>字符串</td>
            <td>增序</td>
            <td>见说明</td>
            <td>/user/bin/python2.7 /monitor/monitor 命令全路径，包含参数部分</td>
        </tr>
        <tr>
            <td>fullcmd</td>
            <td>字符串</td>
            <td>增序</td>
            <td>/user/bin/python2.7</td>
            <td>命令全路径，不包含参数部分</td>
        </tr>
        <tr>
            <td>b</td>
            <td>字符串</td>
            <td>增序</td>
            <td>=</td>
            <td>对照时刻状态，<为不存在，=为存在</td>
        </tr>
        <tr>
            <td>begin_datetime</td>
            <td>字符串</td>
            <td>增序</td>
            <td>2020-09-29T21:58:04</td>
            <td>对照时刻，日期时间格式</td>
        </tr>
        <tr>
            <td>finish_datetime</td>
            <td>字符串</td>
            <td>增序</td>
            <td>2020-09-29T22:03:04</td>
            <td>当前时刻，日期时间格式</td>
        </tr>
        <tr>
            <td>realtime</td>
            <td>数值</td>
            <td>降序</td>
            <td>300</td>
            <td>进程持续的自然时间</td>
        </tr>
    </tbody>
</table>

<a name="单进程指标"/>
# 六、单进程指标

<a name="单进程参数说明"/>
## 6.1、单进程参数说明

　　单进程指标以proc为子命令，运行ssar proc --help即可获取帮助信息，下面以实例形式分别介绍。

```bash
$ ssar proc --help                        # 获取整机指标帮助信息
$ ssar proc -p 1                          # 指定需要展示信息的进程ID，为必选参数
$ ssar proc -p 1 -f 2020-09-29T18:34:00   # 指定时间区间的结束时间点，默认为当前时刻
$ ssar proc -p 1 -f -10                   # 指定时间区间的结束时间点为当前时刻之前10分钟
$ ssar proc -p 1 -f -5.5h                 # 指定时间区间的结束时间点为当前时刻之前5.5小时
$ ssar proc -p 1 -f -1.8d                 # 指定时间区间的结束时间点为当前时刻之前1.8天
$ ssar proc -p 1 -r 60                    # 指定时间区间跨度为60分钟，默认值为300分钟
$ ssar proc -p 1 -b 2020-09-29T17:34:00   # 指定时间区间的开始时间点，默认为结束时间点减时间长度
$ ssar proc -p 1 -i 1                     # 指定时间区间长度范围内展示精度为1分钟，默认为10分钟
$ ssar proc -p 1 -H                       # 选中则不显示指标的标题信息
$ ssar proc -p 1 -P                       # 选中则不显示- * < > =等非数字指标
$ ssar proc -p 1 --api                    # 选中则以json格式输出信息
```

<a name="单进程视图说明"/>
## 6.2、单进程视图说明

　　ssar工具单进程也针对用户使用提供了几个常用的视图（view）。目前已经提供的视图有如下几个。

```bash
$ ssar proc -p 1                          # 进程默认指标组合
$ ssar proc -p 1 --cpu                    # 进程CPU类指标组合
$ ssar proc -p 1 --mem                    # 进程内存类指标组合
```

　　以cpu视图为例，默认包含collect_datetime、etime、cputime、cpu_utime、cpu_stime、pcpu、pucpu、pscpu、vcswchs、ncswchs、s和cmd指标信息。  

```bash
$ ssar proc -i 1 -p 224360 --cpu

collect_datetime            etime       cputime     cpu_utime     cpu_stime    pcpu   pucpu   pscpu vcswchs ncswchs s cmd             
2021-08-21T11:40:00             <             <             <             <       <       <       <       <       < < <               
2021-08-21T11:41:00       0:00:07       0:00:00       0:00:00       0:00:00       -       -       -       -       - S java            
2021-08-21T11:42:00       0:01:07       0:00:19       0:00:17       0:00:02    31.7    28.3     3.3       0       0 S java            
2021-08-21T11:43:00       0:02:07       0:00:23       0:00:20       0:00:03     6.7     5.0     1.7       0       0 S java            
2021-08-21T11:44:00       0:03:07       0:00:27       0:00:23       0:00:04     6.7     5.0     1.7       0       0 S java            
2021-08-21T11:45:00       0:04:07       0:00:30       0:00:26       0:00:04     5.0     5.0     0.0       0       0 S java            
2021-08-21T11:46:00       0:05:07       0:00:34       0:00:29       0:00:05     6.7     5.0     1.7       0       0 S java            
2021-08-21T11:47:00       0:06:07       0:00:38       0:00:32       0:00:06     6.7     5.0     1.7       0       0 S java            
2021-08-21T11:48:00       0:07:07       0:00:41       0:00:34       0:00:07     5.0     3.3     1.7       0       0 S java            
2021-08-21T11:49:00       0:08:07       0:00:48       0:00:40       0:00:08    11.7    10.0     1.7       0       0 S java            
2021-08-21T11:50:00       0:09:07       0:00:51       0:00:42       0:00:09     5.0     3.3     1.7       0       0 S java            
2021-08-21T11:51:00             -             -             -             -       -       -       -       -       - - -            
2021-08-21T11:52:00             -             -             -             -       -       -       -       -       - - -            
2021-08-21T11:53:00       0:12:07       0:01:03       0:00:51       0:00:12       -       -       -       0       0 S java            
2021-08-21T11:54:00       0:13:07       0:01:07       0:00:54       0:00:13     6.7     5.0     1.7       0       0 S java            
2021-08-21T11:55:00       0:14:07       0:01:11       0:00:57       0:00:14     6.7     5.0     1.7       0       0 S java            
2021-08-21T11:56:00             >             >             >             >       >       >       >       >       > > >    
```

　　这里对输出结果中的关键点做一些说明：

* 第一列是指标采集的时间点信息，精确到秒。

* 如果某个采集时间点未采集到数据，则显示为“-”，对于增量数据，如果其前一个时间点未采集到数据，则不论当前时间点是否采集到数据，也显示"-"。

* 如果显示<，则表示进程从当前采集时间和下一个采集时间的周期内启动的。

* 如果显示>，则表示进程前一个采集时间和从当前采集时间的周期内结束的。

<a name="单进程指标组合"/>
## 6.3、单进程指标组合

　　如果已有的视图不能很好的满足需求，我们也可以自己组合既有指标(indicator)进行输出。

```bash
$ ssar proc -p 1 -o pid,pcpu,cmd          # 只输出pid、pcpu和cmd三个指标
$ ssar proc -p 1 --cpu -O cmdline         # 在cpu视图的指标基础上，追加输出cmdline指标
```

　　这里同样要区分小o和大O，小o表示单独输出，大O表示追加输出。

<a name="单进程指标说明"/>
## 6.4、单进程指标说明

　　单进程指标与多进程指标大体相同，参见3.4小结表格。区别是单进程指标多了一个collect_datetime指标。

<a name="整机load指标"/>
# 七、整机load指标

<a name="整机load参数说明"/>
## 7.1、整机load参数说明

　　整机load指标以load5s为子命令，运行ssar load5s --help即可获取帮助信息，下面以实例形式分别介绍

```bash
$ ssar load5s --help                   # 获取整机指标帮助信息
$ ssar load5s                          # 指定需要展示信息的进程ID，为必选参数
$ ssar load5s -f 2020-09-29T18:34:00   # 指定时间区间的结束时间点，默认为当前时刻
$ ssar load5s -f -10                   # 指定时间区间的结束时间点为当前时刻之前10分钟
$ ssar load5s -f -5.5h                 # 指定时间区间的结束时间点为当前时刻之前5.5小时
$ ssar load5s -f -1.8d                 # 指定时间区间的结束时间点为当前时刻之前1.8天
$ ssar load5s -r 60                    # 指定时间区间跨度为60分钟，默认值为300分钟
$ ssar load5s -b 2020-09-29T17:34:00   # 指定时间区间的开始时间点，默认为结束时间点减时间长度
$ ssar load5s -y                       # 选中则只显示触发load5s高阈值的
$ ssar load5s -z                       # 选中则只显示触发load详情采集的
$ ssar load5s -H                       # 选中则不显示指标的标题信息
$ ssar load5s --api                    # 选中则以json格式输出信息
```

<a name="整机load指标说明"/>
## 7.2、整机load指标说明

　　整机load指标包含如下信息。  

```bash
$ ssar load5s

collect_datetime       threads    load1   runq load5s stype sstate zstate    act act_rto   actr actr_rto   actd 
2021-08-20T17:27:50      28608     3.47      1      1 5s    N      U           -       -      -        -      - 
2021-08-20T17:27:55      28609     3.27      2      1 5s    N      U           -       -      -        -      - 
2021-08-20T17:28:00      28613     3.25      3      3 5s    N      U           -       -      -        -      - 
2021-08-20T17:28:05      27515    90.67      4   1095 5s    Y      Z         884  80.73%      4    100.0%   880 
2021-08-20T17:28:10      27510    83.65      2      3 5s    N      U           -       -      -        -      - 
2021-08-20T17:28:15      27509    76.95      2      0 5s    N      U           -       -      -        -      - 
2021-08-20T17:28:20      27504    70.87      5      1 5s    N      U           -       -      -        -      - 
2021-08-20T17:28:25      27503    65.28      2      1 5s    N      U           -       -      -        -      - 
2021-08-20T17:28:30      27502    60.05      1      0 5s    N      U           -       -      -        -      - 
2021-08-20T17:28:35      27507    55.72      6      6 5s    N      U           -       -      -        -      - 
2021-08-20T17:28:40      27502    51.26      2      0 5s    N      U           -       -      -        -      - 
```

　　各字段含义如下：

* 字段collect_datetime：整机load发生更新的时刻，社区标准内核更新周期是5秒。

* 字段threads：当前时刻的整机线程数。

* 字段load1：当前时刻的整机load1值。

* 字段runq：当前时刻的整机runq值，即内核统计的R状态线程数。

* 字段load5s：此值为ssar独家指标，反应的是当前时刻系统真实的load压力大小，比load1更准确。

* 字段stype：当前时刻load5s值的类型，正常为5s。

* 字段sstate： 值为Y表示当前load5s值触发load高阈值，N为未触发。

* 字段zstate：值为Z表示当前load5s值触发load高阈值且触发load详情采集，U为未触发详情采集。

* 字段act：如果触发采集，此值为actr和actd之和；否则值为-。

* 字段act_rto：表示act对load5s的代表性，正常情况下应该接近100%，否则load详情信息的代表性较差。

* 字段actr：如果触发采集，此值为成功采集的R状态线程的个数；否则值为-。

* 字段actr_rto：表示actr对runq的代表性，正常情况下应该接近100%，否则load详情信息的代表性较差。

* 字段actd：如果触发采集，此值为成功采集的D状态线程的个数；否则值为-。

<a name="特异指标load5s深入说明"/>
## 7.3、特异指标load5s深入说明

　　从字面意思来说，很多资料解释load1就是1分钟的load均值，很多资料解释load5就是5分钟的load均值，很多资料解释load15就是15分钟的load均值。那么load5s按此逻辑理解，可以初步解释为5秒钟的load均值。

　　了解内核的同学都知道内核里的active值，这个load5s就是active值。所以，load5s的准确理解是每隔5秒左右的一个R和D状态线程数的刻度值。

　　不了解内核的同学可以通过下面这个比喻来理解。load5s就是油门大小，load1就是车速。真正对操作系统产生压力的不是车速（load1）大小，是油门（load5s）大小。在车速（load1）很低情况下，即使加一个很大的油门（load5s）,车速（load1）也要经过一点时间才能提速，不会瞬间切变。这里说明即使车速（load1）不是特别高，并不代表此时此刻油门（load5s）不大。在车速（load1）较高的情况下，如果没有油门（load5s）了，车速（load1）会缓慢下降。这里说明即使车速（load1）依然很高，但油门（load5s）可能已经没有了。要保持一个较高的车速（load1），就需要不断给一个恰当油门（load5s）。

　　最后说一下ssar load5s没有显示load5和load15这2个指标。上面说了active值（即load5s）是一个刻度值，只能反映那5秒钟的机器压力情况。历史上，linux通过load1、load5和load15来把过去若干分钟内的active值通过适当的加权后，在当前时刻展示出来。可是一旦有了各种sar工具之后，我们就已经记录了过去各个时间点的load1、load5和load15值了，至少我们已经不用在通过load5和load15来观察5分钟或15分钟前的情况了，因为load1的历史数据都已经更加清晰的表达了。如今有了load5s值之后，就更没有load5和load15存在的必要性了。

<a name="Load详情指标"/>
# 八、Load详情指标

<a name="Load详情参数说明"/>
## 8.1、Load详情参数说明

　　整机load指标以load2p为子命令，运行ssar load2p --help即可获取帮助信息，下面以实例形式分别介绍。

```bash
$ ssar load2p --help                                 # 获取Load详情帮助信息
$ ssar load2p -c 2020-10-07T07:45:25                 # 指定需要Load详情的时间点，为必选参数
$ ssar load2p -c 2020-10-07T07:45:25 -l 10           # 针对每一个视图，超过10条信息仅显示10条
$ ssar load2p -c 2020-10-07T07:45:25 -H              # 选中则不显示指标的标题信息
$ ssar load2p -c 2020-10-07T07:45:25 --api           # 选中则以json格式输出信息
$ ssar load2p -c 2020-10-07T07:45:25 --loadd         # 只显示loadd视图 
$ ssar load2p -c 2020-10-07T07:45:25 --stack         # 只显示stack视图
$ ssar load2p -c 2020-10-07T07:45:25 --loadd --stack # 同时显示loadd和stack视图
```

　　特别说明：-c选项的时间参数值，只能是上文中ssar load5s -z命令输出结果中的collect_datetime值选择，其他collect_datetime值作为-c选项参数无法显示结果。

<a name="Load详情聚合指标说明"/>
## 8.2、Load详情聚合指标说明

　　详情聚合视图一共4个loadr、loadd、psr和stackinfo，下面分别介绍：

* 视图loadr，是对R状态线程采集数actr的详细展示。第一列count值是对s和pid两列进行聚合的值。下面第一行表示R状态的pid为60220的线程数为32，后面cmd和cmdline是pid 60220的详细进程名信息。其中第一列所有行count值之和等于前面load5s子命令中同一时刻的actr值。

```bash
$ ssar load2p -c 2021-08-21T06:45:40 --loadr

   count s     pid cmd             cmdline 
      32 R   60220 sensor          /user/bin/sensor 
       5 R   15639 worker          /user/bin/worker      
       4 R    8466 executor        /user/bin/executor
       1 R   43568 start           /home/tops/bin/python2.7 /opt/local_run/2589817/start 
```

* 视图loadd，是对D状态线程采集数actd的详细展示。第一列count值是对s和pid两列进行聚合的值。下面第一行表示D状态的pid为230576的线程数为21，后面cmd和cmdline是pid 230576的详细进程名信息。其中第一列所有行count值之和等于前面load5s子命令中同一时刻的actd值。

```bash
$ ssar load2p -c 2021-08-20T14:45:04 --loadd

   count s     pid cmd             cmdline 
      21 D  230576 ps              ps axo pid,ppid,command      
      19 D  231358 logagent        /opt/plugins/logagent      
      13 D  231930 pidof           /sbin/pidof     
       7 D  232619 hostinfo        /sbin/hostinfo
       3 D  233153 crond           /sbin/crond
```

* 视图psr，对loadr信息的补充。主要反映CPU调度不均和cgroup绑核不均问题。第一行表示运行在34号CPU上的R状态线程有150个。

```bash
ssar load2p -c 2021-08-20T14:41:49 --psr

   count psr 
     150  34 
       6  59 
       5  43 
       5  48 
```

* 视图stackinfo，是对D状态线程的另外一个角度的展示。第一行表示D状态线程中调用栈为call_rwsem_down_read_failed,__do_page_fault,do_page_fault,page_fault的线程数占总体D状态线程数的百分比是80%。其中栈顶函数为call_rwsem_down_read_failed，栈顶函数符号的后6位为3003d4。

```bash
$ ssar load2p -c 2021-08-20T14:41:49 --stackinfo

   count s_unit  nwchan wchan                            stackinfo 
      80 percent 3003d4 call_rwsem_down_read_failed      call_rwsem_down_read_failed,__do_page_fault,do_page_fault,page_fault 
      12 percent 22727e ep_poll                          ep_poll,SyS_epoll_wait,system_call_fastpath 
       5 percent 2ed118 jbd2_journal_commit_transaction  jbd2_journal_commit_transaction,kjournald2,kthread,ret_from_fork 
       3 percent 300403 call_rwsem_down_write_failed     call_rwsem_down_write_failed,vm_mmap_pgoff,SyS_mmap_pgoff,SyS_mmap,system_call_fastpath
```

<a name="Load详情详细指标说明"/>
## 8.3、Load详情详细指标说明

　　各详情详细指标视图一共2个loadrd和stack，上面4个聚合视图主要由详情指标聚合而成。下面分别介绍：

* 视图loadrd详细信息，每一行是一个线程信息。第一列只可能是R或D，第二列tid为线程ID，第三列pid为进程ID，第四列psr是当前线程调度的CPU核号，第五列prio为线程调度优先级，第六列cmd为进程名称。

```bash
$ ssar load2p -c 2021-08-20T14:41:49 --loadrd 

s     tid     pid psr prio cmd             
D  259896  161396  17  120 pidof 
R  259844  161324   7  120 sensor 
D  259045  158797  11  120 hostinfo 
D  258261  155965  21  120 ps  
R  258260  155965  63  120 worker 
D  258243  190249  38  120 crond
R  258231  190367  22  120 start 

```

* 视图stack，每一行是一个抽样的D状态线程信息。当前面load5s子命令中同一时刻的actd值小于100时，这里的行数等于actd值，当actd值大于100时，这里的行数为100。这100个D状态线程为从atcd数量中随机抽样得到。因此我们这100行调用栈的分布比例对整体具有代表性。

```bash
    tid     pid cmd             nwchan wchan                            stackinfo 
   7684    7684 jbd2/sda5       2ed118 jbd2_journal_commit_transaction  jbd2_journal_commit_transaction,kjournald2,kthread,ret_from_fork 
 167978  160971 hostinfo        22727e ep_poll                          ep_poll,SyS_epoll_wait,system_call_fastpath 
 193413  190401 crond           300403 call_rwsem_down_write_failed     call_rwsem_down_write_failed,vm_mmap_pgoff,SyS_mmap_pgoff,SyS_mmap,system_call_fastpath 
 187916  186035 logagent        3003d4 call_rwsem_down_read_failed      call_rwsem_down_read_failed,__do_page_fault,do_page_fault,page_fault 
  49495  162115 pidof           3003d4 call_rwsem_down_read_failed      call_rwsem_down_read_failed,__do_page_fault,do_page_fault,page_fault 
 188437  186035 logagent        3003d4 call_rwsem_down_read_failed      call_rwsem_down_read_failed,__do_page_fault,do_page_fault,page_fault
```

<a name="数据采集器sresar"/>
# 九、数据采集器sresar

<a name="ssar工具简介"/>
## 9.1、ssar工具简介

　　ssar工具整体上分为3部分：数据采集器进程sresar、通用查询器进程ssar、增强查询器进程ssar+和古典查询器tsar2。

* 进程sresar以守护进程模式常驻，负责周期性的采集系统数据。数据存储在文件系统路径/var/log/sre_proc（具体通过配置文件配置）。

* 进程ssar读取采集的数据，负责为用户提供通用的数据查询功能。

* 进程ssar+是基于python语言的脚本程序，通过调用ssar返回的基础数据可以实现功能更加丰富的数据查询功能，也方便使用者开展更加贴近业务的二次开发。

* 进程tsar2是基于python语言的脚本程序，通过调用ssar返回的基础数据实现的兼容tsar的查询功能。

<a name="ssar工作目录"/>
## 9.2、ssar工作目录

　　ssar工具的工作目录由配置文件/etc/ssar/ssar.conf中的work_path='/var/log'选项配置，如配置文件被删除，则默认为/var/log/目录生效。work_path路径下是项目根目录sre_proc及其下子目录和文件，具体如下：

```bash
sre_proc/                                             # 工具根目录
sre_proc/log/                                         # 工具日志存储目录
sre_proc/log/2020093021_sresar.log                    # 工具日志文件，按小时存储
sre_proc/data/                                        # 工具数据存储目录
sre_proc/data/2020093021/                             # 工具数据存储子目录，按小时存储
sre_proc/data/2020093021/20200930215700_sresar24.gz   # 进程数据，按分钟存储，压缩格式
sre_proc/data/2020093021/2020093021_load5s            # 整机load数据，按小时存储
sre_proc/data/2020093021/20200930215919_loadrd        # 整机load详情数据，按需存储
sre_proc/data/2020093021/20200930215919_stack         # D状态调用栈数据，按需存储
sre_proc/data/2020093021/20200930215900_stat          # 除以上外，皆为整机扩展指标数据文件
```

<a name="数据采集器通用选项配置"/>
## 9.3、数据采集器通用选项配置

　　数据采集器通用选项配置在配置文件ssar.conf的main部分，具体如下：

```bash
$ cat /etc/ssar/ssar.conf
[main]
duration_threshold=168                           # 采集数据保存168个小时，最老的数据会被自动清理
inode_use_threshold=90                           # 数据所在分区磁盘inode大于90%开始清理老数据，无法清理时则停止采集
disk_use_threshold=90                            # 数据所在分区磁盘容量大于90%开始清理老数据，无法清理时则停止采集
disk_available_threshold=10000                   # 数据所在分区磁盘容量小于10G开始清理老数据，无法清理时则停止采集
duration_restart=10                              # 每10小时守护进程重启一次，无特别业务意义
work_path='/var/log'                             # 工具的工作目录
scatter_second=0                                 # 分钟级采集数据的打散偏移秒数，删除则随机产生。
load5s_flag=false                                # 单独关闭load信息采集，缺省打开采集
proc_flag=false                                  # 单独关闭进程级信息采集，缺省打开采集
sys_flag=false                                   # 独关闭整机指标信息采集，缺省打开采集
```

<a name="数据采集器load选项配置"/>
## 9.4、数据采集器load选项配置

　　数据采集器load选项配置在配置文件ssar.conf的load部分，具体如下：

```bash
$ cat /etc/ssar/ssar.conf
[load]
load5s_threshold=4                   # 当load5s类型是5s，且大于CPU核数4倍时触发load详情采集。
load1m_threshold=4                   # 当load5s类型是1m，且大于CPU核数4倍时触发load详情采集。
realtime_priority=2                  # load详情采集线程优先级，值域[1-99]实时线程，0表示普通线程
stack_sample_disable=true            # 打开采集所有D状态线程调用栈，默认随机采集
```

<a name="数据采集器整机选项配置"/>
## 9.5、数据采集器整机选项配置

　　数据采集器整机选项配置在配置文件sys.conf的file部分，具体如下：

```bash
$ cat /etc/ssar/sys.conf
[file]
default=[
    {src_path='buddyinfo',         turn=true},
    {src_path='meminfo',           turn=true},
    {src_path='vmstat',            turn=true},
    {src_path='stat',              turn=true},
    {src_path='diskstats',         turn=true},
    {src_path='/proc/net/dev',     turn=true},
    {src_path='/proc/net/netstat', turn=true},
    {src_path='/proc/net/snmp',    turn=true},
    {src_path='softirqs'},
    {src_path='schedstat',         gzip=true},
    {src_path='slabinfo'},
    {src_path='zoneinfo'},
    {src_path='/proc/net/sockstat', cfile='sockstat'},
]

'3.10.0'=[
    {src_path='/proc/net/sockstat3', cfile='sockstat'},
]

'4.9.'=[
    {src_path='/proc/net/sockstat4', cfile='sockstat'},
]
```

　　配置文件详细说明：

* 配置文件采用ini格式的增强格式toml格式，因此这里可以使配置文件的内容更加丰满。

* 支持层叠式配置，有特殊配置的情况特殊配置生效，如“3.10.0-327”，无特殊配置情况default生效。

* 多个特殊配置情况，信息更完整的生效，如“3.10.0-327”会优先“3.10.0-”生效。

* 以snmp部分为例，src_path="/proc/net/snmp"表示采集文件位置为/proc/net/snmp，cfile="snmp"表示采集的文件保存到数据目录的文件名后缀为snmp。

* 如果cfile保存文件的名称和采集文件的名称相同，保存文件名可以省略。如netstat省略了cfile="netstat"。

* 如果采集文件的直接在/proc/目录下，则/proc/可以省略。如src_path="/proc/meminfo"省略为了src_path="meminfo"。

* turn选项可以控制当前采集文件是否开启采集，默认或者设置为false不开启采集，设置为true才开启采集。

* gzip选项可以控制当前采集文件是否采用gzip压缩格式存储，默认或者设置为false不开启压缩，设置为true才开启压缩。

<a name="通用查询器ssar"/>
# 十、通用查询器ssar

<a name="通用查询器整机指标配置"/>
## 10.1、通用查询器整机指标配置

　　通用查询器整机指标配置在配置文件sys.conf的indicator部分，具体如下：

```bash
$ cat /etc/ssar/sys.conf  
[indicator]
[indicator.default]
memfree       = {cfile="meminfo", line=2,                     column=2,  width=10            }
anonpages     = {cfile="meminfo", line_begin="AnonPages:",    column=2,            metric="c"}
shmem         = {cfile="meminfo", line_begin="Shmem:",        column=2,  width=10, metric="c"}
sreclaimable  = {cfile="meminfo", line_begin="SReclaimable:", column=2,  width=10, metric="c"}
sunreclaim    = {cfile="meminfo", line_begin="SUnreclaim:",   column=2,  width=10, metric="c"}
mlocked       = {cfile="meminfo", line_begin="Mlocked:",      column=2,  width=10, metric="c"}
dirty         = {cfile="meminfo", line_begin="Dirty:",        column=2,  width=10, metric="c"}
writeback     = {cfile="meminfo", line_begin="Writeback:",    column=2,  width=10, metric="c"}
activeopens   = {cfile="snmp",    line=8,                     column=6,  width=10, metric="d"}
passiveopens  = {cfile="snmp",    line=8,                     column=7,  width=10, metric="d"}
insegs        = {cfile="snmp",    line=8,                     column=11, width=10, metric="d"}
outsegs       = {cfile="snmp",    line=8,                     column=12, width=10, metric="d"}
estabresets   = {cfile="snmp",    line=8,                     column=9,  width=10, metric="d"}
attemptfails  = {cfile="snmp",    line=8,                     column=8,  width=10, metric="d"}
currestab     = {cfile="snmp",    line=8,                     column=10, width=10, metric="c"}
retranssegs   = {cfile="snmp",    line=8,                     column=13, width=10, metric="d"}
allocstall    = {cfile="vmstat",  line_begin="allocstall",    column=2,  width=10, metric="d"}
pgscan_direct = {cfile="vmstat",  line_begin="pgscan_direct", column=2,  width=10, metric="d"}

[indicator."3.10.0-"]
pgscan_direct = {cfile="vmstat",line_begin="pgscan_direct_normal",column=2,width=10,metric="d"}

[indicator."4.9."]
allocstall    = {cfile="vmstat",line_begin="allocstall_normal",column=2, width=10, metric="d"}
```

　　其中meminfo中的MemFree值和vmstat中的allocstall值，即可以通过配置文件得到简化，即配置文件[indicator.default]部分。：

* 配置项memfree = {file="meminfo", line=2,column=2,  width=10}表示指标名为memfree。其中指标的文件为meminfo（此处和前文的src_path相同），取第2行的第2列。输出字段宽度10个字节，metric默认值为c，即取累积值。

* 配置项allocstall = {file="vmstat", line_begin="allocstall", column=2, width=10, metric="d"}表示指标名为allocstall。其中指标的文件为vmstat，取开头为"allocstall "的行的第2列（注意allocstall后多匹配一个空格）。输出字段宽度10个字节，metric为d，即取delta值。

　　通过配置文件，以上2个指标可以优化为以下方式使用：

```bash
$ ssar -o memfree,allocstall
```

<a name="通用查询器整机视图配置"/>
## 10.2、通用查询器整机视图配置

　　数据采集器整机选项配置在配置文件sys.conf的file部分，具体如下：

```bash
$ cat /etc/ssar/sys.conf  
[view]
mem = ["memfree","anonpages","shmem","sreclaimable","sunreclaim","mlocked","dirty","writeback"]
tcp = ["activeopens","passiveopens","insegs","outsegs","estabresets","attemptfails","currestab","retranssegs"]
vm  = ["pgscan_direct","allocstall"]
```

　　如果实际使用中会经常将多个指标同时输出，可以使用配置文件中的view视图功能，配置项vm = ["pgscan_direct","allocstall"]可以将这2个指标定义为一个试图vm。如下2中方式效果相同。

```bash
$ ssar --vm
$ ssar -o pgscan_direct,allocstall
```

<a name="通用查询器整机指标组配置"/>
## 10.3、通用查询器整机指标组配置

　　如果有些指标在不同内核版本的配置略有区别，可以通过定义指标组实现。比如allocstall指标虽然在3.10内核下是取自vmstat文件的allocstall开头的行的第2列，但是在4.9内核下却取自vmstat文件的allocstall_normal开头的行的第2列。那么我们可以通过增加如下指标组实现4.9内核下的特殊配置。

```bash
[indicator."4.9."]
allocstall    = {cfile="vmstat",line_begin="allocstall_normal",column=2, width=10, metric="d"}
```

<a name="通用查询器整机指标组作用优先级"/>
## 10.4、通用查询器整机指标组作用优先级

　　如果有一台机器的内核release信息是4.9.151-015.x86_64（通过uname -r获取），同时有如下几个指标组，那么指标组针对这个内核版本的作用优先级依次为：

```bash
[indicator."4.9.151-015."]
[indicator."4.9.151-"]
[indicator."4.9."]
[indicator."default"]
```

　　比如我们需要解析allocstall这个指标，当在[4.9.151-015.]这个这个指标组中查找到allocstall后，就其他指标组的allocstall指标定义就不生效了。如果在所有指标组中都没有找到allocstall指标的定义，将抛出指标未定义异常。

<a name="古典查询器tsar2"/>
# 十一、古典查询器tsar2

　　Tsar是业界一款非常经典的sar类型工具，很多同学在日常调查问题中都会经常用到。Tsar2在选项参数和输出格式方面和tsar基本保持一致和兼容。相比较而言，Tsar2有如下3方面的特点：

* Tsar2的数据完整性更强。由于一些原因，当整机处于异常的状态，tsar采集数据的过程可能会受阻，此时会出现连续若干分钟的数据指标缺失的问题。tsar2采用更加鲁棒的采集方式，数据断图的发生概率相比大大降低，将更加有助于诊断机器异常状态的系统问题。

* Tsar2的数据范围适当扩展。在tsar的选项参数框架内，tsar2适当的增强了部分功能，包含3个地方。CPU指标可以精确到每颗CPU的情况，IO指标可以精确到每个磁盘的每个分区的情况，网络流量可以精确到每个网卡的情况，包括物理网卡和虚拟网卡。而且还可以支持多个设备的同时显示。

* 学习和使用Tsar2，有助于增加对Tsar工具的理解。Tsar2选择python语言封装，通过查看tsar2源码可以方便看到各个指标的计算逻辑，也同时增加了对tsar的深入理解。

<a name="时间范围类选项"/>
## 11.1、时间范围类选项

　　Tsar的时间范围类选项一共有4个，分别是--date、--ndays、--watch和--live，这4个选项在内在逻辑上是互斥的，就是说同一个tsar命令里，只有一个能生效。Tsar2为了避免这种混乱，在用户使用时，就强制用户只能选择其中一个选项使用。

<a name="date选项"/>
### 11.1.1、date选项

　　date选项，简写-d，用于指定自然日时间范围，例如--date 20210322。指定后，开始时间为当日0点，结束时间为当日23点59分。如果指定日期为当前日，则结束时间为命令执行时刻。

<a name="ndays选项"/>
#### 11.1.2、ndays选项

　　ndays选项，简写-n，用于指定以当前时刻为基准的n天前时间范围，时间跨度固定为24小时的整倍数。例如--ndays 3，表示开始时刻是现在时刻的3×24小时前，结束时刻是现在时刻。

<a name="watch选项"/>
#### 11.1.3、watch选项

　　watch选项，简写-w，用于指定以当前时刻到n分种前的时间范围。例如--watch 60，表示从60分钟前到当前时刻的时间范围，注意这里仅仅表示时间跨度是60分钟，具体多少条数据显示，还需要结合时间间隔选项。

　　finish选项，简写-f，用于指定查询的时间范围的结束时刻。通常可以和-w选项结合使用，或单独使用也可。
　　如果前面时间范围选项均不选，默认为watch选项生效，且默认参数值为300。

```bash
tsar2 -f -10                         # 当前时刻之前10分钟为结束时刻 
tsar2 -f 31/07/21-15:55              # 指定结束时间
tsar2 -f 2021-07-31T15:55:00         # 指定结束时间 
tsar2 -f 20210731155500              # 指定结束时间
tsar2 -f 202107311555                # 指定结束时间
tsar2 -f 2021073115                  # 指定结束时间
tsar2 -f 20210731                    # 指定结束时间
```

<a name="live选项"/>
#### 11.1.4、live选项

　　live选项，简写-l，用于指定从当前时刻开始到未来时刻的时间范围，直到主动终止进程为止。

<a name="时间间隔选项interval"/>
## 11.2、时间间隔选项interval

　　interval选项，简写-i，用于指定在指定时间范围内，数据指标显示的时间间隔，不指定时，默认值为5。如果--live选项开启，单位为秒，否则单位为分钟。

<a name="数据指标大类选项"/>
## 11.3、数据指标大类选项

　　Tsar的指标大类选项一个11个，除partition选项外，其余10个指标大类tsar2均给予支持，并且保持和tsar的完全兼容。下文详述10个指标大类的详细逻辑，所有相关逻辑也可以通过直接阅读/usr/local/bin/tsar2的python源代码获得。这里用到了一些小节《整机自定义指标说明》语法知识，可以先简单了解下这个小节的内容。

<a name="cpu选项"/>
#### 11.3.1、cpu选项

　　cpu选项的数据源主要来自于/proc/stat伪文件，各个指标的计算逻辑如下：

```bash
$ tsar2 --cpu

## 提取原始指标，例如stat文件cpu字符串开头行第2列值取差值，命名为stat_user
metric=d:cfile=stat:line_begin=cpu:column=2:alias=stat_user
metric=d:cfile=stat:line_begin=cpu:column=3:alias=stat_nice
metric=d:cfile=stat:line_begin=cpu:column=4:alias=stat_system
metric=d:cfile=stat:line_begin=cpu:column=5:alias=stat_idle
metric=d:cfile=stat:line_begin=cpu:column=6:alias=stat_iowait
metric=d:cfile=stat:line_begin=cpu:column=7:alias=stat_irq
metric=d:cfile=stat:line_begin=cpu:column=8:alias=stat_softirq
metric=d:cfile=stat:line_begin=cpu:column=9:alias=stat_steal
metric=d:cfile=stat:line_begin=cpu:column=10:alias=stat_guest
metric=d:cfile=stat:line_begin=cpu:column=11:alias=stat_guest_nice

## 复合运算
stat_cpu_times = stat_user+stat_nice+stat_system+stat_idle+stat_iowait+stat_irq+stat_softirq+stat_steal+stat_guest+stat_guest_nice
user = 100 * stat_user    / stat_cpu_times
sys  = 100 * stat_system  / stat_cpu_times
wait = 100 * stat_iowait  / stat_cpu_times
hirq = 100 * stat_irq     / stat_cpu_times
sirq = 100 * stat_softirq / stat_cpu_times
util = 100 * (stat_cpu_times -stat_idle -stat_iowait -stat_steal)/stat_cpu_times
```
<a name="mem选项"/>
#### 11.3.2、mem选项

　　mem选项的数据源主要来自于/proc/meminfo伪文件，各个指标的计算逻辑如下：

```bash
$ tsar2 --mem

## 提取原始指标，例如meminfo文件MemTotal:字符串开头行第2列值，命名为meminfo_total
metric=c|cfile=meminfo|line_begin=MemTotal:|column=2|alias=meminfo_total
metric=c|cfile=meminfo|line_begin=MemFree:|column=2|alias=meminfo_free
metric=c|cfile=meminfo|line_begin=MemAvailable:|column=2|alias=meminfo_available
metric=c|cfile=meminfo|line_begin=Buffers:|column=2|alias=meminfo_buffers
metric=c|cfile=meminfo|line_begin=Cached:|column=2|alias=meminfo_cached

## 复合运算
total = 1024 * meminfo_total
free  = 1024 * meminfo_free
avail = 1024 * meminfo_available
buff  = 1024 * meminfo_buffers
cach  = 1024 * meminfo_cached
used  = total - free - buff - cach
util  = 100 * used / total
```

<a name="swap选项"/>
#### 11.3.3、swap选项

　　swap选项的数据源主要来自于/proc/meminfo和/proc/vmstat伪文件，各个指标的计算逻辑如下：

```bash
$ tsar2 --swap

## 提取原始指标，例如vmstat文件pswpin字符串开头行第2列值，命名为vmstat_pswpin
metric=d|cfile=vmstat|line_begin=pswpin|column=2|alias=vmstat_pswpin
metric=d|cfile=vmstat|line_begin=pswpout|column=2|alias=vmstat_pswpout
metric=c|cfile=meminfo|line_begin=SwapTotal:|column=2|alias=meminfo_swaptotal
metric=c|cfile=meminfo|line_begin=SwapFree:|column=2|alias=meminfo_swapfree

## 复合运算
swpin  = vmstat_pswpin
swpout = vmstat_pswpout
total  = 1024 * meminfo_swaptotal
free   = 1024 * meminfo_swapfree
util   = 100 - 100 * free / total
```

<a name="tcp选项"/>
#### 11.3.4、tcp选项

　　tcp选项的数据源主要来自于/proc/net/snmp伪文件，各个指标的计算逻辑如下：

```bash
$ tsar2 --tcp

## 提取原始指标，例如snmp文件第8行第13列值取差值，命名为snmp_retranssegs
metric=d|cfile=snmp|line=8|column=6|alias=snmp_activeopens
metric=d|cfile=snmp|line=8|column=7|alias=snmp_passiveopens
metric=d|cfile=snmp|line=8|column=8|alias=snmp_attemptfails
metric=d|cfile=snmp|line=8|column=9|alias=snmp_estabresets
metric=c|cfile=snmp|line=8|column=10|alias=snmp_currestab
metric=d|cfile=snmp|line=8|column=11|alias=snmp_insegs
metric=d|cfile=snmp|line=8|column=12|alias=snmp_outsegs
metric=d|cfile=snmp|line=8|column=13|alias=snmp_retranssegs

## 复合运算
active      = snmp_activeopens
pasive      = snmp_passiveopens
AtmpFa      = snmp_attemptfails
EstRes      = snmp_estabresets
CurrEs      = snmp_currestab
iseg        = snmp_insegs
outseg      = snmp_outsegs
retranssegs = snmp_retranssegs
retran      = 100 * retranssegs / outseg
```

<a name="udp选项"/>
#### 11.3.5、udp选项

　　udp选项的数据源主要来自于/proc/net/snmp伪文件，各个指标的计算逻辑如下：

```bash
$ tsar2 --udp

## 提取原始指标，例如snmp文件第10行第2列值取差值，命名为snmp_indatagrams
metric=d|cfile=snmp|line=10|column=2|alias=snmp_indatagrams
metric=d|cfile=snmp|line=10|column=3|alias=snmp_noports
metric=d|cfile=snmp|line=10|column=4|alias=snmp_inerrors
metric=d|cfile=snmp|line=10|column=5|alias=snmp_outdatagrams

## 复合计算
idgm   = snmp_indatagrams
noport = snmp_noports
idmerr = snmp_inerrors
odgm   = snmp_outdatagrams
```

<a name="traffic选项"/>
#### 11.3.6、traffic选项

　　traffic选项的数据源主要来自于/proc/net/dev伪文件，各个指标的计算逻辑如下：

```bash
$ tsar2 --traffic

## 提取原始指标，例如dev文件以eth0:字符串开头行第2列值取差值，命名为eth0_dev_rx_bytes
position=a|metric=d|cfile=dev|line_begin=eth0:|column=2|alias=eth0_dev_rx_bytes
position=a|metric=d|cfile=dev|line_begin=eth0:|column=3|alias=eth0_dev_rx_packets
position=a|metric=d|cfile=dev|line_begin=eth0:|column=4|alias=eth0_dev_rx_errs
position=a|metric=d|cfile=dev|line_begin=eth0:|column=5|alias=eth0_dev_rx_drop
position=a|metric=d|cfile=dev|line_begin=eth0:|column=10|alias=eth0_dev_tx_bytes
position=a|metric=d|cfile=dev|line_begin=eth0:|column=11|alias=eth0_dev_tx_packets
position=a|metric=d|cfile=dev|line_begin=eth0:|column=12|alias=eth0_dev_tx_errs
position=a|metric=d|cfile=dev|line_begin=eth0:|column=13|alias=eth0_dev_tx_drop

## tsar和tsar2按照设备名为eth或em开头来判断是否是物理网卡设备。然后会将所有物理网卡设备的以上指标代数加和
sum_dev_rx_bytes
sum_dev_rx_packets
sum_dev_rx_errs
sum_dev_rx_drop
sum_dev_tx_bytes
sum_dev_tx_packets
sum_dev_tx_errs
sum_dev_tx_drop

## 复合计算
bytin  = sum_dev_rx_bytes
pktin  = sum_dev_rx_packets
bytout = sum_dev_tx_bytes
pktout = sum_dev_tx_packets
pkterr = sum_dev_rx_errs + sum_dev_tx_errs
pktdrp = sum_dev_rx_drop + sum_dev_tx_drop
```

<a name="io选项"/>
#### 11.3.7、io选项

　　io选项的数据源主要来自于/proc/diskstats伪文件，各个指标的计算逻辑如下：

```bash
$ tsar2 --io

## 提取原始指标，例如diskstats文件以sda字符串开头行第4列值取差值，命名为sda_diskstats_rd_ios
metric=d:cfile=diskstats:line_begin=sda:column=4:alias=sda_diskstats_rd_ios
metric=d:cfile=diskstats:line_begin=sda:column=5:alias=sda_diskstats_rd_merges
metric=d:cfile=diskstats:line_begin=sda:column=6:alias=sda_diskstats_rd_sectors
metric=d:cfile=diskstats:line_begin=sda:column=7:alias=sda_diskstats_rd_ticks
metric=d:cfile=diskstats:line_begin=sda:column=8:alias=sda_diskstats_wr_ios
metric=d:cfile=diskstats:line_begin=sda:column=9:alias=sda_diskstats_wr_merges
metric=d:cfile=diskstats:line_begin=sda:column=10:alias=sda_diskstats_wr_sectors
metric=d:cfile=diskstats:line_begin=sda:column=11:alias=sda_diskstats_wr_ticks
metric=d:cfile=diskstats:line_begin=sda:column=13:alias=sda_diskstats_ticks
metric=d:cfile=diskstats:line_begin=sda:column=14:alias=sda_diskstats_aveq

## io类指标，tsar和tsar2默认以单个设备维度显示。这里以sda盘为例
sda_diskstats_n_ios  = sda_diskstats_rd_ios + sda_diskstats_wr_ios

sda_rrqms  = sda_diskstats_rd_merges
sda_wrqms  = sda_diskstats_wr_merges
sda_%rrqm  = 100 * sda_diskstats_rd_merges / (sda_diskstats_rd_merges + sda_diskstats_rd_ios) 
sda_%wrqm  = 100 * sda_diskstats_wr_merges / (sda_diskstats_wr_merges + sda_diskstats_wr_ios)
sda_rs     = sda_diskstats_rd_ios
sda_ws     = sda_diskstats_wr_ios
sda_rsecs  = sda_diskstats_rd_sectors
sda_wsecs  = sda_diskstats_wr_sectors
sda_rqsize = (sda_diskstats_rd_sectors + sda_diskstats_wr_sectors) / (2 * sda_diskstats_n_ios)
sda_rarqsz = sda_diskstats_rd_sectors / (2 * sda_diskstats_rd_ios)
sda_warqsz = sda_diskstats_wr_sectors / (2 * sda_diskstats_wr_ios)
sda_qusize = sda_diskstats_aveq / 1000
sda_await  = (sda_diskstats_rd_ticks + sda_diskstats_wr_ticks) / sda_diskstats_n_ios
sda_rawait = sda_diskstats_rd_ticks / sda_diskstats_rd_ios
sda_wawait = sda_diskstats_wr_ticks / sda_diskstats_wr_ios
sda_svctm  = sda_diskstats_ticks / sda_diskstats_n_ios
sda_util   = sda_diskstats_ticks / 10
```

<a name="pcsw选项"/>
#### 11.3.8、pcsw选项

　　pcsw选项的数据源主要来自于/proc/stat伪文件，各个指标的计算逻辑如下：

```bash
$ tsar2 --pcsw

## 提取原始指标，例如stat文件以ctxt字符串开头行第2列值取差值，命名为stat_ctxt
metric=d|cfile=stat|line_begin=ctxt|column=2|alias=stat_ctxt
metric=d|cfile=stat|line_begin=processes|column=2|alias=stat_processes

## 复合计算
cswch = stat_ctxt
proc  = stat_processes
```

<a name="tcpx选项"/>
#### 11.3.9、tcpx选项

　　待完善

<a name="load选项"/>
#### 11.3.10、load选项

load选项的数据源主要来自于/proc/loadavg伪文件，各个指标的计算逻辑如下：

```bash
$ tsar2 --load

## 提取原始指标，例如loadavg文件第1行第1列值，以实数型识别，命名为loadavg_load1
metric=c|cfile=loadavg|line=1|column=1|dtype=float|alias=loadavg_load1
metric=c|cfile=loadavg|line=1|column=2|dtype=float|alias=loadavg_load5
metric=c|cfile=loadavg|line=1|column=3|dtype=float|alias=loadavg_load15
metric=c|cfile=loadavg|line=1|column=4|dtype=string|alias=loadavg_runq_plit

## 复合计算
load1  = loadavg_load1
load5  = loadavg_load5
load15 = loadavg_load15
runq   = loadavg_runq_plit.split("/")[0]
plit   = loadavg_runq_plit.split("/")[1]
```

<a name="设备列表选项"/>
## 11.4、设备列表选项

<a name="item选项"/>
#### 11.4.1、item选项

　　item选项，简写-I，用于指定专门的设备列表。tsar的--item选项仅限于对--io指标大类选项下使用，而且只能指定一个磁盘设备。tsar2在这里有较多的扩展，可以支持--io、--cpu和--traffic三个指标大类的设备指定。而且可以同时指定多个设备。

```bash
$ tsar2 --io -I sda,sda3,sdb1     # 可以同时支持sda盘和sda3分区的指标显示
$ tsar2 --cpu -I cpu,cpu1,cpu4    # 可以同时支持整机cpu和单颗cpu的指标显示
$ tsar2 --traffic -I eth0,lo      # 可以同时支持物理网卡eth0和虚拟网络设备lo的指标显示
```

　　如果同时指定多个指标大类选项，比如tsar2 --io --cpu --traffic，那么item选项会按io、traffic和cpu的顺序解析item的选项值，并进行合法化校验。

<a name="merge选项"/>
#### 11.4.2、merge选项

　　merge选项，简写-m，tsar和tsar2目前都只配合--io指标大类选项使用。用于输出对各个磁盘各指标merge的结果。

<a name="指标类选项"/>
## 11.5、指标类选项

<a name="spec选项"/>
#### 11.5.1、spec选项

　　spec选项，简写-s，用户在每个指标大类既有的指标集合中，摘要显示部分指标。spec选项可以搭配多个指标大类选项同时使用。规则是如果有匹配项，则只显示匹配项，如果没有则显示全部指标项。

```bash
$ tsar2 --cpu --mem --udp -s util  # cpu和mem指标大类中有util指标，则只显示util指标，udp中无uti，则显示全部
Time           --cpu-- --mem-- --udp-- --udp-- --udp-- --udp--
Time              util    util    idgm    odgm  noport  idmerr
23/03/21-11:25    1.05    4.23    0.66    1.43    0.00    0.00
23/03/21-11:30    1.14    4.23    0.63    1.40    0.00    0.00
```

<a name="detail选项"/>
#### 11.5.2、detail选项

　　detail选项，简写-D，tsar和tsar2默认会对指标数值进行K/M/G的转换，使用detail可以还原显示原始数值。

<a name="tsar2创新指标"/>
# 十二、tsar2创新指标

　　tsar2命令在全面兼容tsar的基础上，还创新性的推出一些新的指标和组合。在创新指标的命名上，我们尽量保持了内核原有的名称，这样我们可以免去指标含义的考古过程，直接在搜索引擎上找到共鸣。

<a name="网络tcp扩展指标"/>
## 12.1、网络tcp扩展指标

　　tsar在诊断网络问题时，提供了tcp类指标组合，在实际应对生产中网络tcp类问题时还远远不够，我们针对tcp问题的诊断，细化了4组指标组合。

　　当我们发现tsar/tsar2的tcp视图中的retran重传率指标异常时，可以通过这个--retran指标组合进一步查看网络重传的更丰满的信息。

```bash
$ tsar2 --retran
Time           --retran--- --retran--- --retran-- --retran-- -retran- ----retran---- ----retran---- -----retran----- --retran--- --retran--
Time           RetransSegs FastRetrans LossProbes RTORetrans RTORatio LostRetransmit ForwardRetrans SlowStartRetrans RetransFail SynRetrans
12/07/21-13:00        4.49        0.66       4.05       0.76    16.93           0.00           0.00             0.00        0.00       0.75
12/07/21-13:05        3.24        0.47       2.82       0.50    15.43           0.00           0.00             0.00        0.00       0.51
12/07/21-13:10        4.08        2.24       1.75       0.22     5.39           0.00           0.00             0.31        0.00       0.21
12/07/21-13:15        1.70        0.86       1.00       0.00     0.00           0.00           0.00             0.00        0.00       0.00
12/07/21-13:20        2.07        0.29       1.92       0.09     4.35           0.00           0.01             0.15        0.00       0.08
12/07/21-13:25        9.00        0.25      13.39       0.22     2.44           0.00           0.00             0.00        0.00       0.21
12/07/21-13:30        2.03        0.30       1.92       0.04     1.97           0.00           0.00             0.00        0.00       0.04
12/07/21-13:35        4.70        1.30       4.74       0.58    12.34           0.00           0.00             0.17        0.00       0.56
```

　　tcpofo指标组合，对于诊断网络tcp包乱序（Out-Of-Order）问题时，可以起到很重要的作用。

```bash
$ tsar2 --tcpofo
Time           -tcpofo-- ---tcpofo--- ---tcpofo--- -tcpofo- tcpofo- -tcpofo-
Time           OfoPruned DSACKOfoSent DSACKOfoRecv OFOQueue OFODrop OFOMerge
12/07/21-13:00      0.00         0.00         0.00     0.61    0.00     0.00
12/07/21-13:05      0.00         0.00         0.00     0.54    0.00     0.00
12/07/21-13:10      0.00         0.00         0.00     0.35    0.00     0.00
12/07/21-13:15      0.00         0.00         0.00     0.42    0.00     0.00
12/07/21-13:20      0.00         0.00         0.00     0.24    0.00     0.00
12/07/21-13:25      0.00         0.00         0.00     1.15    0.00     0.00
12/07/21-13:30      0.00         0.00         0.00     0.45    0.00     0.00
12/07/21-13:35      0.00         0.00         0.00     0.56    0.00     0.00
```

　　tcpdrop指标组合，用于诊断网络tcp包drop相关问题。

```bash
$ tsar2 --tcpdrop
Time           ----tcpdrop----- --tcpdrop-- ----tcpdrop---- ----tcpdrop---- --tcpdrop-- -tcpdrop-- ----tcpdrop---- --tcpdrop--- tcpdrop
Time           LockDroppedIcmps ListenDrops ListenOverflows PrequeueDropped BacklogDrop MinTTLDrop DeferAcceptDrop ReqQFullDrop OFODrop
12/07/21-13:05             0.00       18.62           18.62            0.00        0.00       0.00            0.00         0.00    0.00
12/07/21-13:10             0.00        4.11            4.11            0.00        0.00       0.00            0.00         0.00    0.00
12/07/21-13:15             0.00        0.00            0.00            0.00        0.00       0.00            0.00         0.00    0.00
12/07/21-13:20             0.00        0.00            0.00            0.00        0.00       0.00            0.00         0.00    0.00
12/07/21-13:25             0.00        0.00            0.00            0.00        0.00       0.00            0.00         0.00    0.00
12/07/21-13:30             0.00        0.00            0.00            0.00        0.00       0.00            0.00         0.00    0.00
12/07/21-13:35             0.00        0.16            0.16            0.00        0.00       0.00            0.00         0.00    0.00
12/07/21-13:40             0.00        0.00            0.00            0.00        0.00       0.00            0.00         0.00    0.00
```

　　tcperr指标组合，用于诊断网络tcp err问题。

```bash
$ tsar2 --tcperr 
Time           ---tcperr--- ---tcperr--- ---tcperr--- ---tcperr---- --tcperr--- -----tcperr----- --------tcperr--------
Time           RenoFailures SackFailures LossFailures AbortOnMemory AbortFailed TimeWaitOverflow FastOpenListenOverflow
12/07/21-13:05         0.00         0.00         0.00          0.00        0.00             0.00                   0.00
12/07/21-13:10         0.00         0.00         0.02          0.00        0.00             0.00                   0.00
12/07/21-13:15         0.00         0.00         0.00          0.00        0.00             0.00                   0.00
12/07/21-13:20         0.00         0.00         0.01          0.00        0.00             0.00                   0.00
12/07/21-13:25         0.00         0.00         0.00          0.00        0.00             0.00                   0.00
12/07/21-13:30         0.00         0.00         0.00          0.00        0.00             0.00                   0.00
12/07/21-13:35         0.00         0.00         0.00          0.00        0.00             0.00                   0.00
12/07/21-13:40         0.00         0.00         0.00          0.00        0.00             0.00                   0.00
```

<a name="tsar2中断子命令irqtop"/>
# 十三、tsar2中断子命令irqtop

　　irqtop子命令tsar2工具中重点解决中断类问题的模块，也同时支持历史数据查询和实时模式查询。

<a name="输出指标项说明"/>
## 13.1、输出指标项说明

　　输出指标中，第一行的四个N1表示这4列为Top 1的指标，四个N2列表示这4列为Top 2的指标，以此类推。每组中的irq表示中断号，name表示中断名称，CPU表示这个中断在这个时间区间分配到的CPU核号，如果是多个CPU将逗号隔开，count表示这个中断在这些CPU上产生的中断数之和。

```bash
$ tsar2 irqtop -C 0-31 -l -i 1 -F 3

Time              --N1--- N1- N1- ---N1--- --N2--- N2- N2- ---N2--- --N3--- N3- N3- ------N3------
Time                count irq CPU name       count irq CPU name       count irq CPU name      
12/07/21-20:41:31    1.9K 096 0   nsa_dma     1.8K 097 0   nsa_dma     1.5K 068 17  virtio6-input.2   
12/07/21-20:41:32    2.0K 096 0   nsa_dma     2.0K 097 0   nsa_dma     1.2K 072 2   virtio6-input.4   
12/07/21-20:41:33    2.0K 096 0   nsa_dma     2.0K 097 0   nsa_dma   721.00 072 2   virtio6-input.4   
12/07/21-20:41:34    1.9K 096 0   nsa_dma     1.9K 097 0   nsa_dma     1.1K 068 17  virtio6-input.2   
12/07/21-20:41:36    2.0K 096 0   nsa_dma     2.0K 097 0   nsa_dma     1.6K 092 25  virtio6-input.14 
```

<a name="输入选项说明"/>
## 13.2、输入选项说明

　　这里对输入选项参数进行说明。

```bash
$ tsar2 irqtop -l                      # 选中为实时模式，支持更细颗粒度数据。默认不选是历史模式；
$ tsar2 irqtop -F 3                    # 用于指定显示的数据列组数，默认值是4组；
$ tsar2 irqtop -N virtio               # 使用中断名称过滤数据结果，这里只显示名称包含virtio的结果；
$ tsar2 irqtop -s count,name           # 用于输出指定的指标项
$ tsar2 irqtop -I 10,40-42             # 用于只显示指定的中断号相关的数据，这里只显示10号和40到42号4个中断；
$ tsar2 irqtop -I MOC -A               # 当需要指定MOC这种特殊中断时，需要结合使用-A选项；
$ tsar2 irqtop -C 7,30-32              # 用于只显示指定的CPU核号相关的数据，这里只显示7号和30到32号4个CPU的中断；
$ tsar2 irqtop -S i                    # 用于指定每行结果按照中断号排序，默认每行结果按照count排序；
```

<a name="大C选项参数特殊说明"/>
## 13.3、大C选项参数特殊说明

　　内核中输出中断统计信息的有2个地方，/proc/stat文件中的intr开头的行和/proc/interrupts文件。其中stat中的是每个中断号的中断汇总信息，而interrupts中则对每个中断号细化到了每个CPU的详细中断信息。基于此，为了整体的性能和体验，当我们没有使用大C选项参数，即不需要CPU级别的细化信息时，将从stat中获取数据源；反之，如果我们使用了大C选项参数，工具将从interrupts文件中获取数据源信息。

```bash
$ tsar2 irqtop -l               # 开始实时模式，数据源取自/proc/stat
$ tsar2 irqtop -l -C 0          # 开始实时模式，数据源取自/proc/interrupts
$ tsar2 irqtop                  # 历史模式，数据源为采集的/proc/stat的历史数据
$ tsar2 irqtop -C 0             # 历史模式，数据源为采集的/proc/interrupts的历史数据，
                                # interrupts默认没有开启采集，因此默认情况下，这个命令没有输出结果；

$ tsar2 irqtop -l -C 0-31       # 以32核机器为例，如果希望查看更详细的中断在CPU上的分布情况，
                                # 需要使用 -C 0-31选项参数；
```

<a name="interrupts数据采集"/>
## 13.4、interrupts数据采集

　　考虑到磁盘存储的成本，interrupts历史数据默认没有开启采集，如果业务需要，可以单独开启。具体方法如下。

```bash
$ vim /etc/ssar/sys.conf
......
{src_path='interrupts', gzip=true, turn=true},           # 新增了turn=true
......

$ sudo systemctl restart sresar                          # 修改完配置文件后，重启采集进程生效 
```

<a name="tsar2的CPU子命令cputop"/>
# 十四、tsar2的CPU子命令cputop

　　在多核CPU情况下，尽管内核的调度算法会尽量让CPU的使用在各个核中平均分配，但包括中断使用内的各种原因，让然会让各个CPU的CPU使用率并不均衡。tsar2的cputop子命令能很好的显示历史和实时的各个cpu之间的不均衡。


```bash
$ tsar2 cputop -l -i 1 -F 3 -S sirq

Time              --N1-- N1- -N1- --N2-- N2- -N2- --N3-- N3- -N3-
Time               value cpu idct  value cpu idct  value cpu idct
19/09/21-16:11:51   2.97  47 sirq   1.98  81 sirq   1.01  92 sirq
19/09/21-16:11:52   4.17  27 sirq   4.08  47 sirq   4.04  26 sirq
19/09/21-16:11:53   1.98  85 sirq   1.02  47 sirq   1.01  92 sirq
19/09/21-16:11:54   2.02  27 sirq   1.98  81 sirq   1.00  26 sirq
19/09/21-16:11:55   1.98  46 sirq   1.96  76 sirq   1.96  47 sirq
```

　　默认情况下，按照cpu util进行降序排序，使用-S选项可以指定按软中断CPU利用率sirq进行降序排序。N1下为Top One的CPU的利用率value、cpu核号cpu和指标名idct。

```bash
$ tsar2 cputop -l -i 1 -F 3 -S idle -r -I 0-63

Time              --N1-- N1- -N1- --N2-- N2- -N2- --N3-- N3- -N3-
Time               value cpu idct  value cpu idct  value cpu idct
19/09/21-16:20:23   0.00  46 idle   0.00  37 idle   0.00  35 idle
19/09/21-16:20:24   0.00   1 idle   0.00  46 idle   0.00  37 idle
19/09/21-16:20:25   0.00   1 idle   0.00  46 idle   0.00  37 idle
19/09/21-16:20:26   0.00   1 idle   0.00  46 idle   0.00  30 idle
19/09/21-16:20:27   0.00  46 idle   0.00  30 idle   0.00  37 idle
```

　　我们还可以选择按照CPU空闲的idle进行排序，-r选项选中则按照升序排序。大I选项参数指定参与排序的cpu核号列表。

<a name="增强查询器ssar+"/>
# 十五、增强查询器ssar+

　　功能规划中

# 十六、技术交流

　　ssar工具还在不断开发和优化过程中，如果大家觉得工具使用有任何疑问、对工具功能有新的建议，或者想贡献代码给ssar工具，请加群交流和反馈信息。钉钉群号：33304007。
