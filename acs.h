#ifndef INCLUSION_GUARD_ACS_H
#define INCLUSION_GUARD_ACS_H

/** @file acs.h
 * @Brief Header file for abstraction of ACS metadata API integration.
 *
 * Handle ACS communication, generate JSON data structes and send the
 * commands using cURL to the ACS server. Provide methods of error checking
 * the communication.
 */

/**
 * Forward-declared handle for ACS object.
 */
typedef struct acs* acs_handle;

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
acs_handle acs_init();

/**
 * Cleanup Metadata Push framework and deallocate resources.
 *
 * @return No return value.
 */
void acs_cleanup(acs_handle *handle_p);

/**
 * Send Metadata to ACS.
 *
 * @param key_value_set  The set of metadata that should be pushed to ACS.
 * @param par_items      List of semi-colon separated values with item names to
 *						 to look up and put into JSON structure.
 * @param error          Location to place error message. If NULL this will be
 *                       ignored AND ACS send opteration will be non-blocking
 *                       which must be done from a event callback context.
 *
 * @return TRUE on success, FALSE on any kind of error.
 */
gboolean acs_run(const acs_handle handle,
				 GList *metadata_items,
	             char **error);

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
void acs_set_username(const acs_handle handle, const char *username);

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
void acs_set_password(const acs_handle handle, const char *password);

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
void acs_set_ipname(const acs_handle handle, const char *ipname);

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
void acs_set_source(const acs_handle handle, const char *source);

void acs_set_enabled(const acs_handle handle, const char *enabled);

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
const char * acs_get_username(const acs_handle handle);

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
const char * acs_get_password(const acs_handle handle);

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
const char * acs_get_ipname(const acs_handle handle);

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
const char * acs_get_source(const acs_handle handle);

const char * acs_get_enabled(const acs_handle handle);



#endif // INCLUSION_GUARD_ACS_H