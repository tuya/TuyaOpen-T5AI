list(APPEND incs reset_reason)
list(APPEND srcs reset_reason/reset_reason.c)

if (CONFIG_RESET_REASON_TEST)
	list(APPEND srcs reset_reason/reset_reason_test.c)
endif()

set(incs ${incs} PARENT_SCOPE)
set(srcs ${srcs} PARENT_SCOPE)

