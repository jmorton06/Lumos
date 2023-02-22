add_executable(imcmd_demo
    main.cpp
)
target_link_libraries(imcmd_demo
PRIVATE
    imcmd
    ${CONAN_LIBS}
)
target_include_directories(imcmd_demo
PRIVATE
    # Project root directory
    ${CMAKE_SOURCE_DIR}/..
)
