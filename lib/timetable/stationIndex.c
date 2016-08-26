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
#include <area51/template.h>
#include <area51/webserver.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <curl/curl.h>
#include "uktrain/handlers.h"
#include <area51/trace.h>

static void getIndexPane(CharBuffer *out) {
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
            charbuffer_append(out, "<th style=\"width:1.5em;\"><a href=\"/station/?s=");
            charbuffer_append(out, (char *) l);
            charbuffer_append(out, "\">");
            charbuffer_append(out, (char *) l);
            charbuffer_append(out, "</a></th>");
        }
        charbuffer_append(out, "</tr>");
    }

    charbuffer_free(b);
}

int ukt_station_index(WEBSERVER_REQUEST *request) {
    CharBuffer *out = charbuffer_new();

    charbuffer_append(out, "<table class=\"wikitable\"><thead>");
    getIndexPane(out);
    charbuffer_append(out, "<tr><th colspan=\"2\">CRS</th><th colspan=\"2\">Stanox</th><th colspan=\"14\">Tiploc</th><th colspan=\"16\">Station name</th></tr>");
    charbuffer_append(out, "</thead><tbody>");

    charbuffer_append(out, "</tbody></table>");

    int len;
    char *text = charbuffer_toarray(out, &len);
    charbuffer_free(out);
    logconsole("%3d %s", len, text);

    TemplateFile *temp = template_new(text, len, free);

    webserver_setRequestAttribute(request, "body", temp, (void (*)(void*))template_free);
    webserver_setRequestAttribute(request, "title", "Station Index", NULL);

    return render_template_name(request, templateEngine);
}
