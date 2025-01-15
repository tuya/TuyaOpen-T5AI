#include "unity.h"
#include <components/log.h>
#include "test_mbedtls.h"

int main(void)
{
	mbedtls_ut_stub();
	unity_run_menu();
	return 0;
}
