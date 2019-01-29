#include <string.h>
#include "myls.h"

int
filecmp(const void *s, const void *t)
{
    return strcmp((*(stat_t **)s)->buf
        , (*(stat_t **)t)->buf)
        * ((option & FILE_REV) ? -1 : 1);
}

int
timecmp(const void *s, const void *t)
{
    return ((*(stat_t **)s)->statbuf.st_mtime
        - (*(stat_t **)t)->statbuf.st_mtime)
        * ((option & FILE_REV) ? -1 : 1);
}

int
sizecmp(const void *s, const void *t)
{
    return ((*(stat_t **)s)->statbuf.st_size
        - (*(stat_t **)t)->statbuf.st_size)
        * ((option & FILE_REV) ? -1 : 1);
}
