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

// Length of "/timetable//schedule/" no *
#define PREFIX_LENGTH 20
// prefix of full timetable path
static const char *prefix = "/timetable/schedule/*";

static void sep(CharBuffer *b) {
    charbuffer_append(b, "</tr><tr><td colspan=\"8\" class=\"tableblankrow\"></td></tr>");
}

static void title(CharBuffer *b, char *t) {
    charbuffer_append(b, "<tr><th colspan=\"8\">");
    charbuffer_append(b, t);
    charbuffer_append(b, "</th>");
}

static void hdr(CharBuffer *b, char *t) {
    charbuffer_append(b, "</tr><tr><th class=\"tableright\">");
    charbuffer_append(b, t);
    charbuffer_append(b, "</th><td colspan=\"7\">");
}

static void hdrStr(CharBuffer *b, struct json_object *sched, char *t, char *n) {
    hdr(b, t);
    charbuffer_append_jsonStr(b, sched, n);
    charbuffer_append(b, "</td>");
}

static void hdrInt(CharBuffer *b, struct json_object *sched, char *t, char *n) {
    hdr(b, t);
    charbuffer_append_jsonInt(b, sched, n);
    charbuffer_append(b, "</td>");
}

static void renderScheduleHead(CharBuffer *b, struct json_object *sched) {
    title(b, "Schedule Information");

    hdr(b, "Applicable");
    charbuffer_append_jsonStr(b, sched, "start");
    charbuffer_append(b, " to ");
    charbuffer_append_jsonStr(b, sched, "end");
    charbuffer_append(b, "</td>");

    hdrStr(b, sched, "Days service runs", "daysRun");

    hdr(b, "Bank Holiday Running");
    //charbuffer_append_jsonStr(b,sched,"daysRun");

    hdrStr(b, sched, "WTT Schedule UID", "uid");
    hdrStr(b, sched, "STP Indicator", "stdInd");
    hdrStr(b, sched, "Identity", "trainId");
    hdrStr(b, sched, "Headcode", "headcode");
    hdrStr(b, sched, "Category", "category");
    hdrStr(b, sched, "Status", "status");
    hdrStr(b, sched, "Operator", "operator");
}

static void renderScheduleTail(CharBuffer *b, struct json_object *sched) {
    title(b, "Additional Information");

    hdrStr(b, sched, "Class", "class");
    hdrStr(b, sched, "Sleepers", "sleepers");
    hdrStr(b, sched, "Reservations", "reservations");
    hdrStr(b, sched, "Catering", "catering");

    hdrInt(b, sched, "Service Code", "serviceCode");
    hdrStr(b, sched, "Power Type", "powerType");
    hdrStr(b, sched, "Timing", "timing");
    hdrInt(b, sched, "Speed", "speed");
    hdrStr(b, sched, "Characteristics", "characteristics");
    hdrStr(b, sched, "Branding", "branding");
    charbuffer_append(b, "</tr>");
}

static void renderScheduleDetail(
        CharBuffer *b,
        struct json_object *sched,
        struct json_object *tiploc,
        struct json_object * activity
        ) {

    title(b, "Planned Schedule");
    charbuffer_append(b, "</tr><tr><th rowspan=\"2\">Location</th><th rowspan=\"2\">Pl</th><th colspan=\"2\" class=\"wttpub\">Public</th><th colspan=\"3\" class=\"wttwork\">Working Timetable</th><th rowspan=\"2\">Activity</th></tr>");
    charbuffer_append(b, "<tr><th class=\"wttpta\">Arr</th><th class=\"wttptd\">Dep</th><th class=\"wttwta\">Arr</th><th class=\"wttwtd\">Dep</th><th class=\"wttwtp\">Pass</th>");

    struct json_object *ent;
    if (!json_object_object_get_ex(sched, "entries", &ent))
        return;

    struct array_list *list = json_object_get_array(ent);
    int len = array_list_length(list);
    for (int i = 0; i < len; i++) {
        ent = (json_object *) array_list_get_idx(list, i);

        charbuffer_append(b, "</tr><tr class=\"");
        charbuffer_append(b, json_isNull(ent, "wtp") ? "wttnormal" : "wttpass");
        charbuffer_append(b, "\"><td>");
        charbuffer_renderTiploc(b, tiploc, json_getString(ent, "tiploc"));

        charbuffer_append(b, "</td><td class=\"wttplat\">");
        charbuffer_append_jsonStr(b, ent, "platform");

        charbuffer_append(b, "</td><td class=\"wttpub wttpta\">");
        charbuffer_appendTime(b, ent, "pta");

        charbuffer_append(b, "</td><td class=\"wttpub wttptd\">");
        charbuffer_appendTime(b, ent, "ptd");

        charbuffer_append(b, "</td><td class=\"wttwork wttwta\">");
        charbuffer_appendTime(b, ent, "wta");

        charbuffer_append(b, "</td><td class=\"wttwork wttwtd\">");
        charbuffer_appendTime(b, ent, "wtd");

        charbuffer_append(b, "</td><td class=\"wttwork wttwtp\">");
        charbuffer_appendTime(b, ent, "wtp");

        charbuffer_append(b, "</td><td>");

        struct json_object *act;
        if (json_object_object_get_ex(ent, "activity", &act)) {
            struct array_list *list = json_object_get_array(act);
            int len = array_list_length(list);
            for (int i = 0; i < len; i++) {
                json_object *ent = (json_object *) array_list_get_idx(list, i);
                char *s = (char *) json_object_get_string(ent);

                if (i)
                    charbuffer_append(b, ", ");

                if (json_object_object_get_ex(activity, s, &ent)) {
                    charbuffer_append(b, s);
                }
            }

        }
        charbuffer_append(b, "</td>");
    }
}

static void renderActivity(CharBuffer *b, struct json_object *activity) {
    if (activity) {
        title(b, "Activities");

        struct json_object_iterator it;
        struct json_object_iterator itEnd;
        it = json_object_iter_begin(activity);
        itEnd = json_object_iter_end(activity);
        while (!json_object_iter_equal(&it, &itEnd)) {
            hdr(b, (char *) json_object_iter_peek_name(&it));
            struct json_object *o = json_object_iter_peek_value(&it);
            charbuffer_append(b, (char *) json_object_get_string(o));
            charbuffer_append(b, "</td>");
            json_object_iter_next(&it);
        }
    }
}

static void renderSchedule(
        CharBuffer *b,
        struct json_object *sched,
        struct json_object *tiploc,
        struct json_object *activity
        ) {

    charbuffer_append(b, "<h2 class=\"wtt\">");
    struct json_object *entryObj;
    if (json_object_object_get_ex(sched, "entries", &entryObj)) {
        struct array_list *list = json_object_get_array(entryObj);
        int len = array_list_length(list);
        if (len) {
            struct json_object * origin = (json_object *) array_list_get_idx(list, 0);
            if (json_getString(origin, "ptd"))
                charbuffer_appendTime(b, origin, "ptd");
            else
                charbuffer_appendTime(b, origin, "wtd");
        }
    }
    charbuffer_renderTiploc(b, tiploc, json_getString(sched, "origin"));
    charbuffer_append(b, " to ");
    charbuffer_renderTiploc(b, tiploc, json_getString(sched, "dest"));
    charbuffer_append(b, "</h2>");

    charbuffer_append(b, "<table class=\"wikitable\">");
    renderScheduleHead(b, sched);
    sep(b);
    renderScheduleDetail(b, sched, tiploc, activity);

    if (activity) {
        sep(b);
        renderActivity(b, activity);
    }

    sep(b);
    renderScheduleTail(b, sched);
    charbuffer_append(b, "</table>");
}

static void renderSchedules(
        CharBuffer *b,
        struct json_object *sched,
        struct json_object *tiploc,
        struct json_object * activity
        ) {
    struct array_list *list = json_object_get_array(sched);
    int len = array_list_length(list);
    if (len) {
        // We either have 0 or 1 when using the date form of the api
        json_object *ent = (json_object *) array_list_get_idx(list, 0);
        renderSchedule(b, ent, tiploc, activity);
    }
}

const char *URL = "http://[::1]:9000/schedule/uid/%s/%04d-%02d-%02d";

/**
 * Show schedule
 * @param request
 * @param station
 * @param date
 * @return 
 */
static int schedule(WEBSERVER_REQUEST *request, const char *uid, struct tm *tm) {
    char url[256];
    url[0] = 0;
    sprintf(url, URL, uid, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour);
    logconsole("get %s", url);

    int succ = MHD_NO;
    CharBuffer *b = charbuffer_new();
    if (b) {
        int ret = curl_get(url, b);
        if (ret)
            logconsole("Failed to get schedules %d: %s", ret, url);
        else {

            int len;
            char *json = charbuffer_tostring(b, &len);
            struct json_object *obj = json_tokener_parse(json);
            free(json);

            if (obj) {

                struct json_object *sched = NULL;
                struct json_object *tiploc = NULL;
                struct json_object *activity = NULL;
                if (json_object_object_get_ex(obj, "schedule", &sched) &&
                        json_object_object_get_ex(obj, "tiploc", &tiploc) &&
                        json_object_object_get_ex(obj, "activity", &activity)) {

                    charbuffer_reset(b);
                    renderSchedules(b, sched, tiploc, activity);
                    json = charbuffer_toarray(b, &len);
                    webserver_setRequestAttribute(request, "schedule", template_new(json, len, free), (void (*)(void*))template_free);

                    TemplateEngine *e = webserver_getUserData(request);
                    TemplateFile *f = template_get(e, "/timetable/schedule.html");
                    if (f) {
                        webserver_setRequestAttribute(request, "title", strdup(uid), free);
                        //webserver_setRequestAttribute(request, "javascript", "/timetable/javascript.html", NULL);
                        webserver_setRequestAttribute(request, "body", f, (void (*)(void*))template_free);

                        succ = render_template_name(request, templateEngine, NULL);
                    }
                }

                json_object_put(obj);
            }
        }

        charbuffer_free(b);
    }
    return succ;
}

static int handler(WEBSERVER_REQUEST * request) {
    const char *url = webserver_getRequestUrl(request);
    if (strlen(url) > PREFIX_LENGTH) {
        // Find first / after prefix
        char *date = strstr(&url[PREFIX_LENGTH], "/");
        if (date && strlen(date) > 1) {
            date++;

            int len = (int) (date - &url[PREFIX_LENGTH]);
            char id[len];
            strncpy(id, &url[PREFIX_LENGTH], len - 1);
            id[len - 1] = 0;

            // Parse date, reject anything before 2016
            struct tm tm;
            if (!parseTime(&tm, date) || tm.tm_year < 116)
                return MHD_NO;

            if (isalpha(id[0]) && isNumeric(&id[1]))
                return schedule(request, id, &tm);
        }
    }

    return MHD_NO;
}

int ukt_timetable_schedule(WEBSERVER *webserver, TemplateEngine * e) {
    webserver_add_handler(webserver, prefix, handler, templateEngine);
}
