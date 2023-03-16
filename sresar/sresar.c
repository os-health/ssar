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

#define _GNU_SOURCE
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <locale.h>
#include <math.h>
#include <pthread.h> 
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <zlib.h>

#include "toml.h"
#include "collection.h"
#include "utils.h"
#include "readprocess.h"
#include "sresar.h"

#define BUF_SIZE           512
#define LIST_SIZE          16
#define EPL_TOUT           200000
#define EPOLL_MAX_NUM      10 
#define D_STATE_NUM        2000
#define D_STATE_SAMPLE     100
#define Hertz              100
#define PARAM_INCORRECT    150
#define Z_FLUSH_TYPE       Z_TREES

#define handle_error(msg)  do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define THREAD_ERROR(format, ...)                                     \
    saved_errno =  errno;                                             \
    get_current_datetime_ms(gbuf);                                    \
    snprintf(basic_msg, BUF_SIZE*4-1, "DATETIME: %s, FUNCTION: %s, CODE: %d, ERROR: %s, INFO:", gbuf, __func__, saved_errno, strerror_r(saved_errno,ebuf,BUF_SIZE));\
    snprintf(user_msg, BUF_SIZE*4-1, format,  ##__VA_ARGS__);             \
    snprintf(full_msg, BUF_SIZE*9-1, "%s %s\n", basic_msg, user_msg);     \
    fputs(full_msg, opts->sresar_stderr);                       \
    fflush(opts->sresar_stderr);                                \
    opts->return_value = saved_errno


#define THREAD_INFO(format, ...)                                      \
    get_current_datetime_ms(gbuf);                                    \
    snprintf(basic_msg, BUF_SIZE*4-1, "DATETIME: %s, FUNCTION: %s, INFO:", gbuf, __func__);\
    snprintf(user_msg, BUF_SIZE*4-1, format,  ##__VA_ARGS__);             \
    snprintf(full_msg, BUF_SIZE*9-1, "%s %s\n", basic_msg, user_msg);     \
    fputs(full_msg, opts->sresar_stderr);                       \
    fflush(opts->sresar_stderr);                                \

const char *sresar_suffix = "sresar25";
const char *cmdline_suffix = "cmdline";
const char *hour_format = "%Y%m%d%H";
const char *second_format = "%Y%m%d%H%M%S";
__thread int  saved_errno;
__thread char gbuf[BUF_SIZE];
__thread char ebuf[BUF_SIZE];
__thread char user_msg[BUF_SIZE * 4];
__thread char basic_msg[BUF_SIZE * 4];
__thread char full_msg[BUF_SIZE * 9];


/***********************************************************************************
 **                                                                                *
 **                               common part                                      * 
 **                                                                                *
 ***********************************************************************************/

void print_elapsed_time(void){
    char            it_datetime[80];
    struct timespec curr;
    if (clock_gettime(CLOCK_REALTIME, &curr) == -1){
        handle_error("clock_gettime");
    }
    char* it_format = "%Y-%m-%dT%H:%M:%S";
    fmt_datetime(curr.tv_sec, it_datetime, it_format);
    printf("%s.%d:\n", it_datetime, (int)floor(curr.tv_nsec/100000000));
}

int get_current_hour_datetime(char * hour_datetime, const char * hour_format){
    struct timespec curr;
    if (clock_gettime(CLOCK_REALTIME, &curr) == -1){
        return -1;
    }
    fmt_datetime(curr.tv_sec, hour_datetime, hour_format);

    return 0;
}

int get_current_hour_timestamp(){
    struct timespec curr;
    if (clock_gettime(CLOCK_REALTIME, &curr) == -1){
        return -1;
    }
    return curr.tv_sec - curr.tv_sec % 3600;
}

long get_current_datetime(char * it_datetime){
    struct timespec curr;
    if (clock_gettime(CLOCK_REALTIME, &curr) == -1){
        return -1;
    }
    fmt_datetime(curr.tv_sec, it_datetime, second_format);
    return curr.tv_sec;
}

void get_current_datetime_ms(char * it_datetime){
    char            datetime[80];
    struct timespec curr;
    if (clock_gettime(CLOCK_REALTIME, &curr) == -1){
        handle_error("clock_gettime");
    }
    char* it_format = "%Y-%m-%dT%H:%M:%S";
    fmt_datetime(curr.tv_sec,datetime, it_format);
    sprintf(it_datetime, "%s.%09d", datetime, (int)floor(curr.tv_nsec/1));
}

int find_collect(collect *collects, char *value){
    char *string;
    collect *collect = collects;
    while(*(string = collect->src_path) != '\0'){
        if(!strcmp(string, value)){
            return 1;
        }
        collect++;
    }

    return 0;
}

void print_usage(FILE *stream){
    fprintf(stream,"\n"
      "Usage: sresar [OPTIONS] \n"
       "\n"
       "Summary: this is a collect system process activity information tools.\n"
       "\n"
       "Options:\n"
       "  -h          Get the help information.\n"
       "  <default>   Display all process info.\n"
       "  -D          Run as a daemon model\n"
       "  -L          Display all R and D task info.\n"
       "  -S          Random display some D task stack info.\n"
       "\nOutput:\n"
       "       start_time tgid ppid pgid sid tpgid nlwp psr state etimes flags sched nice rtprio prio nvcsw nivcsw utime stime size rss maj_flt min_flt cmd cmdline\n"
       "       \n"
);
}

int get_prevnode_state(Node* curr_node, int level){
    Node* cnode = curr_node->prev;
    for(int i = 1; i < level; i++){
        if(cnode->zoom_state){
            return cnode->zoom_state;
        }
        cnode = cnode->prev;
    }

    return 0;
}

/***********************************************************************************
 **                                                                                *
 **                               rotate part                                      * 
 **                                                                                *
 ***********************************************************************************/

int check_partition_usage(seq_options *opts){
    struct statvfs buff;
    if(statvfs(opts->work_path, &buff) < 0){
        THREAD_ERROR("statvfs() has failed.");
        return 1;
    }
    if((float)buff.f_bavail / (float)buff.f_blocks < (100 - (float)opts->disk_use_threshold) / 100){
        opts->partition_insufficient = 1;
        THREAD_INFO("partition usage is over load.");
        return 1;
    }
    if((float)buff.f_favail / (float)buff.f_files < (100 - (float)opts->inode_use_threshold) / 100){
        opts->partition_insufficient = 1;
        THREAD_INFO("partition inode number is over load.");
        return 1;
    }
    if((float)buff.f_bsize * (float)buff.f_bavail / 1024 / 1024 < (float)opts->disk_available_threshold){
        opts->partition_insufficient = 1;
        THREAD_INFO("partition size is not enough.");
        return 1;
    }
    opts->partition_insufficient = 0;

    return 0;
}

int recycle_diskspace_hour(const char* scan_path, const int max_sub){
    struct dirent **namelist;
    int n = scandir(scan_path, &namelist, 0, alphasort);
    int index = 0;
    if(n < 0){
        return -1;
    }else if(n - 2 < max_sub + 1){                // min is 3, inclued . & .. & current 
        while(index < n){
            free(namelist[index]);
            index++;
        }
        free(namelist);
        return 0;
    }
    
    int rm_index = 0;
    char childpath[4096];
    int rm_num = n - 2 - max_sub;
    if(rm_num > 5){
        rm_num = 5;                               //  max rmdir is 5 each time.
    }
    while(index < n){
        if(strcmp(namelist[index]->d_name,".") == 0 || strcmp(namelist[index]->d_name,"..") == 0){
            free(namelist[index]);
            index++;
            continue;
        }
        if(rm_index < rm_num){
            sprintf(childpath,"%s%s", scan_path, namelist[index]->d_name);
            remove_directory(childpath);
            rm_index++;
        }
        free(namelist[index]);
        index++;
    }
    free(namelist);

    return 0;
}

int recycle_diskspace_usage(seq_options *opts){
    struct dirent **namelist;
    int n = scandir(opts->data_path, &namelist, 0, alphasort);
    if(n < 0){
        return -1;
    }else if(n - 2 < 2){                          // min is 3, inclued    . \  .. \  current 
        return 0;
    }
    
    int index = 0;
    char childpath[4096];
    int rm_index = 0;
    int rm_max = n - 2 - 1;
    while(index < n){
        if(strcmp(namelist[index]->d_name, ".") == 0 || strcmp(namelist[index]->d_name, "..") == 0){
            free(namelist[index]);
            index++;
            continue;
        }
        if(rm_max > 0 && rm_index < 5 && check_partition_usage(opts)){
            sprintf(childpath,"%s%s", opts->data_path, namelist[index]->d_name);
            remove_directory(childpath);
            rm_index++;
            rm_max--;
        }
        free(namelist[index]);
        index++;
    }
    free(namelist);
    check_partition_usage(opts);

    return 0;
}

void* rotate_thread(void * args){
    seq_options *opts = (seq_options *)args;

    int epoll_fd;
    if((epoll_fd = epoll_create1(EPOLL_CLOEXEC)) < 0){
        THREAD_ERROR("epoll_create1 failed");
        goto release301;
    }

    int timerfd_fd;
    if((timerfd_fd = timerfd_create(CLOCK_REALTIME, 0)) < 0){
        THREAD_ERROR("timerfd_create failed");
        goto release302;
    }
    struct timespec current_time;
    if(clock_gettime(CLOCK_REALTIME, &current_time) != 0){
        THREAD_ERROR("clock_gettime() error");
        goto release302;
    }
    int remainder = current_time.tv_sec % 60;
    struct itimerspec new_value;
    new_value.it_value.tv_sec = current_time.tv_sec - remainder + opts->scatter_second -2;    // 2 second early.
    if(new_value.it_value.tv_sec <= current_time.tv_sec){
        new_value.it_value.tv_sec = new_value.it_value.tv_sec + 60;
    }

    new_value.it_value.tv_nsec    = 0;
    new_value.it_interval.tv_sec  = 60;
    new_value.it_interval.tv_nsec = 0;
    if(timerfd_settime(timerfd_fd, TFD_TIMER_ABSTIME, &new_value, NULL) != 0){   // TFD_TIMER_ABSTIME = 1
        THREAD_ERROR("timerfd_settime() error");
        goto release302;
    }
    struct epoll_event set_ev_timer;
    memset(&set_ev_timer.data, 0, sizeof(set_ev_timer.data));
    set_ev_timer.events = EPOLLIN | EPOLLET;
    set_ev_timer.data.fd = timerfd_fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timerfd_fd, &set_ev_timer) != 0){
        THREAD_ERROR("epoll_ctl() add timerfd_fd error");
        goto release302;
    }

    int hour_fd;
    if((hour_fd = timerfd_create(CLOCK_REALTIME, 0)) < 0){
        THREAD_ERROR("hour timerfd_create failed");
        goto release303;
    }
    struct timespec current_hour_time;
    if(clock_gettime(CLOCK_REALTIME, &current_hour_time) != 0){
        THREAD_ERROR("clock_gettime() hour error");
        goto release303;
    }
    struct itimerspec new_hour_value;
    new_hour_value.it_value.tv_sec     = (int)(3600*floor(current_hour_time.tv_sec/3600) + 3600);
    new_hour_value.it_value.tv_nsec    = 0;
    new_hour_value.it_interval.tv_sec  = 3600;
    new_hour_value.it_interval.tv_nsec = 0;
    if(timerfd_settime(hour_fd, TFD_TIMER_ABSTIME, &new_hour_value, NULL) != 0){   // TFD_TIMER_ABSTIME = 1
        THREAD_ERROR("timerfd_settime() hour error");
        goto release303;
    }
    struct epoll_event set_ev_hour;
    memset(&set_ev_hour.data, 0, sizeof(set_ev_hour.data));
    set_ev_hour.events = EPOLLIN | EPOLLET;
    set_ev_hour.data.fd = hour_fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, hour_fd, &set_ev_hour) != 0){
        THREAD_ERROR("epoll_ctl() add hour_fd hour error.");
        goto release303;
    }

    struct epoll_event set_ev_thread;
    memset(&set_ev_thread.data, 0, sizeof(set_ev_thread.data));
    set_ev_thread.events = EPOLLIN | EPOLLET;
    set_ev_thread.data.fd = opts->eventfd_thread;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, opts->eventfd_thread, &set_ev_thread) != 0){
        THREAD_ERROR("epoll_ctl() add eventfd error");
        goto release303;
    }

    struct epoll_event ready_events[EPOLL_MAX_NUM];
    struct stat        sb;
    int      ready_num;
    int      ready_i;
    int      ready_fd;
    ssize_t  exp_size;
    uint64_t expiration;
    int      hour_ready;
    int      timerfd_ready;
    int      current_hour_timestamp;
    char     hour_datetime[80];
    FILE    *sresar_stderr;
    int      load5s_fd;
    int      load5s_fd_old;
    int      load5s_fd_new;
    int      log_fd_old;
    int      log_fd_new;
    for(;;){
        fflush(opts->sresar_stderr);
        hour_ready = 0;
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        ready_num = epoll_wait(epoll_fd, ready_events, EPOLL_MAX_NUM, EPL_TOUT);
        if(ready_num < -1){
            THREAD_ERROR("epoll_wait() error < -1.");
            goto release303;
        }else if(ready_num == -1){
            if(errno == EINTR){
                THREAD_INFO("epoll_wait EINTR error.");
                continue;
            }
            THREAD_ERROR("epoll_wait() error.");
            goto release303;
        }else if(ready_num == 0){
            THREAD_INFO("epoll_wait time out");
            continue;
        }else if(ready_num > 1){
            THREAD_INFO("epoll_wait ready_num is %d", ready_num);
        }
        for(ready_i = 0; ready_i < ready_num; ready_i++){
            if(ready_events[ready_i].events & EPOLLIN){
                ready_fd = ready_events[ready_i].data.fd;
                if(ready_fd == hour_fd){
                    hour_ready = 1;
                }else if(ready_fd == timerfd_fd){
                    timerfd_ready = 1;
                }else if(ready_fd == opts->eventfd_thread){
                    THREAD_INFO("stop thread sresar");
                    opts->return_value = 303;
                    goto release303;
                }
                exp_size = read(ready_fd, &expiration, sizeof(uint64_t));   // 需要读出uint64_t大小。清除ready信息。
                if(exp_size != sizeof(uint64_t)){
                    THREAD_INFO("exp_size is error");
                }
            }else if(ready_events[ready_i].events & EPOLLERR){
                THREAD_ERROR("ready_events EPOLLERR.");
                goto release303;
            }
        }
        if(hour_ready){
            opts->hour_lock = 1;
            current_hour_timestamp = get_current_hour_datetime(hour_datetime, hour_format);
            sprintf(opts->data_path_hour, "%s%s/", opts->data_path, hour_datetime);

            THREAD_INFO("modify data_path_hour %s", opts->data_path_hour);
            if(mkdir(opts->data_path_hour, 0755) == -1){
                THREAD_ERROR("mkdir() data_path_hour error.");
                opts->hour_lock = 0;
                goto release303;
            }else{
                THREAD_INFO("mkdir data_path_hour success.");
            }

            sprintf(opts->load5s_file, "%s%s_load5s", opts->data_path_hour, hour_datetime);
            if((load5s_fd = open(opts->load5s_file, O_RDWR | O_APPEND | O_CREAT, 0644)) == -1){
                THREAD_ERROR("fopen load5s file error.");
                opts->hour_lock = 0;
                goto release303;
            }
            load5s_fd_old = opts->load5s_fd;
            load5s_fd_new = dup2(load5s_fd, load5s_fd_old);
            if(load5s_fd_new != load5s_fd_old){
                opts->hour_lock = 0;
                THREAD_ERROR("dup2 load5s_fd fileno error.");
                goto release303;
            }
            opts->load5s_fd = load5s_fd_new;
            close(load5s_fd);

            sprintf(opts->log_file, "%s%s_sresar.log", opts->log_path, hour_datetime);
            if((sresar_stderr = fopen(opts->log_file, "a")) == NULL){
                opts->hour_lock = 0;
                THREAD_ERROR("fopen log file error.");
                goto release303;
            }
            log_fd_old = fileno(opts->sresar_stderr);
            log_fd_new = dup2(fileno(sresar_stderr), log_fd_old);
            if(log_fd_new != log_fd_old){
                opts->hour_lock = 0;
                THREAD_ERROR("dup2 sresar_stderr fileno error.");
                goto release303;
            }
            opts->sresar_stderr = fdopen(log_fd_new, "a");
            if (sresar_stderr) {
                fclose(sresar_stderr);
                sresar_stderr = NULL;
            }
            opts->hour_timestamp_lock = get_current_hour_timestamp();
            opts->hour_lock = 0;

            THREAD_INFO("try to recycle_diskspace_hour."); 
            recycle_diskspace_hour(opts->data_path, opts->duration_threshold);
            recycle_diskspace_hour(opts->log_path, opts->duration_threshold);
        }
        if(timerfd_ready){
            if(check_partition_usage(opts)){
                THREAD_INFO("partition usage insufficient");
                recycle_diskspace_usage(opts);
            }
        }
    }

release303:
    close(hour_fd);
release302:
    close(timerfd_fd);
release301:
    close(epoll_fd);
    THREAD_INFO("send SIGUSR1 from thread rotate");
    fflush(opts->sresar_stderr);
    pthread_kill(opts->main_mtid, SIGUSR1);

    return (void *)&opts->return_value;
}

/***********************************************************************************
 **                                                                                *
 **                               live part                                        * 
 **                                                                                *
 ***********************************************************************************/

int sresar_live(seq_options *opts){
    PROCTAB  PT;
    PROCTAB* ptp;
    proc_t   pc;
    time_t             seconds_since_boot;
    unsigned long long the_start_time;
    unsigned int       the_etimes;
    unsigned long long the_btime;

    seconds_since_boot = get_uptime();
    the_btime          = get_btime();

    utlbuf_s ubuf;
    ubuf.len  = GROWWIDTH;
    ubuf.buf  = (char*)malloc(ubuf.len * sizeof(char*));
    if(ubuf.buf == NULL){
        THREAD_ERROR("memory ubuf allocated error.");
        return -2;
    }

    ptp = openproc(&PT);
    if(!ptp){
        THREAD_INFO("can not access /proc");
        return -1;
    }

    printf("start_time tgid ppid pgid sid tpgid nlwp psr state etimes flags sched nice rtprio prio nvcsw nivcsw utime stime size rss shr maj_flt min_flt cmd cmdline\n");
    int ret;
    while(read_proc(ptp, &pc, 1, &ubuf)){
        the_start_time = the_btime + pc.start_time / Hertz;
        if((unsigned long long)seconds_since_boot > (pc.start_time / Hertz)){
            the_etimes = (unsigned long long)seconds_since_boot - (pc.start_time / Hertz);
        }else{
            the_etimes = 0;
        }

        if(pc.ptype != 1){
            sprintf(ubuf.buf, "[%s]", pc.cmd);
            (ubuf.buf)[strlen(pc.cmd)+2] = '\0';
        }

        ret = fprintf(stdout, "%llu %d %d %d %d %d %d %d %c %u %lu %lu %ld %lu %ld %lu %lu %llu %llu %ld %ld %ld %lu %lu %s %s\n", \
            the_start_time, pc.tgid, pc.ppid, pc.pgid, pc.sid, pc.tpgid, pc.nlwp, pc.processor, pc.state, the_etimes, \
            pc.flags, pc.sched, pc.nice, pc.rtprio, pc.prio, pc.nvcsw, pc.nivcsw, \
            pc.utime/Hertz, pc.stime/Hertz, pc.size, pc.resident, pc.share, pc.maj_flt, pc.min_flt, pc.cmd, ubuf.buf);

    }
    fflush(stdout);
    closeproc(ptp);
    free(ubuf.buf);

    return 0;
}

/***********************************************************************************
 **                                                                                *
 **                               sys part                                         * 
 **                                                                                *
 ***********************************************************************************/

int snapsys(seq_options *opts){
    char sys_buf[4096];
    char target_file[8 * WORK_PATH_SIZE];
    FILE* src_fp;
    gzFile target_fp;
    int src_fd;
    int target_fd;
    int nbytes;

    for(int i = 0; i < COLLECT_SIZE; i++){
        if(!opts->sys_srcs[i][0]){
            break;
        }

        if(opts->sys_gzips[i]){
            if((src_fp = fopen(opts->sys_srcs[i], "r")) == NULL){
                THREAD_ERROR("fail to open src file %s", opts->sys_srcs[i]);
                return -1;
	    }

            get_current_datetime(gbuf);
            sprintf(target_file, "%s%s_%s.gz", opts->data_path_hour, gbuf, opts->sys_targets[i]);
            if((target_fp = gzopen(target_file, "w")) == NULL){
                THREAD_ERROR("fail to open target_file %s", target_file);
                return saved_errno;
	    }
	    while((nbytes = fread(sys_buf, sizeof(char), 1024, src_fp)) > 0){
                if(gzwrite(target_fp, sys_buf, nbytes) == -1){
                    THREAD_INFO("fail to write target file.");
                    return -1;
                }
            }
            if(nbytes == -1){
                THREAD_ERROR("fail to read src file %s", target_file);
                return -1;
            }
            gzflush(target_fp, Z_FLUSH_TYPE);
            gzclose(target_fp);
            if (src_fp) {
                fclose(src_fp);
                src_fp = NULL;
            }
        }else{
            if((src_fd = open(opts->sys_srcs[i], O_RDONLY|O_NONBLOCK)) == -1){
                THREAD_ERROR("fail to open src file %s", opts->sys_srcs[i]);
                return -1;
	    }

            get_current_datetime(gbuf);
            sprintf(target_file, "%s%s_%s", opts->data_path_hour, gbuf, opts->sys_targets[i]);
            if((target_fd = open(target_file,  O_WRONLY|O_CREAT|O_APPEND, 0666)) == -1){
                THREAD_ERROR("fail to open target_file %s", target_file);
                return saved_errno;
	    }
	    while((nbytes = read(src_fd, sys_buf, 4096)) > 0){
                if(write(target_fd, sys_buf, nbytes) == -1){
                    THREAD_INFO("fail to write target file.");
                    return -1;
                }
            }
            if(nbytes == -1){
                THREAD_ERROR("fail to read src file %s", target_file);
                return -1;
            }
            close(src_fd);
            close(target_fd);
        }
    }

    return 0;
}

void* snapsys_thread(void * args){
    seq_options *opts = (seq_options *)args;

    int epoll_fd;
    if((epoll_fd = epoll_create1(EPOLL_CLOEXEC)) < 0){
        THREAD_ERROR("epoll_create1 failed");
        goto release601;
    }

    int timerfd_fd;
    if((timerfd_fd = timerfd_create(CLOCK_REALTIME, 0)) < 0){
        THREAD_ERROR("timerfd_create failed");
        goto release602;
    }
    struct timespec current_time;
    if(clock_gettime(CLOCK_REALTIME, &current_time) != 0){
        THREAD_ERROR("clock_gettime() error");
        goto release602;
    }
    int remainder = current_time.tv_sec % 60;
    struct itimerspec new_value;
    new_value.it_value.tv_sec = current_time.tv_sec - remainder + opts->scatter_second;
    if(new_value.it_value.tv_sec <= current_time.tv_sec){
        new_value.it_value.tv_sec = new_value.it_value.tv_sec + 60;
    }
    new_value.it_value.tv_nsec    = 0;
    new_value.it_interval.tv_sec  = 60;
    new_value.it_interval.tv_nsec = 0;
    if(timerfd_settime(timerfd_fd, TFD_TIMER_ABSTIME, &new_value, NULL) != 0){   // TFD_TIMER_ABSTIME = 1
        THREAD_ERROR("timerfd_settime() error");
        goto release602;
    }
    struct epoll_event set_ev_timer;
    memset(&set_ev_timer.data, 0, sizeof(set_ev_timer.data));
    set_ev_timer.events = EPOLLIN | EPOLLET;
    set_ev_timer.data.fd = timerfd_fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timerfd_fd, &set_ev_timer) != 0){
        THREAD_ERROR("epoll_ctl() add timerfd_fd error");
        goto release602;
    }

    struct epoll_event set_ev_thread;
    memset(&set_ev_thread.data, 0, sizeof(set_ev_thread.data));
    set_ev_thread.events = EPOLLIN | EPOLLET;
    set_ev_thread.data.fd = opts->eventfd_thread;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, opts->eventfd_thread, &set_ev_thread) != 0){
        THREAD_ERROR("epoll_ctl() add eventfd error");
        goto release603;
    }

    struct epoll_event ready_events[EPOLL_MAX_NUM];
    struct stat        sb;
    int      ready_num;
    int      ready_i;
    int      ready_fd;
    ssize_t  exp_size;
    uint64_t expiration;
    int      timerfd_ready;
    int      current_hour_timestamp;
    char     hour_datetime[80];
    FILE    *sresar_stderr;
    int      load5s_fd;
    int      load5s_fd_old;
    int      load5s_fd_new;
    int      log_fd_old;
    int      log_fd_new;
    int      lock_i;
    int      hour_lock_break;
    unsigned int lock_usecs = 1000;
    for(;;){
        fflush(opts->sresar_stderr);
        timerfd_ready = 0;
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        ready_num = epoll_wait(epoll_fd, ready_events, EPOLL_MAX_NUM, EPL_TOUT);
        if(ready_num < -1){
            THREAD_ERROR("epoll_wait() error < -1.");
            goto release603;
        }else if(ready_num == -1){
            if(errno == EINTR){
                THREAD_INFO("epoll_wait EINTR error.");
                continue;
            }
            THREAD_ERROR("epoll_wait() error.");
            goto release603;
        }else if(ready_num == 0){
            THREAD_INFO("epoll_wait time out");
            continue;
        }else if(ready_num > 1){
            THREAD_INFO("epoll_wait ready_num is %d", ready_num);
        }
        for(ready_i = 0; ready_i < ready_num; ready_i++){
            if(ready_events[ready_i].events & EPOLLIN){
                ready_fd = ready_events[ready_i].data.fd;
                if(ready_fd == timerfd_fd){
                    timerfd_ready = 1;
                }else if(ready_fd == opts->eventfd_thread){
                    THREAD_INFO("stop thread snapsys");
                    opts->return_value = 603;
                    goto release603;
                }
                exp_size = read(ready_fd, &expiration, sizeof(uint64_t));      // 需要读出uint64_t大小。清除ready信息。
                if(exp_size != sizeof(uint64_t)){
                    THREAD_INFO("exp_size is error");
                }
            }else if(ready_events[ready_i].events & EPOLLERR){
                THREAD_ERROR("ready_events EPOLLERR.");
                goto release603;
            }
        }
        if(timerfd_ready){
            if(opts->partition_insufficient){
                THREAD_INFO("partition usage insufficient");
                continue;
            }
            hour_lock_break = 0;
            if(opts->hour_timestamp_lock != get_current_hour_timestamp()){
                THREAD_INFO("hour lock wait");
                lock_i = 0;
                while(opts->hour_timestamp_lock != get_current_hour_timestamp()){
                    usleep(lock_usecs);
                    lock_i++;
                    if(lock_i > 1000){                                        // wait 1s = 1ms * 1000
                        hour_lock_break = 1;
                        THREAD_INFO("hour lock break");
                        break;
                    }
                }
            }
            if(hour_lock_break){
                continue;
            }
            lock_i = 0;
            while(snapsys(opts) == ENOENT){
                usleep(lock_usecs);
                lock_i++;
                if(lock_i > 100){                                             // wait 0.1s = 1ms * 100
                    THREAD_INFO("can not open data_path_hour break.");
                    break;
                }
            }
        }
    }

release603:
release602:
    close(timerfd_fd);
release601:
    close(epoll_fd);
    THREAD_INFO("send SIGUSR1 from thread snapsys");
    fflush(opts->sresar_stderr);
    pthread_kill(opts->main_mtid, SIGUSR1);

    return (void *)&opts->return_value;
}

/***********************************************************************************
 **                                                                                *
 **                               process part                                        * 
 **                                                                                *
 ***********************************************************************************/

int sresar_daemon(seq_options *opts){
    PROCTAB  PT;
    PROCTAB* ptp;
    proc_t   pc;
    char    *no_use;
    time_t             seconds_since_boot;
    unsigned long long the_start_time;
    unsigned int       the_etimes;
    unsigned long long the_btime;

    seconds_since_boot = get_uptime();
    the_btime          = get_btime();   
 
    ptp = openproc(&PT);
    if(!ptp){
        THREAD_INFO("can not access /proc");
        return -1;
    }

    char sresar_file[8 * WORK_PATH_SIZE];
    get_current_datetime(gbuf);
    int sresar_fd;
    gzFile sresar_fp = NULL;
    if(opts->proc_gzip_disable){
        sprintf(sresar_file, "%s%s_%s", opts->data_path_hour, gbuf, sresar_suffix); 
        if((sresar_fd = open(sresar_file, O_WRONLY|O_CREAT|O_APPEND, 0666)) == -1){
            THREAD_ERROR("fail to open target_file %s", sresar_file);
            return saved_errno;
        }
    }else{
        no_use = malloc(143337);
        sprintf(sresar_file, "%s%s_%s.gz", opts->data_path_hour, gbuf, sresar_suffix);
        if((sresar_fp = gzopen(sresar_file, "w")) == NULL){
            THREAD_ERROR("fail to open target_file %s", sresar_file);
            return saved_errno;
        }
    }

    int ret;
    char proc_info[256];
    while(read_proc(ptp, &pc, 0, NULL)){
        the_start_time = the_btime + pc.start_time / Hertz;
        if((unsigned long long)seconds_since_boot > (pc.start_time / Hertz)){
            the_etimes = (unsigned long long)seconds_since_boot - (pc.start_time / Hertz);
        }else{
            the_etimes = 0;
        }

        sprintf(proc_info, "%llu %d %d %d %d %d %d %d %c %u %lu %lu %ld %lu %ld %lu %lu %llu %llu %ld %ld %ld %lu %lu %s\n", \
            the_start_time, pc.tgid, pc.ppid, pc.pgid, pc.sid, pc.tpgid, pc.nlwp, pc.processor, pc.state, the_etimes, \
            pc.flags, pc.sched, pc.nice, pc.rtprio, pc.prio, pc.nvcsw, pc.nivcsw, \
            pc.utime/Hertz, pc.stime/Hertz, pc.size, pc.resident, pc.share, pc.maj_flt, pc.min_flt, pc.cmd);
        if(opts->proc_gzip_disable){
            ret = write(sresar_fd, proc_info, strlen(proc_info)); 
        }else{
            ret = gzwrite(sresar_fp, proc_info, strlen(proc_info)); 
        }
        if(ret == -1){
            THREAD_ERROR("fail to write pid %d", pc.tgid);
        }
    }
    if(opts->proc_gzip_disable){
        close(sresar_fd);
    }else{
        gzflush(sresar_fp, Z_FLUSH_TYPE);
        gzclose(sresar_fp);
        free(no_use);
    }
    closeproc(ptp);

    return 0;
}

void* sresar_thread(void * args){
    seq_options *opts = (seq_options *)args;

    int epoll_fd;
    if((epoll_fd = epoll_create1(EPOLL_CLOEXEC)) < 0){
        THREAD_ERROR("epoll_create1 failed");
        goto release401;
    }

    int timerfd_fd;
    if((timerfd_fd = timerfd_create(CLOCK_REALTIME, 0)) < 0){
        THREAD_ERROR("timerfd_create failed");
        goto release402;
    }
    struct timespec current_time;
    if(clock_gettime(CLOCK_REALTIME, &current_time) != 0){
        THREAD_ERROR("clock_gettime() error");
        goto release402;
    }
    int remainder = current_time.tv_sec % 60;
    struct itimerspec new_value;
    new_value.it_value.tv_sec = current_time.tv_sec - remainder + opts->scatter_second;
    if(new_value.it_value.tv_sec <= current_time.tv_sec){
        new_value.it_value.tv_sec = new_value.it_value.tv_sec + 60;
    }

    new_value.it_value.tv_nsec    = 0;
    new_value.it_interval.tv_sec  = 60;
    new_value.it_interval.tv_nsec = 0;
    if(timerfd_settime(timerfd_fd, TFD_TIMER_ABSTIME, &new_value, NULL) != 0){   // TFD_TIMER_ABSTIME = 1
        THREAD_ERROR("timerfd_settime() error");
        goto release402;
    }
    struct epoll_event set_ev_timer;
    memset(&set_ev_timer.data, 0, sizeof(set_ev_timer.data));
    set_ev_timer.events = EPOLLIN | EPOLLET;
    set_ev_timer.data.fd = timerfd_fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timerfd_fd, &set_ev_timer) != 0){
        THREAD_ERROR("epoll_ctl() add timerfd_fd error");
        goto release402;
    }

    struct epoll_event set_ev_thread;
    memset(&set_ev_thread.data, 0, sizeof(set_ev_thread.data));
    set_ev_thread.events = EPOLLIN | EPOLLET;
    set_ev_thread.data.fd = opts->eventfd_thread;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, opts->eventfd_thread, &set_ev_thread) != 0){
        THREAD_ERROR("epoll_ctl() add eventfd error");
        goto release403;
    }

    struct epoll_event ready_events[EPOLL_MAX_NUM];
    struct stat        sb;
    int      ready_num;
    int      ready_i;
    int      ready_fd;
    ssize_t  exp_size;
    uint64_t expiration;
    int      timerfd_ready;
    int      current_hour_timestamp;
    char     hour_datetime[80];
    FILE    *sresar_stderr;
    int      load5s_fd;
    int      load5s_fd_old;
    int      load5s_fd_new;
    int      log_fd_old;
    int      log_fd_new;
    int      lock_i;
    int      hour_lock_break;
    unsigned int lock_usecs = 1000;
    uint64_t sem_count = 1;
    int      write_ret;
    for(;;){
        fflush(opts->sresar_stderr);
        timerfd_ready = 0;
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        ready_num = epoll_wait(epoll_fd, ready_events, EPOLL_MAX_NUM, EPL_TOUT);
        if(ready_num < -1){
            THREAD_ERROR("epoll_wait() error < -1.");
            goto release403;
        }else if(ready_num == -1){
            if(errno == EINTR){
                THREAD_INFO("epoll_wait EINTR error.");
                continue;
            }
            THREAD_ERROR("epoll_wait() error.");
            goto release403;
        }else if(ready_num == 0){
            THREAD_INFO("epoll_wait time out");
            continue;
        }else if(ready_num > 1){
            THREAD_INFO("epoll_wait ready_num is %d", ready_num);
        }
        for(ready_i = 0; ready_i < ready_num; ready_i++){
            if(ready_events[ready_i].events & EPOLLIN){
                ready_fd = ready_events[ready_i].data.fd;
                if(ready_fd == timerfd_fd){
                    timerfd_ready = 1;
                }else if(ready_fd == opts->eventfd_thread){
                    THREAD_INFO("stop thread sresar");
                    opts->return_value = 403;
                    goto release403;
                }
                exp_size = read(ready_fd, &expiration, sizeof(uint64_t));     // 需要读出uint64_t大小。清除ready信息。
                if(exp_size != sizeof(uint64_t)){
                    THREAD_INFO("exp_size is error");
                }
            }else if(ready_events[ready_i].events & EPOLLERR){
                THREAD_ERROR("ready_events EPOLLERR.");
                goto release403;
            }
        }
        if(timerfd_ready){
            if(opts->partition_insufficient){
                THREAD_INFO("partition usage insufficient");
                continue;
            }
            hour_lock_break = 0;
            if(opts->hour_timestamp_lock != get_current_hour_timestamp()){
                THREAD_INFO("hour lock wait");
                lock_i = 0;
                while(opts->hour_timestamp_lock != get_current_hour_timestamp()){
                    usleep(lock_usecs);
                    lock_i++;
                    if(lock_i > 1000){                                        // wait 1s = 1ms * 1000
                        hour_lock_break = 1;
                        THREAD_INFO("hour lock break");
                        break;
                    }
                }
            }
            if(hour_lock_break){
                continue;
            }
            lock_i = 0;
            while(sresar_daemon(opts) == ENOENT){
                usleep(lock_usecs);
                lock_i++;
                if(lock_i > 100){                                             // wait 0.1s = 1ms * 100
                    THREAD_INFO("can not open data_path_hour break.");
                    break;
                }
            }
            write_ret = write(opts->eventfd_cmdinfo, &sem_count, sizeof(sem_count));
            if(write_ret < 0){
                THREAD_INFO("write eventfd_cmdinfo fail");
            }
        }
    }

release403:
release402:
    close(timerfd_fd);
release401:
    close(epoll_fd);
    THREAD_INFO("send SIGUSR1 from thread sresar");
    fflush(opts->sresar_stderr);
    pthread_kill(opts->main_mtid, SIGUSR1);

    return (void *)&opts->return_value;
}

int get_cmdinfo(seq_options *opts, cmdinfo_t *cmdinfo, utlbuf_s *ubuf, utlbuf_s *ufbuf){
    PROCTAB    PT;
    PROCTAB*   ptp;

    ptp = openproc(&PT);
    if(!ptp){
        THREAD_INFO("can not access /proc");
        return -1;
    }

    char sresar_file[8 * WORK_PATH_SIZE];
    get_current_datetime(gbuf);
    int sresar_fd;
    gzFile sresar_fp = NULL;
    if(opts->proc_gzip_disable){
        sprintf(sresar_file, "%s%s_%s", opts->data_path_hour, gbuf, cmdline_suffix);
        if((sresar_fd = open(sresar_file, O_WRONLY|O_CREAT|O_APPEND, 0666)) == -1){
            THREAD_ERROR("fail to open target_file %s", sresar_file);
            return saved_errno;
        }
    }else{
        sprintf(sresar_file, "%s%s_%s.gz", opts->data_path_hour, gbuf, cmdline_suffix); 
        if((sresar_fp = gzopen(sresar_file, "w")) == NULL){
            THREAD_ERROR("fail to open target_file %s", sresar_file);
            return saved_errno;
        }
    }

    int ret;
    while(read_cmdline(ptp, cmdinfo, ubuf)){
        if(cmdinfo->ptype != 1){
            sprintf(ubuf->buf, "[%s]", cmdinfo->cmd);
            (ubuf->buf)[strlen(cmdinfo->cmd)+2] = '\0';
        }

        for(;;){
            if(16 + strlen(ubuf->buf) + 1 >= ufbuf->len){
                ufbuf->len += GROWWIDTH;
                ufbuf->buf  = (char*)realloc(ufbuf->buf, ufbuf->len * sizeof(char*));   // definitely lost
            }else{
                break;
            }
        }
        sprintf(ufbuf->buf, "%d %s\n", cmdinfo->tgid, ubuf->buf);

        if(opts->proc_gzip_disable){
            ret = write(sresar_fd, ufbuf->buf, strlen(ufbuf->buf));
        }else{
            ret = gzwrite(sresar_fp, ufbuf->buf, strlen(ufbuf->buf)); 
        }
        if(ret == -1){
            THREAD_ERROR("fail to printf pid %d", cmdinfo->tgid);
        }
    }

    if(opts->proc_gzip_disable){
        close(sresar_fd);
    }else{
        gzflush(sresar_fp, Z_FLUSH_TYPE);
        gzclose(sresar_fp);
    }
    closeproc(ptp);

    return 0;
}

void* cmdinfo_thread(void * args){
    seq_options *opts = (seq_options *)args;

    int epoll_fd;
    if((epoll_fd = epoll_create1(EPOLL_CLOEXEC)) < 0){
        THREAD_ERROR("epoll_create1 failed");
        goto release502;
    }

    struct epoll_event set_ev_read;
    memset(&set_ev_read.data, 0, sizeof(set_ev_read.data));
    set_ev_read.events = EPOLLIN | EPOLLET;
    set_ev_read.data.fd = opts->eventfd_cmdinfo;                          // add eventfd to epoll
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, opts->eventfd_cmdinfo, &set_ev_read) != 0){
        THREAD_ERROR("epoll_ctl() add eventfd_cmdinfo error");
        goto release502;
    }

    struct epoll_event set_ev_thread;
    memset(&set_ev_thread.data, 0, sizeof(set_ev_thread.data));
    set_ev_thread.events = EPOLLIN | EPOLLET;
    set_ev_thread.data.fd = opts->eventfd_thread;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, opts->eventfd_thread, &set_ev_thread) != 0){
        THREAD_ERROR("epoll_ctl() add eventfd error");
        goto release502;
    }

    cmdinfo_t cmdinfo;
    utlbuf_s  ubuf;
    ubuf.len  = GROWWIDTH;
    ubuf.buf  = (char*)malloc(ubuf.len * sizeof(char*));
    if(ubuf.buf == NULL){
        THREAD_ERROR("memory ubuf allocated error.");
        goto release503;
    }

    utlbuf_s ufbuf;
    ufbuf.len = GROWWIDTH;
    ufbuf.buf = (char*)malloc(ufbuf.len * sizeof(char*));
    if(ufbuf.buf == NULL){
        THREAD_ERROR("memory ufbuf allocated error.");
        goto release504;
    }

    struct epoll_event ready_events[EPOLL_MAX_NUM];
    int      ready_num;
    int      ready_i;
    int      ready_fd;
    ssize_t  exp_size;
    uint64_t expiration;
    int      eventfd_ready;
    int      lock_i;
    int      hour_lock_break;
    unsigned int lock_usecs = 1000;
    for(;;){
        fflush(opts->sresar_stderr);
        eventfd_ready = 0;
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        ready_num = epoll_wait(epoll_fd, ready_events, EPOLL_MAX_NUM, EPL_TOUT);
        if(ready_num < -1){
            THREAD_ERROR("epoll_wait() error < -1.");
            goto release503;
        }else if(ready_num == -1){
            if(errno == EINTR){
                THREAD_INFO("epoll_wait EINTR error.");
                continue;
            }
            THREAD_ERROR("epoll_wait() error.");
            goto release503;
        }else if(ready_num == 0){
            THREAD_INFO("epoll_wait time out");
            continue;
        }else if(ready_num > 1){
            THREAD_INFO("epoll_wait ready_num is %d", ready_num);
        }
        for(ready_i = 0; ready_i < ready_num; ready_i++){
            if(ready_events[ready_i].events & EPOLLIN){
                ready_fd = ready_events[ready_i].data.fd;
                if(ready_fd == opts->eventfd_cmdinfo){
                    eventfd_ready = 1;
                }else if(ready_fd == opts->eventfd_thread){
                    THREAD_INFO("stop thread cmdinfo");
                    opts->return_value = 503;
                    goto release503;
                }
                exp_size = read(ready_fd, &expiration, sizeof(uint64_t));     // 需要读出uint64_t大小。清除ready信息。
                if(exp_size != sizeof(uint64_t)){
                    THREAD_INFO("exp_size is error");
                }
            }else if(ready_events[ready_i].events & EPOLLERR){
                THREAD_ERROR("ready_events EPOLLERR.");
                goto release503;
            }
        }
        if(eventfd_ready){
            if(opts->partition_insufficient){
                THREAD_INFO("partition usage insufficient");
                continue;
            }
            hour_lock_break = 0;
            if(opts->hour_timestamp_lock != get_current_hour_timestamp()){
                THREAD_INFO("hour lock wait");
                lock_i = 0;
                while(opts->hour_timestamp_lock != get_current_hour_timestamp()){
                    usleep(lock_usecs);
                    lock_i++;
                    if(lock_i > 1000){                                        // wait 1s = 1ms * 1000
                        hour_lock_break = 1;
                        THREAD_INFO("hour lock break");
                        break;
                    }
                }
            }
            if(hour_lock_break){
                continue;
            }
            lock_i = 0;
            while(get_cmdinfo(opts, &cmdinfo, &ubuf, &ufbuf) == ENOENT){
                usleep(lock_usecs);
                lock_i++;
                if(lock_i > 100){                                             // wait 0.1s = 1ms * 100
                    THREAD_INFO("can not open data_path_hour break.");
                    break;
                }
            }
        }
    }

release504:
    free(ufbuf.buf);
    ufbuf.buf = NULL;
release503:
    free(ubuf.buf);
    ubuf.buf = NULL;
release502:
    close(epoll_fd);
release501:
    THREAD_INFO("send SIGUSR1 from thread cmdinfo");
    fflush(opts->sresar_stderr);
    pthread_kill(opts->main_mtid, SIGUSR1);

    return (void *)&opts->return_value;
}

/***********************************************************************************
 **                                                                                *
 **                               load part                                        * 
 **                                                                                *
 ***********************************************************************************/

int loadrd(seq_options *opts, utlarr_i *uarr, utlbuf_s *ubuf, utlbuf_s *ufbuf){
    PROCTAB  PT;
    PROCTAB* ptp;
    pid_t    pc;
    proc_t   tk;
    int   i = 0;
    int   j = 0;
    int   real_size;
    char  loadrd_datetime[BUF_SIZE];
    char  it_nwchan[8];
    char *the_stack_info;

    ptp = mini_openproc(&PT);
    if(!ptp){
        THREAD_INFO("can not access /proc");
        exit(1);
    }

    get_current_datetime(loadrd_datetime);
    FILE *loadrd_stdout;
    if(SRC_DAEMON == opts->src){
        char loadrd_file[8 * WORK_PATH_SIZE];
        sprintf(loadrd_file, "%s%s_loadrd", opts->data_path_hour, loadrd_datetime);
        loadrd_stdout = fopen(loadrd_file, "w");
    }else if(SRC_LOADRD == opts->src){
        loadrd_stdout = stdout;
    }
    int ret;
    if(opts->src == SRC_LOADRD){
        printf("state tid tgid psr prio cmd\n");
    }
    while((pc = mini_read_proc(ptp))){
        while(read_task(ptp, &tk, pc)){
            if(!loadrd_stdout){
                continue;
            }
            if(SRC_DAEMON == opts->src){
                ret = fprintf(loadrd_stdout,"%c %d %d %d %ld %s\n", tk.state, tk.tid, tk.tgid, tk.processor, tk.prio, tk.cmd);
            }else if(SRC_LOADRD == opts->src){
                ret = fprintf(loadrd_stdout,"%c %d %d %d %ld %s\n", tk.state, tk.tid, tk.tgid, tk.processor, tk.prio, tk.cmd);
            }
            if('D' == tk.state){
                if(i >= uarr->len){
                    uarr->len += D_STATE_NUM;
                    uarr->arr = (int*)realloc(uarr->arr, uarr->len * sizeof(int));
                }
                uarr->arr[i] = tk.tid;
                i++;
            }
        }
    }
    if(SRC_DAEMON == opts->src){
        fflush(loadrd_stdout);
        if (loadrd_stdout) {
            fclose(loadrd_stdout);
            loadrd_stdout = NULL;
        }
    }
    mini_closeproc(ptp);

    if(SRC_DAEMON == opts->src || SRC_STACK == opts->src){
        FILE *stack_stdout;
        if(SRC_DAEMON == opts->src){
            char sresar_file[8 * WORK_PATH_SIZE];
            sprintf(sresar_file, "%s%s_stack", opts->data_path_hour, loadrd_datetime);
            stack_stdout = fopen(sresar_file, "w");
        }else{
            stack_stdout = stdout;
        }
        if(opts->stack_sample_disable){
            real_size = i;
        }else{
            randomlize(uarr->arr, i);
            real_size = (i < D_STATE_SAMPLE) ? i : D_STATE_SAMPLE;
        }
        if(opts->src == SRC_STACK){
            printf("tid nwchan stack_info\n"); 
        }
        for(j = 0; j < real_size; j++){
            the_stack_info = read_stack_file(uarr->arr[j], ubuf, ufbuf, it_nwchan);
            if(stack_stdout){
                fprintf(stack_stdout, "%d %s %s\n", uarr->arr[j], it_nwchan, the_stack_info);
            }
        }
        if(SRC_DAEMON == opts->src){
            fflush(stack_stdout);
            if (stack_stdout) {
                fclose(stack_stdout);
                stack_stdout = NULL;
            }
        }
    }

    return 0;
}

void* loadrd_thread(void * args){
    seq_options *opts = (seq_options *)args;

    int epoll_fd;
    if((epoll_fd = epoll_create1(EPOLL_CLOEXEC)) < 0){
        THREAD_ERROR("epoll_create1 failed");
        goto release202;
    }
    struct epoll_event set_ev_read;
    memset(&set_ev_read.data, 0, sizeof(set_ev_read.data));
    set_ev_read.events = EPOLLIN | EPOLLET;
    set_ev_read.data.fd = opts->eventfd_load1;                        // add eventfd to epoll
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, opts->eventfd_load1, &set_ev_read) != 0){
        THREAD_ERROR("epoll_ctl() add eventfd_load1 error");
        goto release202;
    }
    struct epoll_event set_ev_thread;
    memset(&set_ev_thread.data, 0, sizeof(set_ev_thread.data));
    set_ev_thread.events = EPOLLIN | EPOLLET;
    set_ev_thread.data.fd = opts->eventfd_thread;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, opts->eventfd_thread, &set_ev_thread) != 0){
        THREAD_ERROR("epoll_ctl() add eventfd_thread error");
        goto release202;
    }

    struct utlarr_i uarr;
    uarr.len  = D_STATE_NUM;
    uarr.arr  = (int*)malloc(uarr.len * sizeof(int));
    if(uarr.arr == NULL){
        THREAD_ERROR("memory uarr allocated error.");
        goto release203;
    }

    utlbuf_s ubuf;
    ubuf.len  = GROWWIDTH;
    ubuf.buf  = (char*)malloc(ubuf.len * sizeof(char*));
    if(ubuf.buf == NULL){
        THREAD_ERROR("memory ubuf allocated error.");
        goto release204;
    }

    utlbuf_s ufbuf;
    ufbuf.len = GROWWIDTH;
    ufbuf.buf = (char*)malloc(ufbuf.len * sizeof(char*));
    if(ufbuf.buf == NULL){
        THREAD_ERROR("memory ufbuf allocated error.");
        goto release205;
    }

    struct epoll_event ready_events[EPOLL_MAX_NUM];
    struct stat        sb;
    int      ready_num;
    int      ready_i;
    int      ready_fd;
    uint64_t expiration = 0;
    ssize_t  exp_size;
    int      eventfd_ready;
    char     hour_datetime[80];
    int      lock_i;
    int      hour_lock_break;
    unsigned int lock_usecs = 1000;
    for(;;){
        fflush(opts->sresar_stderr);
        eventfd_ready = 0;
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        ready_num = epoll_wait(epoll_fd, &ready_events[0], EPOLL_MAX_NUM, EPL_TOUT);
        if(ready_num < -1){
            THREAD_ERROR("epoll_wait() error < -1.");
            goto release205;
        }else if(ready_num == -1){
            if(errno == EINTR){
                THREAD_INFO("epoll_wait EINTR error.");
                continue;
            }
            THREAD_ERROR("epoll_wait() error.");
            goto release205;
        }else if(ready_num == 0){
            THREAD_INFO("epoll_wait time out");
            continue;
        }else if(ready_num > 1){
            if(opts->sresar_stderr){
                fprintf(opts->sresar_stderr, "FUNCTION: %s, INFO: %s, %d\n", __func__, "epoll_wait ready_num is ", ready_num);
            }
        }
        for(ready_i = 0; ready_i < ready_num; ready_i++){
            if(ready_events[ready_i].events & EPOLLIN){
                ready_fd = ready_events[ready_i].data.fd;
                if(ready_fd == opts->eventfd_load1){
                    eventfd_ready = 1;
                }else if(ready_fd == opts->eventfd_thread){
                    THREAD_INFO("stop thread loadrd");
                    opts->return_value = 205;
                    goto release205;
                }
                exp_size = read(ready_fd, &expiration, sizeof(uint64_t));   // 需要读出uint64_t大小。清除ready信息。
                if(exp_size != sizeof(uint64_t)){
                    THREAD_INFO("exp_size is error");
                }
            }else if(ready_events[ready_i].events & EPOLLERR){
                THREAD_ERROR("ready_events EPOLLERR.");
                goto release205;
            }
        }
        if(eventfd_ready){
            if(opts->partition_insufficient){
                THREAD_INFO("partition usage insufficient");
                continue; 
            }
            hour_lock_break = 0;
            if(opts->hour_timestamp_lock != get_current_hour_timestamp()){
                THREAD_INFO("hour lock wait");
                lock_i = 0;
                while(opts->hour_timestamp_lock != get_current_hour_timestamp()){
                    usleep(lock_usecs);
                    lock_i++;
                    if(lock_i > 1000){                                        // wait 1s = 1ms * 1000
                        THREAD_INFO("hour lock break");
                        hour_lock_break = 1;
                        break;
                    }
                }
            }
            if(hour_lock_break){
                continue;
            }
            loadrd(opts, &uarr, &ubuf, &ufbuf);
        }
    }

release205:
    free(ufbuf.buf);
    ufbuf.buf = NULL;
release204:
    free(ubuf.buf);
    ubuf.buf = NULL;
release203:
    free(uarr.arr);
    uarr.arr = NULL;
release202:
    close(epoll_fd);
release201:
    THREAD_INFO("send SIGUSR1 from thread loadrd");
    fflush(opts->sresar_stderr);
    pthread_kill(opts->main_mtid, SIGUSR1);
    
    return (void *)&opts->return_value;
}

void* load1_thread(void * args){
    seq_options *opts = (seq_options *)args;

    int loadavg_fd = open("/proc/loadavg", O_RDONLY | O_NONBLOCK);
    if(loadavg_fd == -1){
        THREAD_ERROR("open() loadavg error");
        goto release102;
    }

    Node* plist = (Node*)malloc(LIST_SIZE * sizeof(Node));
    if(plist == NULL){
        THREAD_ERROR("memory allocated error.");
        goto release103;
    }
    int list_size = LIST_SIZE;
    circular_linkedlist_init(plist, list_size);
    Node* current_node = plist;

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if(epoll_fd < 0){
        THREAD_ERROR("epoll_create1 failed");
        goto release104;
    }

    int timerfd_fd = timerfd_create(CLOCK_REALTIME, 0);                            // 0 or TFD_NONBLOCK
    if(timerfd_fd < 0){
        THREAD_ERROR("timerfd_create failed");
        goto release105;
    }
    struct timespec current_time;
    if(clock_gettime(CLOCK_REALTIME, &current_time) != 0){
        THREAD_ERROR("clock_gettime() error");
        goto release105;
    }
    struct itimerspec new_value;
    new_value.it_value.tv_sec     = current_time.tv_sec + 1;
    new_value.it_value.tv_nsec    = 0;
    new_value.it_interval.tv_sec  = 0;
    new_value.it_interval.tv_nsec = 1000000 * opts->load_collect_interval;         // 1ms * n, n = 10
    if(timerfd_settime(timerfd_fd, TFD_TIMER_ABSTIME, &new_value, NULL) != 0){     // TFD_TIMER_ABSTIME = 1
        THREAD_ERROR("timerfd_settime() error");
        goto release105;
    }

    struct epoll_event set_ev_timer;
    memset(&set_ev_timer.data, 0, sizeof(set_ev_timer.data));
    set_ev_timer.events = EPOLLIN | EPOLLET;
    set_ev_timer.data.fd = timerfd_fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timerfd_fd, &set_ev_timer) != 0){
        THREAD_ERROR("epoll_ctl() add timerfd_fd error");
        goto release105;
    }
    struct epoll_event set_ev_thread;
    memset(&set_ev_thread.data, 0, sizeof(set_ev_thread.data));
    set_ev_thread.events = EPOLLIN | EPOLLET;
    set_ev_thread.data.fd = opts->eventfd_thread;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, opts->eventfd_thread, &set_ev_thread) != 0){
        THREAD_ERROR("epoll_ctl() add eventfd error");
        goto release105;
    }

    struct epoll_event ready_events[EPOLL_MAX_NUM];
    struct stat        sb;
    int         ready_num;
    int         ready_i;
    int         ready_fd;
    ssize_t     exp_size;
    uint64_t    expiration = 0;
    float       load1;
    int         load5s;
    int         loadavg_num;
    char        loadavg_buf[64];
    int         load5s_num;
    char        load5s_buf[64];
    int         sscanf_ret;
    int         load5s_type;
    int         nr_runnable;
    int         nr_threads;
    uint64_t    sem_count = 1;
    int         write_ret;
    int         timerfd_ready;
    char        hour_datetime[80];
    for(;;){
        fflush(opts->sresar_stderr);
        timerfd_ready = 0;
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        ready_num = epoll_wait(epoll_fd, ready_events, EPOLL_MAX_NUM, EPL_TOUT);
        if(ready_num < -1){
            THREAD_ERROR("epoll_wait() error < -1.");
            goto release105;
        }else if(ready_num == -1){
            if(errno == EINTR){
                THREAD_INFO("epoll_wait EINTR error.");
                continue;
            }
            THREAD_ERROR("epoll_wait() error.");
            goto release105;
        }else if(ready_num == 0){
            THREAD_INFO("epoll_wait time out");
            continue;
        }else if(ready_num > 1){
            THREAD_INFO("epoll_wait num %d", ready_num);
        }
        for(ready_i = 0; ready_i < ready_num; ready_i++){
            if(ready_events[ready_i].events & EPOLLIN){
                ready_fd = ready_events[ready_i].data.fd;
                if(ready_fd == timerfd_fd){
                    timerfd_ready = 1;
                }else if(ready_fd == opts->eventfd_thread){
                    THREAD_INFO("stop thread load1");
                    opts->return_value = 105;
                    goto release105;
                }
                exp_size = read(ready_fd, &expiration, sizeof(uint64_t));            // 需要读出uint64_t大小。清除ready信息。
                if(exp_size != sizeof(uint64_t)){
                    THREAD_INFO("exp_size is error");
                }
            }else if(ready_events[ready_i].events & EPOLLERR){
                THREAD_ERROR("ready_events EPOLLERR.");
                goto release105;
            }
        }
        if(timerfd_ready){
            if(lseek(loadavg_fd, 0L, SEEK_SET) == -1){
                THREAD_INFO("lseek() loadavg error");
                continue;
            }
            if((loadavg_num = read(loadavg_fd, loadavg_buf, sizeof(loadavg_buf) - 1)) < 0){
                THREAD_INFO("read() loadavg error");
                continue;
            }
            loadavg_buf[loadavg_num] = '\0';
            sscanf_ret = sscanf(loadavg_buf, "%f %*f %*f %d/%d",&load1, &nr_runnable, &nr_threads);
            if(sscanf_ret < 2){
                THREAD_INFO("sscanf() loadavg error");
                continue;
            }
            if(load1 != current_node->prev->load1){
                if(opts->hour_lock){
                    THREAD_INFO("hour lock");
                    continue;                                                        // 0.1s later will retry.
                }
                current_node->load1 = load1;
                current_node->load5s_state = 0;
                current_node->zoom_state = 0;
                if(!current_node->load1 || !current_node->prev->load1){
                    load5s_type = 1;                                                  // 1m
                    load5s = (int)current_node->load1;
                    if(load5s > opts->load1m_baseline){
                        current_node->load5s_state = 2;
                        if(!get_prevnode_state(current_node, 10)){
                            current_node->zoom_state = 3;
                        }
                    }
                }else{
                    load5s_type = 5;                                                  // 5s
                    load5s=(int)round((2048 * (2048 * current_node->load1 - 10) - 1024 - 1884 * 2048 * current_node->prev->load1)/164/2048);
                    if(load5s > opts->load5s_baseline){
                        current_node->load5s_state = 2;
                        if(((double)load5s > 1.05 * (double)(current_node->prev->load5s) && load1 > 1.05 * current_node->prev->load1)
                            || !get_prevnode_state(current_node, 10)){
                            current_node->zoom_state = 3;
                        }
                    }
                }
                current_node->load5s = load5s;
                if((load5s_num = sprintf(load5s_buf, "%lu %d %d %.2lf %d %d %d %d\n",
                   get_current_datetime(gbuf), nr_threads, nr_runnable, current_node->load1, load5s, load5s_type, current_node->load5s_state, current_node->zoom_state)) == -1){
                    THREAD_ERROR("sprintf() load5s_buf error.");
                }
                if(opts->partition_insufficient){
                    THREAD_INFO("partition usage insufficient"); 
                }else{
                    if(write(opts->load5s_fd, load5s_buf, load5s_num) == -1){
                        THREAD_ERROR("write() load5s_fd error.");
                    }
                }
                if(current_node->zoom_state){
                    write_ret = write(opts->eventfd_load1, &sem_count, sizeof(sem_count));
                    if(write_ret < 0){
                        THREAD_INFO("write eventfd_load1 fail");
                    }
                }
                current_node = current_node->next;
            }
        }
    }

release105:
    close(timerfd_fd);
release104:
    close(epoll_fd);
release103:
    free(plist);
release102:
    close(loadavg_fd);
release101:
    THREAD_INFO("send SIGUSR1 from thread load1");
    fflush(opts->sresar_stderr);
    pthread_kill(opts->main_mtid, SIGUSR1);

    return (void *)&opts->return_value;
}

/***********************************************************************************
 **                                                                                *
 **                               init part                                        * 
 **                                                                                *
 ***********************************************************************************/

void init_pid_lock(seq_options *opts){
    const char* pid_path = "/run/lock/os_health/";
    struct stat sb;
    char pid_buf[32];

    if(0 == stat(pid_path, &sb)){
        if(!S_ISDIR(sb.st_mode)){
            THREAD_INFO("pid path is not dir.");
            exit(1);
        }
    }else{
        if(mkdir("/run/lock/os_health/", 0755) == -1){
            THREAD_ERROR("mkdir /run/lock/os_health/ error ");
            exit(1);
        }
    }
    
    if((opts->pid_fd = open("/run/lock/os_health/sresar.pid", O_RDWR|O_CREAT, 0644)) == -1){  // same to  /var/run/lock/os_health/sresar.pid
        THREAD_ERROR("open sresar.pid failure ");
        exit(saved_errno);
    }
    int lock;
    if((lock = flock(opts->pid_fd, LOCK_EX|LOCK_NB)) != 0){
        saved_errno =  errno;
        if(EWOULDBLOCK == saved_errno){
            printf("process sresar have running now.");
            exit(200);
        }else{
            THREAD_ERROR("lock sresar.pid %d failure", lock);
            exit(saved_errno);
        }
    }

    if(ftruncate(opts->pid_fd, 0) < 0){
        THREAD_ERROR("ftruncate sresar.pid failure");
        exit(saved_errno);
    }
    snprintf(pid_buf, sizeof(pid_buf), "%d", getpid());
    if(write(opts->pid_fd, pid_buf, strlen(pid_buf)) == -1){
        THREAD_ERROR("fail to write pid lock file.");
        exit(saved_errno);
    }
}

int pid_unlock(seq_options *opts){
    int lock;
    if((lock = flock(opts->pid_fd, LOCK_UN)) != 0) {
        THREAD_INFO("unlock failure.");
        return -1;
    }
    close(opts->pid_fd);

    return 0;
}

void init_option(seq_options *opts,int argc, char* argv[]){
    unsigned char opt;
    int src_set = 0;
    while((opt = getopt(argc, argv, ":LDSh"))){
        if(opt == 255){                                            // 0xff
            break;
        }
        switch(opt){
            case 'D':
                if(src_set){
                    print_usage(stdout);
                    exit(0);
                }
                opts->src = SRC_DAEMON;
                src_set = 1;
                break;
            case 'L':
                if(src_set){
                    print_usage(stdout);
                    exit(0);
                }
                opts->src = SRC_LOADRD;
                src_set = 1;
                break;
            case 'S':
                if(src_set){
                    print_usage(stdout);
                    exit(0);
                }
                opts->src = SRC_STACK;
                src_set = 1;
                break;
            case 'h':
                print_usage(stdout);
                exit(0);
                break;
            case '?':
                fprintf(stderr, "undefined option: %s\n", argv[optind-1]);
                print_usage(stderr);
                exit(PARAM_INCORRECT);
                break;
            case ':':
                fprintf(stderr, "option %s missing argument\n", argv[optind-1]);
                print_usage(stderr);
                exit(PARAM_INCORRECT);
                break;
            default:
                fprintf(stderr, "parameter %s is incorrect.\n", argv[optind-1]);
                exit(PARAM_INCORRECT);
                break;
        }
    }
}

int init_basic_config(seq_options* opts){
    int strncpy_length;
    struct stat sb;

    FILE* root_fp = fopen("/etc/ssar/ssar.conf", "rb");
    if(!root_fp){
        goto release1001;
    }

    enum{err_buf_size=0x10,};
    char errbuf[err_buf_size];
    toml_table_t* root_conf = toml_parse_file(root_fp, errbuf, err_buf_size);
    if(!root_conf){
        printf("file is empty or err is %s\n", errbuf);
        goto release1002;
    }

    // parse ssar.conf [main] part
    toml_table_t* main_conf = toml_table_in(root_conf, "main");
    if(!main_conf){
        printf("main is empty\n");
        goto release1003;
    }

    const char* i_key = 0;
    const char* i_value = 0;
    int64_t i_num = 0;
    char*   i_str = 0;
    int     i_bool = 0; 
    int     i;
    for(i = 0; ; i += 1){
        i_key = toml_key_in(main_conf, i);
        if(!i_key){
            break;
        }
        i_value = toml_raw_in(main_conf, i_key);
        if(!i_value){
            i_value = "";
        }

        if(!opts->sresar_stderr){
            exit(150);
        }

        if(!strcmp(i_key, "work_path")){
            if(toml_rtos(i_value, &i_str)){                    //  definitely lost 
                fprintf(opts->sresar_stderr, "ERROR: work_path %s is not correct.\n", i_value); 
                exit(150);
            }
            strncpy_length = (strlen(i_str) < (128-1))?strlen(i_str):(128-1);
            strncpy(opts->work_path, i_str, strncpy_length);
            opts->work_path[strncpy_length] = '\0';
            if('/' == opts->work_path[strncpy_length-1]){
                opts->work_path[strncpy_length-1] = '\0';
            }
            if(0 != stat(opts->work_path, &sb)){
                fprintf(opts->sresar_stderr, "ERROR: path %s is not exist\n", opts->work_path); 
                exit(150);
            }else if(!S_ISDIR(sb.st_mode)){
                fprintf(opts->sresar_stderr, "ERROR: path %s is not dir\n", opts->work_path); 
                exit(150);
            }
        }else if(!strcmp(i_key, "duration_threshold")){
            if(toml_rtoi(i_value, &i_num)){
                fprintf(opts->sresar_stderr, "duration_threshold %s is not correct.\n", i_value);
                exit(150);
            }
            opts->duration_threshold = i_num;
        }else if(!strcmp(i_key, "inode_use_threshold")){
            if(toml_rtoi(i_value, &i_num)){
                fprintf(opts->sresar_stderr, "inode_use_threshold %s is not correct.\n", i_value);
                exit(150);
            }
            opts->inode_use_threshold = i_num;
            if(!(0 < opts->inode_use_threshold && opts->inode_use_threshold < 100)){
                fprintf(opts->sresar_stderr, "inode_use_threshold %lu is not correct.\n", opts->inode_use_threshold);
                exit(150);
            }
        }else if(!strcmp(i_key, "disk_use_threshold")){
            if(toml_rtoi(i_value, &i_num)){
                fprintf(opts->sresar_stderr, "disk_use_threshold %s is not correct.\n", i_value);
                exit(150);
            }
            opts->disk_use_threshold = i_num;
            if(!(0 < opts->disk_use_threshold && opts->disk_use_threshold < 100)){
                fprintf(opts->sresar_stderr,"disk_use_threshold %lu is not correct.\n", opts->disk_use_threshold);
                exit(150);
            }
        }else if(!strcmp(i_key, "disk_available_threshold")){
            if(toml_rtoi(i_value, &i_num)){
                fprintf(opts->sresar_stderr, "disk_available_threshold %s is not correct.\n", i_value);
                exit(150);
            }
            opts->disk_available_threshold = i_num;
        }else if(!strcmp(i_key, "duration_restart")){
            if(toml_rtoi(i_value, &i_num)){
                fprintf(opts->sresar_stderr, "duration_restart %s is not correct.\n", i_value);
                exit(150);
            }
            opts->duration_restart = i_num;
        }else if(!strcmp(i_key, "scatter_second")){
            if(toml_rtoi(i_value, &i_num)){
                fprintf(opts->sresar_stderr, "scatter_second %s is not correct.\n", i_value);
                exit(150);
            }
            opts->scatter_second =  i_num;
            if(!(0 <= opts->scatter_second && opts->scatter_second < 50)){
                fprintf(opts->sresar_stderr, "scatter_second %d is not correct.\n", opts->scatter_second);
                exit(150);
            }
        }else if(!strcmp(i_key, "load5s_flag")){
            if(toml_rtob(i_value, &i_bool)){
                fprintf(opts->sresar_stderr, "load5s_flag %s is not correct.\n", i_value);
                exit(150);
            }
            opts->load5s_flag = i_bool;
        }else if(!strcmp(i_key, "proc_flag")){
            if(toml_rtob(i_value, &i_bool)){
                fprintf(opts->sresar_stderr, "proc_flag %s is not correct.\n", i_value);
                exit(150);
            }
            opts->proc_flag = i_bool;
        }else if(!strcmp(i_key, "sys_flag")){
            if(toml_rtob(i_value, &i_bool)){
                fprintf(opts->sresar_stderr, "sys_flag %s is not correct.\n", i_value);
                exit(150);
            }
            opts->sys_flag = i_bool;
        }else{
            return 0;                                                  // unknown section/name, error
        }
    }

    // parse ssar.conf [load] part
    toml_table_t* load_conf = toml_table_in(root_conf, "load");
    if(!load_conf){
        printf("load is empty\n");
        goto release1003;
    }

    i_key = 0;
    i_value = 0;
    i_num = 0;
    i_str = 0;
    i_bool = false; 
    for(i = 0; ; i += 1){
        i_key = toml_key_in(load_conf, i);
        if(!i_key){
            break;
        }
        i_value = toml_raw_in(load_conf, i_key);
        if(!i_value){
            i_value = "";
        }

        if(!opts->sresar_stderr){
            exit(150);
        }

        if(!strcmp(i_key, "load1m_threshold")){
            if(toml_rtod(i_value, &(opts->load1m_threshold))){
                fprintf(opts->sresar_stderr, "load1m_threshold %s is not correct.\n", i_value);
                exit(150);
            }
        }else if(!strcmp(i_key, "load5s_threshold")){
            if(toml_rtod(i_value, &(opts->load5s_threshold))){
                fprintf(opts->sresar_stderr, "load5s_threshold %s is not correct.\n", i_value);
                exit(150);
            }
        }else if(!strcmp(i_key, "load_collect_interval")){
            if(toml_rtoi(i_value, &i_num)){
                fprintf(opts->sresar_stderr, "load_collect_interval %s is not correct.\n", i_value);
                exit(150);
            }
            opts->load_collect_interval = i_num;
        }else if(!strcmp(i_key, "realtime_priority")){
            if(toml_rtoi(i_value, &i_num)){
                fprintf(opts->sresar_stderr, "realtime_priority %s is not correct.\n", i_value);
                exit(150);
            }
            opts->realtime_priority = i_num;
            if(!(0 <= opts->realtime_priority && opts->realtime_priority < 100)){
                fprintf(opts->sresar_stderr, "realtime_priority %d is not correct.\n", opts->realtime_priority);
                exit(150);
            }
        }else if(!strcmp(i_key, "stack_sample_disable")){
            if(toml_rtob(i_value, &i_bool)){
                fprintf(opts->sresar_stderr, "stack_sample_disable %s is not correct.\n", i_value);
                exit(150);
            }
            opts->stack_sample_disable = i_bool;
        }
    }

    // parse ssar.conf [proc] part
    toml_table_t* proc_conf = toml_table_in(root_conf, "proc");
    if(!proc_conf){
        goto release1003;
    }

    i_key = 0;
    i_value = 0;
    i_num = 0;
    i_str = 0;
    i_bool = false; 
    for(i = 0; ; i += 1){
        i_key = toml_key_in(proc_conf, i);
        if(!i_key){
            break;
        }
        i_value = toml_raw_in(proc_conf, i_key);
        if(!i_value){
            i_value = "";
        }

        if(!opts->sresar_stderr){
            exit(150);
        }

        if(!strcmp(i_key, "proc_gzip_disable")){
            if(toml_rtob(i_value, &i_bool)){
                fprintf(opts->sresar_stderr, "proc_gzip_disable %s is not correct.\n", i_value);
                exit(150);
            }
            opts->proc_gzip_disable = i_bool;
        }
    }

release1003:
    toml_free(root_conf); 
release1002:
    root_conf = NULL;
    if (root_fp) {
        fclose(root_fp);
        root_fp = NULL;
    }
release1001:
    root_fp = NULL;

    return 1;
}

int init_sys_config(seq_options* opts){
    int strncpy_length;
    struct stat sb;

    FILE* root_fp = fopen("/etc/ssar/sys.conf", "rb");
    if(!root_fp){
        return -1;
    }

    enum{err_buf_size=0x10,};
    char errbuf[err_buf_size];
    toml_table_t* root_conf = toml_parse_file(root_fp, errbuf, err_buf_size);
    if(!root_conf){
        printf("file is empty or err is %s\n", errbuf);
        return -1;
    }

    toml_table_t* file_conf = toml_table_in(root_conf, "file");
    if(!file_conf){
        return -1;
    }

    char* j_str = 0;
    int is_collect = 0;

    int64_t i_num = 0;
    char*   i_str = 0;

    int i;
    int j;
    int i_collect = 0;
    int d_collect = 0;
    const char* i_key = 0;
    toml_array_t* i_value = 0;
    collect it_collect;
    collect special_collects[COLLECT_SIZE] = {0,};
    collect default_collects[COLLECT_SIZE] = {0,};
    for(i = 0; ; i++){
        i_key = toml_key_in(file_conf, i);
        if(!i_key){
            break;
        }
        i_value = toml_array_in(file_conf, i_key);
        if(!i_value){
            continue;
        }

        for(j = 0; ; j++){
            toml_table_t* j_table = toml_table_at(i_value, j);      // j_table is a line conf of collect file.
            if(!j_table){
                break;
            }

            const char* i_src_path = toml_raw_in(j_table, "src_path");
            if(i_src_path){
                if(toml_rtos(i_src_path, &j_str)){
                    continue;
                }
                if(strchr(j_str, '/')){
                    strcpy(it_collect.src_path, j_str);
                }else{
                    strcpy(it_collect.src_path, "/proc/");
                    strcat(it_collect.src_path, j_str);
                }
            }else{
                continue;
            }

            const char* i_cfile = toml_raw_in(j_table, "cfile");
            if(i_cfile){
                if(toml_rtos(i_cfile, &j_str)){
                    j_str = strrchr2(it_collect.src_path, '/');
                    j_str++;
                }
            }else{
                j_str = strrchr2(it_collect.src_path, '/');
                j_str++;
            }
            strcpy(it_collect.cfile, j_str);

            it_collect.turn = 0;
            const char* i_turn = toml_raw_in(j_table, "turn");
            if(i_turn){
                if(!toml_rtob(i_turn, &is_collect)){
                    it_collect.turn = is_collect;
                }
            }

            it_collect.gzip = 0;
            const char* i_gzip = toml_raw_in(j_table, "gzip");
            if(i_gzip){
                if(!toml_rtob(i_gzip, &is_collect)){
                    it_collect.gzip = is_collect;
                }
            }

            strcpy(it_collect.uname, i_key);

            if(!strcmp(i_key, "default")){
                default_collects[d_collect] = it_collect;
                d_collect++;
            }else{
                special_collects[i_collect] = it_collect;
                i_collect++;
            }
        }
    }
    toml_free(root_conf); 
    root_conf = NULL;
    if (root_fp) {
        fclose(root_fp);
        root_fp = NULL;
    }

    for(i = 0; i < COLLECT_SIZE; i++){
        if(*(special_collects[i].src_path) == '\0'){
            break;
        }
        for(j = i + 1; j < COLLECT_SIZE ; j++){
            if(*(special_collects[j].src_path) == '\0'){
                break;
            }
            if(strcmp(special_collects[i].uname, special_collects[j].uname) < 0){
                it_collect = special_collects[i];
                special_collects[i] = special_collects[j];
                special_collects[j] = it_collect;
            }
        }
    }

    struct utsname sysinfo;
    if(uname(&sysinfo) == -1){
        perror("uname() failed.");
        exit(150);
    }

    char * uname_pos;
    collect filter_collects[COLLECT_SIZE] = {0,};
    j = 0;
    for(i = 0; i < COLLECT_SIZE; i++){
        if(*(special_collects[i].src_path) == '\0'){
            break;
        }
        uname_pos = strstr(sysinfo.release, special_collects[i].uname);
        if(uname_pos == NULL || uname_pos != sysinfo.release){
            continue;
        }
        if(!find_collect(filter_collects, special_collects[i].src_path)){
            filter_collects[j] = special_collects[i];
            j++;
        }
    }

    for(i = 0; i < COLLECT_SIZE; i++){
        if(*(default_collects[i].src_path) == '\0'){
            break;
        }
        if(!find_collect(filter_collects, default_collects[i].src_path)){
            filter_collects[j] = default_collects[i];
            j++;
        }
    }

    j = 0;
    for(i = 0; i < COLLECT_SIZE; i++){
        if(*(filter_collects[i].src_path) == '\0'){
            break;
        }
        if(!filter_collects[i].turn){
            continue;
        }

        if(!opts->sresar_stderr){
            continue;
        }

        if(stat(filter_collects[i].src_path, &sb) == -1){
            fprintf(opts->sresar_stderr, "ERROR: file %s is not exist.\n", filter_collects[i].src_path); 
            continue;
        }
        if(!S_ISREG(sb.st_mode)){
            fprintf(opts->sresar_stderr, "ERROR: file %s is not a readable file.\n", filter_collects[i].src_path); 
            continue;
        }

        strcpy(opts->sys_srcs[j], filter_collects[i].src_path);
        strcpy(opts->sys_targets[j], filter_collects[i].cfile);
        opts->sys_gzips[j] = filter_collects[i].gzip;
        j++;
    }

    return 1;
}

void init_env(seq_options *opts){
    char hour_datetime[80];
    struct stat sb;

    if(geteuid()){
        printf("Root permission is required to start the sresar process.\n");
        exit(1);
    }
    int priority = -20;
    if(setpriority(PRIO_PROCESS, 0, priority) == -1){
        printf("setpriority error.\n");
    }

    if(opts->scatter_second == -1){
        struct timespec nanos;
        clock_gettime(CLOCK_MONOTONIC, &nanos);
        srand(nanos.tv_nsec);
        opts->scatter_second = rand()%50;
    }

    opts->nprocessors = sysconf(_SC_NPROCESSORS_ONLN);
    opts->load5s_baseline = (int)((double)(opts->nprocessors) * opts->load5s_threshold);
    opts->load1m_baseline = (int)((double)(opts->nprocessors) * opts->load1m_threshold);
    opts->main_mtid = pthread_self();

    sprintf(opts->ssar_path, "%s/sre_proc/", opts->work_path);
    if(0 == stat(opts->ssar_path, &sb)){
        if(!S_ISDIR(sb.st_mode)){
            printf("%s path is not dir.\n", opts->ssar_path);
            exit(1);
        }
    }else{
        if(mkdir(opts->ssar_path, 0755) == -1){
            printf("mkdir error %s\n", strerror_r(errno,ebuf,BUF_SIZE));
            exit(1);
        }
    }

    sprintf(opts->log_path, "%slog/", opts->ssar_path);
    if(0 == stat(opts->log_path, &sb)){
        if(!S_ISDIR(sb.st_mode)){
            printf("%s path is not dir.\n", opts->log_path);
            exit(1);
        }
    }else{
        if(mkdir(opts->log_path, 0755) == -1){
            printf("mkdir error %s\n", strerror_r(errno,ebuf,BUF_SIZE));
            exit(1);
        }
    }

    sprintf(opts->data_path, "%sdata/", opts->ssar_path);
    if(0 == stat(opts->data_path, &sb)){
        if(!S_ISDIR(sb.st_mode)){
            printf("%s path is not dir.\n", opts->data_path);
            exit(1);
        }
    }else{
        if(mkdir(opts->data_path, 0755) == -1){
            printf("mkdir error %s\n", strerror_r(errno,ebuf,BUF_SIZE));
            exit(1);
        }
    }
    
    int current_hour_timestamp = get_current_hour_datetime(hour_datetime, hour_format);
    sprintf(opts->log_file, "%s%s_sresar.log", opts->log_path, hour_datetime);
    if((opts->sresar_stderr = fopen(opts->log_file, "a")) == NULL){
        perror("fopen log file fail\n");
        exit(1);
    }

    init_pid_lock(opts);

    THREAD_INFO("sresar starting, scatter_second is %d", opts->scatter_second);

    sprintf(opts->data_path_hour, "%s%s/", opts->data_path, hour_datetime);
    if(0 == stat(opts->data_path_hour, &sb)){
        if(!S_ISDIR(sb.st_mode)){
            printf("%s path is not dir.\n", opts->data_path_hour);
            exit(1);
        }
    }else{
        if(mkdir(opts->data_path_hour, 0755) == -1){
            THREAD_ERROR("mkdir data_path_hour error.");
            exit(1);
        }
    }
    opts->hour_timestamp_lock = get_current_hour_timestamp();

    sprintf(opts->load5s_file, "%s%s_load5s", opts->data_path_hour, hour_datetime);
    if((opts->load5s_fd = open(opts->load5s_file, O_RDWR | O_APPEND | O_CREAT, 0644)) == -1){
        THREAD_ERROR("open load5s file fail.");
        exit(1);
    }

    if(check_partition_usage(opts)){
        THREAD_INFO("partition usage insufficient in sresar start.");
    }

    fflush(opts->sresar_stderr);
}

void release_env(seq_options *opts){
    pid_unlock(opts);

    if (opts->sresar_stderr) {
        fclose(opts->sresar_stderr);
        opts->sresar_stderr = NULL;
    }
    close(opts->load5s_fd);
}

/***********************************************************************************
 **                                                                                *
 **                               main part                                        * 
 **                                                                                *
 ***********************************************************************************/

int main(int argc, char* argv[]){
    seq_options it_option = {
	.src = SRC_USER,                                           // src
	.load5s_threshold = 5,                                     // load5s/corenum
	.load1m_threshold = 5,                                     // load1m/corenum
	.duration_threshold = 72,                                  // duration_threshold (hours)
	.inode_use_threshold = 70,                                 // inode use (percent)
	.disk_use_threshold = 70,                                  // disk use (percent)
	.disk_available_threshold = 25000,                         // disk avail (MB)
	.duration_restart = 1,                                     // last time (hour)
	.work_path = "/var/log",                                   // work_path
	.sys_srcs = {{0,},},                                       // /proc/file
	.sys_targets = {{0,},},                                    // save file name
	.sys_gzips = {0,},                                         // if open gzip
        .scatter_second = -1,                                      // scatter_second
        .sresar_stderr = stderr,                                   // default log_fd
        .log_file = "",                                            // init logfile content.
        .load_collect_interval = 100,                              // load1 collect interval (ms)
        .proc_gzip_disable = false,                                // if true, will disable process-info gzip
        .stack_sample_disable = false,                             // if true, will collect all D state task stack
        .realtime_priority = 0,                                    // load1 and loadrd threads realtime priority
        .load5s_flag = true,                                       // true will enable load5s collect, false disble.
        .proc_flag = true,                                         // true will enable procs collect,  false disble.
        .sys_flag = true,                                          // true will enable sys collect,    false disble.
    };
    seq_options* opts = &it_option;

    if(init_basic_config(opts) < 0){
        printf("can't load '/etc/ssar/ssar.conf.");
    }
    if(init_sys_config(opts) < 0){
        printf("can't load '/etc/ssar/sys.conf.");
    }
    init_option(opts, argc, argv);
    
    if(opts->src == SRC_DAEMON){
        init_env(opts);

        if((opts->eventfd_load1 = eventfd(0, EFD_SEMAPHORE | EFD_CLOEXEC)) < 0){
            THREAD_ERROR("create eventfd_load1 failed.");
            goto release002;
        }
        if((opts->eventfd_cmdinfo = eventfd(0, EFD_SEMAPHORE | EFD_CLOEXEC)) < 0){
            THREAD_ERROR("create eventfd_cmdinfo failed.");
            goto release003;
        }
        if((opts->eventfd_thread = eventfd(0, EFD_SEMAPHORE | EFD_CLOEXEC)) < 0){
            THREAD_ERROR("create eventfd_thread failed.");
            goto release004;
        }

        sigset_t block_set;
        sigfillset(&block_set);
        if(sigprocmask(SIG_BLOCK, &block_set, NULL) == -1){                       // Block all signals
            THREAD_ERROR("sigprocmask failed.");
            goto release004;
        }
        int signalfd_fd;
        if((signalfd_fd = signalfd(-1, &block_set, 0)) == -1){
            THREAD_ERROR("create signalfd_fd failed.");
            goto release005;
        }

        pthread_t sresar_mtid=0, cmdinfo_mtid=0, load1_mtid=0, loadrd_mtid=0, snapsys_mtid=0, rotate_mtid=0;
        pthread_attr_t it_attr;
        pthread_attr_t *it_attr_p = &it_attr;
        int rt_runtime_fd = open("/sys/fs/cgroup/cpu/cpu.rt_runtime_us", O_RDONLY | O_NONBLOCK);
        if(rt_runtime_fd == -1){
            THREAD_ERROR("open() rt_runtime_fd error");
            it_attr_p = NULL;
            goto create_thread;
        }
        char rt_runtime_buf[64];
        int rt_runtime_num;
        if((rt_runtime_num = read(rt_runtime_fd, rt_runtime_buf, sizeof(rt_runtime_buf) - 1)) < 0){
            THREAD_INFO("read() rt_runtime error");
            it_attr_p = NULL;
            goto create_thread;
        }
        rt_runtime_buf[rt_runtime_num] = '\0'; 
        if(atol(rt_runtime_buf) && opts->realtime_priority > 0){
            if(pthread_attr_init(&it_attr) != 0){
                THREAD_ERROR("pthread_attr_init failed.");
                goto release005;
            }
            int it_inherit;
            if(pthread_attr_getinheritsched(&it_attr, &it_inherit) != 0){
                THREAD_ERROR("pthread_attr_getinheritsched failed.");
                goto release005;
            }
            if(it_inherit == PTHREAD_INHERIT_SCHED){
                it_inherit = PTHREAD_EXPLICIT_SCHED;
                THREAD_INFO("set inherit sched to PTHREAD_EXPLICIT_SCHED.");
                if(pthread_attr_setinheritsched(&it_attr, it_inherit) != 0){
                    THREAD_ERROR("pthread_attr_setinheritsched failed.");
                    goto release005;
                }
            }
            if(pthread_attr_setschedpolicy(&it_attr, SCHED_RR) != 0){
                THREAD_ERROR("pthread_attr_setschedpolicy failed.");
                goto release005;
            }
            struct sched_param it_param;
            memset(&it_param, 0, sizeof(it_param));
            it_param.sched_priority = opts->realtime_priority;
            if(pthread_attr_setschedparam(&it_attr, &it_param) != 0){
                THREAD_ERROR("pthread_attr_setschedparam failed.");
                goto release005;
            }
        }else{
            it_attr_p = NULL;
        }

create_thread:
        if(opts->load5s_flag && pthread_create(&loadrd_mtid, it_attr_p, loadrd_thread, (void *)opts) != 0){
            THREAD_INFO("Cannot create real_time loadrd_thread.");
            if(pthread_create(&loadrd_mtid, NULL, loadrd_thread, (void *)opts) != 0){
                THREAD_INFO("Cannot create loadrd_thread.");
            }
        }
        if(opts->load5s_flag && pthread_create(&load1_mtid, NULL, load1_thread, (void *)opts) != 0){
            THREAD_INFO("Cannot create load1_thread");
        }
        if(opts->proc_flag && pthread_create(&sresar_mtid, NULL, sresar_thread, (void *)opts) != 0){
            THREAD_INFO("Cannot create sresar_thread.");
        }
        if(opts->proc_flag && pthread_create(&cmdinfo_mtid, NULL, cmdinfo_thread, (void *)opts) != 0){
            THREAD_INFO("Cannot create cmdinfo_thread.");
        }
        if(opts->sys_flag && pthread_create(&snapsys_mtid, NULL, snapsys_thread, (void *)opts) != 0){
            THREAD_INFO("Cannot create snapsys_thread.");
        }
        if(pthread_create(&rotate_mtid, NULL, rotate_thread, (void *)opts) != 0){
            THREAD_INFO("Cannot create rotate_thread.");
        }

        alarm(60 * 60 * opts->duration_restart);

        struct signalfd_siginfo signalfd_info;
        ssize_t exp_size;
        uint64_t sem_count = 1;
        int write_ret;
        for(;;){
            exp_size = read(signalfd_fd, &signalfd_info, sizeof(struct signalfd_siginfo));
            if(exp_size != sizeof(struct signalfd_siginfo)){
                THREAD_INFO("read signalfd_fd error.");
            }
            if(signalfd_info.ssi_signo == SIGALRM ||signalfd_info.ssi_signo == SIGINT ||signalfd_info.ssi_signo == SIGQUIT 
		||signalfd_info.ssi_signo == SIGTERM ||signalfd_info.ssi_signo == SIGHUP ||signalfd_info.ssi_signo == SIGUSR1){
                THREAD_INFO("got SIGNAL %d", signalfd_info.ssi_signo);
                write_ret = write(opts->eventfd_thread, &sem_count, sizeof(sem_count));
                if(write_ret < 0){
                    THREAD_INFO("write eventfd_thread fail.");
                    if(load1_mtid){
                        pthread_cancel(load1_mtid);
                    }
                    if(loadrd_mtid){
                        pthread_cancel(loadrd_mtid);
                    }
                    if(sresar_mtid){
                        pthread_cancel(sresar_mtid);
                    }
                    if(cmdinfo_mtid){
                        pthread_cancel(cmdinfo_mtid);
                    }
                    if(snapsys_mtid){
                        pthread_cancel(snapsys_mtid);
                    }
                    if(rotate_mtid){
                        pthread_cancel(rotate_mtid);
                    }
                }else{
                    THREAD_INFO("write eventfd_thread success.");
                }
                break;
            }
        }
        void *sresar_retval, *cmdinfo_retval, *load1_retval, *loadrd_retval, *snapsys_retval, *rotate_retval;
        if(load1_mtid && pthread_join(load1_mtid, &load1_retval) != 0){
            THREAD_INFO("Cannot join load1_thread");
        }
        if(sresar_mtid && pthread_join(sresar_mtid, &sresar_retval) != 0){
            THREAD_INFO("Cannot join sresar_thread");
        }
        if(cmdinfo_mtid && pthread_join(cmdinfo_mtid, &cmdinfo_retval) != 0){
            THREAD_INFO("Cannot join sresar_thread");
        }
        if(snapsys_mtid && pthread_join(snapsys_mtid, &snapsys_retval) != 0){
            THREAD_INFO("Cannot join snapsys_thread");
        }
        if(loadrd_mtid && pthread_join(loadrd_mtid, &loadrd_retval) != 0){
            THREAD_INFO("Cannot join loadrd_thread");
        }
        if(rotate_mtid && pthread_join(rotate_mtid, &rotate_retval) != 0){
            THREAD_INFO("Cannot join rotate_thread");
        }

release005:
        close(signalfd_fd);
release004:
        close(opts->eventfd_thread);
release003:
        close(opts->eventfd_cmdinfo);
release002:
        close(opts->eventfd_load1);
release001:
        release_env(opts);
    }else if(opts->src == SRC_LOADRD || opts->src == SRC_STACK){
        struct utlarr_i uarr;
        uarr.len  = D_STATE_NUM;
        uarr.arr  = (int*)malloc(uarr.len * sizeof(int));
        if(uarr.arr == NULL){
            THREAD_ERROR("memory uarr allocated error.");
            goto release703;
        }

        utlbuf_s ubuf;
        ubuf.len  = GROWWIDTH;
        ubuf.buf  = (char*)malloc(ubuf.len * sizeof(char*));
        if(ubuf.buf == NULL){
            THREAD_ERROR("memory ubuf allocated error.");
            goto release704;
        }

        utlbuf_s ufbuf;
        ufbuf.len = GROWWIDTH;
        ufbuf.buf = (char*)malloc(ufbuf.len * sizeof(char*));
        if(ufbuf.buf == NULL){
            THREAD_ERROR("memory ufbuf allocated error.");
            goto release705;
        }

        loadrd(opts, &uarr, &ubuf, &ufbuf);

release705:
        free(ufbuf.buf);
        ufbuf.buf = NULL;
release704:
        free(ubuf.buf);
        ubuf.buf = NULL;
release703:
        free(uarr.arr);
        uarr.arr = NULL;
    }else if(opts->src == SRC_USER){
        sresar_live(opts);
    }else{
        fprintf(stderr, "ERROR: src is not identified.\n");
    }

    return 0;
}
