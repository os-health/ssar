/*
 *
 *  Copyright (c) 2021 Alibaba Inc.
 *  SRESAR is licensed under Mulan PSL v2.
 *  You can use this software according to the terms and conditions of the Mulan PSL v2.
 *  You may obtain a copy of Mulan PSL v2 at:
 *        http://license.coscl.org.cn/MulanPSL2
 *  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 *  EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 *  MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *  See the Mulan PSL v2 for more details.
 *
 *  Part of SRESAR (Site Reliability Engineering System Activity Reporter)
 *  
 *  Author:  Miles Wen <mileswen@linux.alibaba.com>
 * 
 */


#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"
#include "readprocess.h"

#define STAT_FILE    "/proc/stat"

#define F(x) {#x, sizeof(#x)-1, &&case_##x},
#define NUL  {"", 0, 0},

static int  uptime_fd = -1;
static char buf[8192];

typedef struct status_table_struct {
    unsigned char name[7];
    unsigned char len;
    void *addr;
} status_table_struct;

/************************************************************************************
 ***                                                                                *
 ***                               sysinfo part                                     * 
 ***                                                                                *
 ************************************************************************************/

unsigned long get_btime(void) {
    static unsigned long btime = 0;
    bool found_btime = false;
    FILE *f;

    if(btime){
        return btime;
    }

    if ((f = fopen(STAT_FILE, "r")) == NULL) {
        fputs("/proc must be mounted\n", stderr);
        exit(1);
    }

    while((fgets(buf, sizeof buf, f))) {
        if (sscanf(buf, "btime %lu", &btime) == 1) {
            found_btime = true;
            break;
        }
    }
    if (f) {
        fclose(f); 
        f = NULL;
    }
    
    if(!found_btime) {
        fputs("missing btime in " STAT_FILE "\n", stderr);
        exit(1);
    }

    return btime;
}

int get_uptime() {
    double up=0, idle=0;
    char *savelocale;

    static int local_n;
    if(uptime_fd == -1 && (uptime_fd = open("/proc/uptime", O_RDONLY)) == -1) {
        perror("/proc must be mounted\n");
        exit(2);
    }
    lseek(uptime_fd, 0L, SEEK_SET);
    if ((local_n = read(uptime_fd, buf, sizeof(buf) - 1)) < 0) {
        perror("/proc/uptime");
        exit(3);
    } 
    buf[local_n] = '\0';
    savelocale = strdup(setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, "C");
    if(sscanf(buf, "%lf %lf", &up, &idle) < 2) {
        setlocale(LC_NUMERIC, savelocale);
        free(savelocale);
        perror("bad data in /proc/uptime\n");
        return 0;
    }
    setlocale(LC_NUMERIC, savelocale);
    free(savelocale);

    return up;
}

/************************************************************************************
 ***                                                                                *
 ***                               stack part                                     * 
 ***                                                                                *
 ************************************************************************************/

char *read_stack_file(unsigned tid, utlbuf_s *ubuf, utlbuf_s *ufbuf, char * nwchan){
    char path[GROWWIDTH];
    char *tmp;
    int fd;
    unsigned int num;
    unsigned int tmp_num;
    int total_read = 0;
    int counter  = 0;
    int saved_errno;
    char error_buf[256];
    ubuf->buf[0]  = '\0';
    ufbuf->buf[0] = '\0';
    
    snprintf(path, sizeof(path), "/proc/%d/stack", tid);
    fd = open(path, O_RDONLY|O_NONBLOCK);
    saved_errno =  errno;
    if(fd == -1){
        strerror_r(saved_errno, error_buf, 256);
        replace_char(error_buf, '-', '_');
        replace_char(error_buf, '/', '_');
        replace_char(error_buf, '.', '_');
        replace_char(error_buf, ':', '_');
        replace_char(error_buf, ' ', '_');
        sprintf(ubuf->buf, "%d?%s", saved_errno, error_buf);
        strcpy(nwchan, "000000");
        return ubuf->buf;
    }
    while(0 < (num = read(fd, ufbuf->buf + total_read, GROWWIDTH))){
        if(num == -1){
            strcpy(ubuf->buf, "-");
            strcpy(nwchan, "000000");
            return ubuf->buf;
            break;
        }else if(num == 0){
            break;
        }
        total_read += num;
        
        if(total_read + GROWWIDTH >= ufbuf->len){
            ufbuf->len += GROWWIDTH;
            ufbuf->buf  = (char*)realloc(ufbuf->buf, ufbuf->len * sizeof(char*));
        }
    };
    ufbuf->buf[total_read] = '\0';
    close(fd);

    char* cur_line = ufbuf->buf;
    char* line_break_pos;
    int   is_nwchan = 0;
    while(cur_line){
        line_break_pos = strchr(cur_line, '\n');
        if(line_break_pos){
            *line_break_pos = '\0';
        }
 
        if(!is_nwchan){
            strncpy(nwchan, cur_line + 12, 6);
            is_nwchan = 1;
        }

        if((cur_line = strchr(cur_line, ']')) == NULL){
            break;
        }
        cur_line = cur_line + 2;
        if((tmp = strrchr2(cur_line + 2, '+')) == NULL){
            break;
        }
        tmp_num = tmp - cur_line;
        cur_line[tmp_num] = '\0';
        if(0 != counter){
            if(strlen(ubuf->buf) + strlen(",") >= ubuf->len){
                ubuf->len += GROWWIDTH;
                ubuf->buf  = (char*)realloc(ubuf->buf, ubuf->len * sizeof(char*));
            }
            strcat(ubuf->buf, ","); 
        }
        for(;;){
            if(strlen(ubuf->buf) + tmp_num >= ubuf->len){
                ubuf->len += GROWWIDTH;
                ubuf->buf  = (char*)realloc(ubuf->buf, ubuf->len * sizeof(char*));
            }else{
                break;
            }
        }
        strncat(ubuf->buf, cur_line, tmp_num);
        counter++;

        cur_line = line_break_pos ? (line_break_pos + 1) : NULL;
    }

    return ubuf->buf;
}


/************************************************************************************
 ***                                                                                *
 ***                               cmdline part                                     * 
 ***                                                                                *
 ************************************************************************************/

char *delete_trailing_chars(char *s, const char *bad) {
    char *p, *c = s;

    if(!s){
        return NULL;
    }

    if(!bad){
        bad = "\t\n\r";
    }
    for(p = s; *p; p++){
        if (!strchr(bad, *p)){
            c = p + 1;
        }
    }
    *c = 0;

    return s;
}

int get_process_cmdline(const char* directory, utlbuf_s *ubuf) {
    int     fd;
    char   *t = NULL;
    char    path[100] = {0,};
    ssize_t num;
    size_t  total_read = 0;
    int     ret = 0;
    int     max_loop_time;
    int     i = 0;

    max_loop_time = (int)(2097152 / GROWWIDTH);
        
    if(!ubuf->buf){
        return -ENOMEM;
    }
    
    sprintf(path, "%s/cmdline", directory);
    fd = open(path, O_RDONLY|O_CLOEXEC|O_NONBLOCK);
    while((num = read(fd, ubuf->buf + total_read, GROWWIDTH)) > 0){
        if(i > max_loop_time){
            break;
        }
        total_read += num;
        i++;

        if(total_read + GROWWIDTH >= ubuf->len){
            ubuf->len += GROWWIDTH;
            ubuf->buf  = (char*)realloc(ubuf->buf, ubuf->len * sizeof(char*));
        }
    };
    ubuf->buf[total_read] = '\0';
    if(total_read > 0){
        for(size_t i = 0; i < total_read; i++){
            if(ubuf->buf[i] == '\0' || ubuf->buf[i] == '\x9' || ubuf->buf[i] == '\xa'){
                ubuf->buf[i] = ' ';
            }
        }
        ret = 1;
    }else{
        strncpy(ubuf->buf, "[]", 2);
        ubuf->buf[2] = '\0';
        ret = 2;
    }
    delete_trailing_chars(ubuf->buf, WHITESPACE);
    close(fd);

    return ret;
}

/************************************************************************************
 ***                                                                                *
 ***                               parser part                                      * 
 ***                                                                                *
 ************************************************************************************/

void parse_status(char *S, proc_t *restrict P){

    // 128 entries
    static const unsigned char asso[] = {
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 28, 64,
      64, 64, 64, 64, 64, 64,  8, 25, 23, 25,
       6, 25,  0,  3, 64, 64,  3, 64, 25, 64,
      20,  1,  1,  5,  0, 30,  0,  0, 64, 64,
      64, 64, 64, 64, 64, 64, 64,  3, 64,  0,
       0, 18, 64, 10, 64, 10, 64, 64, 64, 20,
      64, 20,  0, 64, 25, 64,  3, 15, 64,  0,
      30, 64, 64, 64, 64, 64, 64, 64
    };

    static const status_table_struct table[] = {
        F(VmHWM)
        NUL NUL
        F(VmLck)
        NUL
        F(VmSwap)
        F(VmRSS)
        NUL
        F(VmStk)
        NUL
        F(Tgid)
        F(State)
        NUL
        F(VmLib)
        NUL
        F(VmSize)
        F(SigQ)
        NUL
        F(SigIgn)
        NUL
        F(VmPTE)
        F(FDSize)
        NUL
        F(SigBlk)
        NUL
        F(ShdPnd)
        F(VmData)
        NUL
        F(CapInh)
        NUL
        F(PPid)
        NUL NUL
        F(CapBnd)
        NUL
        F(SigPnd)
        NUL NUL
        F(VmPeak)
        NUL
        F(SigCgt)
        NUL NUL
        F(Threads)
        NUL
        F(CapPrm)
        NUL NUL
        F(Pid)
        NUL
        F(CapEff)
        NUL NUL
        F(Gid)
        NUL
        F(VmExe)
        NUL NUL
        F(Uid)
        NUL
        F(Groups)
        NUL NUL
        F(Name)
    };

    char *colon;
    status_table_struct entry;
    int table_index;
    goto base;

    for(;;){
        S = strchr(S, '\n');
        if(unlikely(!S)){
            break;                                          // if no newline
        }
        S++;

    base:
        if(unlikely(!*S)){
            break;
        }
        table_index = 63 & (asso[(int)S[3]] + asso[(int)S[2]] + asso[(int)S[0]]);
        colon = strchr(S, ':');

        if(table_index == 15){
            if(unlikely(!memcmp("voluntary_ctxt_switches",S,colon-S))){
                S = colon+2;
                goto case_nvcsw;
            }
        }
        if(table_index == 0){
            if(unlikely(!memcmp("nonvoluntary_ctxt_switches",S,colon-S))){
                S = colon+2;
                goto case_nivcsw;
            }
        }

        entry = table[table_index];
        if(unlikely(!colon)){
            break;
        }
        if(unlikely(colon[1]!='\t')){
            break;
        }
        if(unlikely(colon-S != entry.len)){
            continue;
        }
        if(unlikely(memcmp(entry.name,S,colon-S))){
            continue;
        }

        S = colon+2;                                        // past the '\t'

        goto *entry.addr;

    case_Name:
    case_ShdPnd:
    case_SigBlk:
    case_SigCgt:
    case_SigIgn:
    case_SigPnd:
    case_State:
    case_Tgid:
    case_Pid:
    case_PPid:
    case_Threads:
    case_Uid:
    case_Gid:
    case_VmData:
    case_VmExe:
    case_VmLck:
    case_VmLib:
    case_VmRSS:
    case_VmSize:
    case_VmStk:
    case_VmSwap:
    case_Groups:
    case_CapBnd:
    case_CapEff:
    case_CapInh:
    case_CapPrm:
    case_FDSize:
    case_SigQ:
    case_VmHWM:
    case_VmPTE:
    case_VmPeak:
        continue;
    case_nvcsw:
        P->nvcsw = strtol(S,&S,10);
        continue;
    case_nivcsw:
        P->nivcsw = strtol(S,&S,10);
        continue;
    }
}

static void parse_stat(const char* S, proc_t *restrict P){
    unsigned num;
    char* tmp;

    // fill in default values
    P->processor = 0;
    P->rtprio    = -1;
    P->sched     = -1;
    P->nlwp      = 0;

    S   = strchr(S, '(') + 1;
    tmp = strrchr2(S, ')');
    num = tmp - S;
    if(unlikely(num >= sizeof(P->cmd))) {
        num = sizeof(P->cmd) - 1;
    }
    memcpy(P->cmd, S, num);
    P->cmd[num] = '\0';
    S = tmp + 2;                            // skip ") "

    long priority;
    long flags;
    num = sscanf(S,
       "%c "
       "%d %d %d %d %d "
       "%lu %lu %lu %lu %lu "
       "%Lu %Lu %Lu %Lu "                   /* utime stime cutime cstime */
       "%ld %ld "
       "%d "
       "%ld "
       "%Lu "                               /* start_time */
       "%lu "
       "%ld "
       "%*u %*u %*u %*u %*u %*u "
       "%*s %*s %*s %*s "
       "%*u %*u %*u "
       "%*d "
       "%d "
       "%lu %lu",
       &P->state,
       &P->ppid,        &P->pgid,       &P->sid,      &P->tty,         &P->tpgid,
       &P->flags,       &P->min_flt,    &P->cmin_flt, &P->maj_flt,     &P->cmaj_flt,
       &P->utime,       &P->stime,      &P->cutime,   &P->cstime,
       &priority,       &P->nice,
       &P->nlwp,
       &P->alarm,
       &P->start_time,
       &P->vsize,
       &P->rss,
       //&P->rss_rlim,  &P->start_code, &P->end_code, &P->start_stack, &P->kstk_esp,  &P->kstk_eip,
       //&P->signal,    &P->blocked,    &P->sigignore,&P->sigcatch,
       //&P->wchan,     &P->nswap,      &P->cnswap,
       //&P->exit_signal, 
       &P->processor,
       &P->rtprio,      &P->sched
    );
    P->prio = priority + 100;
    P->flags = (unsigned)(flags>>6U)&0x7U;

    if(!P->nlwp){
        P->nlwp = 1;
    }

}

static void mini_parse_stat(const char* S, cmdinfo_t *restrict P){
    unsigned num;
    char* tmp;

    S   = strchr(S, '(') + 1;
    tmp = strrchr2(S, ')');
    num = tmp - S;
    if(unlikely(num >= sizeof(P->cmd))) {
        num = sizeof(P->cmd) - 1;
    }
    memcpy(P->cmd, S, num);
    P->cmd[num] = '\0';
    S = tmp + 2;                                                 // skip ") "

    num = sscanf(S, "%c ", &P->state);
}

static void parse_statm(const char* s, proc_t *restrict P){
    int num;
    long size     = 0;
    long resident = 0;
    long share    = 0;
    num = sscanf(s, "%ld %ld %ld %ld %ld %ld %ld",
	   &size, &resident, &share, &P->trs, &P->lrs, &P->drs, &P->dt);
    P->size     = 4 * size;                                         // 4 = 4096 / 1024
    P->resident = 4 * resident;                                     // 4 = 4096 / 1024
    P->share    = 4 * share;                                        // 4 = 4096 / 1024
}

static int file2str(const char *directory, const char *what, utlbuf_s *ubuf){
    char path[PROCPATHLEN];
    int  fd;
    int  num;
    int  total_read = 0;

    ubuf->len = GROWWIDTH;
    if(ubuf->buf){
        ubuf->buf[0] = '\0';
    }else {
        ubuf->buf = xxcalloc(GROWWIDTH);
    }
    sprintf(path, "%s/%s", directory, what);
    if(-1 == (fd = open(path, O_RDONLY, 0))){
        return -1;
    }

    while(0 < (num = read(fd, ubuf->buf + total_read, ubuf->len - total_read))) {
        total_read += num;
        if (total_read < ubuf->len){
            break;
        }
        ubuf->len += GROWWIDTH;
        ubuf->buf = xxrealloc(ubuf->buf, ubuf->len);
    };

    ubuf->buf[total_read] = '\0';
    close(fd);
    if(unlikely(total_read < 1)){
        return -1;
    }
    return total_read;
}

/************************************************************************************
 ***                                                                                *
 ***                               iterator part                                    * 
 ***                                                                                *
 ************************************************************************************/

static int next_pid_proc(PROCTAB *restrict const PT, proc_t *restrict const p) {
    static struct dirent *ent;
    char *restrict const  path = PT->path;

    for(;;){
        ent = readdir(PT->procfs);
        if(unlikely(unlikely(!ent) || unlikely(!ent->d_name))){
            return 0;
        }
        if(likely(likely(*ent->d_name > '0') && likely(*ent->d_name <= '9'))){
            break;
        }
    }
    p->tgid = strtoul(ent->d_name, NULL, 10);
    p->tid  = p->tgid;
    memcpy(path, "/proc/", 6);
    strcpy(path+6, ent->d_name);

    return 1;
}

static int next_pid_cmdline(PROCTAB *restrict const PT, cmdinfo_t *restrict const c) {
    static struct dirent *ent;
    char *restrict const  path = PT->path;

    for(;;){
        ent = readdir(PT->procfs);
        if(unlikely(unlikely(!ent) || unlikely(!ent->d_name))){
            return 0;
        }
        if(likely(likely(*ent->d_name > '0') && likely(*ent->d_name <= '9'))){
            break;
        }
    }
    c->tgid = strtoul(ent->d_name, NULL, 10);
    memcpy(path, "/proc/", 6);
    strcpy(path+6, ent->d_name);

    return 1;
}

static int next_tid_task(PROCTAB *restrict const PT, proc_t *restrict const t, const pid_t p, char *restrict const path) {
    static struct dirent *ent;
    if(PT->taskdir_user != p){
        if(PT->taskdir){
            closedir(PT->taskdir);
        }
        snprintf(path, PROCPATHLEN, "/proc/%d/task", p);
        PT->taskdir = opendir(path);
        if(!PT->taskdir){
            return 0;
        }
        PT->taskdir_user = p;
    }
    for (;;) {
        ent = readdir(PT->taskdir);
        if(unlikely(unlikely(!ent) || unlikely(!ent->d_name))){
            return 0;
        }
        if(likely(likely(*ent->d_name > '0') && likely(*ent->d_name <= '9'))){
            break;
        }
    }
    t->tid = strtoul(ent->d_name, NULL, 10);
    t->tgid = p;
    snprintf(path, PROCPATHLEN, "/proc/%d/task/%s", p, ent->d_name);
    return 1;
}

static proc_t* readinfo_proc(PROCTAB *restrict const PT, proc_t *restrict const p, int cmd_flag, utlbuf_s *ufbuf){
    static utlbuf_s ubuf    = {0, NULL};                      // buf for stat,statm
    static struct stat     sb;                                // stat() buffer
    char *restrict const   path  = PT->path;

    if(unlikely(stat(path, &sb) == -1)){                      // no such dirent
        goto next_proc;
    }

    if(unlikely(file2str(path, "stat", &ubuf) == -1)){        // read /proc/#/stat
        goto next_proc;
    }
    parse_stat(ubuf.buf, p);

    if(likely(file2str(path, "statm", &ubuf) != -1)){         // read /proc/#/statm
        parse_statm(ubuf.buf, p);
    }

    if(likely(file2str(path, "status", &ubuf) != -1)){        // read /proc/#/status
        parse_status(ubuf.buf, p);
    }

    if(cmd_flag){
        p->ptype = get_process_cmdline(path, ufbuf);          // read /proc/#/cmdline
    }

    return p;
next_proc:
    return NULL;
}

static cmdinfo_t* readinfo_cmdline(PROCTAB *restrict const PT, cmdinfo_t *restrict const c, utlbuf_s *ufbuf){
    static utlbuf_s ubuf  = {0, NULL};                        // buf for stat,statm
    static struct stat     sb;                                // stat() buffer
    char *restrict const   path  = PT->path;

    if(unlikely(stat(path, &sb) == -1)){                      // no such dirent
        goto next_proc;
    }

    if(unlikely(file2str(path, "stat", &ubuf) == -1)){        // read /proc/#/stat
        goto next_proc;
    }
    mini_parse_stat(ubuf.buf, c);

    c->ptype = get_process_cmdline(path, ufbuf);              // read /proc/#/cmdline

    return c;
next_proc:
    return NULL;
}

static proc_t* readinfo_task(PROCTAB *restrict const PT, proc_t *restrict const t, const pid_t p, char *restrict const path) {
    static char stat_info[GROWWIDTH] = {0};
    char * S = stat_info;

    char file_path[PROCPATHLEN];
    int fd;
    int read_num;
    //int total_read = 0;

    memset(stat_info,'\0', GROWWIDTH);
    sprintf(file_path, "%s/stat", path);
    if(-1 == (fd = open(file_path, O_RDONLY))){
        return NULL;
    }

    read_num = read(fd, stat_info, GROWWIDTH);
    close(fd);
    if(unlikely(read_num < 1)){
        return NULL;
    }
    stat_info[read_num] = '\0';

    char* tmp;
    unsigned num;

    t->processor = 0;
    t->rtprio    = -1;
    t->sched     = -1;
    t->nlwp      = 0;

    S   = strchr(S, '(') + 1;
    tmp = strrchr2(S, ')');
    num = tmp - S;
    if(unlikely(num >= sizeof(t->cmd))) {
        num = sizeof(t->cmd) - 1;
    }
    memcpy(t->cmd, S, num);
    t->cmd[num] = '\0';
    S = tmp + 2;                            // skip ") "

    long priority;
    num = sscanf(S,
       "%c "
       "%*d %*d %*d %*d %*d "
       "%*u %*u %*u %*u %*u "
       "%Lu %Lu %*u %*u "
       "%ld %*d "
       "%*d "
       "%*d "
       "%Lu "
       "%*u "
       "%*d "
       "%*u %*u %*u %*u %*u %*u "
       "%*s %*s %*s %*s "
       "%*u %*u %*u "
       "%*d "
       "%d "
       "%lu %lu",
       &t->state, &t->utime, &t->stime, &priority, &t->start_time, &t->processor, &t->rtprio, &t->sched
    );
    if(!(t->state == 'D' || t->state == 'R')){
        return NULL;
    }
    t->prio = priority + 100;

    return t;
}

/************************************************************************************
 ***                                                                                *
 ***                               api part                                        * 
 ***                                                                                *
 ************************************************************************************/

PROCTAB* openproc(PROCTAB* PT) {
    int   task_dir_missing;
    struct stat sbuf;
    int did_stat = 0;
    char target_path[10];
    ssize_t len;
    char *ptr;

    PT->task_owner   = -1;
    if(!did_stat){
        task_dir_missing = lstat("/proc/self", &sbuf);
        did_stat = 1;
        if(!task_dir_missing && S_ISLNK(sbuf.st_mode)){
            if((len = readlink("/proc/self", target_path, sizeof(target_path)-1)) != -1){
                target_path[len] = '\0';
                PT->task_owner = strtoul(target_path, &ptr, 10);
            }
        }
    }
    PT->taskdir        = NULL;
    PT->taskdir_user   = -1;
    PT->finder_proc    = next_pid_proc;
    PT->reader_proc    = readinfo_proc;
    PT->finder_cmdline = next_pid_cmdline;
    PT->reader_cmdline = readinfo_cmdline;
    PT->procfs         = opendir("/proc");
    if(!PT->procfs){ 
	free(PT); 
        return NULL; 
    }

    return PT;
}

PROCTAB* mini_openproc(PROCTAB* PT) {
    int         task_dir_missing;
    struct stat sbuf;
    ssize_t     len;
    char        target_path[10];

    PT->taskdir      = NULL;
    PT->taskdir_user = -1;
    PT->finder_task  = next_tid_task;
    PT->reader_task  = readinfo_task;
    PT->procfs       = opendir("/proc");
    PT->task_owner   = -1;
    task_dir_missing = lstat("/proc/self", &sbuf);
    if(!task_dir_missing && S_ISLNK(sbuf.st_mode)){
        if((len = readlink("/proc/self", target_path, sizeof(target_path)-1)) != -1){
            target_path[len] = '\0';
            PT->task_owner = strtoul(target_path, NULL, 10);
        }
    }
    if(!PT->procfs){
        return NULL;
    }

    return PT;
}

proc_t* read_proc(PROCTAB *restrict const PT, proc_t *restrict p, int cmd_flag, utlbuf_s *ubuf) {
    proc_t *ret;

    memset(p, '\0', sizeof(proc_t));
    for(;;){
        if (unlikely(!PT->finder_proc(PT, p))){                     // next_pid_proc
            goto out;
        }
        ret = PT->reader_proc(PT, p, cmd_flag, ubuf);               // readinfo_proc
        if(ret){ 
            return ret;
        }
    }

out:
    return NULL;
}

pid_t mini_read_proc(PROCTAB *restrict const PT) {
    pid_t ret;
    static struct dirent *ent;

    for(;;){
        ent = readdir(PT->procfs);
        if(unlikely(unlikely(!ent) || unlikely(!ent->d_name))){
            return 0;
        }
        if(likely(likely(*ent->d_name > '0') && likely(*ent->d_name <= '9'))){
            ret = strtoul(ent->d_name, NULL, 10);
            if(PT->task_owner == ret){                              // filter current process
                continue;
            }
            break;
        }
    }
    return ret;
}

cmdinfo_t* read_cmdline(PROCTAB *restrict const PT, cmdinfo_t *restrict c, utlbuf_s *ubuf) {
    cmdinfo_t *ret;

    for(;;){
        if (unlikely(!PT->finder_cmdline(PT, c))){                 // next_pid_proc
            goto out;
        }
        ret = PT->reader_cmdline(PT, c, ubuf);                     // readinfo_proc
        if(ret){ 
            return ret;
        }
    }

out:
    return NULL;
}

proc_t* read_task(PROCTAB *restrict const PT, proc_t *restrict t, const pid_t p){
    char path[PROCPATHLEN];
    proc_t *ret;
    
    memset(t,'\0', sizeof(proc_t));
    for(;;){
        if(unlikely(!PT->finder_task(PT,t,p,path))){
            break;
        }
        ret = PT->reader_task(PT, t, p, path);
        if(ret){
            return ret;
        }
    }
    return NULL;
}

void closeproc(PROCTAB* PT) {
    if(PT){
        if (PT->procfs){
            closedir(PT->procfs);
        }
        if (PT->taskdir){
            closedir(PT->taskdir);
        }
    }
}

void mini_closeproc(PROCTAB* PT) {
    if(PT){
        if (PT->procfs){
            closedir(PT->procfs);
        }
        if (PT->taskdir){
            closedir(PT->taskdir);
        }
    }
}

