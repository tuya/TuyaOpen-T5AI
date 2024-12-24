// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include "cli.h"
#include <components/system.h>
#include <driver/wdt.h>

static void cli_reset_reason_help(void)
{
	CLI_LOGI("reset_reason {reason}\r\n");
}

static void cli_reset_reason_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc != 2) {
		cli_reset_reason_help();
		return;
	}

	uint32_t reason = os_strtoul(argv[1], NULL, 16);
	CLI_LOGI("set reset reason: %x\r\n", reason);
	bk_misc_set_reset_reason(reason);
	bk_wdt_start(100);
	int int_level = rtos_disable_int();
	while(1);
	rtos_enable_int(int_level);
}

#define RESET_REASON_CMD_CNT (sizeof(s_reset_reason_commands) / sizeof(struct cli_command))
static const struct cli_command s_reset_reason_commands[] = {
	{"reset_reason", "reset_reason ", cli_reset_reason_cmd},
};

int cli_reset_reason_init(void)
{
	return cli_register_commands(s_reset_reason_commands, RESET_REASON_CMD_CNT);
}
