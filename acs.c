#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>

#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "acs.h"
#include "acs_commands.h"
#include "metadata_pair.h"
#include "debug.h"

/** @file acs.c
 * @Brief Implementation file for abstraction of ACS metadata API integration.
 *
 * Handle ACS communication, generate JSON data structes and send the
 * commands using cURL to the ACS server. Provide methods of error checking
 * the communication.
 */

/******************** MACRO DEFINITION SECTION ********************************/


/******************** LOCAL VARIABLE DECLARATION SECTION **********************/

typedef struct acs
{
    gchar *username;
    gchar *password;
    gchar *ipname;
    gchar *source;
    gchar *enabled;
} acs;

/******************** LOCAL FUNCTION DECLARATION SECTION **********************/

/**
 * Check for JSON error messages when sending ACS command.
 *
 * @param buffer String containing stdout contents from the cURL system call.
 * @param error  Mandatory location to place error message.
 *
 * @return TRUE on success, FALSE on any error.
 */
static gboolean check_jSON_response(const char *buffer, char **error);

static gboolean is_initialized(const acs_handle handle)
{
    if (handle == NULL) {
        return FALSE;
    }

    if (handle->enabled == NULL || g_strcmp0(handle->enabled, "yes") != 0) {
        return FALSE;
    }

    if (handle->username == NULL) {
        return FALSE;
    }

    if (handle->password == NULL) {
        return FALSE;
    }

    if (handle->ipname == NULL) {
        return FALSE;
    }

    if (handle->source == NULL) {
        return FALSE;
    }

    return TRUE;
}


/******************** LOCAL FUNCTION DEFINTION SECTION ************************/

/**
 * Check for JSON error messages when sending ACS command.
 */
static gboolean check_jSON_response(const char *buffer, char **error)
{
    g_assert(error);

    gboolean ret = FALSE;

    if (buffer) {
        gchar http_code[4];
        gchar *code_start = g_strrstr(buffer, "HTTP");

        if (code_start) {
            int hits = sscanf(code_start, "HTTP:%3s", http_code);
            DBG_LOG("Got HTTP code %s", http_code);

            if (hits == 1) {
                if (g_strcmp0(http_code, "200") == 0) {
                    ret = TRUE;
                } else if (g_strcmp0(http_code, "000") == 0) {
                    *error = g_strdup("Bad IP:Port");
                } else if (g_strcmp0(http_code, "401") == 0) {
                    *error = g_strdup("Unauthorized");
                } else if (g_strcmp0(http_code, "400") == 0) {
                    *error = g_strdup("Bad source ID");
                }
            }
        }
    }

    if (*error == NULL) {
        *error = g_strdup("Unknown Error");
    }

    return ret;
}

/******************** GLOBAL FUNCTION DEFINTION SECTION ***********************/

/**
 * Initialize MDP information
 */
acs_handle acs_init()
{
    acs_handle handle = g_new0(acs, 1);

    return handle;
}

/**
 * Cleanup MDP
 */
void acs_cleanup(acs_handle *handle_p)
{
    if (handle_p == NULL) {
        return;
    }

    if (*handle_p == NULL) {
        return;
    }

    acs_handle handle = *handle_p;

    g_free(handle->username);
    g_free(handle->password);
    g_free(handle->ipname);
    g_free(handle->source);
    g_free(handle->enabled);

    g_free(handle);

    handle_p = NULL;
}

/**
*  Send Metadata to ACS
*/
gboolean acs_run(const acs_handle handle,
                 GList *metadata_items,
                 char **error)
{
    gchar **data_items     = NULL;
    gchar *jSON_string     = NULL;
    gchar *tmp_concat      = NULL;
    GList *overlay_data    = NULL;

    char *cmd    = NULL;
    gboolean ret = TRUE;

    if (is_initialized(handle) == FALSE) {
        if (error) {
            *error = g_strdup("Missing config");
        }
        return FALSE;
    }

    /**
     * Retrieve current UTC time as needed by the API
     */
    char outstr[200];
    time_t t;
    struct tm *tmp;

    t = time(NULL);
    tmp = gmtime(&t);


    if (tmp == NULL) {
        ERR("Failed to get time value");
        goto cleanup;
    }

    if (strftime(outstr, sizeof(outstr), "%F %T", tmp) == 0) {
        ERR("Failed to convert time");
        goto cleanup;
    }

    /* Boilerplate JSON command structure */
    jSON_string = g_strdup_printf(\
        "{ \
            \"addExternalDataRequest\": \
                { \"occurrenceTime\": \"%s\",\
                  \"source\": \"%s\",\
                  \"externalDataType\": \
                  \"PointOfSales\", \"data\": {", outstr, handle->source);

    GList *list = metadata_items;
    for (; list != NULL; list = list->next) {
        /* Create JSON data entry and append to jSON string */
        mdp_item_pair *item_pair = list->data;

        gchar *item_string = g_strdup_printf("\"%s\":\"%s\",",
            item_pair->name, item_pair->value);

        overlay_data = g_list_append(
            overlay_data, g_strdup(item_string));

        tmp_concat = g_strconcat(jSON_string, item_string, NULL);

        g_free(item_string);
        g_free(jSON_string);

        jSON_string = tmp_concat;
    }

    tmp_concat = g_strconcat(jSON_string, "}}}", NULL);
    g_free(jSON_string);
    jSON_string = tmp_concat;

    cmd = g_strdup_printf(METABASE, jSON_string, handle->ipname,
                          handle->username, handle->password);

    /**
     * Only perform blocking call if we are checking for error (test reporting).
     * We need to perform a non-blocking call during normal operation because
     * this function will be run in the GMainLoop context.
     *
     * TODO: Look at using worker threads, libcurl etc. if we want error
     * checking for each metadata upload. Possible create an event so user can
     * be notified of reporting errors.
     */
    if (error) {
        gchar *stdout;
        gchar *stderr;

        (void) g_spawn_command_line_sync(cmd,
            &stdout, &stderr, NULL, NULL);

        ret = check_jSON_response(stdout, error);

        g_free(stdout);
        g_free(stderr);
    } else {
        /* Send JSON command, return value is not useful */
        DBG_LOG("Pushing command to ACS");
        (void) g_spawn_command_line_async(cmd, NULL);
    }

cleanup:
    g_free(jSON_string);
    g_strfreev(data_items);
    g_free(cmd);

    return ret;
}

/**
 * Initialize Metadata Push framework.
 *
 * @param username  Username for the ACS server.
 * @param password  Password for the ACS server.
 * @param acs_ip    IP Address for the ACS server.
 * @param source_id Source Key to use for the Metadata.
 *
 * @return No return value.
 */
void acs_set_username(const acs_handle handle, const char *username)
{
    if (handle == NULL) {
        return;
    }

    g_free(handle->username);
    handle->username = g_strdup(username);
}

/**
 * Initialize Metadata Push framework.
 *
 * @param username  Username for the ACS server.
 * @param password  Password for the ACS server.
 * @param acs_ip    IP Address for the ACS server.
 * @param source_id Source Key to use for the Metadata.
 *
 * @return No return value.
 */
void acs_set_password(const acs_handle handle, const char *password)
{
    if (handle == NULL) {
        return;
    }

    g_free(handle->password);
    handle->password = g_strdup(password);
}

/**
 * Initialize Metadata Push framework.
 *
 * @param username  Username for the ACS server.
 * @param password  Password for the ACS server.
 * @param acs_ip    IP Address for the ACS server.
 * @param source_id Source Key to use for the Metadata.
 *
 * @return No return value.
 */
void acs_set_ipname(const acs_handle handle, const char *ipname)
{
    if (handle == NULL) {
        return;
    }

    g_free(handle->ipname);
    handle->ipname = g_strdup(ipname);
}


/**
 * Initialize Metadata Push framework.
 *
 * @param username  Username for the ACS server.
 * @param password  Password for the ACS server.
 * @param acs_ip    IP Address for the ACS server.
 * @param source_id Source Key to use for the Metadata.
 *
 * @return No return value.
 */
void acs_set_source(const acs_handle handle, const char *source)
{
    if (handle == NULL) {
        return;
    }

    g_free(handle->source);
    handle->source = g_strdup(source);
}

void acs_set_enabled(const acs_handle handle, const char *enabled)
{
    if (handle == NULL) {
        return;
    }

    g_free(handle->enabled);
    handle->enabled = g_strdup(enabled);
}

/**
 * Initialize Metadata Push framework.
 *
 * @param username  Username for the ACS server.
 * @param password  Password for the ACS server.
 * @param acs_ip    IP Address for the ACS server.
 * @param source_id Source Key to use for the Metadata.
 *
 * @return No return value.
 */
const char * acs_get_username(const acs_handle handle)
{
    if (handle == NULL) {
        return NULL;
    }

    return handle->username;
}

/**
 * Initialize Metadata Push framework.
 *
 * @param username  Username for the ACS server.
 * @param password  Password for the ACS server.
 * @param acs_ip    IP Address for the ACS server.
 * @param source_id Source Key to use for the Metadata.
 *
 * @return No return value.
 */
const char * acs_get_password(const acs_handle handle)
{
    if (handle == NULL) {
        return NULL;
    }

    return handle->password;
}

/**
 * Initialize Metadata Push framework.
 *
 * @param username  Username for the ACS server.
 * @param password  Password for the ACS server.
 * @param acs_ip    IP Address for the ACS server.
 * @param source_id Source Key to use for the Metadata.
 *
 * @return No return value.
 */
const char * acs_get_ipname(const acs_handle handle)
{
    if (handle == NULL) {
        return NULL;
    }

    return handle->ipname;
}

/**
 * Initialize Metadata Push framework.
 *
 * @param username  Username for the ACS server.
 * @param password  Password for the ACS server.
 * @param acs_ip    IP Address for the ACS server.
 * @param source_id Source Key to use for the Metadata.
 *
 * @return No return value.
 */
const char * acs_get_source(const acs_handle handle)
{
    if (handle == NULL) {
        return NULL;
    }

    return handle->source;
}

const char * acs_get_enabled(const acs_handle handle)
{
    if (handle == NULL) {
        return NULL;
    }

    return handle->enabled;
}