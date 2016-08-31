/*
 * Display the timetable homepage
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

/**
 * Retrieves the index letters from the nrod-timetable microservice showing the
 * current values from the current timetable
 * 
 * @param out CharBuffer
 * @param active NULL or active index
 */
static void generateTiploc(WEBSERVER_REQUEST *request, struct json_object *obj) {
    char *title = NULL;
    CharBuffer *out = charbuffer_new();
    if (!out)
        return;

    charbuffer_append(out, "<table class=\"wikitable\">");
    charbuffer_append(out, "<tr><th colspan=\"5\">Network Rail Details</th></tr>");
    charbuffer_append(out, "<tr><th>Tiploc</th><th>CRS</th><th>Stanox</th><th>NLC</th><th>Description</th></tr>");

    struct array_list *list = json_object_get_array(obj);
    int len = array_list_length(list);
    for (int i = 0; i < len; i++) {
        json_object *ent = (json_object *) array_list_get_idx(list, i);

        charbuffer_append(out, "<tr><td>");
        char *t = json_getString(ent, "tiploc");
        if (t)
            charbuffer_append(out, t);

        charbuffer_append(out, "</td><td>");
        char *crs = json_getString(ent, "crs");
        if (crs)
            charbuffer_append(out, crs);

        charbuffer_append(out, "</td><td>");
        charbuffer_append_jsonInt(out, ent, "stanox");

        charbuffer_append(out, "</td><td>");
        charbuffer_append_jsonInt(out, ent, "nlc");

        charbuffer_append(out, "</td><td>");
        charbuffer_append_jsonStr(out, ent, "desc");

        charbuffer_append(out, "</td></tr>");

        if (!title && crs)
            title = json_getString(ent, "desc");
    }

    charbuffer_append(out, "</table>");

    char *text = charbuffer_toarray(out, &len);
    TemplateFile *temp = template_new(text, len, free);
    webserver_setRequestAttribute(request, "tiploc", temp, (void (*)(void*))template_free);

    // No title then take first one
    if (!title && len) {
        json_object *ent = (json_object *) array_list_get_idx(list, 0);
        title = json_getString(ent, "desc");
    }

    // If we found one, copy & camel case it for the page title
    if (title)
        title = strdup(title);
    if (title) {
        bool lower = false;
        for (char *p = title; *p; p++) {
            if (isalpha(*p) & lower)
                *p = tolower(*p);
            lower = isalpha(*p);
        }
        webserver_setRequestAttribute(request, "title", title, free);

        // Add the link back to the station index page
        if (*title) {
            charbuffer_reset(out);
            charbuffer_append(out, "<a href=\"/station?s=");
            charbuffer_add(out, toupper(title[0]));
            charbuffer_append(out, "\">Station Index</a>");
            charbuffer_add(out, 0);
            webserver_setRequestAttribute(request, "index.url", charbuffer_toarray(out, &len), free);
        }
    }

    charbuffer_free(out);
}

/**
 * Retrieves the stations for a letter of the alphabet from the nrod-timetable
 * microservice showing the values from the current timetable
 * 
 * @param out CharBuffer
 * @param active NULL or active index
 */
static int getStation(WEBSERVER_REQUEST *request, char *tpl) {
    CharBuffer *b = charbuffer_new();
    if (!b)
        return MHD_NO;

    char url[256];

    // get the tiploc - we want just the stanox field
    sprintf(url, "http://[::1]:9000/tiploc/tiploc/%s", tpl);
    logconsole("get %s", url);
    int ret = curl_get(url, b);
    if (ret) {
        logconsole("Failed station details %d", ret);
        charbuffer_free(b);
        return MHD_NO;
    }

    int len;
    char *json = charbuffer_tostring(b, &len);
    struct json_object *obj = json_tokener_parse(json);
    free(json);

    int stanox = json_getInt(obj, "stanox");
    if (stanox < 1) {
        logconsole("No stanox for %s", tpl);
        charbuffer_free(b);
        return MHD_NO;
    }
    json_object_put(obj);

    // Now via stanox to get all of them
    sprintf(url, "http://[::1]:9000/tiploc/stanox/%d", stanox);
    logconsole("get %s", url);
    ret = curl_get(url, b);
    if (ret) {
        logconsole("Failed station details %d", ret);
        charbuffer_free(b);
        return MHD_NO;
    }

    json = charbuffer_tostring(b, &len);
    obj = json_tokener_parse(json);
    free(json);

    // The tiploc detail panel
    generateTiploc(request, obj);

    // Timetable link to current hour
    struct tm tm;
    time_t now;
    time(&now);
    localtime_r(&now, &tm);
    charbuffer_reset(b);
    charbuffer_printf(b,
            "<a href=\"/timetable/station/%d/%04d-%02d-%02dT%02d\">Timetable</a>",
            stanox,
            tm.tm_year + 1900,
            tm.tm_mon + 1,
            tm.tm_mday,
            tm.tm_hour
            );
    webserver_setRequestAttribute(request, "timetable.url", charbuffer_toarray(b, &len), free);

    // Free the json object
    json_object_put(obj);

    charbuffer_free(b);
    return MHD_YES;
}

int ukt_station_details(WEBSERVER_REQUEST *request) {

    const char *url = webserver_getRequestUrl(request);

    // Find start of tiploc, i.e. /station/{tiploc}
    char *p = strstr(&url[1], "/");
    if (!p)
        return MHD_NO;
    p++;

    // Make certain tiploc is all letters & max 7 chars
    char tiploc[8];
    int i;
    for (i = 0; i < 7 && *p; i++) {
        if (!isalnum(*p))
            return MHD_NO;
        tiploc[i] = toupper(*p++);
    }
    tiploc[i] = 0;

    if (getStation(request, tiploc) != MHD_YES)
        return MHD_NO;

    TemplateEngine *templateEngine = webserver_getUserData(request);
    TemplateFile *body = template_get(templateEngine, "/station/detail.html");
    if (!body)
        return MHD_NO;
    webserver_setRequestAttribute(request, "body", body, (void (*)(void*))template_free);



    return render_template_name(request, templateEngine, NULL);
}
