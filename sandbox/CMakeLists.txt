add_executable(sandbox)

aux_source_directory(. SANDBOX_SRC)
target_sources(sandbox PRIVATE ${SANDBOX_SRC})
target_link_libraries(sandbox PRIVATE serious SDL3::SDL3)
CopyDLL(sandbox)
PackageTarget(sandbox)