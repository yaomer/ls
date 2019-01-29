#ifndef _LS_H
#define _LS_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

#define FILE_ALL    00001    /* -a */
#define FILE_AAL    00002    /* -A */
#define FILE_TAG    00004    /* -F */
#define FILE_UND    00010    /* -h */
#define FILE_INO    00020    /* -i */
#define FILE_LON    00040    /* -l */
#define FILE_CMA    00100    /* -m */
#define FILE_NUM    00200    /* -n */
#define FILE_REV    00400    /* -r */
#define FILE_REC    01000    /* -R */
#define FILE_SIZ    02000    /* -S */
#define FILE_TIM    04000    /* -t */


#define GB      (1024 * 1024 * 1024)
#define MB      (1024 * 1024)
#define KB      (1024)

extern int option;
extern int fname_width;      
extern int fsize_width;      
extern int flink_width;      
extern int funame_width;     
extern int fgname_width; 
extern int fuid_width;       
extern int fgid_width;       
extern int finode_width;     

typedef struct {    /*  每个文件的所有信息  */
    struct stat     statbuf;   
    char            buf[FILENAME_MAX + 1];     /*  保存文件名  */
    char            path[PATH_MAX + 1];        /*  保存文件路径  */
} stat_t;

int     (*cmp)(const void *, const void *);
int     filecmp(const void *, const void *);
int     timecmp(const void *, const void *);
int     sizecmp(const void *, const void *);
void    fsize_trans(off_t size);

int     get_fn(char *pathname);
void    get_fname_width(stat_t *statfp[], int fn);
void    get_other_width(stat_t *statfp[], int fn);

void    deal_argfile(int argc, char **argv);
void    show_dir(char *pathname, int fn);
void    show_single_dir(stat_t *statfp[], int fn);
void    show_align(int ws_col, stat_t *statf);
void    show_attribute(stat_t *statf);

void    err_sys(const char *fmt, ...);
void    err_quit(const char *fmt, ...);
void    err_msg(const char *fmt, ...);

#endif    /*  _LS_H  */
