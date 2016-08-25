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

static const char *BASE = "/var/www/uktra.in";

// Don't allow any file begining with . to prevent people trying to scan filesystem
static const char *INVALID = "/.";

/**
 * Handles static content under /var/www. This handles checks for attempts by the client to gain access outside of
 * that directory, then hands the file to microhttpd to stream it back.
 * 
 * @param connection
 * @return 
 */
int oldCmsHandler(struct MHD_Connection * connection, WEBSERVER_HANDLER *handler, const char *url) {
    // Don't intercept lower case characters
    // . & / are invalid anyhow so ignore if first char
    // (remember url always starts with / hence char 1)
    if (!url || !url[0] || !url[1])
        return MHD_NO;

    // Validate url - i.e. must start with / and must not be a lower case character
    if (url[0] != '/' || islower(url[1]))
        return MHD_NO;

    // Validate url - cannot contain /. (also mean /.. as well)
    // This prevents people from scanning outside of /var/www using ../../ style url's
    if (findString((char *) url, (char *) INVALID))
        return MHD_NO;

    // Add "/cms/" 5, E 1 "/index.shtml" 12 = 18
    char path[strlen(url) + 1 + 18];
    strcpy(path, "/cms/");
    path[5] = url[1];
    path[6] = 0;
    strcat(path, url);
    strcat(path,"/index.shtml");

    logconsole("cms \"%s\"", path);

    return templateHandler(connection, handler, path);
}
