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

// Length of "/timetable/station/" no *
#define PREFIX_LENGTH 19
// prefix of full timetable path
static const char *prefix = "/timetable/station/*";

static bool getStation(WEBSERVER_REQUEST *request, const char *station) {
    logconsole("Retrieving station %s", station);
    CharBuffer *b = charbuffer_new();
    if (!b)
        return false;

    bool succ = false;
    char *url = genurl("http://[::1]:9000/tiploc/stanox/", station);
    if (url) {

        int ret = curl_get(url, b);
        free(url);
        if (ret) {
            logconsole("Failed station %d", ret);
        } else {
            int len;
            char *json = charbuffer_tostring(b, &len);
            struct json_object *obj = json_tokener_parse(json);
            free(json);

            struct array_list *list = json_object_get_array(obj);
            len = array_list_length(list);

            for (int i = 0; i < len && !succ; i++) {
                json_object *ent = (json_object *) array_list_get_idx(list, i);

                if (!json_isNull(ent, "crs") && !json_isNull(ent, "desc")) {
                    webserver_setRequestAttribute(request, "crs", strdup(json_getString(ent, "crs")), free);
                    webserver_setRequestAttribute(request, "title", strdup(json_getString(ent, "desc")), free);
                    webserver_setRequestAttribute(request, "desc", strdup(json_getString(ent, "desc")), free);
                    succ = true;
                }
            }

            json_object_put(obj);
        }
    }

    charbuffer_free(b);
    return succ;
}

static const char *schedUrl[] = {
    "http://[::1]:9000/schedule/stanox/%5s/%04d-%02d-%02dT%02d",
    "http://[::1]:9000/schedule/stanox/%5s/%04d-%02d-%02d"
};

static void renderActivity(CharBuffer *b, struct json_object *activity) {
    if (activity) {
        struct json_object_iterator it;
        struct json_object_iterator itEnd;
        it = json_object_iter_begin(activity);
        itEnd = json_object_iter_end(activity);
        while (!json_object_iter_equal(&it, &itEnd)) {
            charbuffer_append(b, "<tr><th>");
            charbuffer_append(b, (char *)json_object_iter_peek_name(&it));
            charbuffer_append(b, "</th><td>");
            struct json_object *o = json_object_iter_peek_value(&it);
            charbuffer_append(b, (char *)json_object_get_string(o));
            charbuffer_append(b, "</td></tr>");
            json_object_iter_next(&it);
        }
    }
}

static void renderSchedule(
        CharBuffer *b,
        struct tm * tm,
        struct json_object *sched,
        struct json_object *tiploc,
        struct json_object * activity
        ) {
    struct json_object *ent;

    charbuffer_append(b, json_isNull(sched, "public") ? "<tr class=\"wttpass\"><td>" : "<tr class=\"wttnormal\"><td>");

    char *uid = json_getString(sched, "uid");
    if (uid)
        charbuffer_printf(b, "<a href=\"/timetable/schedule/%s/%04d-%02d-%02d\">%s</a>",
            uid,
            tm->tm_year + 1900,
            tm->tm_mon + 1,
            tm->tm_mday,
            uid);

    charbuffer_append(b, "</td><td>");
    charbuffer_append_jsonStr(b, sched, "trainId");

    charbuffer_append(b, "</td><td>");
    charbuffer_append_jsonStr(b, sched, "stpInd");

    charbuffer_append(b, "</td><td>");
    if (json_object_object_get_ex(sched, "dest", &ent))
        charbuffer_renderTiploc(b, tiploc, json_getString(ent, "tiploc"));

    if (json_object_object_get_ex(sched, "loc", &ent)) {

        charbuffer_append(b, "</td><td>");
        charbuffer_appendTime(b, ent, "pta");

        charbuffer_append(b, "</td><td>");
        charbuffer_appendTime(b, ent, "ptd");

        charbuffer_append(b, "</td><td>");
        charbuffer_append_jsonStr(b, ent, "platform");

        charbuffer_append(b, "</td><td>");
        charbuffer_appendTime(b, ent, "wta");

        charbuffer_append(b, "</td><td>");
        charbuffer_appendTime(b, ent, "wtd");

        charbuffer_append(b, "</td><td>");
        charbuffer_appendTime(b, ent, "wtp");
    } else
        charbuffer_append(b, "</td><td colspan=\"6\">");

    charbuffer_append(b, "</td><td>");
    if (json_object_object_get_ex(sched, "origin", &ent))
        charbuffer_renderTiploc(b, tiploc, json_getString(ent, "tiploc"));

    charbuffer_append(b, "</td><td>");
    if (json_object_object_get_ex(sched, "loc", &ent))
        if (json_object_object_get_ex(ent, "activity", &ent)) {
            struct array_list *list = json_object_get_array(ent);
            int len = array_list_length(list);
            for (int i = 0; i < len; i++) {
                json_object *ent = (json_object *) array_list_get_idx(list, i);
                char *s = json_object_get_string(ent);

                if (json_object_object_get_ex(activity, s, &ent)) {
                    charbuffer_append(b, "<span class=\"wttact\">");
                    charbuffer_append(b, s);
                    charbuffer_append(b, "</span>");
                }
            }

        }

    charbuffer_append(b, "</td></tr>");
}

static void renderSchedules(
        CharBuffer *b,
        struct tm * tm,
        struct json_object *sched,
        struct json_object *tiploc,
        struct json_object * activity
        ) {
    struct array_list *list = json_object_get_array(sched);
    int len = array_list_length(list);
    for (int i = 0; i < len; i++) {

        json_object *ent = (json_object *) array_list_get_idx(list, i);
        renderSchedule(b, tm, ent, tiploc, activity);
    }
}

static void getSchedules(WEBSERVER_REQUEST *request, const char *station, struct tm * tm) {
    char url[256];
    url[0] = 0;
    if (tm->tm_hour >= 0)
        sprintf(url, schedUrl[0], station, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour);
    else
        sprintf(url, schedUrl[1], station, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);

    logconsole("get %s", url);
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

            struct json_object *sched = NULL;
            struct json_object *tiploc = NULL;
            struct json_object *activity = NULL;
            json_object_object_get_ex(obj, "schedule", &sched);
            json_object_object_get_ex(obj, "tiploc", &tiploc);
            json_object_object_get_ex(obj, "activity", &activity);

            charbuffer_reset(b);
            renderSchedules(b, tm, sched, tiploc, activity);
            json = charbuffer_toarray(b, &len);
            webserver_setRequestAttribute(request, "schedules", template_new(json, len, free), (void (*)(void*))template_free);

            charbuffer_reset(b);
            renderActivity(b, activity);
            json = charbuffer_toarray(b, &len);
            webserver_setRequestAttribute(request, "activity", template_new(json, len, free), (void (*)(void*))template_free);

            json_object_put(obj);
        }

        charbuffer_free(b);
    }
}

/**
 * Perform a timetable search
 * @param request
 * @return 
 */
static int search(WEBSERVER_REQUEST *request, const char *station, struct tm * date) {
    if (!getStation(request, station))
        return MHD_NO;

    TemplateEngine * e = webserver_getUserData(request);
    TemplateFile * f = template_get(e, "/timetable/station.html");
    if (!f)
        return MHD_NO;

    getSchedules(request, station, date);

    webserver_setRequestAttribute(request, "title", "Timetable Search", NULL);
    webserver_setRequestAttribute(request, "javascript", "/timetable/javascript.html", NULL);
    webserver_setRequestAttribute(request, "body", f, (void (*)(void*))template_free);

    return render_template_name(request, templateEngine, NULL);

    return MHD_NO;
}

/**
 * Show schedule
 * @param request
 * @param station
 * @param date
 * @return 
 */
static int schedule(WEBSERVER_REQUEST *request, const char *station, const char *date) {
    trace;
    logconsole("station \"%s\"", station);
    logconsole("   date \"%s\"", date);

    return MHD_NO;
}

static int handler(WEBSERVER_REQUEST * request) {
    trace;
    const char *url = webserver_getRequestUrl(request);
    logconsole("url \"%s\"", url);
    if (strlen(url) > PREFIX_LENGTH) {
        // Find first / after prefix
        char *date = strstr(&url[PREFIX_LENGTH], "/");
        if (date && strlen(date) > 1) {
            date++;

            int len = (int) (date - &url[PREFIX_LENGTH]);
            char id[len];
            strncpy(id, &url[PREFIX_LENGTH], len - 1);
            id[len - 1] = 0;
            logconsole("date \"%s\"", date);
            logconsole("  id \"%s\"", id);

            // Parse date, reject anything before 2016
            struct tm tm;
            if (!parseTime(&tm, date) || tm.tm_year < 116)
                return MHD_NO;

            logconsole("time %04d-%02d-%02d %02d",
                    tm.tm_year, tm.tm_mon, tm.tm_mday,
                    tm.tm_hour);

            if (isNumeric(id))
                return search(request, id, &tm);
        }
    }


    return MHD_NO;
}

int ukt_timetable_search(WEBSERVER *webserver, TemplateEngine * e) {
    webserver_add_handler(webserver, prefix, handler, templateEngine);
}
