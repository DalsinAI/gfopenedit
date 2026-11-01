/* oe_find: find and replace (oe_find.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "oe_find.h"
#include "oe_pat.h"
#include <stdlib.h>
#include <string.h>

/* A line's step budget for a pattern (oe_pat.h), without wrapping past
 * LONG_MAX on a 32-bit long. */
static long line_steps(long n)
{
    if (n > (0x7fffffffL - OE_PAT_STEPS) / OE_PAT_STEPS_PER_CHAR)
        return 0x7fffffffL;
    return OE_PAT_STEPS + n * OE_PAT_STEPS_PER_CHAR;
}

/* The first match in s[from..n) of a line; its end in *e, or -1. A pattern
 * spends at most OE_PAT_STEPS at each start (one that gives up there has no
 * match there, as before), all out of the line's budget *steps; once that
 * is spent the line has no more matches. */
static long in_line(const char *s, long n, long from, const char *what, int wn, int flags, long *e, long *steps)
{
    long i;
    int nocase = !(flags & OE_F_CASE);
    for (i = from; i <= n; i++) {
        const char *end = NULL;
        if (flags & OE_F_PAT) {
            long here = *steps < OE_PAT_STEPS ? *steps : OE_PAT_STEPS, left = here;
            end = oe_pat_at_steps(what, wn, s + i, s + n, nocase, &left);
            *steps -= here - left;
            if (*steps <= 0)
                return -1;              /* the line's budget is spent: gave up */
            if (end == s + i)
                end = NULL;             /* an empty match is no match */
            /* #? first: one that doesn't match here (and didn't give up
             * here) can't match further on. */
            if (!end && left >= 0 && oe_pat_any_first(what, wn))
                return -1;
        } else if (i + wn <= n) {
            int k;
            for (k = 0; k < wn; k++)
                if (s[i + k] != what[k] && (!nocase || oe_lower(s[i + k]) != oe_lower(what[k])))
                    break;
            if (k == wn && wn > 0)
                end = s + i + wn;
        }
        if (end) {
            long j = end - s;
            if (!(flags & OE_F_WORD) ||
                ((i == 0 || !oe_is_word(s[i - 1])) && (j >= n || !oe_is_word(s[j])))) {
                *e = j;
                return i;
            }
        }
    }
    return -1;
}

/* Up to max matches on a line from offset from on. */
static int find_from(oe_doc *d, const char *what, int flags, long line, long from, long *starts, long *ends, int max)
{
    long st = oe_buf_line_start(&d->buf, line), n = oe_buf_line_len(&d->buf, line), a, e;
    long steps = line_steps(n);
    const char *s;
    int wn = (int)strlen(what), k = 0;
    if (!wn || line < 0 || line >= oe_buf_lines(&d->buf))
        return 0;
    s = oe_buf_span(&d->buf, st, n);
    while (k < max && from <= n && (a = in_line(s, n, from, what, wn, flags, &e, &steps)) >= 0) {
        starts[k] = a;
        ends[k] = e;
        k++;
        from = e > a ? e : a + 1;
    }
    return k;
}

int oe_find_line(oe_doc *d, const char *what, int flags, long line, long *starts, long *ends, int max)
{
    return find_from(d, what, flags, line, 0, starts, ends, max);
}

int oe_find(oe_doc *d, const char *what, int flags, long from, int backward, long *a, long *b)
{
    long lines = oe_buf_lines(&d->buf), line, k;
    int wn = (int)strlen(what);
    if (!wn)
        return 0;
    if (from < 0)
        from = 0;
    if (from > oe_buf_len(&d->buf))
        from = oe_buf_len(&d->buf);
    line = oe_buf_line_of(&d->buf, from);
    for (k = 0; k <= lines; k++) {
        long l = backward ? (line - k + lines * 2) % lines : (line + k) % lines;
        long st = oe_buf_line_start(&d->buf, l), n = oe_buf_line_len(&d->buf, l), s0, e;
        long steps = line_steps(n);
        const char *s = oe_buf_span(&d->buf, st, n);
        if (!backward) {
            s0 = in_line(s, n, k == 0 ? from - st : 0, what, wn, flags, &e, &steps);
            /* Back on the first line after wrapping: only before from. */
            if (s0 >= 0 && (k < lines || st + s0 < from)) {
                *a = st + s0;
                *b = st + e;
                return 1;
            }
        } else {
            long best = -1, beste = 0, f = 0;
            while (f <= n && (s0 = in_line(s, n, f, what, wn, flags, &e, &steps)) >= 0) {
                /* The caret's line: before from; again after wrapping: the rest. */
                if (k == 0 && st + e > from)
                    break;
                if (k < lines || k == 0 || st + e > from) {
                    best = s0;
                    beste = e;
                }
                f = e > s0 ? e : s0 + 1;
            }
            if (best >= 0) {
                *a = st + best;
                *b = st + beste;
                return 1;
            }
        }
    }
    return 0;
}

long oe_find_count(oe_doc *d, const char *what, int flags, long at, long *index)
{
    long lines = oe_buf_lines(&d->buf), l, count = 0;
    long st[64], en[64];
    *index = 0;
    for (l = 0; l < lines; l++) {
        long base = oe_buf_line_start(&d->buf, l), from = 0;
        int n, i;
        /* 64 at a time, as many times as the line needs. */
        do {
            n = find_from(d, what, flags, l, from, st, en, 64);
            for (i = 0; i < n; i++) {
                count++;
                if (base + st[i] == at)
                    *index = count;
            }
            if (n)
                from = en[n - 1] > st[n - 1] ? en[n - 1] : st[n - 1] + 1;
        } while (n == 64);
    }
    return count;
}

long oe_replace_all(oe_doc *d, const char *what, int flags, const char *with)
{
    long lines, l, count = 0, wn = (long)strlen(with);
    long st[64], en[64];
    if (d->readonly || !*what)
        return 0;
    oe_doc_begin(d);
    lines = oe_buf_lines(&d->buf);
    for (l = 0; l < lines; l++) {
        int n, i;
        /* A long line may hold more than 64: go round, searching on from
         * past the last replacement (the replacement may hold the text
         * searched for, which mustn't be found again). */
        long skip = 0;
        for (;;) {
            long base = oe_buf_line_start(&d->buf, l), shift = 0;
            n = find_from(d, what, flags, l, skip, st, en, 64);
            for (i = 0; i < n; i++) {
                long a = base + st[i] + shift, b = base + en[i] + shift;
                oe_doc_select(d, a, b);
                if (!oe_doc_insert(d, with, wn, 0)) {
                    oe_doc_end(d);
                    return count;
                }
                shift += wn - (en[i] - st[i]);
                count++;
                skip = en[i] + shift;
            }
            if (n < 64)
                break;
        }
        /* The replacement can't add lines unless it has a newline, which a
         * one-line field never has. */
    }
    oe_doc_end(d);
    return count;
}
