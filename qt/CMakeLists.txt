
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


# Qt example

set(qtexample_SRCS example.cpp)
set(qtexample_RCCS example.qrc)

qt4_add_resources(qtexample_RCC_SRCS ${qtexample_RCCS})

add_executable(qtexample ${qtexample_SRCS} ${qtexample_RCC_SRCS})
target_link_libraries(qtexample ${QT_LIBRARIES} ${QT_QTDECLARATIVE_LIBRARY} psmoveapiqt)


# Trailers example

set(qttrailers_SRCS trailers.cpp)
set(qttrailers_RCCS trailers.qrc)

qt4_add_resources(qttrailers_RCC_SRCS ${qttrailers_RCCS})

add_executable(qttrailers ${qttrailers_SRCS} ${qttrailers_RCC_SRCS})
target_link_libraries(qttrailers ${QT_LIBRARIES} ${QT_QTDECLARATIVE_LIBRARY} psmoveapiqt)


# Multi-controller example

set(qtmultiple_SRCS multiple.cpp)
set(qtmultiple_RCCS multiple.qrc)

qt4_add_resources(qtmultiple_RCC_SRCS ${qtmultiple_RCCS})

add_executable(qtmultiple ${qtmultiple_SRCS} ${qtmultiple_RCC_SRCS})
target_link_libraries(qtmultiple ${QT_LIBRARIES} ${QT_QTDECLARATIVE_LIBRARY} psmoveapiqt)


# System-wide installation of the Qt library

install(TARGETS psmoveapiqt LIBRARY DESTINATION lib)
install(FILES psmoveqt.hpp DESTINATION include/psmoveapi)

