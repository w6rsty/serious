if (WIN32)
    set(SDL3_ROOT "D:/Program Files/SDL3")
    set(SDL3_INCLUDE_DIR "${SDL3_ROOT}/include")
    set(SDL3_LIB_DIR "${SDL3_ROOT}/lib")
    set(SDL3_BIN_DIR "${SDL3_ROOT}/bin")

    add_library(SDL3::SDL3 SHARED IMPORTED GLOBAL)
    set_target_properties(
        SDL3::SDL3 PROPERTIES
            IMPORTED_LOCATION "${SDL3_BIN_DIR}/SDL3.dll"
            IMPORTED_IMPLIB "${SDL3_LIB_DIR}/SDL3.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIR}"
    )

    add_library(SDL3 INTERFACE IMPORTED GLOBAL)
    target_link_libraries(SDL3 INTERFACE SDL3::SDL3)
endif()