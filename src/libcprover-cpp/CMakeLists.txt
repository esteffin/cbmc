#file(GLOB_RECURSE sources "api.cpp")

file(GLOB_RECURSE headers "*.h")


# This step builds the API in the form of a statically linked library (libcprover-api-cpp.a)
#add_library(cprover-lib ${sources})
add_library(cprover-lib api.cpp options.cpp)


# Being a library we should include them privately, but for now fair enough
#generic_includes(cprover-lib)


# Get all propreties that cmake supports
if(NOT CMAKE_PROPERTY_LIST)
    execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)

    # Convert command output into a CMake list
    string(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
    string(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
endif()

function(print_properties)
    message("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction()

function(print_target_properties target)
    if(NOT TARGET ${target})
        message(STATUS "There is no target named '${target}'")
        return()
    endif()

    foreach(property ${CMAKE_PROPERTY_LIST})
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" property ${property})

        # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
        if(property STREQUAL "LOCATION" OR property MATCHES "^LOCATION_" OR property MATCHES "_LOCATION$")
            continue()
        endif()

        get_property(was_set TARGET ${target} PROPERTY ${property} SET)
        if(was_set)
            get_target_property(value ${target} ${property})
            message("${target} ${property} = ${value}")
        endif()
    endforeach()
endfunction()

print_target_properties(cbmc)

target_include_directories(cprover-lib
        PRIVATE
        ${CBMC_BINARY_DIR}
        ${CBMC_SOURCE_DIR}
#        ${CMAKE_CURRENT_BINARY_DIR}
        )



target_include_directories(cprover-lib PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
        "$<INSTALL_INTERFACE:include>")

target_link_libraries(cprover-lib
        PUBLIC
            goto-programs
            util
            langapi
            ansi-c
            analyses
            goto-instrument-lib
            big-int
            linking
            goto-checker
            solvers
            assembler
            xml
            json
            json-symtab-language
            cpp
            jsil
            statement-list
            goto-symex
            pointer-analysis
            cbmc-lib
        )

add_library(combined_crazyness SHARED dummy.cpp)

target_link_libraries(combined_crazyness PUBLIC
        cprover-lib)
#        $<LINK_LIBRARY:WHOLE_ARCHIVE,cprover-lib>)

#for solver_lib in solver_libs {
#println!("cargo:rustc-link-lib=static={}", solver_lib);
#}


#add_library(cprover-api dummy.cpp)
#
#target_include_directories(cprover-api
#        PUBLIC "$<INSTALL_INTERFACE:include>")
#
#get_target_property(_input_type solvers TYPE)
#message(FATAL_ERROR "${_input_type}")

#message(FATAL_ERROR "sat_impl: ${sat_impl}, sat_impl_str: ${sat_impl_str}")
#message(FATAL_ERROR "SAT_IMPL_TARGET_NAMES: ${SAT_IMPL_TARGET_NAMES}")

#add_custom_command(TARGET cprover-lib POST_BUILD
##        COMMAND for f in \\\$(ls ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/lib/*.a); do echo Unpacking \\\${f}; ar -x \\\${f}; done
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:goto-programs>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:util>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:langapi>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:ansi-c>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:analyses>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:goto-instrument-lib>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:big-int>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:linking>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:goto-checker>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:solvers>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:assembler>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:xml>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:json>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:json-symtab-language>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:cpp>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:jsil>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:statement-list>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:goto-symex>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:pointer-analysis>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:cbmc-lib>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:cprover-lib>
#        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:minisat2-condensed> # this fails if more than 1, but ATM only 1
#        COMMAND ${CMAKE_AR} -qcs $<TARGET_FILE:cprover-lib> *.o
##        COMMAND echo "polok polok polok $<$<TARGET_PROPERTY:LINK_LIBRARIES>:cprover-lib> polok polok polok"
#        COMMAND echo polok polok polok $<$<POSITION_INDEPENDENT_CODE>:cprover-lib> polok polok polok
#        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
#        )

set(lib_version 153)

set_target_properties(cprover-lib
        PROPERTIES
        OUTPUT_NAME "cprover" # libcprover.a
        SOVERSION "${lib_version}"
        PUBLIC_HEADER "${headers}"
        )

set(INSTALL_CMAKE_CONFIG_DIR "${CMAKE_INSTALL_DATAROOTDIR}/cmake/cprover")

install(TARGETS         cprover-lib
#        EXPORT          cproverTargets
        ARCHIVE         DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                        COMPONENT lib
        LIBRARY         DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                        COMPONENT lib
        PUBLIC_HEADER   DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cprover"
                        COMPONENT lib
        )

#install(EXPORT cbmcTargets
#        DESTINATION ${INSTALL_CMAKE_CONFIG_DIR}
#        COMPONENT lib)

#include(CmakePackageConfigHelpers)
#
#write_basic_package_version_file("${CMAKE_BINARY_DIR}/cbmcConfigVersion.cmake"
#                                 VERSION ${lib_version}
#                                 COMPATIBILITY SameMajorVersion) # Check this with Fotis
#
## Using relative .. path because the project definition is in cbmc/src...
#configure_package_config_file("${CBMC_SOURCE_DIR}/../cmake/cprover-api-cppConfig.cmake.in"
#                              "${CMAKE_CURRENT_BINARY_DIR}/cbmcConfig.cmake"
#                              INSTALL_DESTINATION "${INSTALL_CMAKE_CONFIG_DIR}")
#
#install(FILES
#            "${CMAKE_BINARY_DIR}/cbmc.cmake"
#            "${CMAKE_CURRENT_BINARY_DIR}/cbmcConfig.cmake"
#        DESTINATION
#            "${INSTALL_CMAKE_CONFIG_DIR}")



