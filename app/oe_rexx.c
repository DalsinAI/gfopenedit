/* oe_rexx: the GFOPENEDIT ARexx port, so other programs can drive GFOpenEdit.
 *
 *   OPEN file [LINE n]   NEW            SAVE           SAVEAS file
 *   CLOSE [FORCE]        GOTO line [col]               TAB n
 *   FIND text            REPLACEALL text with          INSERT text
 *   GETTEXT              GETLINE [n]    GETFILE        GETPOS
 *   FRONT                QUIT [FORCE]
 *   ACTIVATE             DEACTIVATE     VERSION        HELP [command]
 *   FAULT code           LASTERROR
 *
 * ACTIVATE, DEACTIVATE, VERSION, HELP, FAULT and LASTERROR are the
 * commands every Open app has (gfopensdk arexx/apphost.h): ACTIVATE is
 * FRONT, DEACTIVATE answers 5 (GFOpenEdit has no icon to go to), VERSION
 * gives "GFOpenEdit 0.1.2", HELP the commands or one command's arguments,
 * FAULT the text of a DOS error code, LASTERROR why the last command that
 * failed failed.
 *
 * Text with spaces goes in quotes. Results come back in RESULT (OPTIONS
 * RESULTS) when RC is 0. Return codes as amicradle-teams ARexx-RULE.md:
 * 0 done; 5 not done, nothing wrong (FIND finds nothing; CLOSE and QUIT
 * leave unsaved changes alone unless FORCE is given); 10 an error (a
 * missing argument, a file that can't be opened or saved); 20 an unknown
 * command. No command shows a requester: what one would have said is in
 * LASTERROR.
 *
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <exec/types.h>
#include <exec/ports.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <rexx/errors.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
/* The base is ours, typed as the NDK has it whichever way its proto does. */
#define __NOLIBBASE__
#include <proto/rexxsyslib.h>
#undef __NOLIBBASE__

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oe_app.h"
#include "oe_find.h"
#include "oe_pat.h"

struct RxsLib *RexxSysBase;

int oe_rexx_quiet(const char *text)
{
    if (!A.rexx_busy)
        return 0;
    if (!A.rexx_noted) {
        snprintf(A.rexx_error, sizeof A.rexx_error, "%s", text ? text : "");
        A.rexx_noted = 1;
    }
    return 1;
}

/* rc, with LASTERROR's text */
static long fail(long rc, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(A.rexx_error, sizeof A.rexx_error, fmt, ap);
    va_end(ap);
    A.rexx_noted = 1;
    return rc;
}

static int any_unsaved(void)
{
    int i;
    for (i = 0; i < A.ntabs; i++)
        if (oe_doc_modified(&A.tabs[i]->d))
            return 1;
    return 0;
}

int oe_rexx_open(void)
{
    if (!(RexxSysBase = (struct RxsLib *)OpenLibrary((CONST_STRPTR)"rexxsyslib.library", 36)))
        return 0;
    if (!(A.rexx_port = CreateMsgPort()))
        return 0;
    A.rexx_port->mp_Node.ln_Name = (char *)PORT_REXX;
    A.rexx_port->mp_Node.ln_Pri = 0;
    Forbid();
    if (FindPort((CONST_STRPTR)PORT_REXX)) {
        Permit();
        DeleteMsgPort(A.rexx_port);
        A.rexx_port = NULL;
        return 0;
    }
    AddPort(A.rexx_port);
    Permit();
    return 1;
}

void oe_rexx_close(void)
{
    if (A.rexx_port) {
        struct RexxMsg *m;
        RemPort(A.rexx_port);
        while ((m = (struct RexxMsg *)GetMsg(A.rexx_port))) {
            m->rm_Result1 = RC_FATAL;
            m->rm_Result2 = 0;
            ReplyMsg((struct Message *)m);
        }
        DeleteMsgPort(A.rexx_port);
        A.rexx_port = NULL;
    }
    if (RexxSysBase)
        CloseLibrary((struct Library *)RexxSysBase);
    RexxSysBase = NULL;
}

/* The next word, or "quoted text"; 0-ends it in place. */
static char *arg(char **s)
{
    char *a;
    while (**s == ' ' || **s == '\t')
        (*s)++;
    if (!**s)
        return NULL;
    if (**s == '"' || **s == '\'') {
        char q = *(*s)++;
        a = *s;
        while (**s && **s != q)
            (*s)++;
    } else {
        a = *s;
        while (**s && **s != ' ' && **s != '\t')
            (*s)++;
    }
    if (**s)
        *(*s)++ = 0;
    return a;
}

static void after_change(void)
{
    oe_set_title();
    oe_layout();
    oe_draw_all();
}

/* HELP's answers: each command and its arguments */
static const char *const help_text[][2] = {
    { "OPEN", "FILE [LINE n]" }, { "NEW", "" }, { "SAVE", "" }, { "SAVEAS", "FILE" },
    { "CLOSE", "[FORCE]" }, { "GOTO", "LINE [COL]" }, { "TAB", "N" }, { "FIND", "TEXT" },
    { "REPLACEALL", "TEXT WITH" }, { "INSERT", "TEXT" }, { "GETTEXT", "" }, { "GETLINE", "[N]" },
    { "GETFILE", "" }, { "GETPOS", "" }, { "FRONT", "" }, { "QUIT", "[FORCE]" },
    { "ACTIVATE", "" }, { "DEACTIVATE", "" }, { "VERSION", "" }, { "HELP", "[COMMAND]" }, { "FAULT", "CODE" },
    { "LASTERROR", "" },
};

/* Runs one command: the return code, and *res a malloc'd result or NULL. */
static long run(char *cmd, char **res, long *resn)
{
    char *s = cmd, *c = arg(&s), *a;
    oe_doc *d = cur_doc();
    *res = NULL;
    *resn = 0;
    if (!c)
        return fail(RC_ERROR, "No command");
    if (!oe_stricmp(c, "OPEN")) {
        long line = 0;
        if (!(a = arg(&s)))
            return fail(RC_ERROR, "OPEN needs a file");
        {
            char *k = arg(&s), *v = arg(&s);
            if (k && v && !oe_stricmp(k, "LINE"))
                line = atol(v);
        }
        return oe_new_tab(a, line, 0, NULL) ? 0 : RC_ERROR;
    }
    if (!oe_stricmp(c, "NEW"))
        return oe_new_tab(NULL, 0, OE_OPEN_NEW, NULL) ? 0 : RC_ERROR;
    if (!oe_stricmp(c, "FRONT") || !oe_stricmp(c, "ACTIVATE")) {
        WindowToFront(A.win);
        ScreenToFront(A.scr);
        ActivateWindow(A.win);
        return 0;
    }
    if (!oe_stricmp(c, "DEACTIVATE"))
        return fail(RC_WARN, "DEACTIVATE: GFOpenEdit has no icon to go to");
    if (!oe_stricmp(c, "LASTERROR")) {
        if ((*res = malloc(sizeof A.rexx_error)))
            *resn = sprintf(*res, "%s", A.rexx_error);
        return 0;
    }
    if (!oe_stricmp(c, "QUIT")) {
        if ((a = arg(&s)) && !oe_stricmp(a, "FORCE")) {
            int i;
            for (i = 0; i < A.ntabs; i++)
                oe_doc_mark_saved(&A.tabs[i]->d);
        } else if (any_unsaved())
            return fail(RC_WARN, "QUIT left alone: unsaved changes (QUIT FORCE quits)");
        A.quit = 1;
        return 0;
    }
    if (!oe_stricmp(c, "VERSION")) {
        if ((*res = malloc(48)))
            *resn = sprintf(*res, "GFOpenEdit %s", OE_VERSION);
        return 0;
    }
    if (!oe_stricmp(c, "HELP")) {
        size_t i, n = sizeof help_text / sizeof help_text[0];
        if ((a = arg(&s))) {
            for (i = 0; i < n; i++)
                if (!oe_stricmp(a, help_text[i][0])) {
                    if ((*res = malloc(64)))
                        *resn = sprintf(*res, "%s%s%s", help_text[i][0], *help_text[i][1] ? " " : "", help_text[i][1]);
                    return 0;
                }
            return fail(RC_ERROR, "HELP: no command %s", a);
        }
        if ((*res = malloc(n * 12 + 1))) {
            char *p = *res;
            for (i = 0; i < n; i++)
                p += sprintf(p, "%s%s", i ? " " : "", help_text[i][0]);
            *resn = p - *res;
        }
        return 0;
    }
    if (!oe_stricmp(c, "FAULT")) {
        if (!(a = arg(&s)))
            return fail(RC_ERROR, "FAULT needs a code");
        if (!(*res = malloc(100)))
            return fail(RC_FATAL, "Not enough memory");
        Fault(atol(a), NULL, (STRPTR)*res, 100);
        *resn = strlen(*res);
        return 0;
    }
    {
        static const char *const known[] = { "SAVE", "SAVEAS", "CLOSE", "TAB", "GOTO", "FIND", "REPLACEALL",
                                              "INSERT", "GETTEXT", "GETLINE", "GETFILE", "GETPOS", NULL };
        int k;
        for (k = 0; known[k] && oe_stricmp(c, known[k]); k++)
            ;
        if (!known[k])
            return fail(RC_FATAL, "Unknown command %s; HELP lists them", c);
    }
    if (!d)
        return fail(RC_ERROR, "No file is open");
    if (!oe_stricmp(c, "SAVE")) {
        char err[128];
        if (!d->path[0])
            return fail(RC_ERROR, "The file has no name yet: SAVEAS gives it one");
        if (!oe_save_file(d, d->path, err, sizeof err))
            return fail(RC_ERROR, "Couldn't save %s: %s", d->path, err);
        after_change();
        return 0;
    }
    if (!oe_stricmp(c, "SAVEAS")) {
        char err[128];
        if (!(a = arg(&s)))
            return fail(RC_ERROR, "SAVEAS needs a file");
        if (!oe_save_file(d, a, err, sizeof err))
            return fail(RC_ERROR, "Couldn't save %s: %s", a, err);
        after_change();
        return 0;
    }
    if (!oe_stricmp(c, "CLOSE")) {
        a = arg(&s);
        if (a && !oe_stricmp(a, "FORCE"))
            oe_doc_mark_saved(d);
        else if (oe_doc_modified(d))
            return fail(RC_WARN, "%s has unsaved changes (CLOSE FORCE closes it)", d->path[0] ? d->path : d->name);
        return oe_close_tab(A.cur, 1) ? 0 : RC_WARN;
    }
    if (!oe_stricmp(c, "TAB")) {
        long n = (a = arg(&s)) ? atol(a) : 0;
        if (n < 1 || n > A.ntabs)
            return fail(RC_ERROR, "TAB is 1 to %d", A.ntabs);
        oe_switch_tab((int)n - 1);
        return 0;
    }
    if (!oe_stricmp(c, "GOTO")) {
        long line = (a = arg(&s)) ? atol(a) : 0, col = (a = arg(&s)) ? atol(a) : 1;
        if (line < 1)
            return fail(RC_ERROR, "GOTO needs a line from 1");
        oe_doc_set_caret(d, oe_doc_pos_at(d, line - 1, (int)(col > 0 ? col - 1 : 0)), 0);
        oe_show_caret();
        oe_draw_all();
        return 0;
    }
    if (!oe_stricmp(c, "FIND")) {
        if (!(a = arg(&s)))
            return fail(RC_ERROR, "FIND needs text");
        snprintf(A.find_text, sizeof A.find_text, "%s", a);
        return oe_find_next(0, 0) ? 0 : fail(RC_WARN, "Not found: %s", a);
    }
    if (!oe_stricmp(c, "REPLACEALL")) {
        char *with;
        long n;
        if (!(a = arg(&s)) || !(with = arg(&s)))
            return fail(RC_ERROR, "REPLACEALL needs text and its replacement");
        n = oe_replace_all(d, a, 0, with);
        if ((*res = malloc(16)))
            *resn = sprintf(*res, "%ld", n);
        after_change();
        return 0;
    }
    if (!oe_stricmp(c, "INSERT")) {
        while (*s == ' ')
            s++;
        if (*s == '"' || *s == '\'')
            a = arg(&s);
        else
            a = s;
        if (!oe_doc_insert(d, a, (long)strlen(a), 0))
            return fail(RC_ERROR, "Couldn't insert the text (read only, or not enough memory)");
        oe_show_caret();
        after_change();
        return 0;
    }
    if (!oe_stricmp(c, "GETTEXT")) {
        long n;
        *res = oe_doc_text(d, 0, oe_doc_len(d) > 65535 ? 65535 : oe_doc_len(d), &n);
        *resn = n;
        return *res ? 0 : RC_ERROR;
    }
    if (!oe_stricmp(c, "GETLINE")) {
        long line = (a = arg(&s)) ? atol(a) - 1 : oe_doc_line(d), st, n;
        if (line < 0 || line >= oe_doc_lines(d))
            return fail(RC_ERROR, "GETLINE is 1 to %ld", oe_doc_lines(d));
        st = oe_buf_line_start(&d->buf, line);
        *res = oe_doc_text(d, st, st + oe_buf_line_len(&d->buf, line), &n);
        *resn = n;
        return *res ? 0 : RC_ERROR;
    }
    if (!oe_stricmp(c, "GETFILE")) {
        const char *p = d->path[0] ? d->path : d->name;
        if ((*res = malloc(strlen(p) + 1)))
            *resn = sprintf(*res, "%s", p);
        return 0;
    }
    if (!oe_stricmp(c, "GETPOS")) {
        if ((*res = malloc(48)))
            *resn = sprintf(*res, "%ld %d", oe_doc_line(d) + 1, oe_doc_col(d, d->caret) + 1);
        return 0;
    }
    return fail(RC_FATAL, "Unknown command %s; HELP lists them", c);
}

void oe_rexx_news(void)
{
    struct RexxMsg *m;
    if (!A.rexx_port)
        return;
    while ((m = (struct RexxMsg *)GetMsg(A.rexx_port))) {
        char *res = NULL, cmd[512];
        long resn = 0, rc;
        if (!IsRexxMsg(m)) {
            ReplyMsg((struct Message *)m);
            continue;
        }
        snprintf(cmd, sizeof cmd, "%s", (char *)m->rm_Args[0]);
        {
            /* LASTERROR reads the text and keeps it; every other command starts afresh */
            char *w = cmd, first[16];
            int reading, k = 0;
            while (*w == ' ' || *w == '\t')
                w++;
            while (w[k] && w[k] != ' ' && w[k] != '\t' && k < (int)sizeof first - 1)
                first[k] = w[k], k++;
            first[k] = 0;
            reading = !oe_stricmp(first, "LASTERROR");
            A.rexx_busy = 1;
            A.rexx_noted = reading;
            rc = run(cmd, &res, &resn);
            A.rexx_busy = 0;
            if (!reading) {
                if (!rc)
                    A.rexx_error[0] = 0;
                else if (!A.rexx_noted)
                    snprintf(A.rexx_error, sizeof A.rexx_error, "%.40s: RC %ld", w, rc);
            }
        }
        m->rm_Result1 = rc;
        m->rm_Result2 = 0;
        if (!rc && res && (m->rm_Action & RXFF_RESULT))
            m->rm_Result2 = (LONG)CreateArgstring((STRPTR)res, (ULONG)resn);
        free(res);
        ReplyMsg((struct Message *)m);
    }
}
