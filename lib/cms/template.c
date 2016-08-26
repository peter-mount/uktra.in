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
#include <area51/memory.h>
#include <area51/string.h>
#include <area51/webserver.h>
#include <area51/template.h>
#include "uktrain/handlers.h"

static TemplateFile *lookup(char *n, void *c) {
    TemplateEngine *e = c;
    return template_get(e, n);
}

int render_template_name(WEBSERVER_REQUEST *request, TemplateEngine *e) {

    // The layout
    TemplateFile *body = template_get(e, "/layout/body.html");
    if (!body)
        return MHD_NO;

    CharBuffer *b = template_render(request, lookup, e, body);
    template_free(body);

    if (!b)
        return MHD_NO;

    int len = 0;
    void *buf = charbuffer_getBuffer(b, &len);
    charbuffer_free(b);

    struct MHD_Response *response = MHD_create_response_from_buffer(len, buf, MHD_RESPMEM_MUST_FREE);
    return webserver_queueResponse(request, &response);
}
