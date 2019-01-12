project(main)

add_executable(${PROJECT_NAME}
        src/main.cpp)

target_link_libraries(${PROJECT_NAME}
        glad
        glfw)

target_include_directories(${PROJECT_NAME}
        PRIVATE include)

set(SHADERS
        shaders/main.frag
        shaders/main.vert)
add_custom_target(shaders DEPENDS ${SHADERS})

add_custom_command(
        TARGET shaders PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders/
        COMMENT "clearing shaders"
)
add_custom_command(
        TARGET shaders POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ ${CMAKE_CURRENT_BINARY_DIR}/shaders/
        COMMENT "copying shaders"
)

add_dependencies(${PROJECT_NAME} shaders)