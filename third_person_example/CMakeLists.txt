cmake_minimum_required(VERSION 3.9)
project(Third_Person_Example)

OPTION(BUILD_TESTS "Enable test compilation" OFF)

SET(SIMULANT_INCLUDE_FOLDER "" CACHE STRING "Specify the path to the Simulant includes")
SET(SIMULANT_LIBRARY_FOLDER "" CACHE STRING "Specify the path to the Simulant libraries")
SET(USE_GLOBAL_SIMULANT TRUE)

include(CheckIPOSupported)
check_ipo_supported(RESULT FLTO_SUPPORTED OUTPUT FLTO_ERROR)

SET(OpenGL_GL_PREFERENCE "LEGACY")

IF(SIMULANT_FLAVOUR)
    MESSAGE("-- Building Flavour: ${SIMULANT_FLAVOUR}")
ENDIF()

# If an include override was passed, use that
IF(NOT ${SIMULANT_INCLUDE_FOLDER} MATCHES "^$")
    STRING(REPLACE "{PROJECT_ROOT}" "${CMAKE_SOURCE_DIR}" SIMULANT_INCLUDE_FOLDER "${SIMULANT_INCLUDE_FOLDER}")
    MESSAGE("Using ${SIMULANT_INCLUDE_FOLDER} for Simulant includes")
    INCLUDE_DIRECTORIES(BEFORE ${SIMULANT_INCLUDE_FOLDER})
    SET(USE_GLOBAL_SIMULANT FALSE)
ENDIF()

# If a library override was passed, use that
IF(NOT ${SIMULANT_LIBRARY_FOLDER} MATCHES "^$")
    STRING(REPLACE "{PROJECT_ROOT}" "${CMAKE_SOURCE_DIR}" SIMULANT_LIBRARY_FOLDER "${SIMULANT_LIBRARY_FOLDER}")
    MESSAGE("Using ${SIMULANT_LIBRARY_FOLDER} for Simulant library")
    LINK_DIRECTORIES(${SIMULANT_LIBRARY_FOLDER})
    LINK_LIBRARIES(simulant)
    SET(USE_GLOBAL_SIMULANT_FALSE)
ENDIF()

IF(${USE_GLOBAL_SIMULANT})
    # If we were missing either override, use the cmake module
    FIND_PACKAGE(SIMULANT)
    INCLUDE_DIRECTORIES(${SIMULANT_INCLUDE_DIRS})
    LINK_LIBRARIES(${SIMULANT_LIBRARY})
    MESSAGE("-- Using Simulant from OS")
ENDIF()

FIND_PACKAGE(PkgConfig REQUIRED)

IF(PLATFORM_DREAMCAST)
    LINK_LIBRARIES(stdc++ kallisti gcc c kallisti ppp m)
ELSEIF(PLATFORM_PSP)
    ADD_DEFINITIONS(-D_PSP_FW_VERSION=150)
ELSE()
    FIND_PACKAGE(SDL2 REQUIRED)
    FIND_PACKAGE(OpenAL REQUIRED)
    FIND_PACKAGE(OpenGL REQUIRED)

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

    INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${GL_INCLUDE_DIRS})
    LINK_LIBRARIES(${SDL2_LIBRARIES} ${GL_LIBRARIES} m)
ENDIF()

IF(WIN32)
    IF(MINGW)
        # mingw has bugs which prevent flto
        SET(FLTO_SUPPORTED FALSE)
    endif()

    add_definitions(-D_WIN32_WINNT=0x0601 -D_WINSOCK_DEPRECATED_NO_WARNINGS -Dmain=SDL_main)
    link_libraries(wsock32 ws2_32 iphlpapi)
ENDIF()

FILE(GLOB_RECURSE SOURCES FOLLOW_SYMLINKS ${CMAKE_SOURCE_DIR}/sources/*.cpp ${CMAKE_SOURCE_DIR}/sources/*.c)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

IF(PLATFORM_DREAMCAST)
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mfsrra -mfsca -ffast-math")
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -mfsrra -mfsca -ffast-math")
ENDIF()

SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -g0 -s -fomit-frame-pointer -fstrict-aliasing")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 -fstrict-aliasing")

# This should happen automatically, but, it doesn't seem to on the Dreamcast
if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DNDEBUG)
    add_definitions(-DKOS_DEBUG=0)
endif()

IF(PLATFORM_PSP)
    SET(PSP_TITLE "third_person_example")
    SET(PSP_MAJOR_VERSION 1)
    SET(PSP_MINOR_VERSION 0)
    configure_file(${CMAKE_SOURCE_DIR}/conf/psp_module_info.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/psp_module_info_third_person_example.cpp)
    set(SAMPLE_SOURCES ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/psp_module_info_third_person_example.cpp)
endif()

add_definitions("-Wall")
add_executable(third_person_example ${SOURCES})

if(FLTO_SUPPORTED)
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET third_person_example PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${FLTO_ERROR}>")
endif()


IF(PLATFORM_PSP)
    message("Creating PBP")
    create_pbp_file(TARGET third_person_example)
ENDIF()

# On the Dreamcast it's important to move debugging symbols
# from the .elf to a separate file otherwise the generated .elf
# will be too large for the amount of RAM
if(PLATFORM_DREAMCAST)
if(CMAKE_BUILD_TYPE MATCHES Debug)

set(EXE_FILENAME third_person_example.elf)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}.debug
    COMMAND ${CMAKE_OBJCOPY} --only-keep-debug ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME} ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}.debug && ${CMAKE_OBJCOPY} -S ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}
    DEPENDS third_person_example
    COMMENT "Separating debug info for ${EXE_FILENAME}"
)
add_custom_target(${EXE_FILENAME}.debug.target ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${EXE_FILENAME}.debug)

endif()
endif()

FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_BINARY_DIR}/assets/)

IF(BUILD_TESTS)
    ## Add the test executable
    ENABLE_TESTING()

    # This is the path to the Simulant test generator
    SET(TEST_GENERATOR ${CMAKE_SOURCE_DIR}/tools/simulant/test_generator.py)

    # This is the path to the generate test main.cpp
    SET(TEST_MAIN_FILENAME /tests/main.cpp)

    # We include all project source files, except main.cpp so we can link in the tests
    SET(TEST_SOURCES ${SOURCES})
    LIST(REMOVE_ITEM TEST_SOURCES ${CMAKE_SOURCE_DIR}/sources/main.cpp)

    # Find all the tests, these are header files under "tests"
    FILE(GLOB TEST_FILES ${CMAKE_SOURCE_DIR}/tests/*.h)

    IF(TEST_FILES)
        # Generate the test main.cpp
        ADD_CUSTOM_COMMAND(
            OUTPUT ${CMAKE_SOURCE_DIR}${TEST_MAIN_FILENAME}
            COMMAND ${TEST_GENERATOR} --output ${CMAKE_SOURCE_DIR}${TEST_MAIN_FILENAME} ${TEST_FILES}
            DEPENDS ${TEST_FILES} ${TEST_GENERATOR}
        )

        # Add the tests executable
        ADD_EXECUTABLE(
            tests
            tests/main.cpp
            ${TEST_SOURCES}
        )

        ADD_TEST(tests tests)
    ENDIF()
ENDIF(BUILD_TESTS)

# Installation rules!
FILE(GLOB ASSET_FILES ${CMAKE_SOURCE_DIR}/assets/.*)
INSTALL(TARGETS third_person_example RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT RUNTIME)
INSTALL(FILES ${ASSET_FILES} DESTINATION "share/third_person_example/assets")