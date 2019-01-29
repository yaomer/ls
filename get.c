#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include "common.h"
#include "myls.h"

int fname_width;      /*  文件名的输出宽度  */
int fsize_width;      /*  文件大小的输出宽度  */
int flink_width;      /*  文件链接数的输出宽度  */
int funame_width;     /*  文件用户名的输出宽度  */
int fgname_width;     /*  文件所属组名的输出宽度  */
int fuid_width;       /*  文件uid的输出宽度  */
int fgid_width;       /*  文件gid的输出宽度  */
int finode_width;     /*  文件inode的输出宽度  */

/*
 * 获得pathname下的文件总数
 */
int
get_fn(char *pathname)
{
    DIR             *dp;
    struct dirent   *dirp;
    int             fn;

    fn = 0;
    if ((dp = opendir(pathname)) == NULL)
        err_sys("opendir error");
    if (strncmp(pathname, "../", 3) == 0)
        printf("%-s:\n", pathname + 3);
    while ((dirp = readdir(dp)))
        fn++;
    closedir(dp);
    return fn;
}

/*
 * 计算出文件名的输出宽度
 */
void
get_fname_width(stat_t *statfp[], int fn)
{
    int buflen;

    for (int i = 0; i < fn; i++) {
        buflen = strlen(statfp[i]->buf);
        fname_width = max(buflen, fname_width);
    }
}

/*
 * 计算出除文件名以外的其他所需的输出宽度
 */
void
get_other_width(stat_t *statfp[], int fn)
{
    char tmp[BUFSIZ];
    struct passwd   *usr;
    struct group    *grp;
    int len;

    for (int i = 0; i < fn; i++) {
        sprintf(tmp, "%lld", statfp[i]->statbuf.st_size);
        len = strlen(tmp);
        fsize_width = max(len, fsize_width);

        sprintf(tmp, "%d", statfp[i]->statbuf.st_nlink);
        len = strlen(tmp);
        flink_width = max(len, flink_width);

        sprintf(tmp, "%u", statfp[i]->statbuf.st_uid);
        len = strlen(tmp);
        fuid_width = max(len, fuid_width);

        sprintf(tmp, "%u", statfp[i]->statbuf.st_gid);
        len = strlen(tmp);
        fgid_width = max(len, fgid_width);

        sprintf(tmp, "%llu", statfp[i]->statbuf.st_ino);
        len = strlen(tmp);
        finode_width = max(len, finode_width);

        if ((usr = getpwuid(statfp[i]->statbuf.st_uid)) == NULL)
            err_sys("getpwuid error");
        len = strlen(usr->pw_name);
        funame_width = max(len, funame_width);

        if ((grp = getgrgid(statfp[i]->statbuf.st_gid)) == NULL)
            err_sys("getgrgid error");
        len = strlen(grp->gr_name);
        fgname_width = max(len, fgname_width);
    }
}
