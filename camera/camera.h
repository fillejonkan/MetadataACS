/*
 *  Wrapper functions around basic Axis Camera Application Platform SDK 2.x
 *
 *  Author: Pandos Dipp <pandos.dipp@gmx.com> 2014
 *
 * Copyright:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * - Please leave credits in the code, application and documentation.
 *
 */
 
#ifndef _CAMERA_H_
#define _CAMERA_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <glib.h>
#include <gio/gio.h>

typedef GDataOutputStream *CAMERA_HTTP_Reply;
typedef GHashTable		    *CAMERA_HTTP_Options;
typedef void              *CAMERA_EVENT_Options;

typedef void (*CAMERA_HTTP_callback) (const CAMERA_HTTP_Reply http,const CAMERA_HTTP_Options options);
typedef void (*CAMERA_PARAM_callback) ( const char *value );
typedef void (*CAMERA_EVENT_callback) ( const char* topic, const CAMERA_EVENT_Options options, int state );


void camera_init( const char* app_name_ID, const char* app_nicename);
void camera_cleanup();

int  camera_http_setCallback(const char *cgi_name, CAMERA_HTTP_callback theCallback);
void camera_http_sendXMLheader(CAMERA_HTTP_Reply http);
int  camera_http_output( CAMERA_HTTP_Reply http,const char *fmt, ...);
int  camera_http_send( CAMERA_HTTP_Reply http, size_t count, void *data );
void camera_http_sendBadRequest(CAMERA_HTTP_Reply http);
const char* camera_http_getOptionByName(CAMERA_HTTP_Options options,const char* name);
const char* camera_http_getOptionByIndex(CAMERA_HTTP_Options options,int option_index, char* key_return_value, int max_count);
            //Returns the pointer the value string or NULL if the index does not exist
            //Provide a key_return_value string pointer in order to get the parameter name.  The key_return_value may be NULL.
            //The max_count limites how many bytes will be copied to the key_return_value string

// EVENT FLAGS
#define EVENT_SIMPLE       0  // A puls event intended
#define EVENT_STATEFUL     1  // A stateful event with a duration.  E.g. "record video while active"
#define EVENT_APPLICATION  2  // The event or its data is intended for a specific application, not to trigger action rules 
#define EVENT_DEPRECATED   4  // The event is replaced by other event and may be removed in the future.
                              //    Applications should use the new event to trigger action rules

int  camera_event_add(const char* event_id,   // A unique name for the event. Use descriptive names
                      const char* event_name, // A nice name an application may display to the installer or operator
                      int event_flags,        // Use appropriate flags. See definutions for flags EVENT_XXXXXX.  
                      const char *data_id);   // The data attribute id name.  Set to null if no user data is passed
int  camera_event_send(const char* event_id,int value,const char* event_data);

int  camera_param_setCallback(const char* name, CAMERA_PARAM_callback theCallback);
const char* camera_param_get(const char* name, char *return_value, int max_count); //Returns the pointer to return_value or NULL if paramter does not exist
int  camera_param_set(const char* name,const char* value);

#ifdef  __cplusplus
}
#endif

#endif
