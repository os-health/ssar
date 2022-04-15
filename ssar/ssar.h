#ifndef SSAR_SSAR_H
#define SSAR_SSAR_H
#include <string>
#include <chrono>
#include <unordered_map>
#include <exception>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

typedef struct sproc_t {
    int                count;
    int                record_type;
    string             s;
    int                tid;
    int                ppid;
    int                pgid;
    int                sid;
    int                nlwp;
    int                tgid;
    int                pid;
    int                psr;
    long               prio;
    long               nice;
    unsigned long      rtprio;
    unsigned long      sched;
    string             cls;
    unsigned long      flags;
    unsigned long long vcswch;
    unsigned long long ncswch;
    unsigned long long vcswchs;
    unsigned long long ncswchs;
    unsigned long long utime;
    unsigned long long stime;
    unsigned long long cutime;
    unsigned long long cstime;
    unsigned long long start_time;
    unsigned long long etimes;
    int                tty;
    int                tpgid;
    long               rss; 
    long               shr; 
    long               alarm;
    long               size;
    long               resident;
    long               share;
    long               trs;
    long               lrs;
    long               drs;
    long               dt;
    unsigned long      vsize;
    unsigned long      rss_rlim;
    unsigned long      min_flt;
    unsigned long      maj_flt;
    unsigned long      cmin_flt;
    unsigned long      cmaj_flt;
    string             cmd;
    string             fullcmd;
    string             cmdline;
    unsigned long long time;
    unsigned long long realtime;
    unsigned long long time_dlt;
    unsigned long long utime_dlt;
    unsigned long long stime_dlt;
    float              pcpu;
    float              pucpu;
    float              pscpu;
    unsigned long      maj_dlt;
    unsigned long      min_dlt;
    long               rss_dlt;
    long               shr_dlt;
    string             etime;
    string             cputime;
    string             cpu_utime;
    string             cpu_stime;
    string             start_datetime;
    string             begin_datetime;
    string             finish_datetime;
    string             collect_datetime;
    string             b;
    unsigned long      collect_time;
} sproc_t;

typedef struct aproc_t {
    int                tgid;
    string             cmd;
    string             fullcmd;
    string             cmdline;
} aproc_t;

typedef struct load5s_t{
   unsigned long collect_time;
   string        collect_datetime;
   int           threads;
   int           runq;
   double        load1;
   int           load5s;
   int           load5s_type;        // 1(1m) or 5(5s) 
   int           load5s_state;       // 0(N)  or 2(Y)
   int           zoom_state;         // 0(U)  or 3(Z)
   string        stype;              // 1(1m) or 5(5s) 
   string        sstate;             // 0(N)  or 2(Y)
   string        zstate;             // 0(U)  or 3(Z)
   int           act;                // active 
   int           actr;               // active R
   int           actd;               // active D
   double        act_rto;            // act ratio 
   double        actr_rto;           // actr ratio
   int           record_type;
} load5s_t;

typedef struct active_t{
    int          actr;
    int          actd;
} active_t;

typedef struct load2p_t{
    int           count;
    string        s;            // state
    int           tid;
    int           tgid;
    int           pid;
    int           psr;
    int           prio;
    string        cmd;
    string        cmdline;
    string        wchan;
    string        nwchan;
    string        stackinfo;
    string        full_stackinfo;
    string        s_unit;
} load2p_t;

typedef struct indicator_t{
    int    line;
    int    column;
    int    width;
    string cfile;
    string line_begin;
    string position;
    string metric;
    string dtype;
    string indicator;
    string alias;
    int    gzip;               // inherit from cfile
    string src_path;           // inherit from cfile
} indicator_t;

typedef struct cfile_t{
    int     turn;
    int     gzip;
    string  src_path;
    string  cfile;
} cfile_t;

list<string> procs_default_formats = {"start_datetime","pid","ppid","cputime","pcpu","size","rss","etime","nlwp","cmd"};
list<string> procs_cpu_formats     = {"pid","ppid","cputime","cpu_utime","cpu_stime","pcpu","pucpu","pscpu","vcswchs","ncswchs","s","cmd"};
list<string> procs_mem_formats     = {"pid","ppid","size","rss","shr","maj_flt","min_flt","rss_dlt","shr_dlt","maj_dlt","min_dlt","cmd"};
list<string> procs_job_formats     = {"start_datetime","pid","ppid","pgid","sid","tpgid","etime","s","cmd"};
list<string> procs_sched_formats   = {"start_datetime","pid","ppid","flags","cls","nice","rtprio","prio","cmd"};

list<string> proc_default_formats  = {"collect_datetime","etime","cputime","pcpu","size","rss","nlwp","cmd"};
list<string> proc_cpu_formats      = {"collect_datetime","etime","cputime","cpu_utime","cpu_stime","pcpu","pucpu","pscpu","vcswchs","ncswchs","s","cmd"};
list<string> proc_mem_formats      = {"collect_datetime","etime","size","rss","shr","maj_flt","min_flt","rss_dlt","shr_dlt","maj_dlt","min_dlt","cmd"};
list<string> load5s_default_formats= {"collect_datetime","threads","load1","runq","load5s","stype","sstate","zstate","act","act_rto","actr","actr_rto","actd"};
list<string> load2p_default_formats= {"loadr", "loadd", "psr", "stackinfo"};
list<string> load2p_loadpid_formats= {"count", "s", "pid", "cmd", "cmdline"};
list<string> load2p_psr_formats    = {"count", "psr"};
list<string> load2p_stackinfo_formats = {"count", "s_unit", "nwchan","wchan","stackinfo"};
list<string> load2p_loadrd_formats = {"s", "tid", "pid", "psr", "prio","cmd"};
list<string> load2p_stack_formats  = {"tid","pid","cmd","nwchan","wchan","stackinfo"};

//list<string> sys_default_formats   = {"stat:cpu:2:d","stat:cpu:4:d","stat:cpu:6:d","stat:cpu:8:d","meminfo|MemFree:|2","meminfo|AnonPages:|2","meminfo|Shmem:|2","meminfo|Dirty:|2"};
list<string> sys_default_formats   = {"metric=d:src_path=/proc/stat:line_begin=cpu:column=2:alias=user","metric=d:src_path=/proc/stat:line_begin=cpu:column=4:alias=system","metric=d:src_path=/proc/stat:line_begin=cpu:column=6:alias=iowait","metric=d:src_path=/proc/stat:line_begin=cpu:column=8:alias=softirq","src_path=/proc/meminfo|line_begin=MemFree:|column=2|alias=memfree","src_path=/proc/meminfo|line_begin=AnonPages:|column=2|alias=anonpages","src_path=/proc/meminfo|line_begin=Shmem:|column=2|alias=shmem","src_path=/proc/meminfo|line_begin=Dirty:|column=2|alias=dirty"};

unordered_map<int,string> sched2cls {
    {0,"TS"},
    {1,"FF"},
    {2,"RR"},
    {3,"B"},
    {5,"IDL"}
};

unordered_map<string, unordered_map<string,string>> field_attribute {
    {"record_type",{{"f_width","11"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"s",{{"f_width","1"},{"k_order","-"},{"adjust","left"},{"unit","scale"}}},
    {"tid",{{"f_width","7"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"ppid",{{"f_width","7"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"pgid",{{"f_width","7"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"sid",{{"f_width","7"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"nlwp",{{"f_width","4"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"tgid",{{"f_width","7"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"pid",{{"f_width","7"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"psr",{{"f_width","3"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"prio",{{"f_width","4"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"nice",{{"f_width","4"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"rtprio",{{"f_width","6"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"sched",{{"f_width","5"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"cls",{{"f_width","3"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"flags",{{"f_width","5"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"vcswch",{{"f_width","6"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"ncswch",{{"f_width","6"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"vcswchs",{{"f_width","7"},{"k_order","-"},{"adjust","right"},{"unit","scope"}}},
    {"ncswchs",{{"f_width","7"},{"k_order","-"},{"adjust","right"},{"unit","scope"}}},
    {"utime",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"stime",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"cutime",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"cstime",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"start_time",{{"f_width","10"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"etimes",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"tty",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"tpgid",{{"f_width","7"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"rss",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"shr",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"alarm",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"size",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"resident",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"share",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"trs",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"lrs",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"drs",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"dt",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"vsize",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"rss_rlim",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"min_flt",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"maj_flt",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"cmin_flt",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"cmaj_flt",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"cmd",{{"f_width","15"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"fullcmd",{{"f_width","30"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"cmdline",{{"f_width","50"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"time",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"realtime",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scope"}}},
    {"time_dlt",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scope"}}},
    {"utime_dlt",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scope"}}},
    {"stime_dlt",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scope"}}},
    {"pcpu",{{"f_width","7"},{"k_order","-"},{"adjust","right"},{"unit","scope"}}},
    {"pucpu",{{"f_width","7"},{"k_order","-"},{"adjust","right"},{"unit","scope"}}},
    {"pscpu",{{"f_width","7"},{"k_order","-"},{"adjust","right"},{"unit","scope"}}},
    {"maj_dlt",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scope"}}},
    {"min_dlt",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scope"}}},
    {"rss_dlt",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scope"}}},
    {"shr_dlt",{{"f_width","10"},{"k_order","-"},{"adjust","right"},{"unit","scope"}}},
    {"etime",{{"f_width","13"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"cputime",{{"f_width","13"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"cpu_utime",{{"f_width","13"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"cpu_stime",{{"f_width","13"},{"k_order","-"},{"adjust","right"},{"unit","scale"}}},
    {"start_datetime",{{"f_width","19"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"begin_datetime",{{"f_width","19"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"finish_datetime",{{"f_width","19"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"collect_datetime",{{"f_width","19"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"b",{{"f_width","1"},{"k_order","+"},{"adjust","left"},{"unit","scope"}}}
};

unordered_map<string, unordered_map<string,string>> load5s_field_attribute{
    {"collect_datetime",{{"f_width","19"},{"zoom_state","3"},{"adjust","left"},{"unit","scale"}}},
    {"threads",{{"f_width","10"},{"zoom_state","3"},{"adjust","right"},{"unit","scale"}}},
    {"load1",{{"f_width","8"},{"zoom_state","3"},{"adjust","right"},{"unit","scale"}}},
    {"runq",{{"f_width","6"},{"zoom_state","3"},{"adjust","right"},{"unit","scale"}}},
    {"load5s",{{"f_width","6"},{"zoom_state","3"},{"adjust","right"},{"unit","scale"}}},
    {"load5s_type",{{"f_width","5"},{"zoom_state","3"},{"adjust","left"},{"unit","scale"}}},
    {"stype",{{"f_width","5"},{"zoom_state","3"},{"adjust","left"},{"unit","scale"}}},
    {"load5s_state",{{"f_width","6"},{"zoom_state","3"},{"adjust","left"},{"unit","scale"}}},
    {"sstate",{{"f_width","6"},{"zoom_state","3"},{"adjust","left"},{"unit","scale"}}},
    {"zoom_state",{{"f_width","6"},{"zoom_state","3"},{"adjust","left"},{"unit","scale"}}},
    {"zstate",{{"f_width","6"},{"zoom_state","3"},{"adjust","left"},{"unit","scale"}}},
    {"act",{{"f_width","6"},{"zoom_state","0"},{"adjust","right"},{"unit","scale"}}},
    {"act_rto",{{"f_width","7"},{"zoom_state","0"},{"adjust","right"},{"unit","scale"}}},
    {"actr",{{"f_width","6"},{"zoom_state","0"},{"adjust","right"},{"unit","scale"}}},
    {"actr_rto",{{"f_width","8"},{"zoom_state","0"},{"adjust","right"},{"unit","scale"}}},
    {"actd",{{"f_width","6"},{"zoom_state","0"},{"adjust","right"},{"unit","scale"}}}
};

unordered_map<string, unordered_map<string,string>> load2p_field_attribute{
    {"count",{{"f_width","8"},{"k_order","-"},{"adjust","rigth"},{"unit","scale"}}},
    {"s",{{"f_width","1"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"tid",{{"f_width","7"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"tgid",{{"f_width","7"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"pid",{{"f_width","7"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"psr",{{"f_width","3"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"prio",{{"f_width","4"},{"k_order","+"},{"adjust","right"},{"unit","scale"}}},
    {"nwchan",{{"f_width","6"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"wchan",{{"f_width","32"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"s_unit",{{"f_width","7"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"stackinfo",{{"f_width","6"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"full_stackinfo",{{"f_width","6"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"cmd",{{"f_width","15"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}},
    {"cmdline",{{"f_width","6"},{"k_order","+"},{"adjust","left"},{"unit","scale"}}}
};

struct SeqOptions {
    string src;
    bool api;
    chrono::system_clock::time_point time_point_begin;
    chrono::system_clock::time_point time_point_begin_hard;
    chrono::system_clock::time_point time_point_finish; 
    chrono::system_clock::time_point time_point_collect;
    chrono::system_clock::time_point time_point_begin_adjust;
    chrono::system_clock::time_point time_point_finish_adjust; 
    chrono::system_clock::time_point time_point_begin_real;
    chrono::system_clock::time_point time_point_finish_real; 
    int range;
    chrono::duration<int> duration_range;
    chrono::duration<int> duration_real;
    string interval;
    int intervals;
    string begin;
    string finish;
    string collect;
    bool live_mode;
    string key;
    list<pair<string,string>> keys;
    int lines;
    bool noheaders;
    bool purify;
    bool version;
    string format;
    string extra;
    list<string> formats;
    list<indicator_t> formated;
    bool cpu;
    bool mem;
    bool job;
    bool sched;
    bool yes;
    bool zoom;
    bool loadr;
    bool loadd;
    bool psr;
    bool stackinfo;
    bool loadrd;
    bool stack;
    bool adjust;
    string uts_release;
    int pid;
    bool ssar_conf;
    bool sys_conf;
    bool has_cmdline;
    bool proc_gzip_disable;
    string work_path;
    string ssar_path;
    string data_path;
    map<string, bool> views;
    unordered_map<string, cfile_t>              *sys_files;
    map<string, list<string>>                   *sys_view;
    map<string, unordered_map<string, cfile_t>, greater<string>>     *group_files;
    map<string, unordered_map<string, indicator_t>, greater<string>> *sys_indicator;
    map<string, map<int, map<int, string>>>     *sys_hierarchy_line;
    map<string, map<string, map<int, string>>>  *sys_hierarchy_begin;
    map<string, map<int, map<int, string>>>     *sys_hierarchy_dtype_line;
    map<string, map<string, map<int, string>>>  *sys_hierarchy_dtype_begin;
    map<string, vector<string>>                 *delta_hierarchy;
    map<string, vector<string>>                 *cumulate_hierarchy;
    unordered_map<string, string>               *sys_dtype;
    vector<chrono::system_clock::time_point>    *reboot_times;
    explicit SeqOptions(
        const string &it_src = "sys",
        const bool &it_api =  false,
        const chrono::system_clock::time_point &it_time_point_begin = chrono::time_point<chrono::system_clock>{},
        const chrono::system_clock::time_point &it_time_point_begin_hard = chrono::time_point<chrono::system_clock>{},
        const chrono::system_clock::time_point &it_time_point_finish = chrono::system_clock::now(),
        const chrono::system_clock::time_point &it_time_point_begin_adjust = chrono::time_point<chrono::system_clock>{},
        const chrono::system_clock::time_point &it_time_point_finish_adjust = chrono::time_point<chrono::system_clock>{},
        const chrono::system_clock::time_point &it_time_point_begin_real = chrono::time_point<chrono::system_clock>{},
        const chrono::system_clock::time_point &it_time_point_finish_real = chrono::time_point<chrono::system_clock>{},
        const int &it_range = 0, 
        const chrono::duration<int> &it_duration_range = chrono::duration<int>(60),
        const chrono::duration<int> &it_duration_real = chrono::duration<int>(0),
        const string &it_interval = "", 
        const int &it_intervals = 60, 
        const string &it_begin = "",
        const string &it_finish = "",
        const string &it_collect = "",
        const bool &it_live_mode = false,
        const string &it_key = "",
        const list<pair<string,string>> &it_keys = {{"start_time","+"}},
        const int &it_lines = 0,
        const bool &it_noheaders = false,
        const bool &it_purify = false,
        const bool &it_version = false,
        const string &it_format = "",
        const string &it_extra = "",
        const list<string> &it_formats = {},
        const list<indicator_t> &it_formated = {},
        const bool &it_cpu = false,
        const bool &it_mem = false,
        const bool &it_job = false,
        const bool &it_sched = false,
        const bool &it_yes = false,
        const bool &it_zoom = false,
        const bool &it_loadr = false,
        const bool &it_loadd = false,
        const bool &it_psr   = false,
        const bool &it_stackinfo = false,
        const bool &it_loadrd = false,
        const bool &it_stack  = false,
        const bool &it_adjust = false,
        const string &it_uts_release = "",
        const int &it_pid = 0,
        const bool &it_ssar_conf = true,
        const bool &it_sys_conf = true,
        const bool &it_has_cmdline = false,
        const bool &it_proc_gzip_disable = false,
        const string &it_work_path = "/var/log",
        const string &it_ssar_path = "",
        const string &it_data_path = "",
        const map<string,bool> &it_views = {{}},
        unordered_map<string, cfile_t> *const &it_sys_files = NULL,
        unordered_map<string, string> *const &it_file_position = NULL,
        map<string, list<string>> *const &it_sys_view = NULL,
        map<string, unordered_map<string, cfile_t>, greater<string>> *const &it_group_files = NULL,
        map<string, unordered_map<string, indicator_t>, greater<string>> *const &it_sys_indicator = NULL,
        map<string, map<int, map<int, string>>> *const &it_sys_hierarchy_line = NULL,
        map<string, map<string, map<int, string>>> *const &it_sys_hierarchy_begin = NULL,
        map<string, map<int, map<int, string>>> *const &it_sys_hierarchy_dtype_line = NULL,
        map<string, map<string, map<int, string>>> *const &it_sys_hierarchy_dtype_begin = NULL,
        map<string, vector<string>> *const &it_delta_hierarchy = NULL,
        map<string, vector<string>> *const &it_cumulate_hierarchy = NULL,
        unordered_map<string, string> *const &it_sys_dtype = NULL,
        vector<chrono::system_clock::time_point> *const &it_reboot_times =  NULL
        ) : 
        src(it_src),
        api(it_api),
        time_point_begin(it_time_point_begin),
        time_point_begin_hard(it_time_point_begin_hard),
        time_point_finish(it_time_point_finish),
        time_point_begin_adjust(it_time_point_begin_adjust),
        time_point_finish_adjust(it_time_point_finish_adjust),
        time_point_begin_real(it_time_point_begin_real),
        time_point_finish_real(it_time_point_finish_real),
	range(it_range),
        duration_range(it_duration_range),
        duration_real(it_duration_real),
        interval(it_interval), 
        intervals(it_intervals), 
        begin(it_begin), 
        finish(it_finish),
        collect(it_collect),
        live_mode(it_live_mode),
        key(it_key),
        keys(it_keys),
        lines(it_lines),
        noheaders(it_noheaders),
        purify(it_purify),
        version(it_version),
        format(it_format),
        extra(it_extra),
        formats(it_formats),
        formated(it_formated),
        cpu(it_cpu),
        mem(it_mem),
        job(it_job),
        sched(it_sched),
        yes(it_yes),
        zoom(it_zoom),
        loadr(it_loadr),
        loadd(it_loadd),
        psr(it_psr),
        stackinfo(it_stackinfo),
        loadrd(it_loadrd),
        stack(it_stack),
        adjust(it_adjust),
        uts_release(it_uts_release),
        pid(it_pid),
        ssar_conf(it_ssar_conf),
        sys_conf(it_sys_conf),
        has_cmdline(it_has_cmdline),
        proc_gzip_disable(it_proc_gzip_disable),
        work_path(it_work_path),
        ssar_path(it_ssar_path),
        data_path(it_data_path),
        views(it_views),
        sys_files(it_sys_files),
        sys_view(it_sys_view),
        group_files(it_group_files),
        sys_indicator(it_sys_indicator),
        sys_hierarchy_line(it_sys_hierarchy_line),
        sys_hierarchy_begin(it_sys_hierarchy_begin),
        sys_hierarchy_dtype_line(it_sys_hierarchy_dtype_line),
        sys_hierarchy_dtype_begin(it_sys_hierarchy_dtype_begin),
        delta_hierarchy(it_delta_hierarchy),
        cumulate_hierarchy(it_cumulate_hierarchy),
        sys_dtype(it_sys_dtype),
        reboot_times(it_reboot_times)
        {}
};

class SreProc{
private:
    set<string> dirs;
    set<string> files;
    string data_hour_path;
    string data_dir;
    string data_path;
    string data_datetime;
    int data_timestamp;
    string suffix = "";                        // sresar + column_number
    string dir_format = "%Y%m%d%H";            // %Y%m%d
    string file_format_minute = "%Y%m%d%H%M";
    string file_format_second = "%Y%m%d%H%M%S";
    chrono::system_clock::time_point time_point;
    chrono::system_clock::time_point time_point_real;
    int adjust_unit = 60;
    bool adjust = false;
    bool adjusted = false;
    bool adjust_forward = false;

private:
    int GetDirLists(string &path, set<string> &dirs);
    int GetDataFile();
    int GetDataDir();
    int GetDataHourPath(chrono::system_clock::time_point &it_time_point);
    int _MakeDataHourPath(const chrono::system_clock::time_point &it_time_point);

public:
    int MakeDataHours(string data_path);
    int MakeDataHourPath(const chrono::system_clock::time_point &it_time_point);
    string GetFirstHour(){
        return *(this->dirs.cbegin());
    };
    string GetDataHourPath(){
        return this->data_hour_path;
    };
    int GetDataTimestamp(){
        return this->data_timestamp;
    };
    string GetDataDatetime(){
        return this->data_datetime;
    };
    chrono::system_clock::time_point GetDataTimePointReal(){
        return this->time_point_real;
    };
    void EnableAdjust(){
        this->adjust = true;
    }
    void DisableAdjust(){
        this->adjust = false;
    }
    bool GetAdjusted(){
        return this->adjusted;
    }
    void SetDataPath(string data_path){
        this->data_path = data_path;
    }
    void SetSuffix(string suffix){
        this->suffix = suffix;
    }
    void SetAdjustUnit(int adjust_unit){
        this->adjust_unit = adjust_unit;
    }
    void SetAdjustForward(bool adjust_forward){
        this->adjust_forward = adjust_forward;
    }
    SreProc(){
    };
};

class Load5s{
private:
    string data_hour_path;
    string data_path;
    string data_datetime;
    int data_timestamp;
    string suffix = "";                       
    chrono::system_clock::time_point time_point_real;
    bool adjust = false;
    bool adjusted = false;

private:

public:
    int GetActiveCounts(int collect_time, active_t &act);
    void SetDataPath(string data_path){
        this->data_path = data_path;
    }
    void SetSuffix(string suffix){
        this->suffix = suffix;
    }
    Load5s(){
    };
};

class DataException :public exception{
public:
    DataException(string c){
        m_p = c;
    }
    virtual string what(){
        return m_p;
    }
private:
    string m_p;
};

void MakeRangeIndicators(list<sproc_t> &it_list_sproc_t,unordered_map<int,sproc_t> &begin_sproc_t,list<sproc_t> &it_sproc_t,int it_count);
int ReadLoad5sFileData(SeqOptions &seq_option, chrono::system_clock::time_point &it_time_point, list<load5s_t> &it_list_load5s_t);
int ReadProcFileData(SeqOptions &seq_option, string &it_path, string &addon_path, list<sproc_t> &it_sproc_t, int pid = -1);
int ParserDatetime(string &fmt_datetime, chrono::system_clock::time_point &time_point, const string &format);
string FmtDatetime(const chrono::system_clock::time_point &time_point, const string &format);
string FmtTime(unsigned long long time);
void RemoveDuplicateElement(list<string> &input);
void ActivityReporter(SeqOptions &seq_options);
void InitOptions(SeqOptions &seq_options);
void ReportProcs(SeqOptions &seq_options, list<sproc_t> &it_sproc_t);
void ReportProc(SeqOptions &seq_options, list<sproc_t> &it_sproc_t);
void DisplayProcJson(SeqOptions &seq_options, list<sproc_t> &it_sproc_t);
void DisplayProcShell(SeqOptions &seq_options, list<sproc_t> &it_sproc_t);
void InitChecks(SeqOptions &seq_options);
int SprocCompare(const sproc_t &itema, const sproc_t &itemb, const pair<string,string> &key);
int ReadRebootTimes(vector<chrono::system_clock::time_point> &reboot_times);
void DisplayLoad5sShell(SeqOptions &seq_option, list<load5s_t> &it_load5s_t);
void DisplayLoad5sJson(SeqOptions &seq_option, list<load5s_t> &it_load5s_t);
int ReadLoadrdFileData(SeqOptions &seq_option, list<load2p_t> &it_list_load2p_t);
int ReadStackFileData(SeqOptions &seq_option, list<load2p_t> &it_list_load2p_t);
void DisplayLoad2pShell(SeqOptions &seq_option, list<string> it_formats, list<load2p_t> &it_load2p_data);
json DisplayLoad2pJson(SeqOptions &seq_option, list<string> &it_formats, list<load2p_t> &it_list_load2p_t);
void DisplaySysShell(SeqOptions &seq_option, list<unordered_map<string, long long>> &it_combines);
void DisplaySysJson(SeqOptions &seq_option, list<unordered_map<string, long long>> &it_combines);

namespace ssar_cli {
    const string THROW_KEY_EMPTY       = "key is empty ";
    const string THROW_COMPARE_KEY     = "compare key is not set ";
    const string THROW_FINISH_DATETIME = "finish datetime is not correct ";
    const string THROW_BEGIN_DATETIME  = "begin datetime is not correct ";
    const string THROW_BEGIN_COMPARE_FINISH = "begin datetime is not lower then finish datetime ";
    const string THROW_NO_KEY          = " key not found ";
    const string THROW_FINISH_DATA_HOUR_PATH = "finish data path is not exist ";
    const string THROW_BEGIN_DATA_HOUR_PATH = "begin data path is not exist ";
    const string THROW_BEGIN_DATA_FILE = "begin data file is not exist ";
    const string THROW_FINISH_DATA_FILE = "finish data file is not exist ";

    const string DEFAULT_OPTIONS_HELP = R"help(Examples:
  ssar
  ssar -V                     
  ssar --mem
  ssar --cpu --mem
  ssar --api
  ssar -r 60
  ssar -i 1
  ssar -f 2020-09-29T18:34:00
  ssar -f 20210731155500 
  ssar -f 202107311555
  ssar -f 2021073115
  ssar -f 20210731
  ssar -f -10
  ssar -f -5.5h
  ssar -f -1.8d               
  ssar -b 2020-03-02T17:34:00
  ssar -H
  ssar -P
  ssar -o 'shmem,meminfo:2:2,snmp|8|11|d'
  ssar -O "dev|eth0:|2|d;snmp:8:11:d" --cpu
  ssar -o 'metric=d:cfile=snmp:line=8:column=13:alias=retranssegs'
  ssar -o 'metric=c|cfile=meminfo|line_begin=MemFree:|column=2|alias=free'
  ssar -o 'metric=c|cfile=loadavg|line=1|column=1|dtype=float|alias=load1'
  ssar -o 'metric=c|cfile=loadavg|line=1|column=4|dtype=string|alias=runq_plit'
  ssar -o 'metric=d|cfile=stat|line=2|column=2-11|alias=cpu0_{column};'
  ssar -o 'metric=d|cfile=stat|line=2-17|column=5|alias=idle_{line};'
  ssar -f +10
  ssar -f +10 -i 5s

For more details see ssar(1).
)help";

    const string PROCS_OPTIONS_HELP = R"help(Examples:
  ssar procs
  ssar procs --cpu
  ssar procs --mem
  ssar procs --job
  ssar procs --sched
  ssar procs -r 60
  ssar procs -f 2020-03-02T18:34:00
  ssar procs -f -10
  ssar procs -f -5.5h
  ssar procs -f -1.2d
  ssar procs -b 2020-03-02T17:34:00
  ssar procs -k -ppid,pid,+start_datetime
  ssar procs -l 100
  ssar procs -H
  ssar procs -o ppid,pid,start_datetime
  ssar procs -O pgid
  ssar procs --api

For more details see ssar(1).
)help";

    const string PROC_OPTIONS_HELP = R"help(Examples:
  ssar proc -p 1
  ssar proc -p 1 --cpu
  ssar proc -p 1 --mem
  ssar proc -p 1 -r 60
  ssar proc -p 1 -i 1
  ssar proc -p 1 -f 2020-03-02T18:34:00
  ssar proc -p 1 -f -10
  ssar proc -p 1 -f -5.5h
  ssar proc -p 1 -f -1.2d
  ssar proc -p 1 -b 2020-03-02T17:34:00
  ssar proc -p 1 -H
  ssar proc -p 1 -P
  ssar proc -p 1 -o ppid,pid,start_datetime
  ssar proc -p 1 -O pgid
  ssar proc -p 1 --api

For more details see ssar(1).
)help";

    const string LOAD5S_OPTIONS_HELP = R"help(Examples:
  ssar load5s
  ssar load5s -r 60
  ssar load5s -f 2020-03-02T18:34:00
  ssar load5s -f -10
  ssar load5s -f -5.5h
  ssar load5s -f -1.2d
  ssar load5s -b 2020-03-02T17:34:00
  ssar load5s -H
  ssar load5s -y
  ssar load5s -z
  ssar load5s --api

For more details see ssar(1).
)help";

    const string LOAD2P_OPTIONS_HELP = R"help(Examples:
  ssar load2p -c 2020-05-11T19:49:12
  ssar load2p -c 2020-05-11T19:49:12 -l 10
  ssar load2p -c 2020-05-11T19:49:12 -H
  ssar load2p -c 2020-05-11T19:49:12 --loadr
  ssar load2p -c 2020-05-11T19:49:12 --loadd
  ssar load2p -c 2020-05-11T19:49:12 --psr
  ssar load2p -c 2020-05-11T19:49:12 --stackinfo
  ssar load2p -c 2020-05-11T19:49:12 --loadrd
  ssar load2p -c 2020-05-11T19:49:12 --stack
  ssar load2p -c 2020-05-11T19:49:12 --loadd --stackinfo
  ssar load2p -c 2020-05-11T19:49:12 --api

For more details see ssar(1).
)help";

}

#endif
