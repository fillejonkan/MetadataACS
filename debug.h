#ifndef INCLUSION_GUARD_DEBUG_H
#define INCLUSION_GUARD_DEBUG_H

#include <syslog.h>
#include <glib.h>
#include <glib/gprintf.h>

/** @file debug.h
 * @Brief Definition of debug macros and functions.
 *
 * Convenience macros for debug logging and functions to hold central state
 * of enabled / disabled verbose debug logging.
 */

/**
 * Log message macro
 */
#define LOG(fmt, args...) { syslog(LOG_INFO, fmt, ## args); \
    g_message(fmt, ## args); }

/**
 * Error message macro
 */
#define ERR(fmt, args...)   { syslog(LOG_ERR, fmt, ## args); \
    g_warning(fmt, ## args); }

/**
 * Debug Log message macro, control with parameter to reduce log spam.
 */
#define DBG_LOG(fmt, args...) \
    { if (get_debug()) { syslog(LOG_INFO, fmt, ## args); \
    g_message(fmt, ## args);} }

/**
 * Get debug status.
 * 
 * @return Current status of verbose debug enabled / disabled.
 */
gboolean get_debug();

/**
 * Set current status of verbose debug enabled / disabled.
 *
 * @param debug The new debug status.
 *
 * @return No return value.
 */
void set_debug(gboolean debug);


#endif // INCLUSION_GUARD_DEBUG_H