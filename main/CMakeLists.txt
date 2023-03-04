set(srcs "main.c"
	"st7735s.c"
	"fontx.c"
	"my_screen"
	)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")
