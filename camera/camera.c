#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <axsdk/axhttp.h>
#include <axsdk/axevent.h>
#include <axsdk/axparameter.h>
#include "camera.h"

#define LOG(fmt, args...)    { syslog(LOG_INFO, fmt, ## args); printf(fmt, ## args); }
#define LOG_ERROR(fmt, args...)    { syslog(LOG_CRIT, fmt, ## args); printf(fmt, ## args); }


AXHttpHandler  *handler_application_http = 0;
AXEventHandler *handler_application_event = 0;
AXParameter    *handler_application_param = 0;
GHashTable     *table_application_cgi = 0; 
GHashTable     *table_application_param = 0; 
GHashTable     *table_application_event = 0; 
gchar          *string_application_id = 0;
gchar          *string_application_name = 0;

typedef struct 
{
  guint declaration_id;
  int   value;
  int   flags;
  char  data_id[32];
} CAMERA_EVENT_PROPERTIES;

typedef struct 
{
  guint  declaration_id;
  char   topic[128];
  char   state_name[32];
  int   *state;
  CAMERA_EVENT_callback user_callback;
} CAMERA_EVENT_SUBSCRIPTION_PROPERTIES;



void camera_http_init();
void camera_event_init();
void camera_param_init();
void camera_http_cleanup();
void camera_event_cleanup();
void camera_param_cleanup();


void camera_init( const char* app_name_ID, const char* app_nicename)
{
  string_application_id = g_strdup( app_name_ID );
  string_application_name = g_strdup( app_nicename );
  
  camera_http_init();
  camera_event_init();
  camera_param_init();
}

void camera_cleanup()
{
  g_free( string_application_id );
  g_free( string_application_name );
  
  camera_http_cleanup();
  camera_event_cleanup();
  camera_param_cleanup();
}

int
camera_http_output(CAMERA_HTTP_Reply http,const gchar *fmt, ...)
{

  va_list ap;
  gchar *tmp_str;
  GDataOutputStream *stream = (GDataOutputStream *)http;
  if( stream == 0 ) {
    LOG_ERROR("Camera: Cannot send data to http.  Handler = NULL\n");
    return 0;
  }
  
  va_start(ap, fmt);

  tmp_str = g_strdup_vprintf(fmt, ap);
  if(!g_data_output_stream_put_string((GDataOutputStream *)http, tmp_str, NULL, NULL) )
    LOG_ERROR("camera_http_output: Could not send data\n");
  g_free(tmp_str);

  va_end(ap);
  return 1;
}

int
camera_http_send( CAMERA_HTTP_Reply http, size_t count, void *data )
{
  gsize data_sent;
  
  if( count == 0 || data == 0 ) {
    LOG_ERROR("Camera: Problem sending data on HTTP. Count = %d Data = NULL\n", count);
    return 0;
  }
  
  g_output_stream_write_all((GOutputStream *)http, data, count, &data_sent, NULL, NULL);

  if( data_sent < count ){
    LOG_ERROR("Could not send data to http.  %d bytes sent of %d\n", data_sent, count);
    return 0;
  }    
  return 1;
}

const char*
camera_http_getOptionByName(CAMERA_HTTP_Options options,const char* parameter)
{
  gchar *value;
  if(!g_hash_table_lookup_extended((GHashTable *)options,parameter, NULL, (gpointer*)&value))
    return 0;
  return value;   
}


const char*
camera_http_getOptionByIndex(CAMERA_HTTP_Options options,int option_index, char *key_return_value, int max_count)
{
  GHashTableIter iter;
  int i=0;
  gchar *key;
  gchar *table_value;

  g_hash_table_iter_init(&iter, (GHashTable *)options);
  while (g_hash_table_iter_next(&iter, (gpointer*)&key, (gpointer*)&table_value)) {
     if( i == option_index ) {
       if( key_return_value )
         g_strlcpy(key_return_value, key, max_count);
       return table_value;
     }
     i++;
  }
  return 0;
}

void
camera_http_sendXMLheader(CAMERA_HTTP_Reply http)
{
  camera_http_output( http,"Content-Type: text/xml; charset=utf-8; Cache-Control: no-cache\r\n\r\n");
  camera_http_output( http,"<?xml version=\"1.0\"?>\r\n");
}

void
camera_http_sendBadRequest(CAMERA_HTTP_Reply http)
{
  camera_http_output(http,
                   "HTTP/1.1 400 Bad Request\r\n"
                   "Content-Type: text/html\r\n\r\n"
                   "<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n"
                   "<BODY><H1>400 Bad Request</H1>\n"
                   "The request had bad syntax or was inherently impossible to be "
                   "satisfied.\n</BODY></HTML>\n");
}


static void
camera_main_cgi_callback(const gchar *path,const gchar *method,const gchar *query,GHashTable *params,GOutputStream *output_stream,gpointer user_data)
{
  GDataOutputStream *http;
  gchar *key;
  CAMERA_HTTP_callback    user_cgi = 0;
  http = g_data_output_stream_new(output_stream);
  g_hash_table_lookup_extended(table_application_cgi,
                               path,
                               (gpointer*)&key,
                               (gpointer*)&user_cgi);

//  LOG("API: %s?%s\n",path,query);
  if( user_cgi ) {
    user_cgi((CAMERA_HTTP_Reply)http, (CAMERA_HTTP_Options) params);
  }
  else {
    LOG_ERROR("Camera: Cannot locate handler for request %s?%s\n", path,query);
    camera_http_sendBadRequest( (CAMERA_HTTP_Reply)http );
  }

  g_object_unref(http);

  (void) user_data;
}

void
camera_http_init()
{
  if( !handler_application_http )
  {
    handler_application_http = ax_http_handler_new( camera_main_cgi_callback, NULL);
  }
}


int
camera_http_setCallback(const char *cgiPath,CAMERA_HTTP_callback theCallback)
{
  gchar path[64] = "";
  g_strlcat( path, "/local/", 64);
  g_strlcat( path, string_application_id, 64);
  g_strlcat( path, "/", 64);
  g_strlcat( path, cgiPath, 64);

  if( !handler_application_http )
    return 0;

  if( !table_application_cgi ) {
    table_application_cgi = g_hash_table_new_full(g_str_hash, g_str_equal,g_free, NULL);
  }
  g_hash_table_insert(table_application_cgi, g_strdup( path ), (gpointer*)theCallback);
  
  return 1;
}

void
camera_http_cleanup()
{
  if( handler_application_http ) {
    ax_http_handler_free( handler_application_http );
    handler_application_http = 0;
  }

  if( table_application_cgi ) {
    g_hash_table_destroy( table_application_cgi );
    table_application_cgi = 0;
  } 
}

void
camera_event_init()
{
  if( !handler_application_event )
    handler_application_event = ax_event_handler_new();
  
  if( !table_application_event )
    table_application_event = g_hash_table_new_full(g_str_hash, g_str_equal,g_free, g_free);
}

void
camera_event_cleanup()
{
  GHashTableIter iter;
  CAMERA_EVENT_PROPERTIES *props;
  gchar *event_name;

  if( table_application_event ) {

    g_hash_table_iter_init(&iter, table_application_event);
    while (g_hash_table_iter_next(&iter, (gpointer*)&event_name, (gpointer*)&props)) {
      ax_event_handler_undeclare( handler_application_event,props->declaration_id,NULL);
    }

    g_hash_table_destroy( table_application_event );
    table_application_event = 0;
  } 

  if( !handler_application_event ) {
    ax_event_handler_free( handler_application_event );
    handler_application_event = 0;
  }
}

int
camera_event_add(const char* event_id,
                      const char* event_name,
                      int         flags,
                      const char *data_id)
{
  AXEventKeyValueSet      *set = NULL;
  CAMERA_EVENT_PROPERTIES *props;

  if( !handler_application_event ) {
    LOG_ERROR("Camera: Cannot register event %s (handler not initialized)\n", event_name);
    return 0;
  }
  //Define the event properties (used by function camera_event_send() )
  props = (CAMERA_EVENT_PROPERTIES*)g_malloc(sizeof(CAMERA_EVENT_PROPERTIES));
  props->declaration_id = 0;
  props->flags = flags;
  props->value = 0;
  props->data_id[0] = 0;

  set = ax_event_key_value_set_new();

  ax_event_key_value_set_add_key_value(set,"topic0", "tnsaxis", "CameraApplicationPlatform", AX_VALUE_TYPE_STRING,NULL);
  ax_event_key_value_set_add_key_value(set,"topic1", "tnsaxis", string_application_id , AX_VALUE_TYPE_STRING,NULL);
  ax_event_key_value_set_add_nice_names(set,"topic1", "tnsaxis", string_application_id, string_application_name, NULL);
  
  ax_event_key_value_set_add_key_value(set,"topic2", "tnsaxis", event_id , AX_VALUE_TYPE_STRING,NULL);
  ax_event_key_value_set_add_nice_names(set,"topic2", "tnsaxis", event_id, event_name, NULL);

  if( flags & EVENT_APPLICATION ) 
    ax_event_key_value_set_mark_as_user_defined(set, "topic2", "tnsaxis", "isApplicationData", NULL);
  if( flags & EVENT_DEPRECATED ) 
    ax_event_key_value_set_mark_as_user_defined(set, "topic2", "tnsaxis", "isDeprecated", NULL);

  if( flags & EVENT_STATEFUL )
    ax_event_key_value_set_add_key_value(set,"active", NULL, &(props->value) , AX_VALUE_TYPE_BOOL,NULL);
  else
    ax_event_key_value_set_add_key_value(set,"active", NULL, &(props->value) , AX_VALUE_TYPE_INT,NULL);
  
  ax_event_key_value_set_mark_as_data(set, "active", NULL, NULL);

  if( data_id ) {
    strcpy( props->data_id, data_id );
    ax_event_key_value_set_add_key_value(set, data_id, NULL, "" , AX_VALUE_TYPE_STRING,NULL);
    ax_event_key_value_set_mark_as_data(set, data_id, NULL, NULL);
  }

  if (!ax_event_handler_declare(handler_application_event, set,
      !(props->flags & EVENT_STATEFUL),
      &props->declaration_id,
      NULL, //No callback when event is ready
      NULL, //No user data for event callback
      NULL))  //No error handler
  {
    LOG_ERROR("Camera: Cannot declare event %s (internal error)\n",event_id);
    g_free( props );
    return 0;
  }
  g_hash_table_insert(table_application_event, g_strdup( event_id ), (gpointer*)props);
  return 1;
}

int
camera_event_send(const char* event_id,int value, const char* event_data)
{
  AXEventKeyValueSet       *set = NULL;
  AXEvent                  *event = NULL;
  CAMERA_EVENT_PROPERTIES  *props;
  GTimeVal                  time_stamp;
  
  if( !handler_application_event ) {
    LOG_ERROR("Camera: Cannot send event %s. Handler is not initialized\n", event_id);
    return 0;
  }
  
  if( !table_application_event ) {
    LOG_ERROR("Camera: Cannot send event %s. No events are registered\n", event_id);
    return 0;
  }

  if(!g_hash_table_lookup_extended(table_application_event,event_id, NULL, (gpointer*)&props)) {
    LOG_ERROR("Camera: Cannot send event %s.  Event is not registered\n", event_id);
    return 0;
  }

  if( props == 0 ) {
    LOG_ERROR("Camera: Cannot send event %s. Event properties = NULL\n", event_id);
    return 0;
  }

  set = ax_event_key_value_set_new();
  props->value = value;

  if( props->flags & EVENT_STATEFUL ) {  //Non stateful events use INT type
    if( !ax_event_key_value_set_add_key_value(set,"active",NULL, &props->value,AX_VALUE_TYPE_BOOL,NULL) ) {
      ax_event_key_value_set_free(set);
      LOG_ERROR("Camera: Could not send event %s.  Internal error\n", event_id);
    }
  } else {
    if( !ax_event_key_value_set_add_key_value(set,"active",NULL, &props->value,AX_VALUE_TYPE_INT,NULL) ) {
      ax_event_key_value_set_free(set);
      LOG_ERROR("Camera: Could not send event %s.  Internal error\n", event_id);
    }
  }

  if( event_data && strlen( props->data_id ) ) {
    if( !ax_event_key_value_set_add_key_value(set,props->data_id, NULL, event_data ,AX_VALUE_TYPE_STRING, NULL) )  {
      ax_event_key_value_set_free(set);
      LOG_ERROR("Camera: Could not send event %s.  Internal error\n", event_id);
    }
  }

  g_get_current_time(&time_stamp);
  event = ax_event_new(set, &time_stamp);
  ax_event_key_value_set_free(set);

  if( !ax_event_handler_send_event(handler_application_event, props->declaration_id, event, NULL) )  {
    LOG_ERROR("Camera: Could not send event %s (id %d)\n", event_id, props->declaration_id);
    ax_event_free(event);
    return 0;
  }
  ax_event_free(event);
  return 1;
}

void
camera_param_init()
{
  if( !handler_application_param )
  {
    handler_application_param = ax_parameter_new( string_application_id, NULL);
  }

  if( !table_application_param ) {
    table_application_param = g_hash_table_new_full(g_str_hash, g_str_equal,NULL, NULL);
  }

}

void
camera_param_cleanup()
{
  if( handler_application_param ) {
	  ax_parameter_free( handler_application_param );
  }

  if( table_application_param ) {
    g_hash_table_destroy( table_application_param );
    table_application_param = 0;
  } 

  handler_application_param = 0;
}


static void
camera_main_parameter_callback(const gchar *param_name, const gchar *value, gpointer data)
{
  gchar *key;
  gchar *search_key;
  CAMERA_PARAM_callback    user_callback = 0;

  search_key = g_strrstr_len(param_name,32,".");
  
  if( search_key ) {
	  search_key++;
  } else {
    LOG_ERROR("Camera: Cannot dispatch parameter update %s=%s (internal error)\n", param_name, value);
  }
  
  g_hash_table_lookup_extended(table_application_param,
                               search_key,
                               (gpointer*)&key,
                               (gpointer*)&user_callback);
 
  if( user_callback ) {
     user_callback(value);
  }
  else {
    LOG_ERROR("Camera: Cannot dispatch parameter update %s=%s (internal error)\n", param_name, value);
  }
}

int
camera_param_setCallback(const char* name, CAMERA_PARAM_callback theCallback)
{

  if( !handler_application_param ) {
	  LOG_ERROR("Camera: Cannot register callback for %s (handler not initialized)\n", name);
	  return 0;
  }

  if( !ax_parameter_register_callback( handler_application_param, name, camera_main_parameter_callback, NULL, NULL) ) {
	  LOG_ERROR("Camera: Cannot register callback for %s (internal error)\n", name);
  }

  g_hash_table_insert(table_application_param, (gpointer*)name, (gpointer*)theCallback);

  return 1;
}

const char*
camera_param_get(const char* param_name, char* value, int max_count)
{
  gchar  *param_value = NULL;

  if( !handler_application_param ) {
	  LOG_ERROR("Camera: Cannot get parameter %s (handler not initialized)\n", param_name);
	  return 0;
  }

  if (!ax_parameter_get(handler_application_param, param_name, &param_value, NULL)) {
	  LOG_ERROR("Camera: Cannot get parameter %s (internal errro)\n", param_name);
	  value[0]=0;
	  return 0;
  }
  g_strlcpy(value, param_value, max_count);
  g_free( param_value);

  return value;
}

int
camera_param_set(const char* param_name,const char* value)
{
  gchar *key;
  CAMERA_PARAM_callback    user_callback = 0;
  if( !handler_application_param ) {
    LOG_ERROR("Camera: Cannot set parameter %s=%s (handler not initialized)\n", param_name, value);
    return 0;
  }
  
  if (!ax_parameter_set(handler_application_param, param_name , value, TRUE, NULL)) {
    LOG_ERROR("Camera: Cannot set parameter %s=%s (internal error)\n", param_name, value);
    return 0;
  }

  if( !table_application_param ) {
    LOG_ERROR("Camera: Cannot set parameter %s=%s (internal list)\n", param_name, value);
    return 0;
  }

  g_hash_table_lookup_extended(table_application_param,
                               param_name,
                               (gpointer*)&key,
                               (gpointer*)&user_callback);

  if( user_callback ) {
     user_callback(value);
  }
  else {
    LOG_ERROR("Camera: Cannot dispatch updated parameter %s=%s (internal error)\n", param_name, value);
    return 0;
  }
 
  return 1;
}
