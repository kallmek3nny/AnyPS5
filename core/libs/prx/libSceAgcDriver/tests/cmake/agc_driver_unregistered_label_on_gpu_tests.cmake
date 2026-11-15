if(BUILD_TESTING)
    add_test(NAME agc_driver_unregistered_label_on_gpu_inline COMMAND ${CMAKE_COMMAND} -E env
        --unset=APS5_NO_LABEL_RUNS APS5_LABEL_RUNS_INLINE=1 $<TARGET_FILE:agc_driver_unregistered_label_on_gpu_tests>)
    set_tests_properties(agc_driver_unregistered_label_on_gpu_inline PROPERTIES
        ENVIRONMENT "ANYPS5_NO_SHADER_CACHE=1" TIMEOUT 30 SKIP_RETURN_CODE 77 WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    if(WIN32)
        set_tests_properties(agc_driver_unregistered_label_on_gpu_inline PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:libc>")
    endif()
endif()
