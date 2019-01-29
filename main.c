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
#include <errno.h>
#include "common.h"
#include "myls.h"

int option = 0;       /*  设置标志位  */

/*
 * ls命令的简要实现
 */
int
main(int argc, char *argv[])
{
    int c;
    /*  解析输入参数  */
    while (--argc > 0 && (*++argv)[0] == '-')
        while ((c = *++argv[0]))
            switch (c) {
            case 'a':
                option |= FILE_ALL;
                break;
            case 'A':
                option |= FILE_AAL;
                break;
            case 'F':
                option |= FILE_TAG;
                break;
            case 'h':
                option |= FILE_UND;
                break;
            case 'i':
                option |= FILE_INO;
                break;
            case 'l':
                option |= FILE_LON;
                break;
            case 'm':
                option |= FILE_CMA;
                break;
            case 'n':
                option |= FILE_NUM;
                break;
            case 'r':
                option |= FILE_REV;
                break;
            case 'R':
                option |= FILE_REC;
                break;
            case 'S':
                option |= FILE_SIZ;
                break;
            case 't':
                option |= FILE_TIM;
                break;
            default:
                err_quit("usage: ls [-aAFhilmnrRSt] [files ...]");
                break;
            }
    if (argc > 0)
        deal_argfile(argc, argv);    /*  处理文件参数表  */
    else
        show_dir(".", get_fn("."));    /*  默认情况下，处理当前目录  */
    exit(0);
}
