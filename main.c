#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>

#include <syslog.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <axsdk/axevent.h>
#include <axoverlay.h>

#include "metadata_pair.h"
#include "overlay.h"
#include "camera/camera.h"
#include "acs.h"
#include "debug.h"


/** @mainpage Metadata ACS Overview
 *
 * @section intro_sec Introduction
 *
 * The purpose of the application is to subscribe to one user configurable
 * analytic with Metadata.
 *
 * @section Architecture
 *
 * This is a fairly simple application that has a web page which will list
 * all available ACAP VAPIX event containing usable data to push to ACS.
 *
 * User can select which event / category to subscribe to and what data items
 * to push to ACS using a series of dropdown menus.
 *
 * The user can furthermore configure all of the necessary parameters for the
 * ACS communication. Server Address, Source Key ID etc, and there are some
 * functions to help test the reporting chain as well.
 *
 * There is one main HTML file containing the HTML and JavaScript handling logic
 * with populating the dropdowns etc and a few C files handling the ACAP
 * framework stuff and ACS interface.
 *
 *
 * @subsection file_sec Files
 *
 * main.c is the main ACAP entry point and handles event callbacks, parameters
 * etc.
 *
 * It will use metadata_push.c to interface with ACS and push the event
 * data in to the external data search engine with the correct JSON format.
 *
 * debug.c is a small file that handles enabling / disabling of dynamic logging.
 *
 * @subsection Application Parameters
 *
 * - ServerAddress IP address of the ACS Server.
 *
 * - SourceID      Configured Source Key ID in ACS.
 *
 * - Username      Username for ACS Server.
 *
 * - Password      Password for ACS Server.
 *
 * - Enabled       Toggle reporting on / off. Event subscription still active.
 *
 * - Analytic      Selection of Analytic category. E.g. FenceGuard.
 *
 * - Category      Seclection of Subcategory. E.g. Camera1Profile1.
 *                "Uncategorized" is magic and used for ACAPs with no
 *                subcategory.
 *
 * - Items         Semi-colon separated and terminated list of data items.
 *                E.g. plate;description;country;
 *
 * - DebugEnabled    = "no" type="bool:no,yes"
 *
 * @subsection CGIs
 *
 * - settings/testreporting Sends a test command to ACS with the current
 *                         configured data items. Check if command is received
 *                         OK and if not indicate reason to user.
 *
 * - settings/get           Get all of the parameters in one CGI call.
 *
 */

/** @file main.c
 * @Brief Main application framework.
 *
 * Main ACAP framework. Handle user params, CGI, event subscription etc.
 * Distribute accordingly to other entities.
 */

/******************** MACRO DEFINITION SECTION ********************************/

/**
 * APP ID used for logs etc.
 */
#define APP_ID              "MetadataACS"

/**
 * Nice name used for App
 */
#define APP_NICE_NAME       "MetadataACS"

/**
 * Max number of metadata items
 */
#define MAX_ITEMS (20)

/******************** LOCAL VARIABLE DECLARATION SECTION **********************/

/**
 * Main context for GLib.
 */
static GMainLoop *loop;

/**
 * Handle for ACAP event subsystem
 */
static AXEventHandler *event_handler;

/**
 * Subscription ID for zone crossing alarm.
 */
static int event_subscription_id = -1;

/**
* Extra debug logging enabled or not
*/
static char *par_debug_enabled = NULL;

/**
* Selected analytic
*/
static char *par_analytic = NULL;

/**
* Analytic category
*/
static char *par_category = NULL;

/**
* Selected data items on which to report.
*/
static char *par_items = NULL;

/**
* Selected data items on which to report.
*/
static char *par_filter = NULL;

/**
* Handle for metdata push instance
*/
static acs_handle acs            = NULL;

/**
* Handle for overlay instance
*/
static overlay_handle ovl_handle = NULL;

/**
* List with current metadata information being pushed out
*/
static GList *cur_metadata_items = NULL;

/******************** LOCAL FUNCTION DECLARATION SECTION **********************/

/**
 * Callback Function for Metadata events.
 *
 * @param subscription The subscription ID for the event/
 * @param event        The AXEvent instance where we can get data etc.
 * @param token        Unused user_token that is demanded by event API.
 *
 * @return No return value.
 */
static void metadata_event_callback(guint subscription, AXEvent *event,
                                    guint *token);

/**
 * Subscribe to the Metadata event.
 *
 * @return Event ID handle for the subscription.
 */
static guint metadata_event_subscribe();

/**
 * Quit the application when terminate signals is being sent.
 *
 * @param signo Unix signal number.
 *
 * @return No return value.
 */
static void handle_sigterm(int signo);

/**
 * Register callback to SIGTERM and SIGINT signals.
 *
 * @return No return value.
 */
static void init_signals();

/**
 * Callback function for changes to Server Address parameter.
 *
 * @param value The new value for Server Addres.
 *
 * @return No return value.
 */
static void set_server_address(const char *value);

/**
 * Callback function for changes to source ID parameter
 *
 * @param value The new value for Source ID
 *
 * @return No return value.
 */
static void set_source_id(const char *value);

/**
 * Callback function for changes to Username parameter
 * update ACS API credentials if needed.
 *
 * @param value The new value for Username.
 *
 * @return No return value.
 */
static void set_username(const char *value);

/**
 * Callback function for changes to Password parameter
 * update ACS API credentials if needed.
 *
 * @param value The new value for Password.
 *
 * @return No return value.
 */
static void set_password(const char *value);

/**
 * Callback function for Enabled parameter.
 *
 * @param value The new value for Enabled.
 *
 * @return No return value.
 */
static void set_enabled(const char *value);

/**
 * Callback function for Analytic parameter.
 *
 * @param value The new value for Analytic.
 *
 * @return No return value.
 */
static void set_analytic(const char *value);

/**
 * Callback function for Category parameter.
 *
 * @param value The new value for Category.
 *
 * @return No return value.
 */
static void set_category(const char *value);

/**
 * Callback function for Items parameter.
 *
 * @param value The new value for Items.
 *
 * @return No return value.
 */
static void set_items(const char *value);

/**
 * Callback function for Content Filter
 *
 * @param value The new value for Content Filter
 *
 * @return No return value.
 */
static void set_filter(const char *value);

/**
 * Callback function debug enabled parameter. This is used to dynamically
 * enable / disable extra debug printing.
 *
 * @param value The new value for DebugEnabled.
 *
 * @return No return value.
 */
static void set_debug_enabled(const char *value);

/**
 * CGI callback function for testing the reporting chain.
 *
 * @param http    HTTP_Reply object to use for sending response.
 * @param options Unused HTTP options parameter required by API.
 *
 * @return No return value.
 */
static void cgi_test_reporting(CAMERA_HTTP_Reply http,
                               CAMERA_HTTP_Options options);

/**
 * CGI function for getting all parameters at once.
 *
 * @param http    HTTP_Reply object to use for sending response.
 * @param options Unused HTTP options parameter required by API.
 *
 * @return No return value.
 */
static void cgi_settings_get(CAMERA_HTTP_Reply http,
                             CAMERA_HTTP_Options options);

/**
 * Build a key-value par list of metadata items that can then be sent
 * to the different reporting tools like ACS and overlay. Uses the items
 * parameter to determine what items to get.
 *
 * @param key_value_set The set from which to get the item values
 * @param list Return location for the list of mdp_item_pair
 *
 * @return No return value.
 */
static gboolean build_metadata_items(const AXEventKeyValueSet *key_value_set,
                                     GList **list);

/******************** LOCAL FUNCTION DEFINTION SECTION ************************/

/**
 * Callback Function for Metadata events. Avoid blocking here so
 * use async setting when sending data to ACS.
 */
static void metadata_event_callback(guint subscription,
    AXEvent *event, guint *token)
{
    (void) token;

    if (event == NULL) {
        return;
    }

    const AXEventKeyValueSet *key_value_set = ax_event_get_key_value_set(event);

    if (key_value_set == NULL) {
        goto cleanup;
    }

    DBG_LOG("Got event %s/%s event to push to ACS", par_analytic,
        par_category);

    GList *metadata_items = NULL;
    gboolean ret = build_metadata_items(key_value_set, &metadata_items);

    if (ret == FALSE) {
        LOG("Failed to get metadata items");
        goto cleanup;
    }

    /**
     * Trigger sending of metadata.
     */
    (void) acs_run(acs, metadata_items, NULL);

    overlay_set_data(ovl_handle, metadata_items, 3000,
        par_analytic, par_category);

cleanup:
    if (metadata_items != NULL) {
        mdp_destroy_list(&cur_metadata_items);
        cur_metadata_items = metadata_items;
    }
    /* Free the event as specified in SDK Documentation. */
    ax_event_free(event);
}

/**
 * Subscribe to the specified event.
 */
static guint metadata_event_subscribe()
{
    AXEventKeyValueSet *key_value_set;
    guint subscription;
    gboolean result;

    if (g_strcmp0(par_analytic, " ") == 0) {
        DBG_LOG("No analytic configured, skip event subscription");
        return 0;
    }

    key_value_set = ax_event_key_value_set_new();

    DBG_LOG("Subscribing to event");

    /*
     * Create key-value set subscibing to the event.
     */
    ax_event_key_value_set_add_key_value(key_value_set,
        "topic0", "tnsaxis", "CameraApplicationPlatform", AX_VALUE_TYPE_STRING,
        NULL);

    ax_event_key_value_set_add_key_value(key_value_set,
        "topic1", NULL, par_analytic, AX_VALUE_TYPE_STRING,
        NULL);

    if (g_strcmp0(par_category, "") != 0 &&
        g_strcmp0(par_category, "Uncategorized") != 0) {
        ax_event_key_value_set_add_key_value(key_value_set,
        "topic2", NULL, par_category, AX_VALUE_TYPE_STRING,
        NULL);
    } else {
        DBG_LOG("NOT Adding Uncategorized");
    }

    /* Time to setup the subscription. Use the "token" input argument as
    * input data to the callback function "subscription callback"
    */
    result = ax_event_handler_subscribe(event_handler, key_value_set,
        &subscription, (AXSubscriptionCallback)metadata_event_callback, NULL,
        NULL);

    if (!result) {
        ERR("Failed to subscribe to event");
    } else {
        DBG_LOG("Subscribed to event");
    }

    /* The key/value set is no longer needed */
    ax_event_key_value_set_free(key_value_set);

    return subscription;
}

/**
 * Quit the application when terminate signals is being sent.
 */
static void handle_sigterm(int signo)
{
    LOG("GOT SIGTERM OR SIGINT, EXIT APPLICATION");

    if (loop) {
        g_main_loop_quit(loop);
    }
}

/**
 * Register callback to SIGTERM and SIGINT signals.
 */
static void init_signals()
{
    struct sigaction sa;
    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
}

/**
 * Callback function for changes to ServerAddress parameter
 * update ACS API settings.
 */
static void set_server_address(const char *value)
{
    DBG_LOG("Got new Server Address %s", value);
    acs_set_ipname(acs, value);
}

/**
 * Callback function for changes to Source ID parameter
 * update ACS API settings.
 */
static void set_source_id(const char *value)
{
    DBG_LOG("Got new Source ID %s", value);
    acs_set_source(acs, value);
}

/**
 * Callback function for changes to Username parameter
 * update ACS API settings.
 */
static void set_username(const char *value)
{
    DBG_LOG("Got new Username %s", value);
    acs_set_username(acs, value);
}

/**
 * Callback function for changes to Password parameter
 * update ACS API settings.
 */
static void set_password(const char *value)
{
    DBG_LOG("Got new Password %s", value);
    acs_set_password(acs, value);
}

/**
 * Callback function for Enabled parameter.
 */
static void set_enabled(const char *value)
{
    DBG_LOG("Got new Enabled %s", value);
    acs_set_enabled(acs, value);
}

/**
 * Callback function for Analytic parameter. Update event subscription
 * if needed.
 */
static void set_analytic(const char *value)
{
    if (g_strcmp0(value, par_analytic) != 0) {
        DBG_LOG("Got new Analytic %s", value);
        g_free(par_analytic);
        par_analytic = g_strdup(value);

        ax_event_handler_unsubscribe(event_handler,
            event_subscription_id, NULL);

        event_subscription_id = metadata_event_subscribe();
    }
}

/**
 * Callback function for Category parameter. Update event subscription
 * if needed.
 */
static void set_category(const char *value)
{
    if (g_strcmp0(value, par_category) != 0) {
        DBG_LOG("Got new Category %s", value);
        g_free(par_category);
        par_category = g_strdup(value);

        ax_event_handler_unsubscribe(event_handler,
            event_subscription_id, NULL);

        event_subscription_id = metadata_event_subscribe();
    }
}

/**
 * Callback function for Items parameter. Update event subscription
 * if needed.
 */
static void set_items(const char *value)
{
    DBG_LOG("Got new Items %s", value);
    g_free(par_items);
    par_items = g_strdup(value);
}

/**
 * Callback function for Filter parameter.
 * if needed.
 */
static void set_filter(const char *value)
{
    DBG_LOG("Got new Filter %s", value);
    g_free(par_filter);
    par_filter = g_strdup(value);
}

/**
 * Callback function for debug enabled parameter. Used to enable / disable
 * verbose debug printing.
 */
static void set_debug_enabled(const char *value)
{
    if (g_strcmp0(value, par_debug_enabled) != 0) {
        g_free(par_debug_enabled);
        par_debug_enabled = g_strdup(value);

        DBG_LOG("Got new DebugEnabled %s", par_debug_enabled);

        if (g_strcmp0(par_debug_enabled, "yes") == 0) {
            set_debug(TRUE);
            DBG_LOG("Enabled debug logging");
        } else {
            DBG_LOG("Disabling debugg logging");
            set_debug(FALSE);
        }
    }
}

/**
 * Test ACS reporting. Generate dummy event according to current items config
 * but all values replaced with TEST. Perform blocking call to run MDP in order
 * to actually check output. This is done by setting Error to non-NULL.
 */
static void cgi_test_reporting(CAMERA_HTTP_Reply http,
                               CAMERA_HTTP_Options options)
{
    gchar *error  = NULL;
    gchar *result = NULL;
    GList *metadata_items = NULL;

    if (g_strcmp0(par_analytic, " ") == 0) {
        result = g_strdup("Error");
        error  = g_strdup("Save Analytic");
        goto send_xml;
    }

    if (g_strcmp0(par_category, " ") == 0) {
        result = g_strdup("Error");
        error  = g_strdup("Save Category");
        goto send_xml;
    }

    if (g_strcmp0(par_items, " ") == 0) {
        result = g_strdup("Error");
        error  = g_strdup("Save Items");
        goto send_xml;
    }

    if (g_strcmp0(acs_get_enabled(acs), "yes") != 0) {
        result = g_strdup("Error");
        error  = g_strdup("Enable reporting");
        goto send_xml;
    }

    gboolean ret = build_metadata_items(NULL, &metadata_items);

    if (ret == FALSE) {
        result = g_strdup("Item Error");
        goto send_xml;
    }

    ret = acs_run(acs, metadata_items, &error);

    if (ret == FALSE) {
        result = g_strdup("Failure");
        goto send_xml;
    }

    result = g_strdup("Success");
    error  = g_strdup("NA");

send_xml:
    camera_http_sendXMLheader(http);
    camera_http_output(http, "<settings>");
    camera_http_output(http, "<param name='Result' value='%s'/>",
    result);
    camera_http_output(http, "<param name='Error' value='%s'/>",
    error);
    camera_http_output(http, "</settings>");

    mdp_destroy_list(&metadata_items);
    g_free(error);
    g_free(result);
}

/**
 * CGI function for getting all parameters at once.
 */
static void cgi_settings_get(CAMERA_HTTP_Reply http,
                             CAMERA_HTTP_Options options)
{
  gchar *server_address_encode =
    g_uri_escape_string(acs_get_ipname(acs), NULL, FALSE);
  gchar *source_id_encode      =
    g_uri_escape_string(acs_get_source(acs), NULL, FALSE);
  gchar *username_encode       =
    g_uri_escape_string(acs_get_username(acs), NULL, FALSE);
  gchar *password_encode       =
    g_uri_escape_string(acs_get_password(acs), NULL, FALSE);
  gchar *enabled_encode        =
    g_uri_escape_string(acs_get_enabled(acs), NULL, FALSE);
  gchar *analytic_encode       =
    g_uri_escape_string(par_analytic, NULL, FALSE);
  gchar *category_encode       =
    g_uri_escape_string(par_category, NULL, FALSE);
  gchar *items_encode          =
    g_uri_escape_string(par_items, NULL, FALSE);
  gchar *debug_encode          =
    g_uri_escape_string(par_debug_enabled, NULL, FALSE);

  camera_http_sendXMLheader(http);
  camera_http_output(http, "<settings>");
  camera_http_output(http, "<param name='ServerAddress' value='%s'/>",
    server_address_encode);
  camera_http_output(http, "<param name='SourceID' value='%s'/>",
    source_id_encode);
  camera_http_output(http, "<param name='Username' value='%s'/>",
    username_encode);
  camera_http_output(http, "<param name='Password' value='%s'/>",
    password_encode);
  camera_http_output(http, "<param name='Enabled' value='%s'/>",
    enabled_encode);
  camera_http_output(http, "<param name='Analytic' value='%s'/>",
    analytic_encode);
  camera_http_output(http, "<param name='Category' value='%s'/>",
    category_encode);
  camera_http_output(http, "<param name='Items' value='%s'/>",
    items_encode);
  camera_http_output(http, "<param name='DebugEnabled' value='%s'/>",
    debug_encode);
  camera_http_output(http, "</settings>");

  g_free(server_address_encode);
  g_free(source_id_encode);
  g_free(username_encode);
  g_free(password_encode);
  g_free(enabled_encode);
  g_free(analytic_encode);
  g_free(category_encode);
  g_free(items_encode);
  g_free(debug_encode);
}

/**
 * Build list of key-value pairs with metadata info.
 */
static gboolean build_metadata_items(const AXEventKeyValueSet *key_value_set,
                                     GList **list)
{
    g_assert(list);

    /* Get list of current selected data items to send to ACS */
    gchar **data_items     = g_strsplit(par_items, ";", MAX_ITEMS);
    GList *metadata_items  = NULL;
    gboolean ret           = TRUE;

    gchar *filter_key      = NULL;
    gchar *filter_value    = NULL;
    gboolean content_match = TRUE;

    /* Get content filter key, value */
    if (g_strcmp0(par_filter, " ") != 0) {
        content_match = FALSE;

        gchar **filter_items = g_strsplit(par_filter, "=", 2);

        if (filter_items[0] != NULL && filter_items[1] != NULL &&
            filter_items[2] == NULL) {
            filter_key   = g_strdup(filter_items[0]);
            filter_value = g_strdup(filter_items[1]);
        }

        g_strfreev(filter_items);
    }

    int i = 0;
    for (; i < MAX_ITEMS; i++) {
        if (data_items[i] != NULL && g_strcmp0(data_items[i], "") != 0) {
            /**
             * Get corresponding value from key value set.
             * Simplify webcode by just looping over all possible data types
             */
            gchar    *item_value = NULL;
            gboolean found_item  = FALSE;
            gboolean item_value_boolean;
            gint     item_value_int;
            gdouble  item_value_double;

            if (key_value_set == NULL) {
                found_item = TRUE;
                item_value = g_strdup("TEST");
            } else if (ax_event_key_value_set_get_string(key_value_set,
                data_items[i], NULL, &item_value, NULL)) {
                found_item = TRUE;
            } else if (ax_event_key_value_set_get_boolean(key_value_set,
                    data_items[i], NULL, &item_value_boolean, NULL)) {

                if (item_value_boolean == TRUE) {
                    item_value = g_strdup("yes");
                } else {
                    item_value = g_strdup("no");
                }

                found_item = TRUE;
            } else if (ax_event_key_value_set_get_integer(key_value_set,
                    data_items[i], NULL, &item_value_int, NULL)) {
                item_value = g_strdup_printf("%d", item_value_int);
                found_item = TRUE;
            } else if (ax_event_key_value_set_get_double(key_value_set,
                    data_items[i], NULL, &item_value_double, NULL)) {
                item_value = g_strdup_printf("%f", item_value_double);
                found_item = TRUE;
            }

            /* Leave and clean up if couldn't find some of the data */
            if (!found_item) {
                ret = FALSE;
                ERR("Failed to get %s information", data_items[0]);
                g_free(item_value);
                goto cleanup;
            } else if (filter_key && filter_value) {
                if (g_strcmp0(filter_key,   data_items[i]) == 0 &&
                    g_strcmp0(filter_value, item_value) == 0) {
                    content_match = TRUE;
                }
            }


            /* Capitalize first character to make it look better in ACS */
            data_items[i][0] = g_ascii_toupper(data_items[i][0]);

            /* Create metadata pair and insert in list */
            mdp_item_pair *item_pair = g_try_new0(mdp_item_pair, 1);

            if (item_pair == NULL) {
                ret = FALSE;
                ERR("Failed to allocate metadata item");
                g_free(item_value);
                goto cleanup;
            }

            item_pair->name  = g_strdup(data_items[i]);
            item_pair->value = g_strdup(item_value);

            metadata_items   = g_list_append(metadata_items,
                item_pair);

            /* Item value is allocated by ax_event and is freed by caller */
            g_free(item_value);
        } else {
            break;
        }
    }

cleanup:
    g_strfreev(data_items);
    g_free(filter_key);
    g_free(filter_value);

    if (ret == FALSE || content_match == FALSE) {
        mdp_destroy_list(&metadata_items);
        *list = NULL;
        return FALSE;
    }

    *list = metadata_items;

    return TRUE;
}

/******************** GLOBAL FUNCTION DEFINTION SECTION ***********************/

/**
 * Main entry point for application.
 *
 * @param argc Unused number of arguments received on the command line.
 * @param argv Unused vector of commandline arguments.
 *
 * @return Always returns 0.
 */
int main(int argc, char *argv[])
{
    GError *error = NULL;

    openlog(APP_ID, LOG_PID | LOG_CONS, LOG_USER);
    camera_init(APP_ID, APP_NICE_NAME);

    init_signals();


    loop       = g_main_loop_new(NULL, FALSE);
    acs        = acs_init();
    ovl_handle = overlay_init();

    /* Create an AXEventHandler */
    event_handler = ax_event_handler_new();

    char value[50];

    if(camera_param_get("DebugEnabled", value, 50)) {
        set_debug_enabled(value);
    }

    if(camera_param_get("ServerAddress", value, 50)) {
        set_server_address(value);
    }

    if(camera_param_get("SourceID", value, 50)) {
        set_source_id(value);
    }

    if(camera_param_get("Username", value, 50)) {
        set_username(value);
    }

    if(camera_param_get("Password", value, 50)) {
        set_password(value);
    }

    if(camera_param_get("Enabled", value, 50)) {
        set_enabled(value);
    }

    if(camera_param_get("Analytic", value, 50)) {
        set_analytic(value);
    }

    if(camera_param_get("Category", value, 50)) {
        set_category(value);
    }

    if(camera_param_get("Items", value, 300)) {
        set_items(value);
    }

    if(camera_param_get("ContentFilter", value, 300)) {
        set_filter(value);
    }

    camera_param_setCallback("ServerAddress", set_server_address);
    camera_param_setCallback("SourceID",      set_source_id);
    camera_param_setCallback("Username",      set_username);
    camera_param_setCallback("Password",      set_password);
    camera_param_setCallback("Enabled",       set_enabled);
    camera_param_setCallback("Analytic",      set_analytic);
    camera_param_setCallback("Category",      set_category);
    camera_param_setCallback("Items",         set_items);
    camera_param_setCallback("ContentFilter", set_filter);
    camera_param_setCallback("DebugEnabled",  set_debug_enabled);

    camera_http_setCallback("settings/testreporting", cgi_test_reporting);
    camera_http_setCallback("settings/get", cgi_settings_get);

    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    loop = NULL;

    ax_event_handler_unsubscribe(event_handler, event_subscription_id,
        NULL);
    camera_cleanup();
    closelog();
    acs_cleanup(&acs);
    overlay_cleanup(&ovl_handle);
    mdp_destroy_list(&cur_metadata_items);

    LOG("Exiting application");

    g_free(par_analytic);
    g_free(par_category);
    g_free(par_items);
    g_free(par_filter);
    g_free(par_debug_enabled);

    /* TODO: This locks the program on termination for some reason.
    ax_event_handler_free(event_handler);
    */

    return 0;
}
