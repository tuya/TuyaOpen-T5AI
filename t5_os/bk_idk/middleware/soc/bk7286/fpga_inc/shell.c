#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include "shell.h"
#include "bkstdio.h"

#define SHELL_MAX_ARGS  (10)

#ifdef __ARMCC_VERSION
extern uint32_t SHELL_CMDS$$Base;
extern uint32_t SHELL_CMDS$$Limit;
#define __SHELL_CMDS_BEGIN SHELL_CMDS$$Base
#define __SHELL_CMDS_END   SHELL_CMDS$$Limit
#else
extern uint32_t __SHELL_CMDS_BEGIN;
extern uint32_t __SHELL_CMDS_END;
#endif

ShellCmd* shell_find(char* name)
{
    uint32_t  i;
    uint32_t  len = strlen(name);
    uint32_t  cnt = ((uint32_t)&__SHELL_CMDS_END - (uint32_t)&__SHELL_CMDS_BEGIN) / sizeof(ShellCmd);
    ShellCmd* cmd = (ShellCmd*)&__SHELL_CMDS_BEGIN;

    for(i = 0; i < cnt; i++)
    {
        if(len == strlen(cmd[i].name) && memcmp(name, cmd[i].name, len) == 0) return &cmd[i];
    }

    return (ShellCmd*)0;
}

int shell_help(int argc, char *argv)
{
    uint32_t i, cnt = ((uint32_t)&__SHELL_CMDS_END - (uint32_t)&__SHELL_CMDS_BEGIN) / sizeof(ShellCmd);

    ShellCmd* cmd = (ShellCmd*)&__SHELL_CMDS_BEGIN;

    for(i = 0; i < cnt; i++)
    {
        bk_printf("%s\t-\t%s\n", cmd[i].name, cmd[i].desc);
    }

    return 0;
}

int shell_parse(char *line, char *argv[])
{
    int argc = 0;

    while(argc < SHELL_MAX_ARGS)
    {
        /* skip any white space */
        while(isblank(*line)) ++line;

        /* end of line, no more args */
        if(*line == '\0')
        {
            argv[argc] = NULL;
            return argc;
        }

        /* begin of argument string	*/
        argv[argc++] = line;

        /* find end of string */
        while(*line && !isblank(*line)) ++line;

        /* end of line, no more args */
        if(*line == '\0')
        {
            argv[argc] = NULL;
            return argc;
        }

        /* terminate current arg */
        *line++ = '\0';
    }

    return argc;
}

void shell_process(void)
{
    if(uart_rx_done)
    {
        int   argc;
        char* argv[SHELL_MAX_ARGS];
        char  buf[128];

        ShellCmd* cmd;

        buf[uart_rx_index] = '\0';
        memcpy(buf, uart_rx_buf, uart_rx_index);

        uart_rx_esc   = 0;
        uart_rx_flag  = 0;
        uart_rx_done  = 0;
        uart_rx_index = 0;
        uart_rx_value = 0;

        argc = shell_parse(buf, argv);
        cmd  = argc ? shell_find(argv[0]) : 0;

        bk_printf("\n");

        if(cmd)
        {
            cmd->func(argc, argv);
        }
        else
        {
            if(argc) bk_printf("Invalid command \"%s\", please type \"help\" to list all supported commands.\n", argv[0]);
        }

        uart_rx_esc   = 0;
        uart_rx_flag  = 1;
        uart_rx_done  = 0;
        uart_rx_index = 0;
        uart_rx_value = 0;

        bk_printf("\n%s", SHELL_PROMPT_SYMBOL);
    }
}

SHELL_CMD_EXPORT(help, list all commands; type ESC to quit command if blocked, shell_help)
