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
 * namebase pathname [suffix]
 *
 * print the namebase of a pathname
 */
#include "config_ast.h"  // IWYU pragma: keep

#include <string.h>

#include "error.h"
#include "option.h"
#include "sfio.h"
#include "shcmd.h"

static const char usage[] =
    "[-?\n@(#)$Id: basename (AT&T Research) 2010-05-06 $\n]" USAGE_LICENSE
    "[+NAME?basename - strip directory and suffix from filenames]"
    "[+DESCRIPTION?\bbasename\b removes all leading directory components "
    "from the file name defined by \astring\a. If the file name defined by "
    "\astring\a has a suffix that ends in \asuffix\a, it is removed as "
    "well.]"
    "[+?If \astring\a consists solely of \b/\b characters the output will be "
    "a single \b/\b. Trailing "
    "\b/\b characters are removed, and if there are any remaining \b/\b "
    "characters in \astring\a, all characters up to and including the last "
    "\b/\b are removed. Finally, if \asuffix\a is specified, and is "
    "identical the end of \astring\a, these characters are removed. The "
    "characters not removed from \astring\a will be written on a single line "
    "to the standard output.]"
    "[a:all?All operands are treated as \astring\a and each modified "
    "pathname is printed on a separate line on the standard output.]"
    "[s:suffix?All operands are treated as \astring\a and each modified "
    "pathname, with \asuffix\a removed if it exists, is printed on a "
    "separate line on the standard output.]:[suffix]"
    "\n"
    "\n string [suffix]\n"
    "string ...\n"
    "\n"
    "[+EXIT STATUS?]"
    "{"
    "[+0?Successful Completion.]"
    "[+>0?An error occurred.]"
    "}"
    "[+SEE ALSO?\bdirname\b(1), \bgetconf\b(1), \bbasename\b(3)]";

static void namebase(Sfio_t *outfile, char *pathname, char *suffix) {
    char *first, *last;
    int n = 0;
    for (first = last = pathname; *last; last++) {
        ;
    }
    /* back over trailing '/' */
    if (last > first) {
        while (*--last == '/' && last > first) {
            ;
        }
    }
    if (last == first && *last == '/') {
        /* all '/' or "" */
        if (*first == '/') {
            if (*++last == '/') { /* keep leading // */
                last++;
            }
        }
    } else {
        for (first = last++; first > pathname && *first != '/'; first--) {
            ;
        }
        if (*first == '/') first++;
        /* check for trailing suffix */
        if (suffix && (n = strlen(suffix)) && n < (last - first)) {
            if (memcmp(last - n, suffix, n) == 0) last -= n;
        }
    }
    if (last > first) sfwrite(outfile, first, last - first);
    sfputc(outfile, '\n');
}

int b_basename(int argc, char **argv, Shbltin_t *context) {
    char *string;
    char *suffix = NULL;
    int all = 0;
    int n;

    if (cmdinit(argc, argv, context, 0)) return -1;
    while ((n = optget(argv, usage))) {
        switch (n) {  //!OCLINT(MissingDefaultStatement)
            case 'a':
                all = 1;
                break;
            case 's':
                all = 1;
                suffix = opt_info.arg;
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
    if (error_info.errors || argc < 1 || (!all && argc > 2)) {
        error(ERROR_usage(2), "%s", optusage(NULL));
        __builtin_unreachable();
    }
    if (!all) {
        namebase(sfstdout, argv[0], argv[1]);
    } else {
        while ((string = *argv++)) namebase(sfstdout, string, suffix);
    }
    return 0;
}
