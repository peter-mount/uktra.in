/*
 * Display the timetable search page
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <area51/template.h>
#include <area51/webserver.h>
#include <string.h>
#include <ctype.h>
#include "uktrain/handlers.h"

/**
 * Show the search form
 * @param request
 * @return 
 */
static int handler(WEBSERVER_REQUEST *request) {
    TemplateEngine *e = webserver_getUserData(request);
    TemplateFile *f = template_get(e, "/timetable/search.html");
    if (!f)
        return MHD_NO;

    webserver_setRequestAttribute(request, "title", "Timetable Search", NULL);
    webserver_setRequestAttribute(request, "javascript", "/timetable/javascript.html", NULL);
    webserver_setRequestAttribute(request, "body", f, (void (*)(void*))template_free);

    return render_template_name(request, templateEngine, NULL);
}

int ukt_timetable(WEBSERVER *webserver, TemplateEngine *e) {
    webserver_add_handler(webserver, "/timetable/", handler, templateEngine);
    webserver_add_handler(webserver, "/timetable", handler, templateEngine);

}
