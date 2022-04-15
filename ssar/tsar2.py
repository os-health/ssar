#!/usr/bin/env python
# encoding: utf-8

from __future__ import print_function
import time
import argparse
import datetime
import sys
import os
import re
import json
import subprocess
import traceback
from datetime import timedelta
from collections import OrderedDict
from signal import signal, SIGPIPE, SIG_DFL
signal(SIGPIPE,SIG_DFL)

"""

    Copyright (c) 2021 Alibaba Inc. 
    SRESAR is licensed under Mulan PSL v2.
    You can use this software according to the terms and conditions of the Mulan PSL v2.
    You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
    THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
    EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
    MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
    See the Mulan PSL v2 for more details.

    Part of SRESAR (Site Reliability Engineering System Activity Reporter)

    Author:  Miles Wen <mileswen@linux.alibaba.com>
             TonyLu    <tonylu@linux.alibaba.com>
             Dust.Li   <dust.li@linux.alibaba.com>
             Xlpang    <xlpang@linux.alibaba.com>
             Ccheng    <ccheng@linux.alibaba.com>

    Tsar2 program is compatible with tsar

"""

MAX_SUB_HELP_POSITION = 45
MAX_HELP_WIDTH        = 120


#######################################################################################
#                                                                                     #
#                                   comman part                                       #
#                                                                                     #
#######################################################################################


class ItFormatter(argparse.RawTextHelpFormatter):
    def format_help(self):
        help = self._root_section.format_help()
        if help:
            help = self._long_break_matcher.sub('\n\n', help)
            help = help.strip('\n') + '\n'
        return help

def getcpunr():
    "Return the number of CPUs in the system"
    cpunr = -1
    for line in open('/proc/stat').readlines():
        if line[0:3] == 'cpu':
            cpunr = cpunr + 1
    if cpunr <= 0:
        raise Exception("Problem finding number of CPUs in system.")
    return cpunr

def avg(it_list):
    return sum(it_list)/len(it_list)

def format_bytes(s):
    size = float(s)
    if opts.detail:
        return '{:.2f}'.format(size)
    power = 2**10             # 2**10 = 1024
    n = 0
    power_labels = {0 : '', 1: 'K', 2: 'M', 3: 'G', 4: 'T', 5: 'P', 6: 'E'}
    while size > power:
        size /= power
        n += 1
    if n:
        return '{:.1f}'.format(size) + power_labels[n]
    else:
        return '{:.2f}'.format(size)

def printable(major, minor):
    if major == 8 or (major >= 65 and major <= 71) or (major >= 128 and major <= 135):
        return (not (minor & 0x0F) and opts.print_device) or ((minor & 0x0F) and opts.print_partition)
    elif major == 3 or major == 22 or major == 33 or major == 34 or major == 56 or major == 57 or major == 88 or major == 89 or major == 90 or major == 91:
        return (not (minor & 0x3F) and opts.print_device) or ((minor & 0x3F) and opts.print_partition)
    elif (major >= 104 and major <= 111) or (major >= 72 and major <= 79):
        return (not (minor & 0x0F) and opts.print_device) or ((minor & 0x0F) and opts.print_partition)

    return 1

def get_disk_devices():
    disk_devices = []
    disk_addons = []
    with open('/proc/diskstats') as fp:
        for line in fp:
            line_list = line.split()
            if printable(int(line_list[0]), int(line_list[1])):
                disk_devices.append(line_list[2])
            else:
                disk_addons.append(line_list[2])
    return disk_devices, disk_addons

def get_network_devices():
    network_devices = []
    network_addons = []
    with open('/proc/net/dev') as fp:
        for line in fp:
            strip_line = line.strip()
            if strip_line.find("eth") == 0 or strip_line.find("em") == 0:   # ["eth", "em"]
                network_devices.append(strip_line.split(":")[0])
            else:
                network_addons.append(strip_line.split(":")[0])
    return network_devices, network_addons

def get_cpu_devices():
    cpu_devices = []
    with open('/proc/stat') as fp:
        for line in fp:
            if line.find("cpu") == 0:
                cpu_devices.append(line.split()[0])
    return cpu_devices

def get_width(it_view, it_spec):
    width = 7
    if it_view in vi_widths and it_spec in  vi_widths[it_view]:
        width = vi_widths[it_view][it_spec]

    return width

def compare_kver(kver1, kver2):
    major1 = 0
    minor1 = 0
    patch1 = 0
    major2 = 0
    minor2 = 0
    patch2 = 0

    available_kver1 = kver1.split('-')[0].split('.')
    available_kver2 = kver2.split('-')[0].split('.')

    major1 = int(available_kver1[0])
    if len(available_kver1) > 1:
        minor1 = int(available_kver1[1])
    if len(available_kver1) > 2:
        patch1 = int(available_kver1[2])
    major2 = int(available_kver2[0])
    if len(available_kver2) > 1:
        minor2 = int(available_kver2[1])
    if len(available_kver2) > 2:
        patch2 = int(available_kver2[2])

    ret = 0
    if major1 > major2:
        ret = 1
    elif major1 < major2:
        ret = -1
    else:
        if minor1 > minor2:
            ret = 1
        elif minor1 < minor2:
            ret = -1
        else:
            if patch1 > patch2:
                ret = 1
            elif patch1 < patch2:
                ret = -1

    return ret

def get_line_file(line_begin, file_path):
    ret = ""
    for line in open(file_path):
        if line.find(line_begin + " ", 0) == 0:
            ret = line
            break

    return ret

def get_index_word(word, string):
    it_list = string.split()
    ret = -2
    if word in it_list:
        ret = it_list.index(word)

    return ret + 1

def get_irq_names():
    ret = {}
    for line in open('/proc/interrupts'):
        l = line.split()
        if len(l) <= opts.cpunr: 
            continue
        l1 = l[0].split(':')[0]

        if l1.isdigit():
            l2 = ' '.join(l[opts.cpunr+3:])
            l2 = l2.replace('_hcd:', '/')
            l2 = re.sub('@pci[:\d+\.]+', '', l2)
            l2 = re.sub('ahci\[[:\da-z\.]+\]', 'ahci', l2)
        else:
            l2 = l1
        ret[l1] = l2
    return ret

def get_irq_cpus():
    ret = {}
    for line in open('/proc/interrupts'):
        l = line.split()
        if len(l) <= opts.cpunr: 
            continue
        l1 = l[0].split(':')[0]

        ret[l1] = []
        l2 = l[1:opts.cpunr+1]
        for it_number in range(len(l2)):
            if int(l2[it_number]):
                it_cpus = ret[l1]
                it_cpus.append(it_number)
                ret[l1] = it_cpus
    return ret

#######################################################################################
#                                                                                     #
#                                    init part                                        #
#                                                                                     #
#######################################################################################

def parse_param_int(param):
    ret = []
    for it_part in param.split(","):
        num_part = it_part.count("-")
        if num_part == 0:
            if it_part.isdigit():
                ret.append(int(it_part))
            else:
                raise Exception("param:" + param + " is not correct.")
        elif num_part == 1:
            begin_part = it_part.split("-")[0]
            end_part   = it_part.split("-")[1]
            if not begin_part.isdigit() or not end_part.isdigit():
                raise Exception("param:" + param + " is not correct.")
            if int(begin_part) >= int(end_part):
                raise Exception("param:" + param + " is not correct.")
            ret.extend(range(int(begin_part), int(end_part)+1))
        else:
            raise Exception("param:" + param + " is not correct.")
    ret.sort()

    return ret

def parse_param_str(param):
    ret = []
    for it_part in param.split(","):
        num_part = it_part.count("-")
        if num_part == 0:
            ret.append(it_part)
        elif num_part == 1:
            begin_part = it_part.split("-")[0]
            end_part   = it_part.split("-")[1]
            if not begin_part.isdigit() or not end_part.isdigit():
                raise Exception("param:" + param + " is not correct.")
            if int(begin_part) >= int(end_part):
                raise Exception("param:" + param + " is not correct.")
            ret.extend([str(i) for i in range(int(begin_part), int(end_part)+1)])
        else:
            raise Exception("param:" + param + " is not correct.")
    ret.sort()

    return ret

def show_option_exclusive(opt1, opt2):
    print("tsar2: error: argument " + opt1 + ": not allowed with argument " + opt2)
    sys.exit(201);

def check_option_exclusive():
    if opts.finish:
        if opts.ndays:
            show_option_exclusive("--finish/-f","--ndays/-n")
        elif opts.date:
            show_option_exclusive("--finish/-f","--date/-d")
        elif opts.live:
            show_option_exclusive("--finish/-f","--live/-l")

def detect_datetime(fmt_datetime):
    it_size = len(fmt_datetime)
    it_format = ""

    if 19 == it_size:
        it_format = "%Y-%m-%dT%H:%M:%S"
    elif 14 == it_size:
        if -1 == fmt_datetime.find("/"):
            it_format = "%Y%m%d%H%M%S"
        else:
            it_format = "%d/%m/%y-%H:%M"
    elif 12 == it_size:
        it_format = "%Y%m%d%H%M"
    elif 10 == it_size:
        it_format = "%Y%m%d%H"
    elif 8 == it_size:
        it_format = "%Y%m%d"
    else:
        return -1

    return time.mktime(datetime.datetime.strptime(opts.finish, it_format).timetuple())

def init_options():
    opts.area_line        = 19
    opts.begin            = None
    opts.interval_tsar2   = None

    if "live" not in opts:
        opts.live = False
    if "date" not in opts:
        opts.date = False
    if "watch" not in opts:
        opts.watch = False
    if "finish" not in opts:
        opts.finish = False
    if "ndays" not in opts:
        opts.ndays = False
    if "path" not in opts:
        opts.path = False
    if "spec" not in opts:
        opts.spec = False
    if "item" not in opts:
        opts.item = False
    if "merge" not in opts:
        opts.merge = False
    if "detail" not in opts:
        opts.detail = False 
    if "name" not in opts:
        opts.name = False
    if "field" not in opts:
        opts.field = False
    if "interval" not in opts:
        opts.interval = False
    if "cpus" not in opts:
        opts.cpus = False
    if "sort" not in opts:
        opts.sort = False
    if "name" not in opts:
        opts.name = False
    if "all" not in opts:
        opts.all = False
    if "reverse" not in opts:
        opts.reverse = False

    check_option_exclusive()

    if not opts.field:
        opts.field = 4

    # init interval
    if not opts.interval:
        opts.interval = 1
    if opts.live:
        opts.interval_tsar2 = str(opts.interval) + "s"
    else:
        opts.interval_tsar2 = str(opts.interval)

    # init time_point_finish and init time_point_begin
    if opts.live:
        opts.formatted_finish = "+1440"
    elif opts.date:
        it_begin  = datetime.datetime.strptime(opts.date, '%Y%m%d')
        opts.begin = (it_begin + timedelta(seconds=(1))).strftime("%Y-%m-%dT%H:%M:%S")
        if datetime.datetime.today().strftime("%Y%m%d") == opts.date:
            it_now_min = 60 * datetime.datetime.today().hour + datetime.datetime.today().minute
            it_finish_min = it_now_min - it_now_min % int(opts.interval_tsar2)
        else:
            it_finish_min = 1440 - 1440 % int(opts.interval_tsar2)
        it_finish = it_begin + timedelta(minutes=(it_finish_min))
        opts.formatted_finish = it_finish.strftime("%Y-%m-%dT%H:%M:%S")
    elif opts.ndays:
        if opts.ndays == 0:
            opts.ndays = 1;
        now_timestamp = int(time.time())
        adjustive_timestamp = now_timestamp - now_timestamp % (60 * int(opts.interval_tsar2))
        head_timestamp = adjustive_timestamp - 86400 * opts.ndays
        opts.begin = datetime.datetime.fromtimestamp(head_timestamp).strftime("%Y-%m-%dT%H:%M:%S")
        it_finish_min = 1440 * opts.ndays - 1440 % int(opts.interval_tsar2)
        it_finish = datetime.datetime.fromtimestamp(head_timestamp) + timedelta(minutes=(it_finish_min))
        opts.formatted_finish = it_finish.strftime("%Y-%m-%dT%H:%M:%S")
    else:
        if not opts.watch:
            opts.watch = 300
        now_timestamp = int(time.time())
        finish_timestamp = now_timestamp
        if opts.finish:
            if 0 == opts.finish.find("-", 0):
                finish_str = opts.finish[1:]
                finish_int = 0
                if (len(finish_str)-1) == finish_str.rfind("d", (len(finish_str)-1)):
                    finish_int = int(24 * 60 * float(finish_str.substr(0,(len(finish_str)-1))))
                elif (len(finish_str)-1) == finish_str.rfind("h", (len(finish_str)-1)):
                    finish_int = int(60 * float(finish_str.substr(0,(len(finish_str)-1))))
                elif (len(finish_str)-1) == finish_str.rfind("m", (len(finish_str)-1)):
                    finish_int = int(float(finish_str.substr(0,(len(finish_str)-1))))
                else:
                    finish_int = int(float(finish_str))
                finish_timestamp = finish_timestamp - 60 * finish_int
            else:
                finish_timestamp = detect_datetime(opts.finish)
                if -1 == finish_timestamp:
                    print("finish datetime is not correct.")
                    sys.exit(201);
                if finish_timestamp > now_timestamp:
                    print("finish datetime is more than now.")
                    sys.exit(201);
        adjustive_timestamp = finish_timestamp - finish_timestamp % (60 * int(opts.interval_tsar2))
        head_timestamp = adjustive_timestamp - 60 * opts.watch
        opts.begin = datetime.datetime.fromtimestamp(head_timestamp).strftime("%Y-%m-%dT%H:%M:%S")
        it_finish_min = opts.watch - opts.watch % int(opts.interval_tsar2)
        it_finish = datetime.datetime.fromtimestamp(head_timestamp) + timedelta(minutes=(it_finish_min))
        opts.formatted_finish = it_finish.strftime("%Y-%m-%dT%H:%M:%S")

    # init formats
    opts.specs = []
    if opts.spec:
        opts.specs = opts.spec.split(",")

def init_sys():
    opts.print_partition  = 0
    opts.print_device     = 1
    opts.formats_devices  = {}
    opts.formated_devices = {}
    opts.formated_type    = {}
    opts.item_type        = None           # such as io / traffic / cpu
    opts.item_mode        = {}             # such as {'io': 'every'}, avg / sum / max / min, specific

    indicators    = []
    for it_view in vi:
        indicators.extend(vi[it_view])
    if opts.specs:
        for it_spec in opts.specs:
            if it_spec not in indicators:                                                # validate indicator 
                raise Exception(it_spec + " is not correct.")

    # init items
    opts.items = []
    if opts.item:
        opts.items = opts.item.split(",")

    opts.formats  = []       # such as [{'item': 'cpu0', 'view': 'cpu', 'indicator': 'user'},{ }]
    opts.formated = []       # such as [{'header': 'cpu', 'indicator': 'user'},{ }]

    opts.picks    = []       # such as ['cpu', 'mem', 'swap', 'tcp', 'udp', 'traffic', 'io', 'pcsw', 'tcpx', 'load', 'tcpofo', 'retran', 'tcpdrop']
    opts.arrange = {}        # such as {'cpu': ['user', 'sys', 'hirq']}
    for it_view in vi.keys():
        if it_view in opts and getattr(opts, it_view):
            opts.picks.append(it_view)
            opts.arrange[it_view] = []
            spec_hit = False
            for it_indicator in vi[it_view]:
                if it_indicator in opts.specs:                                           # filter indicator
                    spec_hit = True
                    opts.arrange[it_view].append(it_indicator)
            if not spec_hit:
                for it_indicator in vi[it_view]:
                    opts.arrange[it_view].append(it_indicator)

    if not opts.picks:                                                                   # fill with default view and indicator
        if opts.formats:
            raise Exception("formats and picks status not match")
        for it_view in default_vi.keys():
            opts.picks.append(it_view)
            opts.arrange[it_view] = []
            spec_hit = False
            for it_indicator in default_vi[it_view]:
                if it_indicator in opts.specs:
                    spec_hit = True
                    opts.arrange[it_view].append(it_indicator)
            if not spec_hit:
                for it_indicator in default_vi[it_view]:
                    opts.arrange[it_view].append(it_indicator)

    if "io" in opts.picks:                               # io must be first order
        opts.item_type = "io"
    elif "traffic" in opts.picks:
        opts.item_type = "traffic"
    elif "cpu" in opts.picks:
        opts.item_type = "cpu"

    for it_view in opts.picks:
        if it_view == "io":
            disk_devices, disk_addons = get_disk_devices()
            disk_alls = disk_devices + disk_addons
            if opts.merge:
                opts.item_mode['io'] = "sum"
            elif opts.items:                             # opts.item_type == "io"   not needed 
                opts.item_mode['io'] = "specific"
                for it_device in opts.items:
                    if it_device not in disk_alls:       # validate io item
                        raise Exception("disk device " + it_device + " is not correct.")
            else:
                opts.item_mode['io'] = "every"

            if opts.item_mode['io'] == "specific":
                opts.formats_devices['io']  = opts.items
                opts.formated_devices['io'] = opts.items
            elif opts.item_mode['io'] == "every":
                opts.formats_devices['io']  = disk_devices
                opts.formated_devices['io'] = disk_devices
            elif opts.item_mode['io'] == "sum":
                opts.formats_devices['io']  = disk_devices
                opts.formated_devices['io'] = ["sum"]
                opts.formated_type['io'] = True
            else:
                pass

            for it_device in opts.formated_devices['io']:
                for it_indicator in opts.arrange["io"]:
                    if opts.item_mode['io'] == "sum":
                        opts.formated.append({'header':"io", 'item':it_device, 'view':'io', 'indicator':it_indicator})
                    else:
                        opts.formated.append({'header':it_device, 'item':it_device, 'view':'io', 'indicator':it_indicator})

        elif it_view == "traffic":
            network_devices, network_addons = get_network_devices()
            network_alls = network_devices + network_addons
            if opts.items and opts.item_type == "traffic":
                opts.item_mode['traffic'] = "specific"
                for it_device in opts.items:
                    if it_device not in network_alls:       # validate io item
                        raise Exception("network device " + it_device + " is not correct.")
            else:
                opts.item_mode['traffic'] = "sum"

            if opts.item_mode['traffic'] == "specific":
                opts.formats_devices['traffic'] = opts.items
                opts.formated_devices['traffic'] = opts.items
            elif opts.item_mode['traffic'] == "sum":
                opts.formats_devices['traffic'] = network_devices
                opts.formated_devices['traffic'] = ["sum"]
                opts.formated_type['traffic'] = True

            for it_device in opts.formated_devices['traffic']:
                for it_indicator in opts.arrange["traffic"]:
                    if opts.item_mode['traffic'] == "sum":
                        opts.formated.append({'header':"traffic", 'item':it_device, 'view':'traffic', 'indicator':it_indicator})
                    else:       
                        opts.formated.append({'header':it_device, 'item':it_device, 'view':'traffic', 'indicator':it_indicator})

        elif it_view == "cpu":
            cpu_devices = get_cpu_devices()
            if opts.items and opts.item_type == "cpu":
                for it_device in opts.items:
                    if it_device not in cpu_devices:       # validate io item
                        raise Exception("cpu device " + it_device + " is not correct.")
                opts.formats_devices['cpu'] = opts.items
            else:
                opts.formats_devices["cpu"] = ["cpu"]

            for it_device in opts.formats_devices["cpu"]:
                for it_indicator in opts.arrange["cpu"]:
                    opts.formated.append({'header':it_device,'item':it_device, 'view':'cpu', 'indicator':it_indicator})

        else:
            opts.formated_type[it_view] = True
            opts.formated_devices[it_view] = ['total']
            for it_device in opts.formated_devices[it_view]:
                for it_indicator in opts.arrange[it_view]:
                    opts.formated.append({'header':it_view,'item':it_device, 'view':it_view, 'indicator':it_indicator})

def init_irqtop():
    opts.cpunr = getcpunr()     

    if opts.cpus:
        opts.list_cpu = parse_param_int(opts.cpus)
        if max(opts.list_cpu) >= opts.cpunr:
            raise Exception("param2:" + opts.cpus + " is not correct.")
        opts.irq_cpu_filter = True
    else:
        opts.list_cpu = range(0, opts.cpunr)
        opts.irq_cpu_filter = False

    if not opts.spec:
        if opts.irq_cpu_filter:
            opts.specs = ["count", "irq", "cpu", "name"]
        else:
            opts.specs = ["count", "irq", "name"]
    for it_spec in opts.specs:
        if it_spec not in ["count", "irq", "cpu", "name"]:                  # validate indicator 
            raise Exception(it_spec + " is not correct.")

    if not opts.sort:
        opts.sort = "c"
    if opts.sort not in ["c", "i"]:                                         # validate sort
        raise Exception(opts.sort + " is not correct.")

    # init items
    opts.items = []
    if opts.item:
        opts.items = parse_param_str(opts.item) 

    opts.irq_names = get_irq_names()
    opts.irq_cpus  = get_irq_cpus()
    cpu2irqs       = get_irqofcpu(opts.irq_cpus)
  
    if opts.items:
        items_tmp = [] 
        for i in opts.items:
            if i in opts.irq_names:
                items_tmp.append(i)
        opts.items = items_tmp
        if not opts.items:
            raise Exception(opts.item + " is not correct.")
    else:
        opts.items = opts.irq_names.keys()    

def init_cputop():
    opts.cpunr = getcpunr()     

    if not opts.spec:
        opts.specs = ["value", "cpu", "idct"]
    for it_spec in opts.specs:
        if it_spec not in ["value", "cpu", "idct"]:                         # validate indicator 
            raise Exception(it_spec + " is not correct.")

    if not opts.sort:
        opts.sort = "util"
    if opts.sort not in ["user", "sys", "wait", "hirq", "sirq", "idle", "util"]:   # validate sort
        raise Exception(opts.sort + " is not correct.")

    # init items
    opts.items = []
    if opts.item:
        opts.items = parse_param_int(opts.item)
        if max(opts.items) >= opts.cpunr:
            raise Exception("param2:" + opts.item + " is not correct.")
    else:
        opts.items = range(0, opts.cpunr)

def init_env():
    uts_releases  = os.uname()[2].split(".", 2)
    opts.kversion = uts_releases[0] + "." + uts_releases[1]


#######################################################################################
#                                                                                     #
#                                    workers part                                     #
#                                                                                     #
#######################################################################################


def workers():
    print('tsar2 workers comming soon.', opts)
    #print('workers === ', args)


#######################################################################################
#                                                                                     #
#                                    procs part                                       #
#                                                                                     #
#######################################################################################


def procs():
    print('tsar2 procs comming soon.', opts)
    #print('procs === ', args)

#######################################################################################
#                                                                                     #
#                                    cputop part                                      #
#                                                                                     #
#######################################################################################

def concatenate_cputop():
    scmd = "ssar -P --api -o '"
    for it_device in opts.items:
        scmd += "metric=d:cfile=stat:line_begin=cpu{item}:column=2:alias={item}_user;".format(item=it_device)
        scmd += "metric=d:cfile=stat:line_begin=cpu{item}:column=3:alias={item}_nice;".format(item=it_device)
        scmd += "metric=d:cfile=stat:line_begin=cpu{item}:column=4:alias={item}_system;".format(item=it_device)
        scmd += "metric=d:cfile=stat:line_begin=cpu{item}:column=5:alias={item}_idle;".format(item=it_device)
        scmd += "metric=d:cfile=stat:line_begin=cpu{item}:column=6:alias={item}_iowait;".format(item=it_device)
        scmd += "metric=d:cfile=stat:line_begin=cpu{item}:column=7:alias={item}_irq;".format(item=it_device)
        scmd += "metric=d:cfile=stat:line_begin=cpu{item}:column=8:alias={item}_softirq;".format(item=it_device)
        scmd += "metric=d:cfile=stat:line_begin=cpu{item}:column=9:alias={item}_steal;".format(item=it_device)
        scmd += "metric=d:cfile=stat:line_begin=cpu{item}:column=10:alias={item}_guest;".format(item=it_device)
        scmd += "metric=d:cfile=stat:line_begin=cpu{item}:column=11:alias={item}_guest_nice;".format(item=it_device)

    scmd += "'"
    if opts.formatted_finish:
        scmd += " -f " + opts.formatted_finish
    if opts.begin:
        scmd += " -b " + opts.begin
    if opts.interval_tsar2:
        scmd += " -i " + opts.interval_tsar2
    if opts.path:
        scmd += " --path=" + opts.path

    return scmd

def display_cputop_lines(it_line, i):
    it_collect_datetime = it_line['collect_datetime']
    del it_line['collect_datetime']

    new_line = {}
    for it_device in opts.items:
        it_stat_user       = float(it_line[str(it_device) + '_user'])
        it_stat_nice       = float(it_line[str(it_device) + '_nice'])
        it_stat_system     = float(it_line[str(it_device) + '_system'])
        it_stat_idle       = float(it_line[str(it_device) + '_idle'])
        it_stat_iowait     = float(it_line[str(it_device) + '_iowait'])
        it_stat_irq        = float(it_line[str(it_device) + '_irq'])
        it_stat_softirq    = float(it_line[str(it_device) + '_softirq'])
        it_stat_steal      = float(it_line[str(it_device) + '_steal'])
        it_stat_guest      = float(it_line[str(it_device) + '_guest'])
        it_stat_guest_nice = float(it_line[str(it_device) + '_guest_nice'])

        it_stat_cpu_times = it_stat_user +it_stat_nice +it_stat_system +it_stat_idle +it_stat_iowait +it_stat_irq +it_stat_softirq +it_stat_steal +it_stat_guest +it_stat_guest_nice
        it_cpu_user = 100 * it_stat_user    / it_stat_cpu_times
        it_cpu_sys  = 100 * it_stat_system  / it_stat_cpu_times
        it_cpu_wait = 100 * it_stat_iowait  / it_stat_cpu_times
        it_cpu_hirq = 100 * it_stat_irq     / it_stat_cpu_times
        it_cpu_sirq = 100 * it_stat_softirq / it_stat_cpu_times
        it_cpu_idle = 100 * it_stat_idle    / it_stat_cpu_times
        it_cpu_util = 100 * (it_stat_cpu_times - it_stat_idle - it_stat_iowait - it_stat_steal) / it_stat_cpu_times

        if opts.sort == "user":
            new_line[str(it_device) + "_" + opts.sort] = it_cpu_user
        elif opts.sort == "sys":
            new_line[str(it_device) + "_" + opts.sort] = it_cpu_sys
        elif opts.sort == "wait":
            new_line[str(it_device) + "_" + opts.sort] = it_cpu_wait
        elif opts.sort == "hirq":
            new_line[str(it_device) + "_" + opts.sort] = it_cpu_hirq
        elif opts.sort == "sirq":
            new_line[str(it_device) + "_" + opts.sort] = it_cpu_sirq
        elif opts.sort == "idle":
            new_line[str(it_device) + "_" + opts.sort] = it_cpu_idle
        elif opts.sort == "util":
            new_line[str(it_device) + "_" + opts.sort] = it_cpu_util
 
    #for it_key, it_value in it_line.items():
    #    if not it_value:
    #        continue
    #    new_line[it_key] = float(it_value)
   
    if opts.reverse:
        sorted_list = sorted(new_line.items(), key = lambda item: item[1], reverse = False)
    else:
        sorted_list = sorted(new_line.items(), key = lambda item: item[1], reverse = True)
    sorted_list = sorted_list[:opts.field]
    sorted_line = OrderedDict(sorted_list)
    
    if opts.live:
        sorted_datetime = datetime.datetime.strptime(it_collect_datetime, "%Y-%m-%dT%H:%M:%S").strftime("%d/%m/%y-%H:%M:%S")
    else:
        sorted_datetime = datetime.datetime.strptime(it_collect_datetime, "%Y-%m-%dT%H:%M:%S").strftime("%d/%m/%y-%H:%M")
    
    if i % opts.area_line == 0:
        print("")
        if not opts.live:
            line_out = '{:<14}'.format("Time")
        else:
            line_out = '{:<17}'.format("Time")
        for it_order in range(opts.field):
            if "value" in opts.specs:
                line_out += " "
                line_out += '{:-^6}'.format("N"+str(it_order+1))
            if "cpu" in opts.specs:
                line_out += " "
                line_out += '{:-^3}'.format("N"+str(it_order+1))
            if "idct" in opts.specs:
                line_out += " "
                line_out += '{:-^4}'.format("N"+str(it_order+1))
        print(line_out)
        if not opts.live:
            line_out = '{:<14}'.format("Time")
        else:
            line_out = '{:<17}'.format("Time")
        for it_order in range(opts.field):
            if "value" in opts.specs:
                line_out += " "
                line_out += '{:>6}'.format("value")
            if "cpu" in opts.specs:
                line_out += " "
                line_out += '{:<3}'.format("cpu")
            if "idct" in opts.specs:
                line_out += " "
                line_out += '{:<4}'.format("idct")
        print(line_out)

    line_out = sorted_datetime
    for it_key, it_value in sorted_line.items():
        it_cpu  = it_key.split("_")[0]
        it_idct = it_key.split("_")[1]
        if "value" in opts.specs:
            line_out += " "
            line_out += '{:>6}'.format(format_bytes(it_value))
        if "cpu" in opts.specs:
            line_out += " "
            line_out += '{:>3}'.format(it_cpu)
        if "idct" in opts.specs:
            line_out += " "
            line_out += '{:<4}'.format(it_idct)
    print(line_out)
    sys.stdout.flush()

def cputop_live():
    ssar_cmd = concatenate_cputop()

    ssar_output = subprocess.Popen(ssar_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, bufsize=1)
    i = 0
    while True:
        line = ssar_output.stdout.readline().strip()
        if not line: 
            break
        else:
            display_cputop_lines(json.loads(line), i)
            i = i + 1

def cputop():
    ssar_cmd = concatenate_cputop()
        
    ssar_output = subprocess.Popen(ssar_cmd, shell=True, stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    output, errors = ssar_output.communicate()
    list_data = []
    if ssar_output.returncode == 0:
        list_data = json.loads(output)

    i = 0
    for it_line in list_data:
        display_cputop_lines(it_line, i)
        i = i+1

#######################################################################################
#                                                                                     #
#                                    irqtop part                                      #
#                                                                                     #
#######################################################################################

def get_irqofcpu(lstcpu):
    ret = {}
    for key, value in lstcpu.items():
        if value:
            for it_cpu in value:
                if it_cpu in ret:
                    it_irqs = ret[it_cpu]
                    it_irqs.append(key)
                    ret[it_cpu] = it_irqs
                else:
                    it_irqs = []
                    it_irqs.append(key)
                    ret[it_cpu] = it_irqs
    return ret

def concatenate_irqtop_interrupts():
    it_column_str = ','.join(str(x+2) for x in opts.list_cpu)

    scmd = "ssar -P --api -o '"
    for it_item in opts.items:
        scmd += "metric=d|cfile=interrupts|line_begin={intr}:|column={it_column}|alias={intr}_{{column}};".format(intr = it_item, it_column = it_column_str)

    scmd += "'"
    if opts.formatted_finish:
        scmd += " -f " + opts.formatted_finish
    if opts.begin:
        scmd += " -b " + opts.begin
    if opts.interval_tsar2:
        scmd += " -i " + opts.interval_tsar2
    if opts.path:
        scmd += " --path=" + opts.path

    return scmd

def concatenate_irqtop_intr():
    intset = []
    for line in open('/proc/stat').read().splitlines():
        it_line = line.split()
        if it_line[0] == 'intr':
            intset = [ int(i) for i in it_line[2:] ]

    max_intr = 0
    i = 0
    for it_intr in intset:
        if it_intr:
            max_intr = i
        i = i + 1

    max_intr = max_intr + 1
    intset = intset[:max_intr]                   # subprocess param max 131071 

    scmd = "ssar -P --api -o '"
    for it_intr in range(len(intset)):
        scmd += "metric=d:cfile=stat:line_begin=intr:column={item1}:alias={item2};".format(item1=it_intr + 3, item2=it_intr)

    scmd += "'"
    if opts.formatted_finish:
        scmd += " -f " + opts.formatted_finish
    if opts.begin:
        scmd += " -b " + opts.begin
    if opts.interval_tsar2:
        scmd += " -i " + opts.interval_tsar2
    if opts.path:
        scmd += " --path=" + opts.path

    return scmd

def display_intr_lines(it_line, i):
    it_collect_datetime = it_line['collect_datetime']
    del it_line['collect_datetime']
    
    sum_line = {}
    precise_irq_cpus = {}
    if opts.irq_cpu_filter:
        for it_key, it_value in it_line.items():
            intr_part = it_key.split("_")[0]
            cpu_part  = int(it_key.split("_")[1])-2
   
            if not float(it_value):
                if intr_part not in sum_line:
                    sum_line[intr_part] = 0
                if intr_part not in precise_irq_cpus:
                    precise_irq_cpus[intr_part] = []
                continue
 
            if intr_part in sum_line:
                sum_line[intr_part] = sum_line[intr_part] + float(it_value)
            else:
                sum_line[intr_part] = float(it_value)
    
            if intr_part in precise_irq_cpus:
                it_cpus = precise_irq_cpus[intr_part]
                it_cpus.append(cpu_part)
                it_cpus.sort()
                precise_irq_cpus[intr_part] = it_cpus
            else:
                it_cpus = []
                it_cpus.append(cpu_part)
                precise_irq_cpus[intr_part] = it_cpus
    else:
        sum_line = it_line
     
    new_line = {}
    for it_key, it_value in sum_line.items():
        if it_key not in opts.irq_names or it_key not in opts.items:
            continue
        if opts.name and opts.irq_names[it_key].find(opts.name) == -1:
            continue
        if not opts.all and not it_key.isdigit():
            continue
        if not it_value:
            continue
        new_line[it_key] = float(it_value)
    
    if opts.sort == "c":
        sorted_list = sorted(new_line.items(), key=lambda item: item[1], reverse=True)
    else:
        sorted_list = sorted(new_line.items(), key=lambda item: int(item[0]) if item[0].isdigit() else item[0], reverse=False)
    sorted_list = sorted_list[:opts.field]
    sorted_line = OrderedDict(sorted_list)
    
    if opts.live:
        sorted_datetime = datetime.datetime.strptime(it_collect_datetime, "%Y-%m-%dT%H:%M:%S").strftime("%d/%m/%y-%H:%M:%S")
    else:
        sorted_datetime = datetime.datetime.strptime(it_collect_datetime, "%Y-%m-%dT%H:%M:%S").strftime("%d/%m/%y-%H:%M")
    
    if i % opts.area_line == 0:
        print("")
        if not opts.live:
            line_out = '{:<14}'.format("Time")
        else:
            line_out = '{:<17}'.format("Time")
        for it_order in range(opts.field):
            if "count" in opts.specs:
                line_out += " "
                line_out += '{:-^7}'.format("N"+str(it_order+1))
            if "irq" in opts.specs:
                line_out += " "
                line_out += '{:-^3}'.format("N"+str(it_order+1))
            if "cpu" in opts.specs:
                line_out += " "
                line_out += '{:-^3}'.format("N"+str(it_order+1))
            if "name" in opts.specs:
                line_out += " "
                line_out += '{:-^18}'.format("N"+str(it_order+1))
        print(line_out)
        if not opts.live:
            line_out = '{:<14}'.format("Time")
        else:
            line_out = '{:<17}'.format("Time")
        for it_order in range(opts.field):
            if "count" in opts.specs:
                line_out += " "
                line_out += '{:>7}'.format("count")
            if "irq" in opts.specs:
                line_out += " "
                line_out += '{:>3}'.format("irq")
            if "cpu" in opts.specs:
                line_out += " "
                line_out += '{:<3}'.format("cpu")
            if "name" in opts.specs:
                line_out += " "
                line_out += '{:<18}'.format("name")
        print(line_out)

    line_out = sorted_datetime
    for it_key, it_value in sorted_line.items():
        if it_key in opts.irq_names:
            it_name = opts.irq_names[it_key]
        else:
            it_name = ""
        if "count" in opts.specs:
            line_out += " "
            line_out += '{:>7}'.format(format_bytes(it_value))
        if "irq" in opts.specs:
            line_out += " "
            line_out += '{:0>3}'.format(it_key)
        if "cpu" in opts.specs:
            line_out += " "
            if opts.irq_cpu_filter:
                line_out += '{:<3}'.format(','.join(str(x) for x in precise_irq_cpus[it_key]))
            else:
                line_out += '{:<3}'.format(','.join(str(x) for x in opts.irq_cpus[it_key]))
        if "name" in opts.specs:
            line_out += " "
            line_out += '{:<18}'.format(it_name)
    print(line_out)
    sys.stdout.flush()

def irqtop_live():
    if opts.irq_cpu_filter:
        ssar_cmd = concatenate_irqtop_interrupts()
    else:
        ssar_cmd = concatenate_irqtop_intr()

    ssar_output = subprocess.Popen(ssar_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, bufsize=1)
    i = 0
    while True:
        line = ssar_output.stdout.readline().strip()
        if not line: 
            break
        else:
            display_intr_lines(json.loads(line), i)
            i = i + 1

def irqtop():
    if opts.irq_cpu_filter:
        ssar_cmd = concatenate_irqtop_interrupts()
    else:
        ssar_cmd = concatenate_irqtop_intr()
        
    ssar_output = subprocess.Popen(ssar_cmd, shell=True, stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    output, errors = ssar_output.communicate()
    list_data = []
    if ssar_output.returncode == 0:
        list_data = json.loads(output)

    i = 0
    for it_line in list_data:
        display_intr_lines(it_line, i)
        i = i+1



#######################################################################################
#                                                                                     #
#                                     sys part                                        #
#                                                                                     #
#######################################################################################

def concatenate_ssar():
    scmd = "ssar -P --api -o '"
    for it_view in opts.picks:
        if it_view == "cpu":
            for it_device in opts.formats_devices['cpu']:
                scmd += "metric=d:cfile=stat:line_begin={item}:column=2:alias={item}_stat_user;".format(item=it_device)
                scmd += "metric=d:cfile=stat:line_begin={item}:column=3:alias={item}_stat_nice;".format(item=it_device)
                scmd += "metric=d:cfile=stat:line_begin={item}:column=4:alias={item}_stat_system;".format(item=it_device)
                scmd += "metric=d:cfile=stat:line_begin={item}:column=5:alias={item}_stat_idle;".format(item=it_device)
                scmd += "metric=d:cfile=stat:line_begin={item}:column=6:alias={item}_stat_iowait;".format(item=it_device)
                scmd += "metric=d:cfile=stat:line_begin={item}:column=7:alias={item}_stat_irq;".format(item=it_device)
                scmd += "metric=d:cfile=stat:line_begin={item}:column=8:alias={item}_stat_softirq;".format(item=it_device)
                scmd += "metric=d:cfile=stat:line_begin={item}:column=9:alias={item}_stat_steal;".format(item=it_device)
                scmd += "metric=d:cfile=stat:line_begin={item}:column=10:alias={item}_stat_guest;".format(item=it_device)
                scmd += "metric=d:cfile=stat:line_begin={item}:column=11:alias={item}_stat_guest_nice;".format(item=it_device)
        elif it_view == "mem":
            scmd += "metric=c|cfile=meminfo|line_begin=MemTotal:|column=2|alias=meminfo_total;"
            scmd += "metric=c|cfile=meminfo|line_begin=MemFree:|column=2|alias=meminfo_free;"
            scmd += "metric=c|cfile=meminfo|line_begin=MemAvailable:|column=2|alias=meminfo_available;"
            scmd += "metric=c|cfile=meminfo|line_begin=Buffers:|column=2|alias=meminfo_buffers;"
            scmd += "metric=c|cfile=meminfo|line_begin=Cached:|column=2|alias=meminfo_cached;"
        elif it_view == "swap":
            scmd += "metric=d|cfile=vmstat|line_begin=pswpin|column=2|alias=vmstat_pswpin;"
            scmd += "metric=d|cfile=vmstat|line_begin=pswpout|column=2|alias=vmstat_pswpout;"
            scmd += "metric=c|cfile=meminfo|line_begin=SwapTotal:|column=2|alias=meminfo_swaptotal;"
            scmd += "metric=c|cfile=meminfo|line_begin=SwapFree:|column=2|alias=meminfo_swapfree;"
        elif it_view == "tcp":
            scmd += "metric=d|cfile=snmp|line=8|column=6|alias=snmp_activeopens;"
            scmd += "metric=d|cfile=snmp|line=8|column=7|alias=snmp_passiveopens;"
            scmd += "metric=d|cfile=snmp|line=8|column=8|alias=snmp_attemptfails;"
            scmd += "metric=d|cfile=snmp|line=8|column=9|alias=snmp_estabresets;"
            scmd += "metric=c|cfile=snmp|line=8|column=10|alias=snmp_currestab;"
            scmd += "metric=d|cfile=snmp|line=8|column=11|alias=snmp_insegs;"
            scmd += "metric=d|cfile=snmp|line=8|column=12|alias=snmp_outsegs;"
            scmd += "metric=d|cfile=snmp|line=8|column=13|alias=snmp_RetransSegs;"
        elif it_view == "udp":
            scmd += "metric=d|cfile=snmp|line=10|column=2|alias=snmp_indatagrams;"
            scmd += "metric=d|cfile=snmp|line=10|column=3|alias=snmp_noports;"
            scmd += "metric=d|cfile=snmp|line=10|column=4|alias=snmp_inerrors;"
            scmd += "metric=d|cfile=snmp|line=10|column=5|alias=snmp_outdatagrams;"
        elif it_view == "traffic":
            for it_device in opts.formats_devices['traffic']:
                scmd += "position=a|metric=d|cfile=dev|line_begin={item}:|column=2|alias={item}_dev_rx_bytes;".format(item=it_device)
                scmd += "position=a|metric=d|cfile=dev|line_begin={item}:|column=3|alias={item}_dev_rx_packets;".format(item=it_device)
                scmd += "position=a|metric=d|cfile=dev|line_begin={item}:|column=4|alias={item}_dev_rx_errs;".format(item=it_device)
                scmd += "position=a|metric=d|cfile=dev|line_begin={item}:|column=5|alias={item}_dev_rx_drop;".format(item=it_device)
                scmd += "position=a|metric=d|cfile=dev|line_begin={item}:|column=10|alias={item}_dev_tx_bytes;".format(item=it_device)
                scmd += "position=a|metric=d|cfile=dev|line_begin={item}:|column=11|alias={item}_dev_tx_packets;".format(item=it_device)
                scmd += "position=a|metric=d|cfile=dev|line_begin={item}:|column=12|alias={item}_dev_tx_errs;".format(item=it_device)
                scmd += "position=a|metric=d|cfile=dev|line_begin={item}:|column=13|alias={item}_dev_tx_drop;".format(item=it_device)
        elif it_view == "io":
            for it_device in opts.formats_devices['io']:
                scmd += "metric=d:cfile=diskstats:line_begin={item}:column=4:alias={item}_diskstats_rd_ios;".format(item=it_device)
                scmd += "metric=d:cfile=diskstats:line_begin={item}:column=5:alias={item}_diskstats_rd_merges;".format(item=it_device)
                scmd += "metric=d:cfile=diskstats:line_begin={item}:column=6:alias={item}_diskstats_rd_sectors;".format(item=it_device)
                scmd += "metric=d:cfile=diskstats:line_begin={item}:column=7:alias={item}_diskstats_rd_ticks;".format(item=it_device)
                scmd += "metric=d:cfile=diskstats:line_begin={item}:column=8:alias={item}_diskstats_wr_ios;".format(item=it_device)
                scmd += "metric=d:cfile=diskstats:line_begin={item}:column=9:alias={item}_diskstats_wr_merges;".format(item=it_device)
                scmd += "metric=d:cfile=diskstats:line_begin={item}:column=10:alias={item}_diskstats_wr_sectors;".format(item=it_device)
                scmd += "metric=d:cfile=diskstats:line_begin={item}:column=11:alias={item}_diskstats_wr_ticks;".format(item=it_device)
                scmd += "metric=d:cfile=diskstats:line_begin={item}:column=13:alias={item}_diskstats_ticks;".format(item=it_device)
                scmd += "metric=d:cfile=diskstats:line_begin={item}:column=14:alias={item}_diskstats_aveq;".format(item=it_device)
        elif it_view == "pcsw":
            scmd += "metric=d|cfile=stat|line_begin=ctxt|column=2|alias=stat_ctxt;"
            scmd += "metric=d|cfile=stat|line_begin=processes|column=2|alias=stat_processes;"
        elif it_view == "tcpx":
            scmd += "metric=c|cfile=meminfo|line_begin=Cached:|column=2|alias=meminfo_cached;"
        elif it_view == "load":
            scmd += "metric=c|cfile=loadavg|line=1|column=1|dtype=float|alias=loadavg_load1;"
            scmd += "metric=c|cfile=loadavg|line=1|column=2|dtype=float|alias=loadavg_load5;"
            scmd += "metric=c|cfile=loadavg|line=1|column=3|dtype=float|alias=loadavg_load15;"
            scmd += "metric=c|cfile=loadavg|line=1|column=4|dtype=string|alias=loadavg_runq_plit;"
        elif it_view == "tcpofo":
            tcp_ext = get_line_file("TcpExt:", "/proc/net/netstat")

            column_OfoPruned    = get_index_word("OfoPruned",       tcp_ext)
            column_DSACKOfoSent = get_index_word("TCPDSACKOfoSent", tcp_ext)
            column_DSACKOfoRecv = get_index_word("TCPDSACKOfoRecv", tcp_ext)
            column_OFOQueue     = get_index_word("TCPOFOQueue",     tcp_ext)
            column_OFODrop      = get_index_word("TCPOFODrop",      tcp_ext)
            column_OFOMerge     = get_index_word("TCPOFOMerge",     tcp_ext)
            
            '''
            3.10 and default:
                column_OfoPruned    = 8
                column_DSACKOfoSent = 57
                column_DSACKOfoRecv = 59
                column_OFOQueue     = 85
                column_OFODrop      = 86
                column_OFOMerge     = 87
            4.9:
                column_OFOQueue     = 86
                column_OFODrop      = 87
                column_OFOMerge     = 88
            4.19:
                column_DSACKOfoSent = 48
                column_DSACKOfoRecv = 50
                column_OFOQueue     = 79
                column_OFODrop      = 80
                column_OFOMerge     = 81
            '''
            
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_OfoPruned;".format(column_num=column_OfoPruned)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_DSACKOfoSent;".format(column_num=column_DSACKOfoSent)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_DSACKOfoRecv;".format(column_num=column_DSACKOfoRecv)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_OFOQueue;".format(column_num=column_OFOQueue)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_OFODrop;".format(column_num=column_OFODrop)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_OFOMerge;".format(column_num=column_OFOMerge)
        elif it_view == "retran":
            tcp_ext = get_line_file("TcpExt:", "/proc/net/netstat")

            column_LostRetransmit   = get_index_word("TCPLostRetransmit",   tcp_ext)
            column_FastRetrans      = get_index_word("TCPFastRetrans",      tcp_ext)
            column_ForwardRetrans   = get_index_word("TCPForwardRetrans",   tcp_ext)
            column_SlowStartRetrans = get_index_word("TCPSlowStartRetrans", tcp_ext)
            column_Timeouts         = get_index_word("TCPTimeouts",         tcp_ext)
            column_LossProbes       = get_index_word("TCPLossProbes",       tcp_ext)
            column_RetransFail      = get_index_word("TCPRetransFail",      tcp_ext)
            column_SynRetrans       = get_index_word("TCPSynRetrans",       tcp_ext)

            '''
            3.10 and default:
                column_LostRetransmit   = 42
                column_FastRetrans      = 46
                column_ForwardRetrans   = 47
                column_SlowStartRetrans = 48
                column_Timeouts         = 49
                column_LossProbes       = 50
                column_RetransFail      = 83
                column_SynRetrans       = 102
            4.9:
                column_RetransFail      = 84
                column_SynRetrans       = 103
            4.19:
                column_LostRetransmit   = 35
                column_FastRetrans      = 39
                column_ForwardRetrans   = -1               # ForwardRetrans is not exist in 4.19
                column_SlowStartRetrans = 40
                column_Timeouts         = 41
                column_LossProbes       = 42
                column_RetransFail      = 77
                column_SynRetrans       = 97
            '''

            scmd += "metric=d|cfile=snmp|line=8|column=13|alias=snmp_RetransSegs;"
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_LostRetransmit;".format(column_num=column_LostRetransmit)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_FastRetrans;".format(column_num=column_FastRetrans)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_ForwardRetrans;".format(column_num=column_ForwardRetrans)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_SlowStartRetrans;".format(column_num=column_SlowStartRetrans)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_RTORetrans;".format(column_num=column_Timeouts)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_LossProbes;".format(column_num=column_LossProbes)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_RetransFail;".format(column_num=column_RetransFail)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_SynRetrans;".format(column_num=column_SynRetrans)
        elif it_view == "tcpdrop":
            tcp_ext = get_line_file("TcpExt:", "/proc/net/netstat")

            column_LockDroppedIcmps = get_index_word("LockDroppedIcmps",   tcp_ext)
            column_ListenDrops      = get_index_word("ListenDrops",        tcp_ext) 
            column_ListenOverflows  = get_index_word("ListenOverflows",    tcp_ext)
            column_PrequeueDropped  = get_index_word("TCPPrequeueDropped", tcp_ext)
            column_BacklogDrop      = get_index_word("TCPBacklogDrop",     tcp_ext)
            column_MinTTLDrop       = get_index_word("TCPMinTTLDrop",      tcp_ext)
            column_DeferAcceptDrop  = get_index_word("TCPDeferAcceptDrop", tcp_ext)
            column_ReqQFullDrop     = get_index_word("TCPReqQFullDrop",    tcp_ext)
            column_OFODrop          = get_index_word("TCPOFODrop",         tcp_ext)

            '''
            3.10 and default:
                column_LockDroppedIcmps = 10
                column_ListenDrops      = 22
                column_ListenOverflows  = 21
                column_PrequeueDropped  = 26
                column_BacklogDrop      = 76
                column_MinTTLDrop       = 77
                column_DeferAcceptDrop  = 78
                column_ReqQFullDrop     = 82
                column_OFODrop          = 86
            4.9:
                column_BacklogDrop      = 77
                column_MinTTLDrop       = 78
                column_DeferAcceptDrop  = 79
                column_ReqQFullDrop     = 83
                column_OFODrop          = 87
            4.19:
                column_BacklogDrop      = 77
                column_ListenDrops      = 21
                column_ListenOverflows  = 20
                column_PrequeueDropped  = -1               # PrequeueDropped is not exist in 4.19
                column_BacklogDrop      = 69
                column_MinTTLDrop       = 71
                column_DeferAcceptDrop  = 72
                column_ReqQFullDrop     = 76
                column_OFODrop          = 80
            '''
 
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_LockDroppedIcmps;".format(column_num=column_LockDroppedIcmps)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_ListenDrops;".format(column_num=column_ListenDrops)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_ListenOverflows;".format(column_num=column_ListenOverflows)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_PrequeueDropped;".format(column_num=column_PrequeueDropped)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_BacklogDrop;".format(column_num=column_BacklogDrop)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_MinTTLDrop;".format(column_num=column_MinTTLDrop)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_DeferAcceptDrop;".format(column_num=column_DeferAcceptDrop)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_ReqQFullDrop;".format(column_num=column_ReqQFullDrop)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_OFODrop;".format(column_num=column_OFODrop)
        elif it_view == "tcperr":
            tcp_ext = get_line_file("TcpExt:", "/proc/net/netstat")

            column_RenoFailures           = get_index_word("TCPRenoFailures",           tcp_ext)
            column_SackFailures           = get_index_word("TCPSackFailures",           tcp_ext)
            column_LossFailures           = get_index_word("TCPLossFailures",           tcp_ext)
            column_AbortOnMemory          = get_index_word("TCPAbortOnMemory",          tcp_ext)
            column_AbortFailed            = get_index_word("TCPAbortFailed",            tcp_ext)
            column_TimeWaitOverflow       = get_index_word("TCPTimeWaitOverflow",       tcp_ext)
            column_FastOpenListenOverflow = get_index_word("TCPFastOpenListenOverflow", tcp_ext)

            '''
            3.10 and default:
                column_RenoFailures           = 43
                column_SackFailures           = 44
                column_LossFailures           = 45
                column_AbortOnMemory          = 62
                column_AbortFailed            = 65
                column_TimeWaitOverflow       = 80
                column_FastOpenListenOverflow = 94
            4.9:
                column_TimeWaitOverflow       = 81
                column_FastOpenListenOverflow = 95
            4.19:
                column_RenoFailures           = 36
                column_SackFailures           = 37
                column_LossFailures           = 38
                column_AbortOnMemory          = 53
                column_AbortFailed            = 56
                column_TimeWaitOverflow       = 74
                column_FastOpenListenOverflow = 88
            '''

            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_RenoFailures;".format(column_num=column_RenoFailures)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_SackFailures;".format(column_num=column_SackFailures)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_LossFailures;".format(column_num=column_LossFailures)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_AbortOnMemory;".format(column_num=column_AbortOnMemory)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_AbortFailed;".format(column_num=column_AbortFailed)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_TimeWaitOverflow;".format(column_num=column_TimeWaitOverflow)
            scmd += "metric=d|cfile=netstat|line_begin=TcpExt:|column={column_num}|alias=netstat_FastOpenListenOverflow;".format(column_num=column_FastOpenListenOverflow)
        else:
            raise Exception("unrecognized view " + it_view)

    scmd += "'"
    if opts.formatted_finish:
        scmd += " -f " + opts.formatted_finish
    if opts.begin:
        scmd += " -b " + opts.begin
    if opts.interval_tsar2:
        scmd += " -i " + opts.interval_tsar2
    if opts.path:
        scmd += " --path=" + opts.path

    return scmd

def display_lines(it_line, i, agregates = {}):
    if opts.live:
        it_line['datetime'] = datetime.datetime.strptime(it_line['collect_datetime'], "%Y-%m-%dT%H:%M:%S").strftime("%d/%m/%y-%H:%M:%S")
    else:
        it_line['datetime'] = datetime.datetime.strptime(it_line['collect_datetime'], "%Y-%m-%dT%H:%M:%S").strftime("%d/%m/%y-%H:%M")

    if "cpu" in opts.picks:
        for it_device in opts.formats_devices['cpu']:
            it_stat_user       = float(it_line[it_device+'_stat_user'])
            it_stat_nice       = float(it_line[it_device+'_stat_nice'])
            it_stat_system     = float(it_line[it_device+'_stat_system'])
            it_stat_idle       = float(it_line[it_device+'_stat_idle'])
            it_stat_iowait     = float(it_line[it_device+'_stat_iowait'])
            it_stat_irq        = float(it_line[it_device+'_stat_irq'])
            it_stat_softirq    = float(it_line[it_device+'_stat_softirq'])
            it_stat_steal      = float(it_line[it_device+'_stat_steal'])
            it_stat_guest      = float(it_line[it_device+'_stat_guest'])
            it_stat_guest_nice = float(it_line[it_device+'_stat_guest_nice'])

            it_stat_cpu_times = it_stat_user +it_stat_nice +it_stat_system +it_stat_idle +it_stat_iowait +it_stat_irq +it_stat_softirq +it_stat_steal +it_stat_guest +it_stat_guest_nice
            it_cpu_user = 100 * it_stat_user    / it_stat_cpu_times
            it_cpu_sys  = 100 * it_stat_system  / it_stat_cpu_times
            it_cpu_wait = 100 * it_stat_iowait  / it_stat_cpu_times
            it_cpu_hirq = 100 * it_stat_irq     / it_stat_cpu_times
            it_cpu_sirq = 100 * it_stat_softirq / it_stat_cpu_times
            it_cpu_util = 100 * (it_stat_cpu_times - it_stat_idle - it_stat_iowait - it_stat_steal) / it_stat_cpu_times

            if not opts.live:
                agregates[it_device+'_cpu_user'].append(it_cpu_user)
                agregates[it_device+'_cpu_sys'].append(it_cpu_sys)
                agregates[it_device+'_cpu_wait'].append(it_cpu_wait)
                agregates[it_device+'_cpu_hirq'].append(it_cpu_hirq)
                agregates[it_device+'_cpu_sirq'].append(it_cpu_sirq)
                agregates[it_device+'_cpu_util'].append(it_cpu_util)

            it_line[it_device+'_cpu_user'] = '{:>7}'.format(format_bytes(it_cpu_user))
            it_line[it_device+'_cpu_sys']  = '{:>7}'.format(format_bytes(it_cpu_sys))
            it_line[it_device+'_cpu_wait'] = '{:>7}'.format(format_bytes(it_cpu_wait))
            it_line[it_device+'_cpu_hirq'] = '{:>7}'.format(format_bytes(it_cpu_hirq))
            it_line[it_device+'_cpu_sirq'] = '{:>7}'.format(format_bytes(it_cpu_sirq))
            it_line[it_device+'_cpu_util'] = '{:>7}'.format(format_bytes(it_cpu_util))

    if "mem" in opts.picks:
        meminfo_total     = 1024 * int(it_line['meminfo_total'])
        meminfo_free      = 1024 * int(it_line['meminfo_free'])
        meminfo_available = 1024 * int(it_line['meminfo_available'])
        meminfo_buffers   = 1024 * int(it_line['meminfo_buffers'])
        meminfo_cached    = 1024 * int(it_line['meminfo_cached'])
        meminfo_used      = meminfo_total - meminfo_free - meminfo_buffers - meminfo_cached
        mem_util          = 100 * float(meminfo_used) / float(meminfo_total)

        if not opts.live:
            agregates['total_mem_free'].append(meminfo_free)
            agregates['total_mem_used'].append(meminfo_used)
            agregates['total_mem_buff'].append(meminfo_buffers)
            agregates['total_mem_cach'].append(meminfo_cached)
            agregates['total_mem_total'].append(meminfo_total)
            agregates['total_mem_avail'].append(meminfo_available)
            agregates['total_mem_util'].append(mem_util)

        it_line['total_mem_free']  = '{:>7}'.format(format_bytes(meminfo_free))
        it_line['total_mem_used']  = '{:>7}'.format(format_bytes(meminfo_used))
        it_line['total_mem_buff']  = '{:>7}'.format(format_bytes(meminfo_buffers))
        it_line['total_mem_cach']  = '{:>7}'.format(format_bytes(meminfo_cached))
        it_line['total_mem_total'] = '{:>7}'.format(format_bytes(meminfo_total))
        it_line['total_mem_avail'] = '{:>7}'.format(format_bytes(meminfo_available))
        it_line['total_mem_util']  = '{:>7}'.format(format_bytes(mem_util))

    if "swap" in opts.picks:
        vmstat_pswpin     = float(it_line['vmstat_pswpin'])
        vmstat_pswpout    = float(it_line['vmstat_pswpout'])
        meminfo_swaptotal = 1024 * int(it_line['meminfo_swaptotal'])
        meminfo_swapfree  = 1024 * int(it_line['meminfo_swapfree'])

        swap_util         = (100 - 100 * meminfo_swapfree / meminfo_swaptotal) if meminfo_swaptotal else 0

        if not opts.live:
            agregates['total_swap_swpin'].append(vmstat_pswpin)
            agregates['total_swap_swpout'].append(vmstat_pswpout)
            agregates['total_swap_total'].append(meminfo_swaptotal)
            agregates['total_swap_util'].append(swap_util)

        it_line['total_swap_swpin']  = '{:>7}'.format(format_bytes(vmstat_pswpin))
        it_line['total_swap_swpout'] = '{:>7}'.format(format_bytes(vmstat_pswpout))
        it_line['total_swap_total']  = '{:>7}'.format(format_bytes(meminfo_swaptotal))
        it_line['total_swap_util']   = '{:>7}'.format(format_bytes(swap_util))

    if "tcp" in opts.picks:
        snmp_activeopens  = float(it_line['snmp_activeopens'])
        snmp_passiveopens = float(it_line['snmp_passiveopens'])
        snmp_attemptfails = float(it_line['snmp_attemptfails'])
        snmp_estabresets  = float(it_line['snmp_estabresets'])
        snmp_currestab    = float(it_line['snmp_currestab'])
        snmp_insegs       = float(it_line['snmp_insegs'])
        snmp_outsegs      = float(it_line['snmp_outsegs'])
        snmp_RetransSegs  = float(it_line['snmp_RetransSegs'])

        snmp_retran       = 100 * snmp_RetransSegs / snmp_outsegs if snmp_outsegs else 0.0

        if not opts.live:
            agregates['total_tcp_active'].append(snmp_activeopens)
            agregates['total_tcp_pasive'].append(snmp_passiveopens)
            agregates['total_tcp_AtmpFa'].append(snmp_attemptfails)
            agregates['total_tcp_EstRes'].append(snmp_estabresets)
            agregates['total_tcp_CurrEs'].append(snmp_currestab)
            agregates['total_tcp_iseg'].append(snmp_insegs)
            agregates['total_tcp_outseg'].append(snmp_outsegs)
            agregates['total_tcp_retran'].append(snmp_retran)

        it_line['total_tcp_active']  = '{:>7}'.format(format_bytes(snmp_activeopens))
        it_line['total_tcp_pasive']  = '{:>7}'.format(format_bytes(snmp_passiveopens))
        it_line['total_tcp_AtmpFa']  = '{:>7}'.format(format_bytes(snmp_attemptfails))
        it_line['total_tcp_EstRes']  = '{:>7}'.format(format_bytes(snmp_estabresets))
        it_line['total_tcp_CurrEs']  = '{:>7}'.format(format_bytes(snmp_currestab))
        it_line['total_tcp_iseg']    = '{:>7}'.format(format_bytes(snmp_insegs))
        it_line['total_tcp_outseg']  = '{:>7}'.format(format_bytes(snmp_outsegs))
        it_line['total_tcp_retran']  = '{:>7}'.format(format_bytes(snmp_retran))

    if "udp" in opts.picks:
        snmp_indatagrams  = float(it_line['snmp_indatagrams'])
        snmp_noports      = float(it_line['snmp_noports'])
        snmp_inerrors     = float(it_line['snmp_inerrors'])
        snmp_outdatagrams = float(it_line['snmp_outdatagrams'])

        if not opts.live:
            agregates['total_udp_idgm'].append(snmp_indatagrams)
            agregates['total_udp_odgm'].append(snmp_outdatagrams)
            agregates['total_udp_noport'].append(snmp_noports)
            agregates['total_udp_idmerr'].append(snmp_inerrors)

        it_line['total_udp_idgm']   = '{:>7}'.format(format_bytes(snmp_indatagrams))
        it_line['total_udp_odgm']   = '{:>7}'.format(format_bytes(snmp_outdatagrams))
        it_line['total_udp_noport'] = '{:>7}'.format(format_bytes(snmp_noports))
        it_line['total_udp_idmerr'] = '{:>7}'.format(format_bytes(snmp_inerrors))

    if "traffic" in opts.picks:
        sum_dev_rx_bytes   = 0
        sum_dev_rx_packets = 0
        sum_dev_tx_bytes   = 0
        sum_dev_tx_packets = 0
        sum_dev_errs       = 0
        sum_dev_drop       = 0
        for it_device in opts.formats_devices['traffic']:
            it_dev_rx_bytes   = float(it_line[it_device+'_dev_rx_bytes'])
            it_dev_rx_packets = float(it_line[it_device+'_dev_rx_packets'])
            it_dev_rx_errs    = float(it_line[it_device+'_dev_rx_errs'])
            it_dev_rx_drop    = float(it_line[it_device+'_dev_rx_drop'])
            it_dev_tx_bytes   = float(it_line[it_device+'_dev_tx_bytes'])
            it_dev_tx_packets = float(it_line[it_device+'_dev_tx_packets'])
            it_dev_tx_errs    = float(it_line[it_device+'_dev_tx_errs'])
            it_dev_tx_drop    = float(it_line[it_device+'_dev_tx_drop'])
            it_dev_errs       = it_dev_rx_errs + it_dev_tx_errs
            it_dev_drop       = it_dev_rx_drop + it_dev_tx_drop

            if opts.item_mode['traffic'] == "sum":
                sum_dev_rx_bytes   += it_dev_rx_bytes
                sum_dev_rx_packets += it_dev_rx_packets
                sum_dev_tx_bytes   += it_dev_tx_bytes
                sum_dev_tx_packets += it_dev_tx_packets
                sum_dev_errs       += it_dev_errs
                sum_dev_drop       += it_dev_drop
            else:
                if not opts.live:
                    agregates[it_device+'_traffic_bytin'].append(it_dev_rx_bytes)
                    agregates[it_device+'_traffic_pktin'].append(it_dev_rx_packets)
                    agregates[it_device+'_traffic_bytout'].append(it_dev_tx_bytes)
                    agregates[it_device+'_traffic_pktout'].append(it_dev_tx_packets)
                    agregates[it_device+'_traffic_pkterr'].append(it_dev_errs)
                    agregates[it_device+'_traffic_pktdrp'].append(it_dev_drop)

                it_line[it_device+'_traffic_bytin']  = '{:>7}'.format(format_bytes(it_dev_rx_bytes))
                it_line[it_device+'_traffic_pktin']  = '{:>7}'.format(format_bytes(it_dev_rx_packets))
                it_line[it_device+'_traffic_bytout'] = '{:>7}'.format(format_bytes(it_dev_tx_bytes))
                it_line[it_device+'_traffic_pktout'] = '{:>7}'.format(format_bytes(it_dev_tx_packets))
                it_line[it_device+'_traffic_pkterr'] = '{:>7}'.format(format_bytes(it_dev_errs))
                it_line[it_device+'_traffic_pktdrp'] = '{:>7}'.format(format_bytes(it_dev_drop))

        if opts.item_mode['traffic'] == "sum":
            if not opts.live:
                agregates['sum_traffic_bytin'].append(sum_dev_rx_bytes)
                agregates['sum_traffic_pktin'].append(sum_dev_rx_packets)
                agregates['sum_traffic_bytout'].append(sum_dev_tx_bytes)
                agregates['sum_traffic_pktout'].append(sum_dev_tx_packets)
                agregates['sum_traffic_pkterr'].append(sum_dev_errs)
                agregates['sum_traffic_pktdrp'].append(sum_dev_drop)

            it_line['sum_traffic_bytin']  = '{:>7}'.format(format_bytes(sum_dev_rx_bytes))
            it_line['sum_traffic_pktin']  = '{:>7}'.format(format_bytes(sum_dev_rx_packets))
            it_line['sum_traffic_bytout'] = '{:>7}'.format(format_bytes(sum_dev_tx_bytes))
            it_line['sum_traffic_pktout'] = '{:>7}'.format(format_bytes(sum_dev_tx_packets))
            it_line['sum_traffic_pkterr'] = '{:>7}'.format(format_bytes(sum_dev_errs))
            it_line['sum_traffic_pktdrp'] = '{:>7}'.format(format_bytes(sum_dev_drop))

    if "io" in opts.picks:
        # IO Notes:
        #     n_ios  = rd_ios + wr_ios
        #     rrqms  = rd_merges
        #     wrqms  = wr_merges
        #     %rrqm  = 100 * rd_merges / (rd_merges + rd_ios)  if rd_merges + rd_ios else 0.0
        #     %wrqm  = 100 * wr_merges / (wr_merges + wr_ios)  if wr_merges + wr_ios else 0.0
        #     rs     = rd_ios 
        #     ws     = wr_ios 
        #     rsecs  = rd_sectors 
        #     wsecs  = wr_sectors
        #     rqsize = (rd_sectors + wr_sectors) / (2 * n_ios) if n_ios              else 0.0
        #     rarqsz = rd_sectors / (2 * rd_ios)               if rd_ios             else 0.0
        #     warqsz = wr_sectors / (2 * wr_ios)               if wr_ios             else 0.0  
        #     qusize = aveq / 1000
        #     await  = (rd_ticks + wr_ticks) / n_ios           if n_ios              else 0.0
        #     rawait = rd_ticks / rd_ios                       if rd_ios             else 0.0  
        #     wawait = wr_ticks / wr_ios                       if wr_ios             else 0.0
        #     svctm  = ticks / n_ios                           if n_ios              else 0.0
        #     util   = ticks / 10                              if (ticks / 10) < 100 else 100

        sum_diskstats_rd_merges  = 0
        sum_diskstats_wr_merges  = 0
        sum_diskstats_rd_ios     = 0
        sum_diskstats_wr_ios     = 0
        sum_diskstats_rd_sectors = 0
        sum_diskstats_wr_sectors = 0
        sum_diskstats_rd_ticks   = 0
        sum_diskstats_wr_ticks   = 0
        sum_diskstats_ticks      = 0
        sum_diskstats_aveq       = 0
        device_size = len(opts.formats_devices['io'])
        for it_device in opts.formats_devices['io']:
            it_diskstats_rd_merges  = float(it_line[it_device+'_diskstats_rd_merges'])
            it_diskstats_wr_merges  = float(it_line[it_device+'_diskstats_wr_merges'])
            it_diskstats_rd_ios     = float(it_line[it_device+'_diskstats_rd_ios'])
            it_diskstats_wr_ios     = float(it_line[it_device+'_diskstats_wr_ios'])
            it_diskstats_rd_sectors = float(it_line[it_device+'_diskstats_rd_sectors'])
            it_diskstats_wr_sectors = float(it_line[it_device+'_diskstats_wr_sectors'])
            it_diskstats_rd_ticks   = float(it_line[it_device+'_diskstats_rd_ticks'])
            it_diskstats_wr_ticks   = float(it_line[it_device+'_diskstats_wr_ticks'])
            it_diskstats_ticks      = float(it_line[it_device+'_diskstats_ticks'])
            it_diskstats_aveq       = float(it_line[it_device+'_diskstats_aveq'])

            it_diskstats_n_ios  = it_diskstats_rd_ios + it_diskstats_wr_ios
            it_diskstats_prrqm  = 100 * it_diskstats_rd_merges / (it_diskstats_rd_merges + it_diskstats_rd_ios) if it_diskstats_rd_merges + it_diskstats_rd_ios else 0.0
            it_diskstats_pwrqm  = 100 * it_diskstats_wr_merges / (it_diskstats_wr_merges + it_diskstats_wr_ios) if it_diskstats_wr_merges + it_diskstats_wr_ios else 0.0
            it_diskstats_rqsize = (it_diskstats_rd_sectors + it_diskstats_wr_sectors) / (2 * it_diskstats_n_ios) if it_diskstats_n_ios else 0.0
            it_diskstats_rarqsz = it_diskstats_rd_sectors / (2 * it_diskstats_rd_ios) if it_diskstats_rd_ios else 0.0 
            it_diskstats_warqsz = it_diskstats_wr_sectors / (2 * it_diskstats_wr_ios) if it_diskstats_wr_ios else 0.0
            it_diskstats_qusize = it_diskstats_aveq / 1000
            it_diskstats_await  = (it_diskstats_rd_ticks + it_diskstats_wr_ticks) / it_diskstats_n_ios if it_diskstats_n_ios else 0.0
            it_diskstats_rawait = it_diskstats_rd_ticks / it_diskstats_rd_ios if it_diskstats_rd_ios else 0.0
            it_diskstats_wawait = it_diskstats_wr_ticks / it_diskstats_wr_ios if it_diskstats_wr_ios else 0.0
            it_diskstats_svctm  = it_diskstats_ticks / it_diskstats_n_ios if it_diskstats_n_ios else 0.0
            it_diskstats_util   = it_diskstats_ticks / 10 if (it_diskstats_ticks / 10) < 100 else 100

            if opts.item_mode['io'] == "sum":
                sum_diskstats_rd_merges  += it_diskstats_rd_merges
                sum_diskstats_wr_merges  += it_diskstats_wr_merges
                sum_diskstats_rd_ios     += it_diskstats_rd_ios
                sum_diskstats_wr_ios     += it_diskstats_wr_ios
                sum_diskstats_rd_sectors += it_diskstats_rd_sectors
                sum_diskstats_wr_sectors += it_diskstats_wr_sectors
                sum_diskstats_rd_ticks   += it_diskstats_rd_ticks
                sum_diskstats_wr_ticks   += it_diskstats_wr_ticks
                sum_diskstats_ticks      += it_diskstats_ticks
                sum_diskstats_aveq       += it_diskstats_aveq
            else:
                if not opts.live:
                    agregates[it_device+'_io_rrqms'].append(it_diskstats_rd_merges)
                    agregates[it_device+'_io_wrqms'].append(it_diskstats_wr_merges)
                    agregates[it_device+'_io_%rrqm'].append(it_diskstats_prrqm)
                    agregates[it_device+'_io_%wrqm'].append(it_diskstats_pwrqm)
                    agregates[it_device+'_io_rs'].append(it_diskstats_rd_ios)
                    agregates[it_device+'_io_ws'].append(it_diskstats_wr_ios)
                    agregates[it_device+'_io_rsecs'].append(it_diskstats_rd_sectors)
                    agregates[it_device+'_io_wsecs'].append(it_diskstats_wr_sectors)
                    agregates[it_device+'_io_rqsize'].append(it_diskstats_rqsize)
                    agregates[it_device+'_io_rarqsz'].append(it_diskstats_rarqsz)
                    agregates[it_device+'_io_warqsz'].append(it_diskstats_warqsz)
                    agregates[it_device+'_io_qusize'].append(it_diskstats_qusize)
                    agregates[it_device+'_io_await'].append(it_diskstats_await)
                    agregates[it_device+'_io_rawait'].append(it_diskstats_rawait)
                    agregates[it_device+'_io_wawait'].append(it_diskstats_wawait)
                    agregates[it_device+'_io_svctm'].append(it_diskstats_svctm)
                    agregates[it_device+'_io_util'].append(it_diskstats_util)

                it_line[it_device+'_io_rrqms']  = '{:>7}'.format(format_bytes(it_diskstats_rd_merges))
                it_line[it_device+'_io_wrqms']  = '{:>7}'.format(format_bytes(it_diskstats_wr_merges))
                it_line[it_device+'_io_%rrqm']  = '{:>7}'.format(format_bytes(it_diskstats_prrqm))
                it_line[it_device+'_io_%wrqm']  = '{:>7}'.format(format_bytes(it_diskstats_pwrqm))
                it_line[it_device+'_io_rs']     = '{:>7}'.format(format_bytes(it_diskstats_rd_ios))
                it_line[it_device+'_io_ws']     = '{:>7}'.format(format_bytes(it_diskstats_wr_ios))
                it_line[it_device+'_io_rsecs']  = '{:>7}'.format(format_bytes(it_diskstats_rd_sectors))
                it_line[it_device+'_io_wsecs']  = '{:>7}'.format(format_bytes(it_diskstats_wr_sectors))
                it_line[it_device+'_io_rqsize'] = '{:>7}'.format(format_bytes(it_diskstats_rqsize))
                it_line[it_device+'_io_rarqsz'] = '{:>7}'.format(format_bytes(it_diskstats_rarqsz))
                it_line[it_device+'_io_warqsz'] = '{:>7}'.format(format_bytes(it_diskstats_warqsz))
                it_line[it_device+'_io_qusize'] = '{:>7}'.format(format_bytes(it_diskstats_qusize))
                it_line[it_device+'_io_await']  = '{:>7}'.format(format_bytes(it_diskstats_await))
                it_line[it_device+'_io_rawait']  = '{:>7}'.format(format_bytes(it_diskstats_rawait))
                it_line[it_device+'_io_wawait']  = '{:>7}'.format(format_bytes(it_diskstats_wawait))
                it_line[it_device+'_io_svctm']  = '{:>7}'.format(format_bytes(it_diskstats_svctm))
                it_line[it_device+'_io_util']   = '{:>7}'.format(format_bytes(it_diskstats_util))

        if opts.item_mode['io'] == "sum":
            sum_diskstats_n_ios  = sum_diskstats_rd_ios + sum_diskstats_wr_ios
            sum_diskstats_prrqm  = 100 * sum_diskstats_rd_merges / (sum_diskstats_rd_merges + sum_diskstats_rd_ios) if sum_diskstats_rd_merges + sum_diskstats_rd_ios else 0.0
            sum_diskstats_pwrqm  = 100 * sum_diskstats_wr_merges / (sum_diskstats_wr_merges + sum_diskstats_wr_ios) if sum_diskstats_wr_merges + sum_diskstats_wr_ios else 0.0
            sum_diskstats_rqsize = (sum_diskstats_rd_sectors + sum_diskstats_wr_sectors) / (2 * sum_diskstats_n_ios) if sum_diskstats_n_ios else 0.0
            sum_diskstats_rarqsz = sum_diskstats_rd_sectors / (2 * sum_diskstats_rd_ios) if sum_diskstats_rd_ios else 0.0 
            sum_diskstats_warqsz = sum_diskstats_wr_sectors / (2 * sum_diskstats_wr_ios) if sum_diskstats_wr_ios else 0.0
            sum_diskstats_qusize = sum_diskstats_aveq / 1000 / device_size
            sum_diskstats_await  = (sum_diskstats_rd_ticks + sum_diskstats_wr_ticks) / sum_diskstats_n_ios if sum_diskstats_n_ios else 0.0
            sum_diskstats_rawait = sum_diskstats_rd_ticks / sum_diskstats_rd_ios if sum_diskstats_rd_ios else 0.0
            sum_diskstats_wawait = sum_diskstats_wr_ticks / sum_diskstats_wr_ios if sum_diskstats_wr_ios else 0.0
            sum_diskstats_svctm  = sum_diskstats_ticks / sum_diskstats_n_ios if sum_diskstats_n_ios else 0.0
            sum_diskstats_util   = sum_diskstats_ticks / 10 / device_size if (sum_diskstats_ticks / 10 / device_size) < 100 else 100

            if not opts.live:
                agregates['sum_io_rrqms'].append(sum_diskstats_rd_merges)
                agregates['sum_io_wrqms'].append(sum_diskstats_wr_merges)
                agregates['sum_io_%rrqm'].append(sum_diskstats_prrqm)
                agregates['sum_io_%wrqm'].append(sum_diskstats_pwrqm)
                agregates['sum_io_rs'].append(sum_diskstats_rd_ios)
                agregates['sum_io_ws'].append(sum_diskstats_wr_ios)
                agregates['sum_io_rsecs'].append(sum_diskstats_rd_sectors)
                agregates['sum_io_wsecs'].append(sum_diskstats_wr_sectors)
                agregates['sum_io_rqsize'].append(sum_diskstats_rqsize)
                agregates['sum_io_rarqsz'].append(sum_diskstats_rarqsz)
                agregates['sum_io_warqsz'].append(sum_diskstats_warqsz)
                agregates['sum_io_qusize'].append(sum_diskstats_qusize)
                agregates['sum_io_await'].append(sum_diskstats_await)
                agregates['sum_io_rawait'].append(sum_diskstats_rawait)
                agregates['sum_io_wawait'].append(sum_diskstats_wawait)
                agregates['sum_io_svctm'].append(sum_diskstats_svctm)
                agregates['sum_io_util'].append(sum_diskstats_util)

            it_line['sum_io_rrqms']  = '{:>7}'.format(format_bytes(sum_diskstats_rd_merges))
            it_line['sum_io_wrqms']  = '{:>7}'.format(format_bytes(sum_diskstats_wr_merges))
            it_line['sum_io_%rrqm']  = '{:>7}'.format(format_bytes(sum_diskstats_prrqm))
            it_line['sum_io_%wrqm']  = '{:>7}'.format(format_bytes(sum_diskstats_pwrqm))
            it_line['sum_io_rs']     = '{:>7}'.format(format_bytes(sum_diskstats_rd_ios))
            it_line['sum_io_ws']     = '{:>7}'.format(format_bytes(sum_diskstats_wr_ios))
            it_line['sum_io_rsecs']  = '{:>7}'.format(format_bytes(sum_diskstats_rd_sectors))
            it_line['sum_io_wsecs']  = '{:>7}'.format(format_bytes(sum_diskstats_wr_sectors))
            it_line['sum_io_rqsize'] = '{:>7}'.format(format_bytes(sum_diskstats_rqsize))
            it_line['sum_io_rarqsz'] = '{:>7}'.format(format_bytes(sum_diskstats_rarqsz))
            it_line['sum_io_warqsz'] = '{:>7}'.format(format_bytes(sum_diskstats_warqsz))
            it_line['sum_io_qusize'] = '{:>7}'.format(format_bytes(sum_diskstats_qusize))
            it_line['sum_io_await']  = '{:>7}'.format(format_bytes(sum_diskstats_await))
            it_line['sum_io_rawait'] = '{:>7}'.format(format_bytes(sum_diskstats_rawait))
            it_line['sum_io_wawait'] = '{:>7}'.format(format_bytes(sum_diskstats_wawait))
            it_line['sum_io_svctm']  = '{:>7}'.format(format_bytes(sum_diskstats_svctm))
            it_line['sum_io_util']   = '{:>7}'.format(format_bytes(sum_diskstats_util))

    if "pcsw" in opts.picks:
        stat_ctxt      = float(it_line['stat_ctxt'])
        stat_processes = float(it_line['stat_processes'])

        if not opts.live:
            agregates['total_pcsw_cswch'].append(stat_ctxt)
            agregates['total_pcsw_proc'].append(stat_processes)

        it_line['total_pcsw_cswch'] = '{:>7}'.format(format_bytes(stat_ctxt))
        it_line['total_pcsw_proc']  = '{:>7}'.format(format_bytes(stat_processes))

    if "tcpx" in opts.picks:
        print("tcpx comming soon.")
        sys.exit(200);

    if "load" in opts.picks:
        loadavg_load1     = float(it_line['loadavg_load1'])
        loadavg_load5     = float(it_line['loadavg_load5'])
        loadavg_load15    = float(it_line['loadavg_load15'])
        loadavg_runq_plit = it_line['loadavg_runq_plit']
        loadavg_runq      = float(loadavg_runq_plit.split("/")[0])
        loadavg_plit      = float(loadavg_runq_plit.split("/")[1])
            
        if not opts.live:
            agregates['total_load_load1'].append(loadavg_load1)
            agregates['total_load_load5'].append(loadavg_load5)
            agregates['total_load_load15'].append(loadavg_load15)
            agregates['total_load_runq'].append(loadavg_runq)
            agregates['total_load_plit'].append(loadavg_plit)

        it_line['total_load_load1']  = '{:>7}'.format(format_bytes(loadavg_load1))
        it_line['total_load_load5']  = '{:>7}'.format(format_bytes(loadavg_load5))
        it_line['total_load_load15'] = '{:>7}'.format(format_bytes(loadavg_load15))
        it_line['total_load_runq']   = '{:>7}'.format(format_bytes(loadavg_runq))
        it_line['total_load_plit']   = '{:>7}'.format(format_bytes(loadavg_plit))

    if "tcpofo" in opts.picks:
        netstat_OfoPruned    = float(it_line['netstat_OfoPruned'])
        netstat_DSACKOfoSent = float(it_line['netstat_DSACKOfoSent'])
        netstat_DSACKOfoRecv = float(it_line['netstat_DSACKOfoRecv'])
        netstat_OFOQueue     = float(it_line['netstat_OFOQueue'])
        netstat_OFODrop      = float(it_line['netstat_OFODrop'])
        netstat_OFOMerge     = float(it_line['netstat_OFOMerge'])

        if not opts.live:
            agregates['total_tcpofo_OfoPruned'].append(netstat_OfoPruned)
            agregates['total_tcpofo_DSACKOfoSent'].append(netstat_DSACKOfoSent)
            agregates['total_tcpofo_DSACKOfoRecv'].append(netstat_DSACKOfoRecv)
            agregates['total_tcpofo_OFOQueue'].append(netstat_OFOQueue)
            agregates['total_tcpofo_OFODrop'].append(netstat_OFODrop)
            agregates['total_tcpofo_OFOMerge'].append(netstat_OFOMerge)

        it_line['total_tcpofo_OfoPruned']    = '{:>{width}}'.format(format_bytes(netstat_OfoPruned), width=get_width('tcpofo', 'OfoPruned'))
        it_line['total_tcpofo_DSACKOfoSent'] = '{:>{width}}'.format(format_bytes(netstat_DSACKOfoSent), width=get_width('tcpofo', 'DSACKOfoSent'))
        it_line['total_tcpofo_DSACKOfoRecv'] = '{:>{width}}'.format(format_bytes(netstat_DSACKOfoRecv), width=get_width('tcpofo', 'DSACKOfoRecv'))
        it_line['total_tcpofo_OFOQueue']     = '{:>{width}}'.format(format_bytes(netstat_OFOQueue), width=get_width('tcpofo', 'OFOQueue'))
        it_line['total_tcpofo_OFODrop']      = '{:>{width}}'.format(format_bytes(netstat_OFODrop), width=get_width('tcpofo', 'OFODrop'))
        it_line['total_tcpofo_OFOMerge']     = '{:>{width}}'.format(format_bytes(netstat_OFOMerge), width=get_width('tcpofo', 'OFOMerge'))

    if "retran" in opts.picks:
        snmp_RetransSegs         = float(it_line['snmp_RetransSegs'])
        netstat_LostRetransmit   = float(it_line['netstat_LostRetransmit'])
        netstat_FastRetrans      = float(it_line['netstat_FastRetrans'])
        if "netstat_ForwardRetrans" in it_line:
            netstat_ForwardRetrans = float(it_line['netstat_ForwardRetrans'])
        else:
            netstat_ForwardRetrans = 0;                                             # ForwardRetrans is not exist in 4.19, replace with 0         
        netstat_SlowStartRetrans = float(it_line['netstat_SlowStartRetrans'])
        netstat_RTORetrans       = float(it_line['netstat_RTORetrans'])
        netstat_LossProbes       = float(it_line['netstat_LossProbes'])
        netstat_RetransFail      = float(it_line['netstat_RetransFail'])
        netstat_SynRetrans       = float(it_line['netstat_SynRetrans'])
        retran_RTORatio          = float(100 * netstat_RTORetrans / snmp_RetransSegs) if snmp_RetransSegs else 0.0

        if not opts.live:
            agregates['total_retran_RetransSegs'].append(snmp_RetransSegs)
            agregates['total_retran_LostRetransmit'].append(netstat_LostRetransmit)
            agregates['total_retran_FastRetrans'].append(netstat_FastRetrans)
            agregates['total_retran_ForwardRetrans'].append(netstat_ForwardRetrans)
            agregates['total_retran_SlowStartRetrans'].append(netstat_SlowStartRetrans)
            agregates['total_retran_RTORetrans'].append(netstat_RTORetrans)
            agregates['total_retran_LossProbes'].append(netstat_LossProbes)
            agregates['total_retran_RetransFail'].append(netstat_RetransFail)
            agregates['total_retran_SynRetrans'].append(netstat_SynRetrans)
            agregates['total_retran_RTORatio'].append(retran_RTORatio)

        it_line['total_retran_RetransSegs']      = '{:>{width}}'.format(format_bytes(snmp_RetransSegs), width=get_width('retran', 'RetransSegs'))
        it_line['total_retran_LostRetransmit']   = '{:>{width}}'.format(format_bytes(netstat_LostRetransmit), width=get_width('retran', 'LostRetransmit'))
        it_line['total_retran_FastRetrans']      = '{:>{width}}'.format(format_bytes(netstat_FastRetrans), width=get_width('retran', 'FastRetrans'))
        it_line['total_retran_ForwardRetrans']   = '{:>{width}}'.format(format_bytes(netstat_ForwardRetrans), width=get_width('retran', 'ForwardRetrans'))
        it_line['total_retran_SlowStartRetrans'] = '{:>{width}}'.format(format_bytes(netstat_SlowStartRetrans), width=get_width('retran', 'SlowStartRetrans'))
        it_line['total_retran_RTORetrans']       = '{:>{width}}'.format(format_bytes(netstat_RTORetrans), width=get_width('retran', 'RTORetrans'))
        it_line['total_retran_LossProbes']       = '{:>{width}}'.format(format_bytes(netstat_LossProbes), width=get_width('retran', 'LossProbes'))
        it_line['total_retran_RetransFail']      = '{:>{width}}'.format(format_bytes(netstat_RetransFail), width=get_width('retran', 'RetransFail'))
        it_line['total_retran_SynRetrans']       = '{:>{width}}'.format(format_bytes(netstat_SynRetrans), width=get_width('retran', 'SynRetrans'))
        it_line['total_retran_RTORatio']         = '{:>{width}}'.format(format_bytes(retran_RTORatio), width=get_width('retran', 'RTORatio'))

    if "tcpdrop" in opts.picks:
        netstat_LockDroppedIcmps = float(it_line['netstat_LockDroppedIcmps'])
        netstat_ListenDrops      = float(it_line['netstat_ListenDrops'])
        netstat_ListenOverflows  = float(it_line['netstat_ListenOverflows'])
        if "netstat_PrequeueDropped" in it_line:
            netstat_PrequeueDropped  = float(it_line['netstat_PrequeueDropped'])
        else:
            netstat_PrequeueDropped = 0;                                           # PrequeueDropped is not exist in 4.19, replace with 0         
        netstat_BacklogDrop      = float(it_line['netstat_BacklogDrop'])
        netstat_MinTTLDrop       = float(it_line['netstat_MinTTLDrop'])
        netstat_DeferAcceptDrop  = float(it_line['netstat_DeferAcceptDrop'])
        netstat_ReqQFullDrop     = float(it_line['netstat_ReqQFullDrop'])
        netstat_OFODrop          = float(it_line['netstat_OFODrop'])

        if not opts.live:
            agregates['total_tcpdrop_LockDroppedIcmps'].append(netstat_LockDroppedIcmps)
            agregates['total_tcpdrop_ListenDrops'].append(netstat_ListenDrops)
            agregates['total_tcpdrop_ListenOverflows'].append(netstat_ListenOverflows)
            agregates['total_tcpdrop_PrequeueDropped'].append(netstat_PrequeueDropped)
            agregates['total_tcpdrop_BacklogDrop'].append(netstat_BacklogDrop)
            agregates['total_tcpdrop_MinTTLDrop'].append(netstat_MinTTLDrop)
            agregates['total_tcpdrop_DeferAcceptDrop'].append(netstat_DeferAcceptDrop)
            agregates['total_tcpdrop_ReqQFullDrop'].append(netstat_ReqQFullDrop)
            agregates['total_tcpdrop_OFODrop'].append(netstat_OFODrop)

        it_line['total_tcpdrop_LockDroppedIcmps'] = '{:>{width}}'.format(format_bytes(netstat_LockDroppedIcmps), width=get_width('tcpdrop', 'LockDroppedIcmps'))
        it_line['total_tcpdrop_ListenDrops']      = '{:>{width}}'.format(format_bytes(netstat_ListenDrops), width=get_width('tcpdrop', 'ListenDrops'))
        it_line['total_tcpdrop_ListenOverflows']  = '{:>{width}}'.format(format_bytes(netstat_ListenOverflows), width=get_width('tcpdrop', 'ListenOverflows'))
        it_line['total_tcpdrop_PrequeueDropped']  = '{:>{width}}'.format(format_bytes(netstat_PrequeueDropped), width=get_width('tcpdrop', 'PrequeueDropped'))
        it_line['total_tcpdrop_BacklogDrop']      = '{:>{width}}'.format(format_bytes(netstat_BacklogDrop), width=get_width('tcpdrop', 'BacklogDrop'))
        it_line['total_tcpdrop_MinTTLDrop']       = '{:>{width}}'.format(format_bytes(netstat_MinTTLDrop), width=get_width('tcpdrop', 'MinTTLDrop'))
        it_line['total_tcpdrop_DeferAcceptDrop']  = '{:>{width}}'.format(format_bytes(netstat_DeferAcceptDrop), width=get_width('tcpdrop', 'DeferAcceptDrop'))
        it_line['total_tcpdrop_ReqQFullDrop']     = '{:>{width}}'.format(format_bytes(netstat_ReqQFullDrop), width=get_width('tcpdrop', 'ReqQFullDrop'))
        it_line['total_tcpdrop_OFODrop']          = '{:>{width}}'.format(format_bytes(netstat_OFODrop), width=get_width('tcpdrop', 'OFODrop'))

    if "tcperr" in opts.picks:
        netstat_RenoFailures           = float(it_line['netstat_RenoFailures'])
        netstat_SackFailures           = float(it_line['netstat_SackFailures'])
        netstat_LossFailures           = float(it_line['netstat_LossFailures'])
        netstat_AbortOnMemory          = float(it_line['netstat_AbortOnMemory'])
        netstat_AbortFailed            = float(it_line['netstat_AbortFailed'])
        netstat_TimeWaitOverflow       = float(it_line['netstat_TimeWaitOverflow'])
        netstat_FastOpenListenOverflow = float(it_line['netstat_FastOpenListenOverflow'])

        if not opts.live:
            agregates['total_tcperr_RenoFailures'].append(netstat_RenoFailures)
            agregates['total_tcperr_SackFailures'].append(netstat_SackFailures)
            agregates['total_tcperr_LossFailures'].append(netstat_LossFailures)
            agregates['total_tcperr_AbortOnMemory'].append(netstat_AbortOnMemory)
            agregates['total_tcperr_AbortFailed'].append(netstat_AbortFailed)
            agregates['total_tcperr_TimeWaitOverflow'].append(netstat_TimeWaitOverflow)
            agregates['total_tcperr_FastOpenListenOverflow'].append(netstat_FastOpenListenOverflow)

        it_line['total_tcperr_RenoFailures']           = '{:>{width}}'.format(format_bytes(netstat_RenoFailures), width=get_width('tcperr', 'RenoFailures'))
        it_line['total_tcperr_SackFailures']           = '{:>{width}}'.format(format_bytes(netstat_SackFailures), width=get_width('tcperr', 'SackFailures'))
        it_line['total_tcperr_LossFailures']           = '{:>{width}}'.format(format_bytes(netstat_LossFailures), width=get_width('tcperr', 'LossFailures'))
        it_line['total_tcperr_AbortOnMemory']          = '{:>{width}}'.format(format_bytes(netstat_AbortOnMemory), width=get_width('tcperr', 'AbortOnMemory'))
        it_line['total_tcperr_AbortFailed']            = '{:>{width}}'.format(format_bytes(netstat_AbortFailed), width=get_width('tcperr', 'AbortFailed'))
        it_line['total_tcperr_TimeWaitOverflow']       = '{:>{width}}'.format(format_bytes(netstat_TimeWaitOverflow), width=get_width('tcperr', 'TimeWaitOverflow'))
        it_line['total_tcperr_FastOpenListenOverflow'] = '{:>{width}}'.format(format_bytes(netstat_FastOpenListenOverflow), width=get_width('tcperr', 'FastOpenListenOverflow'))

    #print opts.formated
    if i % opts.area_line == 0:
        if not opts.live:
            line_out = '{:<14}'.format("Time")
        else:
            line_out = '{:<17}'.format("Time")
        for it_combine in opts.formated:
            line_out += " " + '{:-^{width}}'.format(it_combine['header'], width=get_width(it_combine['view'], it_combine['indicator']))
        print(line_out)
        if not opts.live:
            line_out = '{:<14}'.format("Time")
        else:
            line_out = '{:<17}'.format("Time")
        for it_combine in opts.formated:
            line_out += " " + '{:>{width}}'.format(it_combine['indicator'], width=get_width(it_combine['view'], it_combine['indicator']))
        print(line_out)

    line_out = it_line['datetime']
    for it_combine in opts.formated:
        line_out += " " + it_line[it_combine['item'] + '_' + it_combine['view'] + '_' + it_combine['indicator']]
    print(line_out)
    sys.stdout.flush()

def display_agregates(label, func, agregates):
    it_line = {}
    for it_view in opts.picks:
        if opts.formated_type.get(it_view) and opts.formated_type[it_view]:
            it_devices = opts.formated_devices[it_view]
        else:
            it_devices = opts.formats_devices[it_view]

        for it_device in it_devices:
            for it_indicator in opts.arrange[it_view]:
                it_result = 0
                if agregates[it_device+'_'+it_view+'_'+it_indicator]:
                    it_result =  (eval(func)(agregates[it_device+'_'+it_view+'_'+it_indicator]))
                it_line[it_device+'_'+it_view+'_'+it_indicator] = '{:>{width}}'.format(format_bytes(it_result), width=get_width(it_view, it_indicator))

    line_out = '{:<14}'.format(label)
    for it_combine in opts.formated:
        line_out += " " + it_line[it_combine['item'] + '_' + it_combine['view'] + '_' + it_combine['indicator']]
    print(line_out)

def report_live_sys():
    ssar_cmd = concatenate_ssar()
    ssar_output = subprocess.Popen(ssar_cmd, shell=True, stdout=subprocess.PIPE,stderr=subprocess.PIPE, bufsize=1)
    i = 0
    while True:
        line = ssar_output.stdout.readline().strip()
        if not line: 
            break
        else:
            display_lines(json.loads(line), i)
            i = i + 1

def report_sys():
    ssar_cmd = concatenate_ssar()
    ssar_output = subprocess.Popen(ssar_cmd, shell=True, stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    output, errors = ssar_output.communicate()
    list_data = []
    if ssar_output.returncode == 0:
        list_data = json.loads(output)

    agregates = {}
    for it_combine in opts.formated:
        for it_indicator in vi[it_combine['view']]:
            agregates[it_combine['item']+'_'+it_combine['view']+'_'+it_indicator] = []

    i = 0
    for it_line in list_data:
        display_lines(it_line, i, agregates)
        i = i+1

    print("")
    display_agregates("MAX",  "max", agregates)
    display_agregates("MEAN", "avg", agregates)
    display_agregates("MIN",  "min", agregates)

def activity_reporter(optl):
    global opts
    opts = optl
    try:
        init_options()
        init_env()
    except Exception as e:
        print("Exception: " + str(e))
        traceback.print_exc()
        sys.exit(200);

    if "sys" == opts.cmd:
        try:
            init_sys()
            if opts.live:
                report_live_sys()
            else:
                report_sys()
        except KeyboardInterrupt as e:
            sys.stdout.write('\n')
        except Exception as e:
            print("Exception: " + str(e))
            traceback.print_exc()
            sys.exit(205);
    elif "irqtop" == opts.cmd:
        try:
            init_irqtop()
            if opts.live:
                irqtop_live()
            else:
                irqtop()
        except KeyboardInterrupt as e:
            sys.stdout.write('\n')
        except Exception as e:
            print("Exception: " + str(e))
            traceback.print_exc()
            sys.exit(205);
    elif "cputop" == opts.cmd:
        try:
            init_cputop()
            if opts.live:
                cputop_live()
            else:
                cputop()
        except KeyboardInterrupt as e:
            sys.stdout.write('\n')
        except Exception as e:
            print("Exception: " + str(e))
            traceback.print_exc()
            sys.exit(205);
    elif "procs" == opts.cmd:
        procs()
    elif "workers" == opts.cmd:
        workers()
    else:
        print("unidentified cmd: " + opts.cmd)

def main():
    global vi
    global default_vi
    global vi_widths

    vi = OrderedDict()
    vi['cpu']     = ['user', 'sys', 'wait', 'hirq', 'sirq', 'util']
    vi['mem']     = ['free', 'used', 'buff', 'cach', 'total', 'util', 'avail']
    vi['swap']    = ['swpin', 'swpout', 'total', 'util']
    vi['tcp']     = ['active','pasive','iseg','outseg','EstRes','AtmpFa','CurrEs','retran']
    vi['udp']     = ['idgm', 'odgm', 'noport', 'idmerr']
    vi['traffic'] = ['bytin', 'bytout', 'pktin', 'pktout','pkterr','pktdrp']
    vi['io']      = ['rrqms', 'wrqms', '%rrqm', '%wrqm', 'rs', 'ws', 'rsecs', 'wsecs', 'rqsize', 'rarqsz', 'warqsz', 'qusize', 'await', 'rawait', 'wawait', 'svctm', 'util']
    vi['pcsw']    = ['cswch', 'proc']
    vi['tcpx']    = ['recvq', 'sendq', 'est', 'twait', 'fwait1', 'fwait2', 'lisq', 'lising', 'lisove', 'cnest', 'ndrop', 'edrop', 'rdrop', 'pdrop', 'kdrop']
    vi['load']    = ['load1', 'load5', 'load15', 'runq', 'plit']
    vi['tcpofo']  = ['OfoPruned', 'DSACKOfoSent', 'DSACKOfoRecv', 'OFOQueue', 'OFODrop', 'OFOMerge']
    vi['retran']  = ['RetransSegs', 'FastRetrans', 'LossProbes', 'RTORetrans', 'RTORatio', 'LostRetransmit', 'ForwardRetrans', 'SlowStartRetrans', 'RetransFail', 'SynRetrans']
    vi['tcpdrop'] = ['LockDroppedIcmps', 'ListenDrops', 'ListenOverflows', 'PrequeueDropped', 'BacklogDrop', 'MinTTLDrop', 'DeferAcceptDrop', 'ReqQFullDrop', 'OFODrop']
    vi['tcperr']  = ['RenoFailures', 'SackFailures', 'LossFailures', 'AbortOnMemory', 'AbortFailed', 'TimeWaitOverflow', 'FastOpenListenOverflow']

    vi_widths = {}
    vi_widths['tcpofo']  = {'OfoPruned':9, 'DSACKOfoSent':12, 'DSACKOfoRecv':12, 'OFOQueue':8, 'OFOMerge':8}
    vi_widths['retran']  = {"RetransSegs":11, "LostRetransmit":14, "FastRetrans":11, "ForwardRetrans":14, "SlowStartRetrans":16, "RTORetrans":10, "RetransFail":11, "SynRetrans":10, "LossProbes":10, "RTORatio":8}
    vi_widths['tcpdrop'] = {"LockDroppedIcmps":16, "ListenDrops":11, "ListenOverflows":15, "PrequeueDropped":15, "BacklogDrop":11, "MinTTLDrop":10, "DeferAcceptDrop":15, "ReqQFullDrop":12}
    vi_widths['tcperr']  = {'RenoFailures':12, 'SackFailures':12, 'LossFailures':12, 'AbortOnMemory':13, 'AbortFailed':11, 'TimeWaitOverflow':16, 'FastOpenListenOverflow':22}

    default_vi = OrderedDict()
    default_vi['cpu']     = ['util']
    default_vi['mem']     = ['util']
    default_vi['tcp']     = ['retran']
    default_vi['traffic'] = ['bytin', 'bytout']
    default_vi['io']      = ['util']
    default_vi['load']    = ['load1']

    subcommand_list = ['cputop', 'irqtop', 'procs', 'workers']
    subcommand_info = "Subcommands:" + "\n"
    for it_subcommand in subcommand_list:
        subcommand_info = subcommand_info + "  tsar2 " + it_subcommand + " -h\n"

    root_footer_info = '''
Examples:
  tsar2 --cpu
  tsar2 --mem
'''
    procs_footer_info = '''
Examples:
  tsar2 procs --cpu1
  tsar2 procs --mem1
'''
    irqtop_footer_info = '''
Examples:
  tsar2 irqtop -l
  tsar2 irqtop -i 1
'''
    cputop_footer_info = '''
Examples:
  tsar2 cputop -l
  tsar2 cputop -i 1
'''

    ItFormatterClass = lambda prog: ItFormatter(prog, max_help_position = MAX_SUB_HELP_POSITION, width = MAX_HELP_WIDTH)
    root_parser = argparse.ArgumentParser(add_help = False, formatter_class = ItFormatterClass, usage = "%(prog)s [OPTIONS] [SUBCOMMAND]", description="%(prog)s program is compatible with tsar", epilog = subcommand_info + root_footer_info)
    groupo = root_parser.add_argument_group('Options')
    groupo.add_argument("-h","--help"    ,dest='root_help',action="store_true",default=False, help = "Display help information")   
    groupo.add_argument('-D','--detail'  ,dest='detail'   ,action='store_true',         help='do not conver data to K/M/G')
    groupo.add_argument('-i','--interval',dest='interval' ,type=int, metavar='N',       help='specify intervals numbers, in minutes if with --live, it is in seconds')
    groupo.add_argument('-s','--spec'    ,dest='spec'     ,type=str,                    help='show spec field data, %(prog)s --cpu -s sys,util')
    groupo.add_argument('-f','--finish'  ,dest='finish'   ,type=str,                    help='finish datetime, %(prog)s -f 19/09/21-10:25')
    groupo.add_argument(     '--path'    ,dest='path'     ,type=str,                    help='specify a dir path as input')
 
    group1 = groupo.add_mutually_exclusive_group()
    group1.add_argument('-w','--watch'   ,dest='watch'    ,type=int, metavar='N',       help='display last records in N mimutes. %(prog)s --watch 30 --cpu')
    group1.add_argument('-n','--ndays'   ,dest='ndays'    ,type=int, metavar='N',       help='show the value for the past days')
    group1.add_argument('-d','--date'    ,dest='date'     ,type=str, metavar='YYYYmmdd',help='show the value for the specify day')
    group1.add_argument('-l','--live'    ,dest='live'     ,action='store_true',         help='running print live mode, which module will print')
    
    group2 = groupo.add_mutually_exclusive_group()
    group2.add_argument('-I','--item'    ,dest='item'     ,type=str,                    help='show spec item data, %(prog)s --io -I sda')
    group2.add_argument('-m','--merge'   ,dest='merge'    ,action='store_true',         help='merge multiply item to one')
    
    groupm = root_parser.add_argument_group('Modules')
    groupm.add_argument('--cpu',          dest='cpu'      ,action='store_true',         help='CPU share (user, system, interrupt, nice, & idle)')
    groupm.add_argument('--mem',          dest='mem'      ,action='store_true',         help='Physical memory share (active, inactive, cached, free, wired)')
    groupm.add_argument('--swap',         dest='swap'     ,action='store_true',         help='swap usage')
    groupm.add_argument('--tcp',          dest='tcp'      ,action='store_true',         help='TCP traffic     (v4)')
    groupm.add_argument('--udp',          dest='udp'      ,action='store_true',         help='UDP traffic     (v4)')
    groupm.add_argument('--traffic',      dest='traffic'  ,action='store_true',         help='Net traffic statistics')
    groupm.add_argument('--io',           dest='io'       ,action='store_true',         help='Linux I/O performance')
    groupm.add_argument('--pcsw',         dest='pcsw'     ,action='store_true',         help='Process (task) creation and context switch')
    groupm.add_argument('--tcpx',         dest='tcpx'     ,action='store_true',         help='TCP connection data')
    groupm.add_argument('--load',         dest='load'     ,action='store_true',         help='System Run Queue and load average')
    groupm.add_argument('--tcpofo',       dest='tcpofo'   ,action='store_true',         help='Tcp Out-Of-Order')
    groupm.add_argument('--retran',       dest='retran'   ,action='store_true',         help='Tcp Retrans')
    groupm.add_argument('--tcpdrop',      dest='tcpdrop'  ,action='store_true',         help='Tcp Drop')
    groupm.add_argument('--tcperr',       dest='tcperr'   ,action='store_true',         help='Tcp Err')
    
    common_parser = argparse.ArgumentParser()
    subparsers = common_parser.add_subparsers(dest='cmd')

    irqtop_parser = subparsers.add_parser("irqtop", help = "irqtop Command")
    irqtop_parser.add_argument('--finish',  '-f',dest='finish'   ,type=str,                    help='finish datetime. %(prog)s -f 19/09/21-10:25')
    irqtop_parser.add_argument('--watch',   '-w',dest='watch'    ,type=int, metavar='N',       help='display last records in N mimutes. %(prog)s --watch 30')
    irqtop_parser.add_argument('--ndays',   '-n',dest='ndays'    ,type=int, metavar='N',       help='show the value for the past days')
    irqtop_parser.add_argument('--date',    '-d',dest='date'     ,type=str, metavar='YYYYmmdd',help='show the value for the specify day')
    irqtop_parser.add_argument('--live',    '-l',dest='live'     ,action='store_true',         help='running print live mode, which module will print')
    irqtop_parser.add_argument('--path',         dest='path'     ,type=str,                    help='specify a dir path as input')
    irqtop_parser.add_argument('--detail',  '-D',dest='detail'   ,action='store_true',         help='do not conver data to K/M/G')
    irqtop_parser.add_argument('--interval','-i',dest='interval' ,type=int, metavar='N',       help='specify intervals numbers, in minutes if with --live, it is in seconds')
    irqtop_parser.add_argument('--spec',    '-s',dest='spec'     ,type=str,                    help='show spec field data. %(prog)s -s count,irq')
    irqtop_parser.add_argument('--item',    '-I',dest='item'     ,type=str,                    help='show spec item data. %(prog)s -I MOC,0-3,10')
    irqtop_parser.add_argument('--field',   '-F',dest='field'    ,type=int, metavar='N',       help='show top N interrupts. %(prog)s -F 5')
    irqtop_parser.add_argument('--cpus',    '-C',dest='cpus'     ,type=str,                    help='filter interrupts with CPU. %(prog)s -C 0-10,20')
    irqtop_parser.add_argument('--name',    '-N',dest='name'     ,type=str,                    help='filter interrupts with name. %(prog)s -N virtio')
    irqtop_parser.add_argument('--sort',    '-S',dest='sort'     ,type=str,                    help='sort order,default by count, other is irq. %(prog)s -s i')
    irqtop_parser.add_argument('--all',     '-A',dest='all'      ,action='store_true',         help='Display all irq, include MOC RES')
    irqtop_parser.set_defaults(callback = activity_reporter)

    cputop_parser = subparsers.add_parser("cputop", help = "cputop Command")
    cputop_parser.add_argument('--finish',  '-f',dest='finish'   ,type=str,                    help='finish datetime. %(prog)s -f 19/09/21-10:25')
    cputop_parser.add_argument('--watch',   '-w',dest='watch'    ,type=int, metavar='N',       help='display last records in N mimutes. %(prog)s --watch 30')
    cputop_parser.add_argument('--ndays',   '-n',dest='ndays'    ,type=int, metavar='N',       help='show the value for the past days')
    cputop_parser.add_argument('--date',    '-d',dest='date'     ,type=str, metavar='YYYYmmdd',help='show the value for the specify day')
    cputop_parser.add_argument('--live',    '-l',dest='live'     ,action='store_true',         help='running print live mode, which module will print')
    cputop_parser.add_argument('--path',         dest='path'     ,type=str,                    help='specify a dir path as input')
    cputop_parser.add_argument('--detail',  '-D',dest='detail'   ,action='store_true',         help='do not conver data to K/M/G')
    cputop_parser.add_argument('--interval','-i',dest='interval' ,type=int, metavar='N',       help='specify intervals numbers, in minutes if with --live, it is in seconds')
    cputop_parser.add_argument('--spec',    '-s',dest='spec'     ,type=str,                    help='show spec field data. %(prog)s -s value,cpu')
    cputop_parser.add_argument('--item',    '-I',dest='item'     ,type=str,                    help='show spec item data. %(prog)s -I 0-3,10')
    cputop_parser.add_argument('--field',   '-F',dest='field'    ,type=int, metavar='N',       help='show top N cpu. %(prog)s -F 5')
    cputop_parser.add_argument('--sort',    '-S',dest='sort'     ,type=str,                    help='sort indicator,default by util. %(prog)s -S sirq')
    cputop_parser.add_argument('--reverse', '-r',dest='reverse'  ,action='store_true',         help='sort order, default by decrease, %(prog)s -r')
    cputop_parser.set_defaults(callback = activity_reporter)
    
    procs_parser = subparsers.add_parser("procs", help = "procs subcommand", formatter_class = ItFormatterClass, epilog = procs_footer_info)
    procs_parser.add_argument("-o", "--output", help = "output info")
    procs_parser.add_argument("--cpu", action = "store_true", dest = "seq_option.cpu", help = "Display procs cpu information")
    procs_parser.add_argument("--mem", action = "store_true", dest = "seq_option.mem", help = "Display procs mem information")
    procs_parser.set_defaults(callback = activity_reporter)
    
    workers_parser = subparsers.add_parser("workers", help = "workers Command")
    workers_parser.set_defaults(callback = activity_reporter)
    
    root_args, root_extras = root_parser.parse_known_args()
    if len(root_extras) > 0:
        if root_extras[0] in subcommand_list:
            common_args, common_extras = common_parser.parse_known_args()
            if len(common_extras) > 0:
                subparsers._name_parser_map[root_extras[0]].print_help()        # procs_parser.print_help()
                exit(201)
            else:
                common_args.callback(common_args)
        else:
            root_parser.print_help()
            exit(202)
    else:
        if root_args.root_help:
            root_parser.print_help()
            exit(203)
        else:
            root_args.cmd = "sys"
            activity_reporter(root_args)
     
    try:
        sys.stdout.close()
    except:
        pass
    try:
        sys.stderr.close()
    except:
        pass

if __name__ == "__main__":
    main()

