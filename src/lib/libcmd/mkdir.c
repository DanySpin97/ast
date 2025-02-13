/***********************************************************************
 *                                                                      *
 *               This software is part of the ast package               *
 *          Copyright (c) 1992-2012 AT&T Intellectual Property          *
 *                      and is licensed under the                       *
 *                 Eclipse Public License, Version 1.0                  *
 *                    by AT&T Intellectual Property                     *
 *                                                                      *
 *                A copy of the License is available at                 *
 *          http://www.eclipse.org/org/documents/epl-v10.html           *
 *         (with md5 checksum b35adb5213ca9657e911e9befb180842)         *
 *                                                                      *
 *              Information and Software Systems Research               *
 *                            AT&T Research                             *
 *                           Florham Park NJ                            *
 *                                                                      *
 *               Glenn Fowler <glenn.s.fowler@gmail.com>                *
 *                    David Korn <dgkorn@gmail.com>                     *
 *                                                                      *
 ***********************************************************************/
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * mkdir
 */
#include "config_ast.h"  // IWYU pragma: keep

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ast.h"
#include "error.h"
#include "option.h"
#include "shcmd.h"

static const char usage[] = "[-?\n@(#)$Id: mkdir (AT&T Research) 2010-04-08 $\n]" USAGE_LICENSE
                            "[+NAME?mkdir - make directories]"
                            "[+DESCRIPTION?\bmkdir\b creates one or more directories.  By "
                            "default, the mode of created directories is \ba=rwx\b minus the "
                            "bits set in the \bumask\b(1).]"
                            "[m:mode]:[mode?Set the mode of created directories to \amode\a.  "
                            "\amode\a is symbolic or octal mode as in \bchmod\b(1).  Relative "
                            "modes assume an initial mode of \ba=rwx\b.]"
                            "[p:parents?Create any missing intermediate pathname components. For "
                            "each dir operand that does not name an existing directory, effects "
                            "equivalent to those caused by the following command shall occur: "
                            "\vmkdir -p -m $(umask -S),u+wx $(dirname dir) && mkdir [-m mode]] "
                            "dir\v where the \b-m\b mode option represents that option supplied to "
                            "the original invocation of \bmkdir\b, if any. Each dir operand that "
                            "names an existing directory shall be ignored without error.]"
                            "[v:verbose?Print a message on the standard error for each created "
                            "directory.]"
                            "\n"
                            "\ndirectory ...\n"
                            "\n"
                            "[+EXIT STATUS?]{"
                            "[+0?All directories created successfully, or the \b-p\b option "
                            "was specified and all the specified directories now exist.]"
                            "[+>0?An error occurred.]"
                            "}"
                            "[+SEE ALSO?\bchmod\b(1), \brmdir\b(1), \bumask\b(1)]";

#define DIRMODE (S_IRWXU | S_IRWXG | S_IRWXO)

int b_mkdir(int argc, char **argv, Shbltin_t *context) {
    char *path;
    int n;
    mode_t mode = DIRMODE;
    mode_t mask = 0;
    int mflag = 0;
    int pflag = 0;
    int vflag = 0;
    int made;
    char *part;
    mode_t dmode;
    struct stat st;

    if (cmdinit(argc, argv, context, 0)) return -1;
    while ((n = optget(argv, usage))) {
        switch (n) {  //!OCLINT(MissingDefaultStatement)
            case 'm':
                mflag = 1;
                mode = strperm(opt_info.arg, &part, mode);
                if (*part) error(ERROR_exit(0), "%s: invalid mode", opt_info.arg);
                break;
            case 'p':
                pflag = 1;
                break;
            case 'v':
                vflag = 1;
                break;
            case ':':
                error(2, "%s", opt_info.arg);
                break;
            case '?':
                error(ERROR_usage(2), "%s", opt_info.arg);
                __builtin_unreachable();
        }
    }
    argv += opt_info.index;
    if (error_info.errors || !*argv) {
        error(ERROR_usage(2), "%s", optusage(NULL));
        __builtin_unreachable();
    }

    mask = umask(0);
    if (mflag || pflag) {
        dmode = DIRMODE & ~mask;
        if (!mflag) mode = dmode;
        dmode |= S_IWUSR | S_IXUSR;
    } else {
        mode &= ~mask;
        umask(mask);
        mask = 0;
    }
    while (*argv) {
        path = *argv++;
        if (!mkdir(path, mode)) {
            if (vflag) error(0, "%s: directory created", path);
            made = 1;
        } else if (!pflag || !(errno == ENOENT || errno == EEXIST || errno == ENOTDIR)) {
            error(ERROR_system(0), "%s:", path);
            continue;
        } else if (errno == EEXIST) {
            continue;
        } else {
            /*
             * -p option, preserve intermediates
             * first eliminate trailing /'s
             */

            made = 0;
            n = strlen(path);
            while (n > 0 && path[--n] == '/') {
                ;  // empty loop
            }
            path[n + 1] = 0;
            for (part = path, n = *part; n;) {
                /* skip over slashes */
                while (*part == '/') part++;
                /* skip to next component */
                while ((n = *part) && n != '/') part++;
                *part = 0;
                if (mkdir(path, n ? dmode : mode) < 0 && errno != EEXIST &&
                    access(path, F_OK) < 0) {
                    error(ERROR_system(0), "%s: cannot create intermediate directory", path);
                    *part = n;
                    break;
                }
                if (vflag) error(0, "%s: directory created", path);
                *part = n;
                if (!(*part)) {
                    made = 1;
                    break;
                }
            }
        }
        if (made && (mode & (S_ISVTX | S_ISUID | S_ISGID))) {
            if (stat(path, &st)) {
                error(ERROR_system(0), "%s: cannot stat", path);
                break;
            }
            if ((st.st_mode & (S_ISVTX | S_ISUID | S_ISGID)) !=
                    (mode & (S_ISVTX | S_ISUID | S_ISGID)) &&
                chmod(path, mode)) {
                char *st_mode_str = strdup(fmtperm(st.st_mode & (S_ISVTX | S_ISUID | S_ISGID)));
                error(ERROR_system(0), "%s: cannot change mode from %s to %s", path, st_mode_str,
                      fmtperm(mode));
                free(st_mode_str);
                break;
            }
        }
    }
    if (mask) umask(mask);
    return error_info.errors != 0;
}
