
include(${QT_USE_FILE})

set(qtexample_SRCS
    example.cpp
    psmove.cpp
)

set(qtexample_MOC_HDRS
    psmove.hpp
)

set(qtexample_RCCS
    example.qrc
)

qt4_wrap_cpp(qtexample_MOC_SRCS ${qtexample_MOC_HDRS})
qt4_add_resources(qtexample_RCC_SRCS ${qtexample_RCCS})

include_directories(${PSMOVEAPI_SOURCE_DIR} ${QT_QTDECLARATIVE_INCLUDE_DIR})

add_executable(qtexample ${qtexample_SRCS} ${qtexample_MOC_SRCS} ${qtexample_RCC_SRCS} ../psmove.c ../hidapi/hid.c)

target_link_libraries(qtexample ${QT_LIBRARIES} ${IOKIT} ${COREFOUNDATION} ${QT_QTDECLARATIVE_LIBRARY})

