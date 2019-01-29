#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include "common.h"
#include "myls.h"

/*
 * 处理文件列表参数，并判断其是目录还是文件
 */
void
deal_argfile(int argc, char **argv)
{
    DIR             *dp;
    char            *dirbuf[argc];    /* 保存文件参数表中的目录名 */
    stat_t          *statfp[argc];    /* 保存文件参数表中的文件的详细信息 */
    int             dn, fn, i;

    dn = fn = 0;
    while (argc-- > 0)
        if ((dp = opendir(*argv)) == NULL) {   /*  可能是文件  */
            malloc_node(statfp[fn], stat_t);
            if (lstat(*argv, &statfp[fn]->statbuf) < 0)
                err_sys("lstat error");
            strcpy(statfp[fn++]->buf, *argv++);
        } else {
            closedir(dp);   /*  是目录  */
            malloc_str(dirbuf[dn++], *argv);
            argv++;
        }

    if (fn > 0) {     /*  将文件参数表中的所有文件一起打印出来  */
        show_single_dir(statfp, fn);
        if (dn > 0)
            putchar('\n');
        for (i = 0; i < fn; i++)
            free(statfp[i]);
    }

    for (i = 0; i < dn; i++)    /*  处理目录列表  */
        show_dir(dirbuf[i], get_fn(dirbuf[i]));
}

/*
 * 输出单个目录下的内容
 */
void
show_single_dir(stat_t *statfp[], int fn)
{
    struct winsize  size;     /*  终端窗口的大小  */

    if (isatty(1)) {     /*  输出至终端  */
        if (ioctl(1, TIOCGWINSZ, &size) < 0)    /*  获取终端窗口的大小  */
            err_sys("ioctl error");
        get_fname_width(statfp, fn);    /*  获得文件名的输出宽度  */
    }

    get_other_width(statfp, fn);     /*  获得除文件名以外的其他输出宽度  */

    cmp = ((option & FILE_SIZ) ? sizecmp : ((option & FILE_TIM) ? timecmp : filecmp));
    qsort(statfp, fn, sizeof(statfp[0]), cmp);     /*  根据可选参数对文件进行排序  */

    for (int i = 0; i < fn; i++)     /*  打印文件列表  */
        if (option & (FILE_LON | FILE_NUM))
            show_attribute(statfp[i]);
        else
            show_align(size.ws_col, statfp[i]);
    if (!(option & (FILE_LON | FILE_NUM)))
        putchar('\n');
}

/*
 * 将目录下的所有文件按列对齐输出
 * 如果输出到终端，则对齐输出；如果输出重定向到了一个文件，则一行输出一个
 */
void
show_align(int ws_col, stat_t *statf)
{
    static int row;

    if (fname_width) {
        if (option & FILE_INO) {    /*  文件名前加上inode编号  */
            if ((row += (fname_width + finode_width)) > ws_col) {
                putchar('\n');
                row = fname_width + finode_width + 1;
            } else
                row += 1;
        } else {
            if ((row += fname_width) > ws_col) {
                putchar('\n');
                row = fname_width + 1;    /*  文件间以空格隔开  */
            } else
                row += 1;
        }
    }

    if (option & FILE_INO)
        printf("%*llu ", finode_width, statf->statbuf.st_ino);

    if (option & FILE_CMA)
        printf("%s, ", statf->buf);
    else
        printf("%-*s ", fname_width, statf->buf);
}

/*
 * 打印文件的详细属性信息
 */
void
show_attribute(stat_t *statf)
{
    struct passwd   *usr;
    struct group    *grp;
    struct stat     stat;
    char            *timeptr;

    stat = statf->statbuf;
    /*  打印文件的inode编号  */
    if (option & FILE_INO)
        printf("%*llu ", finode_width, stat.st_ino);

    /*  打印文件的类型信息  */
    if (S_ISREG(stat.st_mode))
        printf("-");
    else if (S_ISDIR(stat.st_mode))
        printf("d");
    else if (S_ISLNK(stat.st_mode))
        printf("l");
    else if (S_ISCHR(stat.st_mode))
        printf("c");
    else if (S_ISBLK(stat.st_mode))
        printf("b");
    else if (S_ISFIFO(stat.st_mode))
        printf("f");
    else if (S_ISSOCK(stat.st_mode))
        printf("s");

    /*  打印文件的权限信息  */
    printf("%c", (stat.st_mode & S_IRUSR) ? 'r' : '-');
    printf("%c", (stat.st_mode & S_IWUSR) ? 'w' : '-');
    printf("%c", (stat.st_mode & S_IXUSR) ? 'x' : '-');
    printf("%c", (stat.st_mode & S_IRGRP) ? 'r' : '-');
    printf("%c", (stat.st_mode & S_IWGRP) ? 'w' : '-');
    printf("%c", (stat.st_mode & S_IXGRP) ? 'x' : '-');
    printf("%c", (stat.st_mode & S_IROTH) ? 'r' : '-');
    printf("%c", (stat.st_mode & S_IWOTH) ? 'w' : '-');
    printf("%c", (stat.st_mode & S_IXOTH) ? 'x' : '-');

    /*  打印文件的链接数  */
    printf("  %*d ", flink_width, stat.st_nlink);

    /*  打印文件的所有者和用户组  */
    if (option & FILE_NUM) {
        printf("%*d ", fuid_width, stat.st_uid);
        printf("%*d ", fgid_width, stat.st_gid);
    } else {
        if ((usr = getpwuid(stat.st_uid)) == NULL)
            err_sys("getpwuid error");
        if ((grp = getgrgid(stat.st_gid)) == NULL)
            err_sys("getgrgid error");
        printf("%-*s ", funame_width, usr->pw_name);
        printf("%-*s ", fgname_width, grp->gr_name);
    }

    /*  打印文件的大小  */
    if (option & FILE_UND)
        fsize_trans(stat.st_size);
    else
        printf("%*lld ", fsize_width, stat.st_size);

    /*  打印文件的时间  */
    if ((timeptr = ctime(&stat.st_mtime)) == NULL)
        err_sys("ctime error");
    if (timeptr[strlen(timeptr) - 1] == '\n')
        timeptr[strlen(timeptr) - 1] = '\0';
    printf("%s ", timeptr);

    printf("%-s\n", statf->buf);
}

/*  show函数：列出目录下的文件列表  */
void show_dir(char *pathname, int fn)
{
    DIR             *dp;
    struct dirent   *dirp;
    stat_t          *statfp[fn];
    int             i;

    for (i = 0; i < fn; i++)
        malloc_node(statfp[i], stat_t);

    if ((dp = opendir(pathname)) == NULL)
        err_sys("opendir error");
    for (i = 0; (dirp = readdir(dp)); i++) {
        if (!(option & FILE_ALL)) {
            if (option & FILE_AAL) {
                if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0) {
                    i--;
                    continue;
                }
            } else if (dirp->d_name[0] == '.') {
                i--;
                continue;
            }
        }
        /* 保存目录下每个文件的详细信息 */
        strcpy(statfp[i]->buf, dirp->d_name);
        strcpy(statfp[i]->path, pathname);
        strcat(statfp[i]->path, "/");
        strcat(statfp[i]->path, statfp[i]->buf);
        if (lstat(statfp[i]->path, &statfp[i]->statbuf) < 0)
            err_sys("lstat error");

        if (option & FILE_TAG) {
            if (S_ISDIR(statfp[i]->statbuf.st_mode))
                strcat(statfp[i]->buf, "/");
            if (S_ISLNK(statfp[i]->statbuf.st_mode))
                strcat(statfp[i]->buf, "@");
            if (S_ISFIFO(statfp[i]->statbuf.st_mode))
                strcat(statfp[i]->buf, "|");
            if (S_ISSOCK(statfp[i]->statbuf.st_mode))
                strcat(statfp[i]->buf, "=");
            if (S_ISREG(statfp[i]->statbuf.st_mode)) {
                if (statfp[i]->statbuf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
                    strcat(statfp[i]->buf, "*");
            }
        }
    }
    fn = i;   /* 更新fn */
    closedir(dp);

    /* 打印单个目录下的所有内容 */
    show_single_dir(statfp, fn);

    if (option & FILE_REC) {    /*  递归遍历目录  */
        for (i = 0; i < fn; i++)
            if (strcmp(statfp[i]->buf, ".") != 0
            && strcmp(statfp[i]->buf, "..") != 0
            && S_ISDIR(statfp[i]->statbuf.st_mode))
                show_dir(statfp[i]->path, get_fn(statfp[i]->path));
    }

    for (i = 0; i < fn; i++)
        free(statfp[i]);
}

/*
 * 以人类易懂的方式打印文件大小
 */
void
fsize_trans(off_t size)
{
    if (size / GB)
        printf("%6.1lfG ", 1.0 * size / GB);
    else if (size / MB)
        printf("%6.1lfM ", 1.0 * size / MB);
    else if (size / KB)
        printf("%6.1lfK ", 1.0 * size / KB);
    else
        printf("%6lldB ", size);
}
