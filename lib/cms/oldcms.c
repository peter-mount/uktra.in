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
#include "uktrain/handlers.h"

struct ctx {
    TemplateEngine *main;
    TemplateEngine *cms;
};

static TemplateFile *lookup(char *n, void *c) {
    struct ctx *ctx = c;
    TemplateFile *f = template_get(ctx->main, n);
    if (!f)
        f = template_get(ctx->cms, n);
    return f;
}

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
int handler(WEBSERVER_REQUEST *request) {
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

    struct ctx *ctx = webserver_getUserData(request);

    // Get the content
    TemplateFile *f = template_get(ctx->cms, path);
    if (!f)
        return MHD_NO;

    // The layout
    TemplateFile *body = template_get(ctx->main, "/layout/body.html");
    if (!body) {
        template_free(f);
        return MHD_NO;
    }

    // Render
    webserver_setRequestAttribute(request, "title", (char *) &url[1], NULL);
    webserver_setRequestAttribute(request, "body", f, (void (*)(void *))template_free);

    CharBuffer *b = template_render(request, lookup, ctx, body);
    template_free(body);

    if (!b)
        return MHD_NO;

    int len = 0;
    void *buf = charbuffer_getBuffer(b, &len);
    charbuffer_free(b);

    struct MHD_Response *response = MHD_create_response_from_buffer(len, buf, MHD_RESPMEM_MUST_FREE);
    return webserver_queueResponse(request, &response);
}

int addOldCMS(WEBSERVER *s, const char *prefix, TemplateEngine *main, TemplateEngine *cms) {
    struct ctx *ctx = malloc(sizeof (struct ctx));
    if (!ctx)
        return EXIT_FAILURE;

    memset(ctx, 0, sizeof (struct ctx));
    ctx->main = main;
    ctx->cms = cms;
    webserver_add_handler(s, prefix, handler, ctx);
    return EXIT_SUCCESS;
}