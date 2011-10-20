LIBRARY_NAME=PsMoveApi
LIBRARY_VERSION=0.1

valac --library=${LIBRARY_NAME} \
    -h ${LIBRARY_NAME}.h \
    ${LIBRARY_NAME}.vala ${LIBRARY_NAME}.c \
    -X -fPIC \
    -X -shared \
    -o ${LIBRARY_NAME}.so \
    -X -lpsmoveapi \
    --gir ${LIBRARY_NAME}-${LIBRARY_VERSION}.gir

g-ir-compiler --shared-library=${LIBRARY_NAME}.so \
    --output=${LIBRARY_NAME}-${LIBRARY_VERSION}.typelib \
    ${LIBRARY_NAME}-${LIBRARY_VERSION}.gir

