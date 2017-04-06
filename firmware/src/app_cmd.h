/* 
 * File:   app_cmd.h
 * Author: Javier
 *
 * Created on February 21, 2017, 11:20 PM
 */

#define LIB_CMD

#include "app_globals.h"
#include "app_io.h"
#include "app_main.h"

inline unsigned char Cmd_checkCmd(unsigned char idBuffer, unsigned char *pCommand, unsigned char *pArgs);
// APP Commands
void APP_CMD_ping(unsigned char idBuffer, unsigned char *pArgs);
void APP_CMD_version(unsigned char idBuffer, unsigned char *pArgs);
void APP_CMD_status(unsigned char idBuffer, unsigned char *pArgs);
void APP_CMD_reset(unsigned char idBuffer, unsigned char *pArgs);
void APP_CMD_uptime(unsigned char idBuffer, unsigned char *pArgs);
void APP_CMD_debug(unsigned char idBuffer, unsigned char *pArgs);
void APP_CMD_read(unsigned char idBuffer, unsigned char *pArgs);
void APP_CMD_write(unsigned char idBuffer, unsigned char *pArgs);
void APP_CMD_var(unsigned char idBuffer, unsigned char *pArgs);
void APP_CMD_mem(unsigned char idBuffer, unsigned char *pArgs);
void APP_CMD_dump(unsigned char idBuffer, unsigned char *pArgs);