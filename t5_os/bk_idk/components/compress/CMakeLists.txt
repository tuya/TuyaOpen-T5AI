set(incs)
set(srcs)

list(APPEND srcs
	decompress.c
	)

list(APPEND incs
	include
	)

if (CONFIG_LZMA1900)

	list(APPEND incs 
		lzma1900
	)

	list(APPEND srcs
		lzma1900/LzmaDec.c
		lzma.c
	)

endif()

if (CONFIG_LZMA2301)

	list(APPEND incs 
		lzma2301
	)

	list(APPEND srcs
		lzma2301/LzmaDec.c
		lzma.c
	)

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)