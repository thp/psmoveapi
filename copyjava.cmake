FILE(GLOB myglob "*.java")
foreach(THEFILE ${myglob})
	CONFIGURE_FILE(${THEFILE} ${CMAKE_CURRENT_BINARY_DIR}/io/thp/psmove/ COPYONLY)
endforeach(THEFILE)

