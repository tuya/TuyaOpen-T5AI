/**
 **************************************************************************************
 * @file    shell.h
 * @brief   SHELL command line interface
 * 
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2022 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __SHELL_H__
#define __SHELL_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#define SHELL_PROMPT_SYMBOL     ">>"

typedef int (*shell_func)(int argc, char* argv[]);

typedef struct _ShellCmd
{
    const char* name; /* the name of command        */
    const char* desc; /* description of command     */
    shell_func  func; /* function pointer of command*/
}ShellCmd;

#define SHELL_CMD_EXPORT(cmd, desc, func)                                           \
const char __shell_##cmd##_name[] __attribute__((section(".rodata.name"))) = #cmd;  \
const char __shell_##cmd##_desc[] __attribute__((section(".rodata.desc"))) = #desc; \
const ShellCmd __shell_##cmd      __attribute__((section("SHELL_CMDS"), used)) =    \
{                                                                                   \
    __shell_##cmd##_name,                                                           \
    __shell_##cmd##_desc,                                                           \
    (shell_func)&func                                                               \
};

void shell_process(void);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__SHELL_H__
