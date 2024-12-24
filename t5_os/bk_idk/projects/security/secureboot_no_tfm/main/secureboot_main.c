#include <components/log.h>
#include "bk_private/bk_init.h"

#define TAG "example"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

void user_app_main(void){

}

int main(void)
{
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	//BK_LOGI(TAG, "OTA successfully\r\n");
	bk_init();
	return 0;
}
