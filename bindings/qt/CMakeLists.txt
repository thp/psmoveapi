
find_package(Qt4 REQUIRED)

include(${QT_USE_FILE})

include_directories(${PSMOVEAPI_SOURCE_DIR} ${QT_QTDECLARATIVE_INCLUDE_DIR})

add_definitions(-DQT_DECLARATIVE_LIB)


# Shared library

set(psmoveapiqt_SRCS psmoveqt.cpp)
set(psmoveapiqt_MOC_HDRS psmoveqt.hpp)

qt4_wrap_cpp(psmoveapiqt_MOC_SRCS ${psmoveapiqt_MOC_HDRS})

add_library(psmoveapiqt SHARED ${psmoveapiqt_SRCS} ${psmoveapiqt_MOC_SRCS})
target_link_libraries(psmoveapiqt psmoveapi ${QT_LIBRARIES} ${QT_QTDECLARATIVE_LIBRARY})
set_target_properties(psmoveapiqt PROPERTIES VERSION ${PSMOVEAPI_VERSION} SOVERSION ${PSMOVEAPI_VERSION_SONAME})


# System-wide installation of the Qt library

install(TARGETS psmoveapiqt RUNTIME DESTINATION bin LIBRARY DESTINATION lib)
install(FILES psmoveqt.hpp DESTINATION include/psmoveapi)

