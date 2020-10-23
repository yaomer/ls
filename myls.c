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

#define max(a, b) ((a) > (b) ? (a) : (b))

#define error(s) do { \
    fprintf(stderr, "error: %s\n", s); \
    exit(1); \
} while (0)

typedef struct {    /*  每个文件的所有信息  */
    struct stat     statbuf;
    char            buf[FILENAME_MAX + 1];     /*  保存文件名  */
    char            path[PATH_MAX + 1];        /*  保存文件路径  */
} stat_t;

int     (*cmp)(const void *, const void *);

static int option = 0;       /*  设置标志位  */

static int fname_width;      /*  文件名的输出宽度  */
static int fsize_width;      /*  文件大小的输出宽度  */
static int flink_width;      /*  文件链接数的输出宽度  */
static int funame_width;     /*  文件用户名的输出宽度  */
static int fgname_width;     /*  文件所属组名的输出宽度  */
static int fuid_width;       /*  文件uid的输出宽度  */
static int fgid_width;       /*  文件gid的输出宽度  */
static int finode_width;     /*  文件inode的输出宽度  */

/*
 * 获得pathname下的文件总数
 */
static int get_files(char *pathname)
{
    DIR             *dp;
    struct dirent   *dirp;
    int             fn;

    fn = 0;
    if ((dp = opendir(pathname)) == NULL)
        error(strerror(errno));
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
static void get_fname_width(stat_t *statfp[], int fn)
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
static void get_other_width(stat_t *statfp[], int fn)
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
            error(strerror(errno));
        len = strlen(usr->pw_name);
        funame_width = max(len, funame_width);

        if ((grp = getgrgid(statfp[i]->statbuf.st_gid)) == NULL)
            error(strerror(errno));
        len = strlen(grp->gr_name);
        fgname_width = max(len, fgname_width);
    }
}

static int filecmp(const void *s, const void *t)
{
    return strcmp((*(stat_t **)s)->buf
        , (*(stat_t **)t)->buf)
        * ((option & FILE_REV) ? -1 : 1);
}

static int timecmp(const void *s, const void *t)
{
    return ((*(stat_t **)s)->statbuf.st_mtime
        - (*(stat_t **)t)->statbuf.st_mtime)
        * ((option & FILE_REV) ? -1 : 1);
}

static int sizecmp(const void *s, const void *t)
{
    return ((*(stat_t **)s)->statbuf.st_size
        - (*(stat_t **)t)->statbuf.st_size)
        * ((option & FILE_REV) ? -1 : 1);
}

/*
 * 以人类易懂的方式打印文件大小
 */
static void fsize_trans(off_t size)
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

/*
 * 打印文件的详细属性信息
 */
static void show_attribute(stat_t *statf)
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
            error(strerror(errno));
        if ((grp = getgrgid(stat.st_gid)) == NULL)
            error(strerror(errno));
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
        error(strerror(errno));
    if (timeptr[strlen(timeptr) - 1] == '\n')
        timeptr[strlen(timeptr) - 1] = '\0';
    printf("%s ", timeptr);

    printf("%-s\n", statf->buf);
}

/*
 * 将目录下的所有文件按列对齐输出
 * 如果输出到终端，则对齐输出；如果输出重定向到了一个文件，则一行输出一个
 */
static void show_align(int ws_col, stat_t *statf)
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
 * 输出单个目录下的内容
 */
static void show_single_dir(stat_t *statfp[], int fn)
{
    struct winsize  size;     /*  终端窗口的大小  */

    if (isatty(1)) {     /*  输出至终端  */
        if (ioctl(1, TIOCGWINSZ, &size) < 0)    /*  获取终端窗口的大小  */
            error(strerror(errno));
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

/*  show函数：列出目录下的文件列表  */
static void show_dir(char *pathname, int fn)
{
    DIR             *dp;
    struct dirent   *dirp;
    stat_t          *statfp[fn];
    int             i;

    for (i = 0; i < fn; i++) {
        statfp[i] = malloc(sizeof(stat_t));
        assert(statfp[i]);
    }

    if ((dp = opendir(pathname)) == NULL)
        error(strerror(errno));
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
            error(strerror(errno));

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
                show_dir(statfp[i]->path, get_files(statfp[i]->path));
    }

    for (i = 0; i < fn; i++)
        free(statfp[i]);
}

/*
 * 处理文件列表参数，并判断其是目录还是文件
 */
static void deal_argfile(int argc, char **argv)
{
    DIR             *dp;
    char            *dirbuf[argc];    /* 保存文件参数表中的目录名 */
    stat_t          *statfp[argc];    /* 保存文件参数表中的文件的详细信息 */
    int             dn, fn, i;

    dn = fn = 0;
    while (argc-- > 0)
        if ((dp = opendir(*argv)) == NULL) {   /*  可能是文件  */
            statfp[fn] = malloc(sizeof(stat_t));
            assert(statfp[fn]);
            if (lstat(*argv, &statfp[fn]->statbuf) < 0)
                error(strerror(errno));
            strcpy(statfp[fn++]->buf, *argv++);
        } else {
            closedir(dp);   /*  是目录  */
            dirbuf[dn++] = strdup(*argv);
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
        show_dir(dirbuf[i], get_files(dirbuf[i]));
}

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
                error("usage: ls [-aAFhilmnrRSt] [files ...]");
                break;
            }
    if (argc > 0)
        deal_argfile(argc, argv);    /*  处理文件参数表  */
    else
        show_dir(".", get_files("."));    /*  默认情况下，处理当前目录  */
}
