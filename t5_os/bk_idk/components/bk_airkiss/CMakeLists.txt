set(incs .)

if(CONFIG_BK_AIRKISS)
set(srcs
	airkiss_main.c
	airkiss_pingpong.c
	bk_airkiss.c
	)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES lwip_intf_v2_1)
