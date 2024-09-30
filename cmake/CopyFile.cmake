macro(CopyDLL target_name)
    if (WIN32)
        add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${SDL3_BIN_DIR}/SDL3.dll $<TARGET_FILE_DIR:${target_name}>)

    endif()
endmacro(CopyDLL)

macro(CopyShader target_name)
    if (WIN32)
        add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:${target_name}>/shaders"
        )
    endif()
endmacro(CopyShader)


function(PackageTarget target_name)
    set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/target")
    file(MAKE_DIRECTORY ${OUTPUT_DIR})
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "$<TARGET_FILE:${target_name}>"
        "${OUTPUT_DIR}/$<TARGET_FILE_NAME:${target_name}>"
        COMMENT "Copying target ${target_name} to ${OUTPUT_DIR}"
    )

    file(GLOB SPV_FILES "${CMAKE_SOURCE_DIR}/shaders/*.spv")

    foreach(SPV_FILE ${SPV_FILES})
        get_filename_component(FILE_NAME ${SPV_FILE} NAME)
        add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${SPV_FILE}"
            "${OUTPUT_DIR}/shaders/${FILE_NAME}"
            COMMENT "Copying ${FILE_NAME} to ${OUTPUT_DIR}"
        )
    endforeach()

    if (WIN32)
        add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${SDL3_BIN_DIR}/SDL3.dll"
            "${OUTPUT_DIR}/SDL3.dll"
            COMMENT "Copying SDL3.dll to ${OUTPUT_DIR}"
        )
    endif()
endfunction(PackageTarget)
