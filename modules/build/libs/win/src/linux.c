/*

    Implementation of POSIX directory browsing functions and types for Win32.

    Author:  Kevlin Henney (kevlin@acm.org, kevlin@curbralan.com)
    History: Created March 1997. Updated June 2003 and July 2012.
    Rights:  See end of file.

*/

#include <dirent.h>
#include <errno.h>
#include <io.h> /* _findfirst and _findnext set errno iff they return -1 */
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

DIR*
opendir(
    const char *dirname)
{
    struct DIR *dirp;

    /* Must have directory name */
    if (dirname == NULL  ||  dirname[0] == '\0') {
        dirent_set_errno (ENOENT);
        return NULL;
    }

    /* Allocate memory for DIR structure */
    dirp = (DIR*) malloc (sizeof (struct DIR));
    if (!dirp) {
        return NULL;
    }
    {
        int error;
        wchar_t wname[PATH_MAX + 1];
        size_t n;

        /* Convert directory name to wide-character string */
        error = dirent_mbstowcs_s(
            &n, wname, PATH_MAX + 1, dirname, PATH_MAX + 1);
        if (error) {
            /*
             * Cannot convert file name to wide-character string.  This
             * occurs if the string contains invalid multi-byte sequences or
             * the output buffer is too small to contain the resulting
             * string.
             */
            goto exit_free;
        }


        /* Open directory stream using wide-character name */
        dirp->wdirp = _wopendir (wname);
        if (!dirp->wdirp) {
            goto exit_free;
        }

    }

    /* Success */
    return dirp;

    /* Failure */
exit_free:
    free (dirp);
    return NULL;
}


int setenv(const char *name, const char *value, int overwrite)
{
    int errcode = 0;
    if(!overwrite) {
        size_t envsize = 0;
        errcode = getenv_s(&envsize, NULL, 0, name);
        if(errcode || envsize) return errcode;
    }
    return _putenv_s(name, value);
}
        
		/*
 * Open directory stream using plain old C-string.
 */

#ifdef __cplusplus
}
#endif

