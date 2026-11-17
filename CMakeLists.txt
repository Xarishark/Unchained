# Unchained: Whiplash (original Xbox, 2003), statically recompiled

cmake_minimum_required(VERSION 3.25)
project(unchained LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# the SDK: sdk/ here, two folders up inside the SDK's templates, a checkout beside this repository, else downloaded
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sdk/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/sdk)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/../..)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBE-Recomp/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBE-Recomp)
else()
    include(FetchContent)
    FetchContent_Declare(xbe_sdk GIT_REPOSITORY https://github.com/TGP482/XboxXBE-Recomp.git GIT_TAG main)
    FetchContent_MakeAvailable(xbe_sdk)
    set(SDK ${xbe_sdk_SOURCE_DIR})
endif()
include(${SDK}/cmake/XbeRecomp.cmake)

file(GLOB XBE_HOST_SOURCES CONFIGURE_DEPENDS src/*/*.cpp)
list(FILTER XBE_HOST_SOURCES EXCLUDE REGEX "/src/game/")

# partitions as xbe_symbols wrote them into config/game.toml
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS config/game.toml)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/game.toml)
    file(STRINGS config/game.toml GAME_PARTITIONS REGEX "^partitions *=")
    string(REGEX MATCH "[0-9]+" GAME_PARTITIONS "${GAME_PARTITIONS}")
endif()
# no config yet: build.bat writes it with xbe_symbols, the change reconfigures
if(NOT GAME_PARTITIONS)
    set(GAME_PARTITIONS 8)
endif()

# target name = GAME_EXE in build.bat, xbe = GAME_XBE
add_xbe_image(Unchained default.xbe game.toml ${GAME_PARTITIONS} game)

# Bring-up: release builds drop Info and Debug log lines (the SDK's log.cpp filters on NDEBUG). This compiles only that
# file without NDEBUG, so a release build logs everything to logs/unchained_NNN.log, one file per run.
option(UNCHAINED_VERBOSE_LOG "Info and Debug lines in release logs" OFF)
if(UNCHAINED_VERBOSE_LOG)
    set_source_files_properties(${SDK}/runtime/core/log.cpp PROPERTIES COMPILE_OPTIONS "-UNDEBUG")
endif()

# the icon of the exe and its windows, drawn by tools/installer/art.py (no art from the game)
if(WIN32)
    target_sources(Unchained PRIVATE src/res/unchained.rc)
    # the file's version in Explorer's details: src/common/version.h, as every screen and the installer have it
    file(STRINGS src/common/version.h UNCHAINED_VERSION REGEX "kUnchainedVersion =")
    string(REGEX MATCH "\"([^\"]+)\"" UNCHAINED_VERSION "${UNCHAINED_VERSION}")
    set(UNCHAINED_VERSION "${CMAKE_MATCH_1}")
    # "0.1" or "0.1.2": the parts it leaves out are 0
    string(REGEX MATCHALL "[0-9]+" UNCHAINED_VERSION_PARTS "${UNCHAINED_VERSION}")
    list(APPEND UNCHAINED_VERSION_PARTS 0 0 0 0)
    list(SUBLIST UNCHAINED_VERSION_PARTS 0 4 UNCHAINED_VERSION_PARTS)
    list(JOIN UNCHAINED_VERSION_PARTS "," UNCHAINED_VERSION_NUMBERS)
    set_source_files_properties(src/res/unchained.rc PROPERTIES COMPILE_DEFINITIONS
        "UNCHAINED_VERSION=\"${UNCHAINED_VERSION}\";UNCHAINED_VERSION_NUMBERS=${UNCHAINED_VERSION_NUMBERS}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS src/common/version.h)
endif()

# the fonts every window of ours draws with, the same on Windows and Linux (fonts/, SIL Open Font License), beside the
# executable as the releases have them
add_custom_command(TARGET Unchained POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:Unchained>/fonts
    # the boot splash (GameInfo::splash), drawn by tools/installer/art.py --splash
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/res/splash.png $<TARGET_FILE_DIR:Unchained>)
