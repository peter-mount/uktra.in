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

/**
 * Retrieves the index letters from the nrod-timetable microservice showing the
 * current values from the current timetable
 * 
 * @param out CharBuffer
 * @param active NULL or active index
 */
static void getIndexList(CharBuffer *out, char *active) {
    CharBuffer *b = charbuffer_new();
    if (!b)
        return;

    int ret = curl_get("http://[::1]:9000/station", b);
    if (ret)
        logconsole("Failed station index %d", ret);
    else {
        int len;
        char *json = charbuffer_tostring(b, &len);
        struct json_object *obj = json_tokener_parse(json);
        free(json);

        struct array_list *list = json_object_get_array(obj);
        len = array_list_length(list);

        charbuffer_append(out, "<tr>");
        for (int i = 0; i < len; i++) {
            json_object *ent = (json_object *) array_list_get_idx(list, i);

            const char *l = json_object_get_string(ent);
            bool act = strcmp(l, active) == 0;

            charbuffer_append(out, "<th style=\"width:1.5em;\"");
            if (act)
                charbuffer_append(out, " class=\"active\">");
            else {
                charbuffer_append(out, "><a href=\"/station?s=");
                charbuffer_append(out, (char *) l);
                charbuffer_append(out, "\">");
            }

            charbuffer_append(out, (char *) l);

            if (!act)
                charbuffer_append(out, "</a>");

            charbuffer_append(out, "</th>");
        }
        charbuffer_append(out, "</tr>");
    }

    charbuffer_free(b);
}

/**
 * Retrieves the stations for a letter of the alphabet from the nrod-timetable
 * microservice showing the values from the current timetable
 * 
 * @param out CharBuffer
 * @param active NULL or active index
 */
static void getStations(CharBuffer *out, char *active) {
    CharBuffer *b = charbuffer_new();
    if (!b)
        return;

    char *url = genurl("http://[::1]:9000/station/", active);
    if (!url) {
        charbuffer_free(b);
        return;
    }

    int ret = curl_get(url, b);
    free(url);
    if (ret)
        logconsole("Failed station index %d", ret);
    else {
        int len;
        char *json = charbuffer_tostring(b, &len);
        struct json_object *obj = json_tokener_parse(json);
        free(json);

        struct array_list *list = json_object_get_array(obj);
        len = array_list_length(list);

        for (int i = 0; i < len; i++) {
            json_object *ent = (json_object *) array_list_get_idx(list, i);

            const char *l = json_object_get_string(ent);
            bool act = strcmp(l, active) == 0;

            charbuffer_append(out, "<tr><td colspan=\"4\">");
            char *tiploc = json_getString(ent, "tiploc");
            if (tiploc) {
                // No stanox then no station detail page
                if (json_isNull(ent, "stanox"))
                    charbuffer_append(out, tiploc);
                else {
                    charbuffer_append(out, "<a href=\"/station/");
                    charbuffer_append(out, tiploc);
                    charbuffer_append(out, "\">");
                    charbuffer_append(out, tiploc);
                    charbuffer_append(out, "</a>");
                }
            }

            charbuffer_append(out, "</td><td colspan=\"2\">");
            if (!json_isNull(ent, "crs"))
                charbuffer_append(out, json_getString(ent, "crs"));

            charbuffer_append(out, "</td><td align=\"right\" colspan=\"2\">");
            if (!json_isNull(ent, "stanox"))
                charbuffer_append_int(out, json_getInt(ent, "stanox"), 0);

            charbuffer_append(out, "</td><td colspan=\"16\">");
            if (!json_isNull(ent, "desc"))
                charbuffer_append_jsonStr(out, ent, "desc");

            charbuffer_append(out, "</td></tr>");
        }

        // Free the json object
        json_object_put(obj);
    }

    charbuffer_free(b);
}

int ukt_station_index(WEBSERVER_REQUEST *request) {

    char active[2];
    const char *paramS = webserver_getRequestParameter(request, "s");
    active[0] = paramS && isalpha(*paramS) ? toupper(*paramS) : 'A';
    active[1] = 0;

    CharBuffer *out = charbuffer_new();

    charbuffer_append(out, "<table class=\"wikitable wtt\"><thead>");
    getIndexList(out, active);
    charbuffer_append(out, "<tr><th colspan=\"4\" rowspan=\"2\">Tiploc</th><th colspan=\"4\" class=\"wttwork\">Network Rail</th><th colspan=\"16\" rowspan=\"2\">Station name</th></tr>");
    charbuffer_append(out, "<tr><th colspan=\"2\" class=\"wttwta\">CRS</th><th colspan=\"2\" class=\"wttwtp\">Stanox</th></tr>");
    charbuffer_append(out, "</thead><tbody>");
    getStations(out, active);
    charbuffer_append(out, "</tbody></table>");

    int len;
    char *text = charbuffer_toarray(out, &len);
    charbuffer_free(out);

    TemplateFile *temp = template_new(text, len, free);

    webserver_setRequestAttribute(request, "body", temp, (void (*)(void*))template_free);
    webserver_setRequestAttribute(request, "title", "Station Index", NULL);

    return render_template_name(request, templateEngine, NULL);
}
