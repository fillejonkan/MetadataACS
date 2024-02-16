#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>
#include <cairo/cairo.h>
#include <axoverlay.h>

#include "overlay.h"
#include "debug.h"
#include "metadata_pair.h"

/** @file overlay.c
 * @Brief Overlay implementation
 *
 */

/******************** MACRO DEFINITION SECTION ********************************/

#define OVERLAY_WIDTH 600
#define OVERLAY_HEIGHT 400

#define ANIMATION_FPS 10

/******************** LOCAL VARIABLE DECLARATION SECTION **********************/

typedef struct overlay
{
    gint animation_timer;
    gint overlay_id;
    GList *cur_list;
    gchar *analytic_text;
    struct timespec start;
    struct timespec stop;
    gint timeout_us;
    gboolean timer_elapsed;
} overlay;

/******************** LOCAL FUNCTION DECLARATION SECTION **********************/

static void reset_clock(const overlay_handle handle)
{
    g_assert(handle);

    clock_gettime(CLOCK_MONOTONIC, &handle->start);
    handle->timer_elapsed = FALSE;
}

static void check_elapsed_time(const overlay_handle handle)
{
    struct timespec *stop  = &handle->stop;
    struct timespec *start = &handle->start; 

    clock_gettime(CLOCK_MONOTONIC, stop);

    double result = (stop->tv_sec - start->tv_sec) * 1e6 + 
        (stop->tv_nsec - start->tv_nsec) / 1e3;

    if ((int) result >= handle->timeout_us) {
        handle->timer_elapsed = TRUE;
    }
}

static gboolean update_overlay_cb(gpointer data)
{
    g_assert(data);

    GError *error = NULL;

    overlay_handle handle = data;

    check_elapsed_time(handle);

    /* Request a redraw of the overlay */
    axoverlay_redraw(&error);
    if (error != NULL) {
    /*
     * If redraw fails then it is likely due to that overlayd has
     * crashed. Don't exit instead wait for overlayd to restart and
     * for axoverlay to restore the connection.
     */
    ERR("Failed to redraw overlay (%d): %s\n", error->code, error->message);
    g_error_free(error);
    }

    return G_SOURCE_CONTINUE;
}

static void render_overlay_cb(gpointer render_context, gint id,
                   struct axoverlay_stream_data *stream,
                   enum axoverlay_position_type postype, gfloat overlay_x,
                   gfloat overlay_y, gint overlay_width, gint overlay_height,
                   gpointer user_data)
{
    cairo_t *cr = render_context;

    if (user_data == NULL) {
        return;
    }

    overlay_handle handle = user_data;

    /* Clear background */
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle(cr, 0, 0, OVERLAY_WIDTH, OVERLAY_HEIGHT);
    cairo_fill(cr);

    /* Draw the text */
    cairo_set_source_rgba(cr, 0.2, 0.8, 1.0, 1.0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    cairo_select_font_face(cr, "sans-serif",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 40);

    cairo_move_to(cr, 0, 40);
    cairo_show_text(cr, handle->analytic_text);

    /* Don't add metadata in case the timer elapsed */
    if (handle->timer_elapsed == TRUE) {
        return;
    }

    GList *list = handle->cur_list;
    int offset = 90;
    for (; list != NULL; list = list->next) {
        mdp_item_pair *item_pair = list->data;
        gchar *text = g_strdup_printf("%s : %s", item_pair->name,
            item_pair->value);

        cairo_move_to(cr, 0, offset);
        cairo_show_text(cr, text);
        offset += 50;

        g_free(text);
    }
}


/******************** LOCAL FUNCTION DEFINTION SECTION ************************/


/******************** GLOBAL FUNCTION DEFINTION SECTION ***********************/

/**
 * Initialize overlays
 */
overlay_handle overlay_init()
{
    GError *error = NULL;

    /* Create AxOverlay backend using Cairo */
    if(!axoverlay_is_backend_supported(AXOVERLAY_CAIRO_IMAGE_BACKEND)) {
        ERR("AXOVERLAY_CAIRO_IMAGE_BACKEND is not supported");
        return NULL;
    }

    /* Initialize the library */
    struct axoverlay_settings settings;
    axoverlay_init_axoverlay_settings(&settings);
        settings.render_callback = render_overlay_cb;
        settings.adjustment_callback = NULL;
        settings.select_callback = NULL;
        settings.backend = AXOVERLAY_CAIRO_IMAGE_BACKEND;
    axoverlay_init(&settings, &error);
    
    if (error != NULL) {
        ERR("Failed to initialize axoverlay: %s", error->message);
        g_error_free(error);
        return NULL;
    }

    overlay_handle handle   = g_new0(overlay, 1);

    /* Create an overlay */
    struct axoverlay_overlay_data data;
    axoverlay_init_overlay_data(&data);
    data.postype = AXOVERLAY_CUSTOM_SOURCE;
    data.anchor_point = AXOVERLAY_ANCHOR_TOP_LEFT;
    data.x = 0.0;
    data.y = 0.0;
    data.width = OVERLAY_WIDTH;
    data.height = OVERLAY_WIDTH;
    data.colorspace = AXOVERLAY_COLORSPACE_ARGB32;
    data.scale_to_stream = TRUE;
    handle->overlay_id = axoverlay_create_overlay(&data, handle, &error);
    if (error != NULL) {
        printf("Failed to create first overlay: %s", error->message);
        g_free(handle);
        g_error_free(error);
        return NULL;
    }

        /* Draw overlays */
    axoverlay_redraw(&error);
    if (error != NULL) {
        printf("Failed to draw overlays: %s", error->message);
        axoverlay_destroy_overlay(handle->overlay_id, &error);
        axoverlay_cleanup();
        g_free(handle);
        g_error_free(error);
        return NULL;
    }

    /**
     * Initialize state
     */
    handle->cur_list        = NULL;
    handle->analytic_text   = NULL;
    handle->timeout_us      = 3e6;
    handle->timer_elapsed   = TRUE;

    /* Start animation timer */
    handle->animation_timer = 
        g_timeout_add(1000/ANIMATION_FPS, update_overlay_cb, handle);

    return handle;
}
/**
 * Cleanup overlays
 */
void overlay_cleanup(overlay_handle *handle_p)
{
    if (handle_p == NULL) {
        return;
    }

    if (*handle_p == NULL) {
        return;
    }

    overlay_handle handle = *handle_p;

    g_free(handle->analytic_text);
    axoverlay_destroy_overlay(handle->overlay_id, NULL);

    /* The data list is owned by our creator so do not free that */

    /* Release library resources */
    axoverlay_cleanup();

    handle = NULL;
}

gboolean overlay_set_data(const overlay_handle handle,
                          GList *list,
                          unsigned int time,
                          const char *analytic,
                          const char *category)
{
    g_assert(analytic);
    g_assert(category);

    if (handle == NULL) {
        return FALSE;
    }

    g_free(handle->analytic_text);

    if (g_strcmp0(category, "Uncategorized")) {
        handle->analytic_text = g_strdup_printf("%s:", analytic);
    } else {
        handle->analytic_text = g_strdup_printf("%s/%s:", analytic, category); 
    }

    handle->cur_list = list;

    reset_clock(handle);

    return TRUE;
}