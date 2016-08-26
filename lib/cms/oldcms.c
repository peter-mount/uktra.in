/*
 * Based on static handler, handles the old style CMS pages.
 * 
 * This handler works by capturing all requests for /X* where X is an
 * upper case letter or any other non-letter character
 */
#include <microhttpd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <area51/string.h>
#include <area51/webserver.h>
#include <area51/template.h>

/**
 * Handles static content stored in the old CMS format.
 * 
 * This format takes a file, /Excuses (always starts with an upper case character
 * or digit) and converts that to /E/Excuses/index.shtml which is then read from
 * the filesystem (in this instance a TemplateEngine set to the directory
 * containing the local mirror of the static content).
 * 
 * @param connection
 * @return 
 */
int oldCmsHandler(WEBSERVER_REQUEST *request) {
    const char *url = webserver_getRequestUrl(request);

    // Don't intercept lower case characters
    if (!webserver_isUrlValid(request) || !url[1] || islower(url[1]))
        return MHD_NO;

    // Add /E 2 "/index.shtml" 12 = 14
    char path[strlen(url) + 1 + 14];
    path[0] = '/';
    path[1] = url[1];
    path[2] = 0;
    strcat(path, url);
    strcat(path, "/index.shtml");

    TemplateEngine *e = webserver_getUserData(request);
    TemplateFile *f = template_get(e, path);
    if (!f)
        return MHD_NO;

    return template_respondTemplate(request, f);
}
