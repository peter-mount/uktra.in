
/* 
 * Timetable API daemon
 */

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <area51/curl.h>
#include <area51/json.h>
#include <area51/log.h>
#include <area51/hashmap.h>
#include <area51/rest.h>
#include <area51/template.h>
#include <area51/webserver.h>
#include <uktrain/handlers.h>

int verbose = 0;

static WEBSERVER *webserver = NULL;
TemplateEngine *templateEngine = NULL;

static int about() {
    logconsole("Usage: timetabled [-ip4] [-ip6] [-p{port}]\n");
    return EXIT_FAILURE;
}
static const char *BASE = "/usr/share/uktrain/";
static char *base = NULL;

static int parseargs(WEBSERVER *webserver, int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        char *s = argv[i];
        if (s[0] == '-' && s[1] != 0) {
            if (strncmp("-ip4", s, 4) == 0)
                webserver_enableIPv4(webserver);
            else if (strncmp("-ip6", s, 4) == 0)
                webserver_enableIPv6(webserver);
            else switch (s[1]) {
                    case 'p':
                        if (s[2])
                            // Need to add this to api
                            webserver_setPort(webserver, atoi(&s[2]));
                        break;

                    case 'b':
                        base = &s[2];
                        break;

                    case 'v':
                        verbose++;
                        break;

                    default:
                        logconsole("Unsupported option %s", s);
                        return about();
                }
            /*
                    } else if (s[0]) {
                        if (database) {
                            return about();
                        }
                        database = s;
             */
        }
    }

    if (webserver_getPort(webserver) < 1 || webserver_getPort(webserver) > 65535)
        return about();

    if (!base)
        base = (char *) BASE;

    return 0;
}

/**
 * Dummy handler that acts like a Servlet filter in Java.
 * 
 * Here we add some request attributes to the current date/time of the request
 * @param r WEBSERVER_REQUEST
 * @return always MHD_NO
 */
static int handler(WEBSERVER_REQUEST *r) {
    struct tm tm;
    time_t now;
    char temp[16];

    time(&now);
    localtime_r(&now, &tm);

    // Year in YYYY format - used in the page footer most of the time
    snprintf(temp, 16, "%04d", tm.tm_year + 1900);
    webserver_setRequestAttribute(r, "date.year", strdup(temp), free);

    // Date in ISO format, ie YYYY-MM-DD
    snprintf(temp, 16, "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    webserver_setRequestAttribute(r, "date.iso", strdup(temp), free);

    // Time in ISO format, ie HH:MM:SS
    snprintf(temp, 16, "%02d:%02d:02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    webserver_setRequestAttribute(r, "date.time", strdup(temp), free);

    // Hour of the day, i.e. 0..23, used in some select elements
    snprintf(temp, 16, "%d", tm.tm_hour);
    webserver_setRequestAttribute(r, "date.hour", strdup(temp), free);

    // Always return MHD_NO to continue scanning for the correct action
    return MHD_NO;
}

int main(int argc, char** argv) {
    // We must run in UK time regardless of the underlying timezone
    // If we don't then we could show the wrong date/time's on forms
    putenv("TZ=Europe/London");

    curl_pool_init(10);

    webserver = webserver_new();
    if (!webserver) {
        logconsole(("Failed to initialise webserver"));
        return EXIT_FAILURE;
    }

    int rc = parseargs(webserver, argc, argv);
    if (rc)
        return rc;

    templateEngine = template_init(base);
    if (!templateEngine) {
        logconsole(("Failed to initialise template engine"));
        return EXIT_FAILURE;
    }

    webserver_set_defaults(webserver);

    // Add fields to request
    webserver_add_handler(webserver, "/*", handler, NULL);

    // ---- Station index
    webserver_add_handler(webserver, "/station", ukt_station_index, NULL);
    webserver_add_handler(webserver, "/station/", ukt_station_index, NULL);
    webserver_add_handler(webserver, "/station/*", ukt_station_details, templateEngine);

    // ---- Timetables
    ukt_timetable(webserver, templateEngine);
    ukt_timetable_search(webserver, templateEngine);
    ukt_timetable_schedule(webserver, templateEngine);

    // ---- Old cms ----
    TemplateEngine *cms = template_init("/var/www/uktra.in");
    // Handle old cms image requests
    template_addHandler_r(webserver, "/images/*", cms);

    // Handle old cms page requests - these get templated in to the skin
    addOldCMS(webserver, "/*", templateEngine, cms);
    // ---- Old cms ----

    // Must be last, any app static content
    template_addHandler(webserver, templateEngine);

    // Preload permanent templates
    //template_loadPermanent(templateEngine, "/css/main.css");

    //logconsole("Starting webserver on port %d", webserver->port);
    webserver_start(webserver);

    while (1) {
        sleep(60);
        logconsole("Tick");
    }
}
