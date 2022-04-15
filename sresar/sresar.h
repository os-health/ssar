#ifndef SRESAR_SRESAR_H
#define SRESAR_SRESAR_H

#define SRC_USER   10001
#define SRC_DAEMON 10002
#define SRC_LOADRD 10003
#define SRC_STACK  10004

#define COLLECT_SIZE 256
#define FILE_SIZE    32
#define UNAME_SIZE   32
#define WORK_PATH_SIZE 128

typedef struct{
    int src;
    double load5s_threshold;
    double load1m_threshold;
    int load5s_baseline;
    int load1m_baseline;
    int duration_threshold;
    unsigned long inode_use_threshold;
    unsigned long disk_use_threshold;
    unsigned long disk_available_threshold;
    int duration_restart;
    int scatter_second;
    int realtime_priority;
    int load_collect_interval;
    bool stack_sample_disable;
    bool proc_gzip_disable;
    bool load5s_flag;
    bool proc_flag;
    bool sys_flag;
    char work_path[WORK_PATH_SIZE];
    char sys_srcs[COLLECT_SIZE][8 * FILE_SIZE];
    char sys_targets[COLLECT_SIZE][FILE_SIZE];
    int sys_gzips[COLLECT_SIZE];
    int pid_fd;
    int nprocessors;
    char ssar_path[2 * WORK_PATH_SIZE];
    char log_path[2 * WORK_PATH_SIZE];
    char data_path[2 * WORK_PATH_SIZE];
    char data_path_hour[2 * WORK_PATH_SIZE];
    char log_file[2 * WORK_PATH_SIZE];
    char load5s_file[2 * WORK_PATH_SIZE];
    FILE *sresar_stderr;
    int load5s_fd;
    int eventfd_load1;
    int eventfd_cmdinfo;
    int eventfd_thread;
    int return_value;
    pthread_t main_mtid;
    volatile int partition_insufficient;
    volatile int hour_lock;
    volatile int hour_timestamp_lock;
}seq_options;

typedef struct{
    int  turn;
    int  gzip;
    char src_path[8 * FILE_SIZE];
    char cfile[FILE_SIZE];
    char uname[UNAME_SIZE];
} collect;

#endif
