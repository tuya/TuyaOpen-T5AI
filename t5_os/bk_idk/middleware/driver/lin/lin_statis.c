// Copyright 2023-2024 Beken
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

#include <os/mem.h>
//#include "lin_types.h"
#include "lin_hw.h"
#include "lin_statis.h"

#if CONFIG_LIN_STATIS

#define TAG "lin_statis"

static lin_statis_t s_lin_statis[LIN_IDENT_MAX] = {0};

bk_err_t lin_statis_init(void)
{
	os_memset(&s_lin_statis, 0, sizeof(s_lin_statis));
	return BK_OK;
}

bk_err_t lin_statis_id_init(lin_id_t id)
{
	os_memset(&s_lin_statis[id], 0, sizeof(s_lin_statis[id]));
	return BK_OK;
}

lin_statis_t* lin_statis_get_statis(lin_id_t id)
{
	return &(s_lin_statis[id]);
}

void lin_statis_dump(lin_id_t id)
{
	BK_LOGI(TAG, "dump lin statis:\r\n");
	BK_LOGI(TAG, "isr_cnt:     %d\r\n", s_lin_statis[id].isr_cnt);
	BK_LOGI(TAG, "error_cnt:       %d\r\n", s_lin_statis[id].error_cnt);
	BK_LOGI(TAG, "completed_cnt:       %d\r\n", s_lin_statis[id].completed_cnt);
	BK_LOGI(TAG, "data_req_cnt:      %d\r\n", s_lin_statis[id].data_req_cnt);
	BK_LOGI(TAG, "recv_timeout_cnt: %d\r\n", s_lin_statis[id].trans_cnt);
	BK_LOGI(TAG, "trans_cnt:       %d\r\n", s_lin_statis[id].wakeup_cnt);
	BK_LOGI(TAG, "aborted_cnt:         %d\r\n", s_lin_statis[id].aborted_cnt);
	BK_LOGI(TAG, "bus_idle_timeout_cnt:        %d\r\n", s_lin_statis[id].bus_idle_timeout_cnt);
	BK_LOGI(TAG, "active_cnt: %x\r\n", s_lin_statis[id].active_cnt);
	BK_LOGI(TAG, "bit_err_cnt:    %d\r\n", s_lin_statis[id].bit_err_cnt);
	BK_LOGI(TAG, "chk_err_cnt:   %d\r\n", s_lin_statis[id].chk_err_cnt);
	BK_LOGI(TAG, "timeout_err_cnt:  %d\r\n", s_lin_statis[id].timeout_err_cnt);
	BK_LOGI(TAG, "parity_err_cnt:  %d\r\n", s_lin_statis[id].parity_err_cnt);
}

#endif
