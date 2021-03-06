//===-- Windows.cpp ---------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// This file provides Windows support functions

#include "lldb/Host/windows/windows.h"
#include "lldb/Host/windows/win32.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <cerrno>
#include <ctype.h>

// These prototypes are defined in <direct.h>, but it also defines chdir() and getcwd(), giving multiply defined errors
extern "C"
{
    char *_getcwd(char *buffer, int maxlen);
    int _chdir(const char *path);
}

int vasprintf(char **ret, const char *fmt, va_list ap)
{
    char *buf;
    int len;
    size_t buflen;
    va_list ap2;

#if defined(_MSC_VER) || defined(__MINGW64)
    ap2 = ap;
    len = _vscprintf(fmt, ap2);
#else
    va_copy(ap2, ap);
    len = vsnprintf(NULL, 0, fmt, ap2);
#endif

    if (len >= 0 && (buf = (char*) malloc ((buflen = (size_t) (len + 1)))) != NULL) {
        len = vsnprintf(buf, buflen, fmt, ap);
        *ret = buf;
    } else {
        *ret = NULL;
        len = -1;
    }

    va_end(ap2);
    return len;
}

char* strcasestr(const char *s, const char* find)
{
    char c, sc;
    size_t len;

    if ((c = *find++) != 0) {
        c = tolower((unsigned char) c);
        len = strlen(find);
        do {
            do {
                if ((sc = *s++) == 0)
                    return 0;
            } while ((char) tolower((unsigned char) sc) != c);
        } while (strncasecmp(s, find, len) != 0);
        s--;
    }
    return ((char *) s);
}

char* realpath(const char * name, char * resolved)
{
    char *retname = NULL;  /* we will return this, if we fail */

    /* SUSv3 says we must set `errno = EINVAL', and return NULL,
    * if `name' is passed as a NULL pointer.
    */

    if (name == NULL)
        errno = EINVAL;

    /* Otherwise, `name' must refer to a readable filesystem object,
    * if we are going to resolve its absolute path name.
    */

    else if (access(name, 4) == 0)
    {
        /* If `name' didn't point to an existing entity,
        * then we don't get to here; we simply fall past this block,
        * returning NULL, with `errno' appropriately set by `access'.
        *
        * When we _do_ get to here, then we can use `_fullpath' to
        * resolve the full path for `name' into `resolved', but first,
        * check that we have a suitable buffer, in which to return it.
        */

        if ((retname = resolved) == NULL)
        {
            /* Caller didn't give us a buffer, so we'll exercise the
            * option granted by SUSv3, and allocate one.
            *
            * `_fullpath' would do this for us, but it uses `malloc', and
            * Microsoft's implementation doesn't set `errno' on failure.
            * If we don't do this explicitly ourselves, then we will not
            * know if `_fullpath' fails on `malloc' failure, or for some
            * other reason, and we want to set `errno = ENOMEM' for the
            * `malloc' failure case.
            */

            retname = (char*) malloc(_MAX_PATH);
        }

        /* By now, we should have a valid buffer.
        * If we don't, then we know that `malloc' failed,
        * so we can set `errno = ENOMEM' appropriately.
        */

        if (retname == NULL)
            errno = ENOMEM;

        /* Otherwise, when we do have a valid buffer,
        * `_fullpath' should only fail if the path name is too long.
        */

        else if ((retname = _fullpath(retname, name, _MAX_PATH)) == NULL)
            errno = ENAMETOOLONG;
    }

    /* By the time we get to here,
    * `retname' either points to the required resolved path name,
    * or it is NULL, with `errno' set appropriately, either of which
    * is our required return condition.
    */

    if (retname != NULL)
    {
        // Do a LongPath<->ShortPath roundtrip so that case is resolved by OS
        int initialLength = strlen(retname);
        TCHAR buffer[MAX_PATH];
        GetShortPathName(retname, buffer, MAX_PATH);
        GetLongPathName(buffer, retname, initialLength + 1);

        // Force drive to be upper case
        if (retname[1] == ':')
            retname[0] = toupper(retname[0]);
    }

    return retname;
}

#ifdef _MSC_VER

char* basename(char *path)
{
    char* l1 = strrchr(path, '\\');
    char* l2 = strrchr(path, '/');
    if (l2 > l1) l1 = l2;
    if (!l1) return path; // no base name
    return &l1[1];
}

// use _getcwd() instead of GetCurrentDirectory() because it updates errno
char* getcwd(char* path, int max)
{
    return _getcwd(path, max);
}

// use _chdir() instead of SetCurrentDirectory() because it updates errno
int chdir(const char* path)
{
    return _chdir(path);
}

char *dirname(char *path)
{
    char* l1 = strrchr(path, '\\');
    char* l2 = strrchr(path, '/');
    if (l2 > l1) l1 = l2;
    if (!l1) return NULL; // no dir name
    *l1 = 0;
    return path;
}

int strcasecmp(const char* s1, const char* s2)
{
    return stricmp(s1, s2);
}

int strncasecmp(const char* s1, const char* s2, size_t n)
{
    return strnicmp(s1, s2, n);
}

int usleep(uint32_t useconds)
{
    Sleep(useconds / 1000);
    return 0;
}

int snprintf(char *buffer, size_t count, const char *format, ...)
{
    int old_errno = errno;
    va_list argptr;
    va_start(argptr, format);
    int r = vsnprintf(buffer, count, format, argptr);
    int new_errno = errno;
    buffer[count-1] = '\0';
    if (r == -1 || r == count)
    {
        FILE *nul = fopen("nul", "w");
        int bytes_written = vfprintf(nul, format, argptr);
        fclose(nul);
        if (bytes_written < count)
            errno = new_errno;
        else
        {
            errno = old_errno;
            r = bytes_written;
        }
    }
    va_end(argptr);
    return r;
}

#endif // _MSC_VER
