#ifndef INCLUSION_GUARD_METADATA_ACS_COMMANDS_H
#define INCLUSION_GUARD_METADATA_ACS_COMMANDS_H

/** @file acs_commands.h
 * @Brief Macro for cURL commands
 *
 * Just a header file to hide the ugly cURL macro.
 */

/**
 * Macro used to form the cURL command used when communicating with ACS.
 *
 */
#define METABASE "curl --insecure --anyauth -H \"Content-Type: application/json\
\" --data \'%s\' -sw 'HTTP:%%{http_code}' --max-time 2 https://%s/A\
cs/Api/ExternalDataFacade/AddExternalData  --user \"%s:%s\"\
"

#endif // INCLUSION_GUARD_ACS_COMMANDS_H