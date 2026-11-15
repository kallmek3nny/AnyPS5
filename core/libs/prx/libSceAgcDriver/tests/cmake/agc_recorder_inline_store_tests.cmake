add_test_executable(agc_recorder_inline_store_tests EXCLUDE_FROM_ALL tests/InlineStoreOrdering.cpp)
target_include_directories(agc_recorder_inline_store_tests PRIVATE ${LIBS_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/core/shader/recompiler ${ANYPS5_VULKAN_HEADERS_INCLUDE_DIR})
target_link_libraries(agc_recorder_inline_store_tests PRIVATE libSceAgcDriver libc)
set_target_properties(agc_recorder_inline_store_tests PROPERTIES CXX_EXTENSIONS OFF)
configure_windows_unwind(agc_recorder_inline_store_tests)

if(BUILD_TESTING)
    set_target_properties(agc_recorder_inline_store_tests PROPERTIES EXCLUDE_FROM_ALL OFF)
    add_test(NAME agc_recorder_inline_store_order COMMAND ${CMAKE_COMMAND} -E env
        --unset=APS5_NO_LABEL_RUNS APS5_LABEL_RUNS_INLINE=1 $<TARGET_FILE:agc_recorder_inline_store_tests>)
    set_tests_properties(agc_recorder_inline_store_order PROPERTIES TIMEOUT 20)
    if(WIN32)
        set_tests_properties(agc_recorder_inline_store_order PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:libc>")
    endif()
endif()
