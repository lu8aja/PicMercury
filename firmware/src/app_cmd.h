/* 
 * File:   app_cmd.h
 * Author: Javier
 *
 * Created on February 21, 2017, 11:20 PM
 */

#include "app_globals.h"
#include "app_io.h"
#include "app_main.h"

// APP Commands
void APP_CMD_ping(Ring_t *pBuffer, unsigned char *pArgs);
void APP_CMD_version(Ring_t *pBuffer, unsigned char *pArgs);
void APP_CMD_uptime(Ring_t *pBuffer, unsigned char *pArgs);
void APP_CMD_debug(Ring_t *pBuffer, unsigned char *pArgs);
void APP_CMD_read(Ring_t *pBuffer, unsigned char *pArgs);
void APP_CMD_write(Ring_t *pBuffer, unsigned char *pArgs);
void APP_CMD_var(Ring_t *pBuffer, unsigned char *pArgs);
void APP_CMD_mem(Ring_t *pBuffer, unsigned char *pArgs);
void APP_CMD_dump(Ring_t *pBuffer, unsigned char *pArgs);