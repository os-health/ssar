#ifndef SRESAR_READPROC_H
#define SRESAR_READPROC_H

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include "utils.h"

#define PROCPATHLEN    512                            // must hold /proc/2000222000/task/2000222000/cmdline
#define GROWWIDTH      512
#define WHITESPACE     " \t\n\r"


#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)

typedef struct proc_t {
    int                ptype;                     // (special)       1 user process, 2 kernel process
    char               state;                     // stat,status     single-char code for process state
    int                tid;                       // (special)       task id, the POSIX thread ID
    int	               ppid;                      // stat,status     pid of parent process
    int                pgid;                      // stat            process group id
    int                sid;                       // stat            session id
    int                nlwp;                      // stat,status     number of threads
    int                tgid;                      // (special)       thread group ID, the POSIX PID
    int                tpgid;                     // stat            terminal process group id
    int                processor;                 // stat            current CPU number
    long               prio;                      // stat            kernel scheduling priority
    long               nice;                      // stat            standard unix nice level of process
    long               rss;                       // stat            identical to 'resident'
    long               size;                      // statm           total virtual memory
    long               share;                     // statm           shared (mmap'd) memory
    unsigned long      rtprio;                    // stat            real-time priority
    unsigned long      sched;                     // stat            scheduling class
    unsigned long      flags;                     // stat            kernel flags for the process
    unsigned long long utime;                     // stat            user-mode CPU time accumulated by process
    unsigned long long stime;                     // stat            kernel-mode CPU time accumulated by process
    unsigned long long start_time;                // stat            start time of process -- seconds since 1970-01-01T00:00:00
    unsigned long      min_flt;                   // stat            number of minor page faults since process start
    unsigned long      maj_flt;                   // stat            number of major page faults since process start
    unsigned long      nvcsw;                     // status          voluntary  context switches
    unsigned long      nivcsw;                    // status          nvoluntary context switches
    char               cmd[16];                   // stat,status     basename of executable file in call to exec(2)
    int                tty;                       // stat            full device number of controlling terminal
    long               alarm;                     // stat            
    long               resident;                  // statm           resident non-swapped memory
    long               trs;                       // statm           text (exe) resident set
    long               lrs;                       // statm           library resident set
    long               drs;                       // statm           data+stack resident set
    long               dt;                        // statm           dirty pages
    unsigned long      vsize;                     // stat            number of pages of virtual memory
    unsigned long      rss_rlim;                  // stat            resident set size limit
    unsigned long      cmin_flt;                  // stat            cumulative min_flt of process and child processes
    unsigned long      cmaj_flt;                  // stat            cumulative maj_flt of process and child processes
    unsigned long      vm_size;                   // status          equals size, as kb
    unsigned long      vm_lock;                   // status          locked pages, as kb
    unsigned long      vm_rss;                    // status          equals rss and/or resident, as kb
    unsigned long      vm_data;                   // status          data only size, as kb
    unsigned long      vm_stack;                  // status          stack only size, as kb
    unsigned long      vm_swap;                   // status          swap ents, as kb
    unsigned long      vm_exe;                    // status          equals trs, as kb
    unsigned long      vm_lib;                    // status          total, not just used, library pages
    unsigned long long cutime;                    // stat            cumulative utime of process and reaped children
    unsigned long long cstime;                    // stat            cumulative stime of process and reaped children
} proc_t;

typedef struct cmdinfo_t {
    int                ptype;                     // (special)       1 user process, 2 kernel process
    char               state;                     // stat,status     single-char code for process state
    int                tgid;                      // (special)       thread group ID, the POSIX PID
    char               cmd[16];                   // stat,status     basename of executable file in call to exec(2)
} cmdinfo_t;

typedef struct task_t {
    char               state;
    int                tid;
    int                ppid;
    int                pgid;
    int                sid;
    int                nlwp;
    int                tgid;
    int                processor;
    long               prio;
    long               nice;
    unsigned long      rtprio;
    unsigned long      sched;
    unsigned long      flags;
    char               cmd[16];
    unsigned long long utime;
    unsigned long long stime;
    unsigned long long cutime;
    unsigned long long cstime;
    unsigned long long start_time;
} task_t;

typedef struct PROCTAB {
    DIR*	procfs;
    DIR*	taskdir;                          // for threads
    pid_t	taskdir_user;                     // for threads
    pid_t	task_owner;                       // the current process pid.
    char        path[PROCPATHLEN];                // must hold /proc/2000222000/task/2000222000/cmdline
    int       (*finder_proc    )(struct PROCTAB *__restrict const, proc_t    *__restrict const);
    int       (*finder_task    )(struct PROCTAB *__restrict const, proc_t    *__restrict const, const pid_t, char *__restrict const);
    int       (*finder_cmdline )(struct PROCTAB *__restrict const, cmdinfo_t *__restrict const);
    proc_t*   (*reader_proc    )(struct PROCTAB *__restrict const, proc_t    *__restrict const, int cmd_flag, utlbuf_s *ufbuf);
    proc_t*   (*reader_task    )(struct PROCTAB *__restrict const, proc_t    *__restrict const, const pid_t, char *__restrict const);
    cmdinfo_t*(*reader_cmdline )(struct PROCTAB *__restrict const, cmdinfo_t *__restrict const, utlbuf_s *ufbuf);
}PROCTAB;

// init a PROCTAB structure
extern PROCTAB*      openproc(PROCTAB* PT);
extern PROCTAB*      mini_openproc(PROCTAB* PT);

// clean open files, etc from the openproc()
extern void          closeproc(PROCTAB* PT);
extern void          mini_closeproc(PROCTAB* PT);

extern proc_t*       read_proc(     PROCTAB *__restrict const PT,    proc_t *__restrict p, int cmd_flag, utlbuf_s *ubuf);
extern pid_t         mini_read_proc(PROCTAB *__restrict const PT                                                       );
extern cmdinfo_t*    read_cmdline(  PROCTAB *__restrict const PT, cmdinfo_t *__restrict c,               utlbuf_s *ubuf);
extern proc_t*       read_task(     PROCTAB *__restrict const PT,    proc_t *__restrict t, const pid_t p               );


extern char         *read_stack_file(unsigned tid, utlbuf_s *ubuf, utlbuf_s *ufbuf, char* nwchan);
extern int           get_uptime(void);
extern unsigned long get_btime(void);

#endif
