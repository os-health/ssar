#ifndef SRESAR_UTILS_H
#define SRESAR_UTILS_H

#define MALLOC __attribute__ ((__malloc__))

// Utility buffer 
typedef struct utlbuf_s {
    int   len;
    char *buf;
} utlbuf_s;

// Utility Arrry
typedef struct utlarr_i {
    int  len;
    int *arr;
} utlarr_i;

extern int find_char(char **strings, char *value);
extern int remove_directory(const char *path);
extern void randomlize(int *arr, int n);
extern void fmt_datetime(time_t timestamp, char *fmt_datetime, const char *format);
extern char* replace_char(char* str, char find, char replace);

typedef void (*message_fn)(const char *__restrict, ...) __attribute__((format(printf,1,2)));
extern message_fn xalloc_err_handler;
extern void *xxcalloc(unsigned int size) MALLOC;
extern void *xxmalloc(size_t size) MALLOC;
extern void *xxrealloc(void *oldp, unsigned int size) MALLOC;
extern char *strrchr2(register const char *s, int c);


#endif



