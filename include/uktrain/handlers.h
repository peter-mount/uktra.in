/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   handlers.h
 * Author: peter
 *
 * Created on 25 August 2016, 08:49
 */

#ifndef UKTRAIN_HANDLERS_H
#define UKTRAIN_HANDLERS_H

#include <microhttpd.h>
#include <area51/memory.h>
#include <area51/template.h>
#include <area51/webserver.h>

#ifdef __cplusplus
extern "C" {
#endif
    extern TemplateEngine *templateEngine;

    extern int addOldCMS(WEBSERVER *, const char *, TemplateEngine *, TemplateEngine *);

    extern int render_template_name(WEBSERVER_REQUEST *, TemplateEngine *, char *);

    extern int ukt_station_index(WEBSERVER_REQUEST *);
    
#ifdef __cplusplus
}
#endif

#endif /* UKTRAIN_HANDLERS_H */

