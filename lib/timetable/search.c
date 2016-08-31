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

int ukt_timetable_search(WEBSERVER_REQUEST *request) {

    TemplateEngine *e = webserver_getUserData(request);
    TemplateFile *f = template_get(e,"/timetable/search.html");
    if(!f)
        return MHD_NO;
    
    webserver_setRequestAttribute(request, "title", "Timetable Search", NULL);
    webserver_setRequestAttribute(request, "javascript", "/timetable/javascript.html", NULL);
    webserver_setRequestAttribute(request, "body", f, (void (*)(void*))template_free);

    return render_template_name(request, templateEngine, NULL);
}
