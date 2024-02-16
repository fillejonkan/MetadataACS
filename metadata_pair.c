#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>

#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "metadata_pair.h"
#include "debug.h"

/** @file metadata_pair.c
 * @Brief Implementation file meta data pair and lists.
 *
 */

/******************** MACRO DEFINITION SECTION ********************************/

/******************** LOCAL VARIABLE DECLARATION SECTION **********************/

/******************** LOCAL FUNCTION DECLARATION SECTION **********************/

/**
 * Helper function to use for freeing and mdp item pair.
 *
 * @param data Pointer to the data pair.
 *
 * @return No return value.
 */
static void mdp_free_item_pair(gpointer data);

/******************** LOCAL FUNCTION DEFINTION SECTION ************************/

static void mdp_free_item_pair(gpointer data)
{
    if (data == NULL) {
        return;
    }

    mdp_item_pair * item_pair = data;

    g_free(item_pair->name);
    g_free(item_pair->value);
    g_free(item_pair);
}

/******************** GLOBAL FUNCTION DEFINTION SECTION ***********************/

/**
 * Free tuple of metadata items, used in g_list_free_full
 */
void mdp_destroy_list(GList **list)
{
    if (list == NULL) {
        return;
    }

    if (*list == NULL) {
        return;
    }

    g_list_free_full(*list, mdp_free_item_pair);

    *list = NULL;
}