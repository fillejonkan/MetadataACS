#include "debug.h"

/** @file debug.c
 * @Brief Implentation of public debug status functions.
 *
 */

/******************** MACRO DEFINITION SECTION ********************************/


/******************** LOCAL VARIABLE DECLARATION SECTION **********************/

/**
 * Status of debug logs on/off.
 */
static gboolean debug_enabled = FALSE;

/******************** LOCAL FUNCTION DECLARATION SECTION **********************/


/******************** LOCAL FUNCTION DEFINTION SECTION ************************/


/******************** GLOBAL FUNCTION DEFINTION SECTION ***********************/

gboolean get_debug()
{
    return debug_enabled;
}

void set_debug(gboolean debug)
{
    debug_enabled = debug;
}




