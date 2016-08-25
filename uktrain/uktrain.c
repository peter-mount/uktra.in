
/* 
 * Timetable API daemon
 */

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <area51/json.h>
#include <area51/log.h>
#include <area51/hashmap.h>
#include <area51/rest.h>
#include <area51/template.h>
#include <area51/webserver.h>
#include <uktrain/handlers.h>

int verbose = 0;

static int about() {
    logconsole("Usage: timetabled [-ip4] [-ip6] [-p{port}]\n");
    return EXIT_FAILURE;
}
static const char *BASE = "/usr/share/uktrain/";
static char *base = NULL;

static int parseargs(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        char *s = argv[i];
        if (s[0] == '-' && s[1] != 0) {
            if (strncmp("-ip4", s, 4) == 0)
                webserver_enableIPv4();
            else if (strncmp("-ip6", s, 4) == 0)
                webserver_enableIPv6();
            else switch (s[1]) {
                    case 'p':
                        if (s[2])
                            webserver.port = atoi(&s[2]);
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

    if (webserver.port < 1 || webserver.port > 65535)
        return about();

    if (!base)
        base = (char *) BASE;

    return 0;
}

int main(int argc, char** argv) {

    webserver_initialise();

    int rc = parseargs(argc, argv);
    if (rc)
        return rc;

    if (template_init(base)) {
        logconsole(("Failed to initialise template engine"));
        return EXIT_FAILURE;
    }

    webserver_set_defaults();

    // Handle old cms requests
    webserver_add_handler("/*", oldCmsHandler);

    // Must be last
    webserver_add_handler("/*", templateHandler);

    // Preload permanent templates
    template_loadPermanent("/css/main.css");

    logconsole("Starting webserver on port %d", webserver.port);
    webserver_start();

    while (1) {
        sleep(60);
        logconsole("Tick");
    }
}
