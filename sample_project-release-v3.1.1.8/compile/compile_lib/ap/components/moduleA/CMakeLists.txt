set(incs)
set(srcs)

set(incs ./inc)

set(srcs
	src/moduleA.c
)

armino_component_register(SRCS "${srcs}"
			INCLUDE_DIRS "${incs}"
			PRIV_REQUIRES bk_common include
	)