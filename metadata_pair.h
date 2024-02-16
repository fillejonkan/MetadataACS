#ifndef INCLUSION_GUARD_METADATA_PAIR_H
#define INCLUSION_GUARD_METADATA_PAIR_H

/** @file metadata_pair.h
 * @Brief Header file abstracting metadata pair object and lists.
 *
 */

/**
 * Key-value pair of metadata items. Used for the different reporting methods.
 */
typedef struct mdp_item_pair
{
    gchar *name;
    gchar *value;
} mdp_item_pair;

/**
 * Destroy a GList of mdp_item_pair objects.
 */
void mdp_destroy_list(GList **list);

#endif // INCLUSION_GUARD_METADATA_PAIR_H