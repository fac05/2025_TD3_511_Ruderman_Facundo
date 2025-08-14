# Crear la biblioteca estática "ds1307" con los archivos fuente
add_library(ds1307 STATIC
    src/ds1307.c
)

# Linkeo dependencias de la bibliotecas
target_link_libraries(ds1307
    pico_stdlib
    hardware_i2c
)

# Incluir las cabeceras de la biblioteca
target_include_directories(ds1307 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)
