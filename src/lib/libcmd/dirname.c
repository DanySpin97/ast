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
 * dirname path [suffix]
 *
 * print the dirname of a pathname
 */
#include "config_ast.h"  // IWYU pragma: keep

#include <limits.h>
#include <stdlib.h>

#include "ast.h"
#include "error.h"
#include "option.h"
#include "sfio.h"
#include "shcmd.h"

static const char usage[] =
    "[-?\n@(#)$Id: dirname (AT&T Research) 2009-01-31 $\n]" USAGE_LICENSE
    "[+NAME?dirname - return directory portion of file name]"
    "[+DESCRIPTION?\bdirname\b treats \astring\a as a file name and returns "
    "the name of the directory containing the file name by deleting "
    "the last component from \astring\a.]"
    "[+?If \astring\a consists solely of \b/\b characters the output will "
    "be a single \b/\b. Trailing \b/\b characters are removed, and if "
    "there are no remaining \b/\b characters in \astring\a, "
    "the string \b.\b will be written to standard output.  "
    "Otherwise, all characters following the last \b/\b are removed. "
    "If the remaining string consists solely of \b/\b characters, "
    "the output will be as if the original string had consisted solely "
    "as \b/\b characters as described above.  Otherwise, all "
    "trailing slashes are removed and the output will be this string "
    "unless this string is empty.  If empty the output will be \b.\b.]"
    "[f:file?Print the \b$PATH\b relative regular file path for \astring\a.]"
    "[r:relative?Print the \b$PATH\b relative readable file path for \astring\a.]"
    "[x:executable?Print the \b$PATH\b relative executable file path for \astring\a.]"
    "\n"
    "\nstring\n"
    "\n"
    "[+EXIT STATUS?]{"
    "[+0?Successful Completion.]"
    "[+>0?An error occurred.]"
    "}"
    "[+SEE ALSO?\bbasename\b(1), \bgetconf\b(1), \bdirname\b(3), \bpathname\b(3)]";

static void l_dirname(Sfio_t *outfile, const char *pathname) {
    const char *last;
    /* go to end of path */
    for (last = pathname; *last; last++) {
        ;
    }
    /* back over trailing '/' */
    while (last > pathname && *--last == '/') {
        ;
    }
    /* back over non-slash chars */
    for (; last > pathname && *last != '/'; last--) {
        ;
    }
    if (last == pathname) {
        /* all '/' or "" */
        if (*pathname != '/') last = pathname = ".";
    } else {
        /* back over trailing '/' */
        for (; *last == '/' && last > pathname; last--) {
            ;
        }
    }
    /* preserve // */
    if (last != pathname && pathname[0] == '/' && pathname[1] == '/') {
        while (pathname[2] == '/' && pathname < last) pathname++;
        if (last != pathname && pathname[0] == '/' && pathname[1] == '/') {
            pathname++;
        }
    }
    sfwrite(outfile, pathname, last + 1 - pathname);
    sfputc(outfile, '\n');
}

int b_dirname(int argc, char **argv, Shbltin_t *context) {
    int mode = 0;
    int n;

    if (cmdinit(argc, argv, context, 0)) return -1;
    while ((n = optget(argv, usage))) {
        switch (n) {  //!OCLINT(MissingDefaultStatement)
            case 'f':
                mode |= PATH_REGULAR;
                break;
            case 'r':
                mode &= ~PATH_REGULAR;
                mode |= PATH_READ;
                break;
            case 'x':
                mode |= PATH_EXECUTE;
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
    argc -= opt_info.index;
    if (error_info.errors || argc != 1) {
        error(ERROR_usage(2), "%s", optusage(NULL));
        __builtin_unreachable();
    }

    if (!mode) {
        l_dirname(sfstdout, argv[0]);
        return 0;
    }

    char path[PATH_MAX];
    char *p = pathpath(argv[0], "", mode, path, sizeof(path));
    if (p) {
        sfputr(sfstdout, path, '\n');
    } else {
        // How is this possible? A filesystem problem of some sort? Such as the CWD no longer being
        // valid?
        error(1 | ERROR_WARNING, "%s: relative path not found", argv[0]);
    }
    return 0;
}
