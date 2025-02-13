/***********************************************************************
 *                                                                      *
 *               This software is part of the ast package               *
 *          Copyright (c) 1992-2013 AT&T Intellectual Property          *
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
 * output the beginning portion of one or more files
 */
#include "config_ast.h"  // IWYU pragma: keep

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "option.h"
#include "sfio.h"
#include "shcmd.h"

static const char usage[] =
    "[-n?\n@(#)$Id: head (AT&T Research) 2013-09-19 $\n]" USAGE_LICENSE
    "[+NAME?head - output beginning portion of one or more files ]"
    "[+DESCRIPTION?\bhead\b copies one or more input files to standard "
    "output stopping at a designated point for each file or to the end of "
    "the file whichever comes first. Copying ends at the point indicated by "
    "the options. By default a header of the form \b==> \b\afilename\a\b "
    "<==\b is output before all but the first file but this can be changed "
    "with the \b-q\b and \b-v\b options.]"
    "[+?If no \afile\a is given, or if the \afile\a is \b-\b, \bhead\b "
    "copies from standard input starting at the current location.]"
    "[+?The option argument for \b-c\b, and \b-s\b can optionally be "
    "followed by one of the following characters to specify a different unit "
    "other than a single byte:]"
    "{"
    "[+b?512 bytes.]"
    "[+k?1-killobyte.]"
    "[+m?1-megabyte.]"
    "}"
    "[+?For backwards compatibility, \b-\b\anumber\a is equivalent to \b-n\b "
    "\anumber\a.]"
    "[n:lines?Copy \alines\a lines from each file.]#[lines:=10]"
    "[c:bytes?Copy \achars\a bytes from each file.]#[chars]"
    "[q:quiet|silent?Never ouput filename headers.]"
    "[s:skip?Skip \askip\a characters or lines from each file before "
    "copying.]#[skip]"
    "[v:verbose?Always ouput filename headers.]"
    "\n\n"
    "[ file ... ]"
    "\n\n"
    "[+EXIT STATUS?]"
    "{"
    "[+0?All files copied successfully.]"
    "[+>0?One or more files did not copy.]"
    "}"
    "[+SEE ALSO?\bcat\b(1), \btail\b(1)]";

int b_head(int argc, char **argv, Shbltin_t *context) {
    static const char header_fmt[] = "\n==> %s <==\n";

    Sfio_t *fp;
    char *cp;
    off_t keep = 10;
    off_t skip = 0;
    int delim = '\n';
    off_t moved;
    int header = 1;
    char *format = (char *)header_fmt + 1;
    int n;

    if (cmdinit(argc, argv, context, 0)) return -1;
    while ((n = optget(argv, usage))) {
        switch (n) {  //!OCLINT(MissingDefaultStatement)
            case 'c':
                delim = -1;
                /*FALLTHROUGH*/
            case 'n':
                if (opt_info.offset && argv[opt_info.index][opt_info.offset] == 'c') {
                    delim = -1;
                    opt_info.offset++;
                }
                keep = opt_info.number;
                if (keep <= 0) {
                    error(2, "%s: %I*d: positive numeric option argument expected", opt_info.name,
                          sizeof(keep), keep);
                }
                break;
            case 'q':
                header = argc;
                break;
            case 'v':
                header = 0;
                break;
            case 's':
                skip = opt_info.number;
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
    if (error_info.errors) {
        error(ERROR_usage(2), "%s", optusage(NULL));
        __builtin_unreachable();
    }
    cp = *argv;
    if (cp) argv++;
    do {
        if (!cp || !strcmp(cp, "-")) {
            cp = "/dev/stdin";
            fp = sfstdin;
            sfset(fp, SF_SHARE, 1);
        } else if (!(fp = sfopen(NULL, cp, "r"))) {
            error(ERROR_system(0), "%s: cannot open", cp);
            continue;
        }
        if (argc > header) sfprintf(sfstdout, format, cp);
        format = (char *)header_fmt;
        if (skip > 0) {
            if ((moved = sfmove(fp, NULL, skip, delim)) < 0 && !ERROR_PIPE(errno) &&
                errno != EINTR) {
                error(ERROR_system(0), "%s: skip error", cp);
            }
            if (delim >= 0 && moved < skip) goto next;
        }
        moved = sfmove(fp, sfstdout, keep, delim);
        if ((moved < 0 && !ERROR_PIPE(errno) && errno != EINTR) ||
            (delim >= 0 && moved < keep && sfmove(fp, sfstdout, SF_UNBOUND, -1) < 0 &&
             !ERROR_PIPE(errno) && errno != EINTR)) {
            error(ERROR_system(0), "%s: read error", cp);
        }
    next:
        if (fp != sfstdin) sfclose(fp);
        cp = *argv++;
    } while (cp);
    if (sfsync(sfstdout)) error(ERROR_system(0), "write error");
    return error_info.errors != 0;
}
