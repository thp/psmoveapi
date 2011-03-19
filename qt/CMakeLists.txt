
include(${QT_USE_FILE})

include_directories(${PSMOVEAPI_SOURCE_DIR} ${QT_QTDECLARATIVE_INCLUDE_DIR})

set(qtexample_SRCS
    example.cpp
    psmoveqt.cpp
)

set(qtexample_MOC_HDRS
    psmoveqt.hpp
)

set(qtexample_RCCS
    example.qrc
)

qt4_wrap_cpp(qtexample_MOC_SRCS ${qtexample_MOC_HDRS})
qt4_add_resources(qtexample_RCC_SRCS ${qtexample_RCCS})

add_executable(qtexample ${qtexample_SRCS} ${qtexample_MOC_SRCS} ${qtexample_RCC_SRCS} ${PSMOVE_SRC})

target_link_libraries(qtexample ${QT_LIBRARIES} ${PSMOVE_REQUIRED_LIBS} ${QT_QTDECLARATIVE_LIBRARY})


set(qttrailers_SRCS
    trailers.cpp
    psmoveqt.cpp
)

set(qttrailers_MOC_HDRS
    psmoveqt.hpp
)

set(qttrailers_RCCS
    trailers.qrc
)

qt4_wrap_cpp(qttrailers_MOC_SRCS ${qttrailers_MOC_HDRS})
qt4_add_resources(qttrailers_RCC_SRCS ${qttrailers_RCCS})

add_executable(qttrailers ${qttrailers_SRCS} ${qttrailers_MOC_SRCS} ${qttrailers_RCC_SRCS} ${PSMOVE_SRC})

target_link_libraries(qttrailers ${QT_LIBRARIES} ${PSMOVE_REQUIRED_LIBS} ${QT_QTDECLARATIVE_LIBRARY})

