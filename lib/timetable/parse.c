/*
 * Display the timetable search page
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <area51/charbuffer.h>
#include <area51/curl.h>
#include <area51/json.h>
#include <area51/log.h>
#include <area51/memory.h>
#include <area51/rest.h>
#include <area51/string.h>
#include <area51/template.h>
#include <area51/webserver.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <curl/curl.h>
#include "uktrain/handlers.h"
#include <area51/trace.h>

bool isNumeric(char *s) {
    int l = strlen(s);
    if (l == 0)
        return false;

    for (int i = 0; i < l; i++)
        if (!isdigit(s[i]))
            return false;
    return true;
}

static const char *pat[] = {
    "%Y-%m-%dT%H:%M:%S",
    "%Y-%m-%dT%H:%M",
    "%Y-%m-%dT%H",
    "%Y-%m-%d",
    NULL
};

/**
 * Parse an ISO date/datetime into struct tm.
 * 
 * Note: In this instance if no time is present then tm_hour is set to -1
 * @param t struct tm to set
 * @param s iso date/datetime
 * @return true if parsed, false if not.
 */
bool parseTime(struct tm *t, char *s) {
    memset(t, 0, sizeof (struct tm));

    const char *fmt = pat[0];
    while (fmt && t->tm_year == 0) {
        memset(t, 0, sizeof (struct tm));
        strptime(s, fmt, t);
    }

    if (fmt) {
        if (strchr(s, 'T') == NULL)
            t->tm_hour = -1;
        return true;
    }

    return false;
}

void charbuffer_append_jsonStr(CharBuffer *b, struct json_object *ent, const char *k) {
    char *s = json_getString(ent, k);
    if (s) {
        for (bool lower = false; *s; s++) {
            if (lower && isupper(*s))
                charbuffer_add(b, tolower(*s));
            else
                charbuffer_add(b, *s);
            lower = isalpha(*s);
        }
    }
}

void charbuffer_append_jsonInt(CharBuffer *b, struct json_object *ent, const char *k) {
    int i = json_getInt(ent, k);
    if (i > INT_MIN)
        charbuffer_append_int(b, i, 0);
}

void charbuffer_appendTime(CharBuffer *b, struct json_object *ent, const char *k) {
    char *s = json_getString(ent, k);
    if (s) {
        switch (strlen(s)) {
            case 5:
                charbuffer_append(b, s);
                break;

            case 8:
                charbuffer_put(b, s, 5);
                charbuffer_append(b, s[5] == ':' && s[6] == '3' ? "&frac12;" : "&emsp;");
                break;

            default:
                break;
        }
    }
}

void charbuffer_renderTiploc(CharBuffer *b, struct json_object *tiploc, char *tpl) {
    if (tiploc && tpl) {
        struct json_object *t = NULL;
        if (json_object_object_get_ex(tiploc, tpl, &t)) {
            int stanox = json_getInt(t, "stanox");
            if (stanox > 0) {
                charbuffer_append(b, "<a href=\"/station/");
                charbuffer_append(b, tpl);
                charbuffer_append(b, "\">");
                if (json_getString(t, "desc"))
                    charbuffer_append_jsonStr(b, t, "desc");
                else
                    charbuffer_append(b, tpl);
                charbuffer_append(b, "</a>");

                return;
            }
        }

        charbuffer_append(b, tpl);
    }
}
