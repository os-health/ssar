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

#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

#include "utils.h"

#define _unlikely_(x) (__builtin_expect(!!(x), 0))


static void xxdefault_error(const char *restrict fmts, ...) __attribute__((format(printf,1,2)));
static void xxdefault_error(const char *restrict fmts, ...) {
    va_list va;

    va_start(va, fmts);
    fprintf(stderr, fmts, va);
    va_end(va);
}

message_fn xxalloc_err_handler = xxdefault_error;


void *xxcalloc(unsigned int size) {
    void * p;

    if (size == 0){
        ++size;
    }
    p = calloc(1, size);
    if (!p) {
        xxalloc_err_handler("%s failed to allocate %u bytes of memory", __func__, size);
        exit(EXIT_FAILURE);
    }
    return p;
}

void *xxmalloc(size_t size) {
    void *p;

    if (size == 0){
        ++size;
    }
    p = malloc(size);
    if (!p) {
        xxalloc_err_handler("%s failed to allocate %zu bytes of memory", __func__, size);
        exit(EXIT_FAILURE);
    }
    return(p);
}

void *xxrealloc(void *oldp, unsigned int size) {
    void *p;

    if (size == 0){
        ++size;
    }
    p = realloc(oldp, size);
    if (!p) {
        xxalloc_err_handler("%s failed to allocate %u bytes of memory", __func__, size);
        exit(EXIT_FAILURE);
    }
    return(p);
}


void randomlize(int *arr, int n){
    int i;
    int j;
    int tmp;
    struct timespec nanos;
    clock_gettime(CLOCK_MONOTONIC, &nanos);
    srand(nanos.tv_nsec);
    for(i = 0; i < n; i++){        
        j = rand() % (n-i) + i;                             //rand_r(); 
        tmp    = arr[i];        
        arr[i] = arr[j];        
        arr[j] = tmp;        
    }    
}

void fmt_datetime(time_t timestamp, char *it_datetime, const char *format){
    struct tm tm;
    time_t tt = timestamp;
    localtime_r(&tt, &tm);
    strftime(it_datetime, 80, format, &tm);                 // %Y-%m-%dT%H:%M:%S
}

int remove_directory(const char *path){
    struct stat statbuf;
    if(!stat(path, &statbuf)){
        if(!S_ISDIR(statbuf.st_mode)){
            return unlink(path);
        }
    }else{
        return -1;
    }

    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if(d){
        struct dirent *p;
        r = 0;

        while(!r && (p=readdir(d))){
            int r2 = -1;
            char *buf;
            size_t len;

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if(!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")){
                continue;
            }
            len = path_len + strlen(p->d_name) + 2; 
            buf = malloc(len);

            if(buf){
                snprintf(buf, len, "%s/%s", path, p->d_name);
                if(!stat(buf, &statbuf)){
                    if(S_ISDIR(statbuf.st_mode)){
                        r2 = remove_directory(buf);
                    }else{
                        r2 = unlink(buf);
                    }
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }
    if(!r){
        r = rmdir(path);
    }

    return r;
}

char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str, find);
    while(current_pos){
        *current_pos = replace;
        current_pos = strchr(current_pos, find);
    }
    return str;
}

int find_char(char **strings, char *value){
    char *string;                                // 字符串循环变量
    while((string = *strings++) !=  NULL){
        if(!strcmp(string, value)){
            return 1;
        }
    }

    return 0;
}

char *strrchr2(register const char *s, int c){
  char *rtnval = 0;

  do {
    if (*s == c)
      rtnval = (char*) s;
  } while (*s++);
  return (rtnval);
}

