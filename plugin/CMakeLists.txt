cmake_minimum_required(VERSION 3.21)

project(
    ArcaneRecoil
    VERSION 1.2.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# ── Dependencies ────────────────────────────────────────────────────────────
find_package(CommonLibSSE CONFIG REQUIRED)
find_package(spdlog        CONFIG REQUIRED)

# ── Plugin target ───────────────────────────────────────────────────────────
# add_commonlibsse_plugin is provided by CommonLibSSE; it creates a .dll
# with the correct SKSE metadata and links against CommonLibSSE automatically.
add_commonlibsse_plugin(${PROJECT_NAME}
    NAME        "Arcane Recoil"
    AUTHOR      "BigQuinn"
    DESCRIPTION "Reactive spell shield — charging spell absorbs incoming damage"
    SOURCES
        src/Main.cpp
        src/Settings.cpp
        src/ArcaneRecoil.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        spdlog::spdlog_header_only
)

# Precompiled header speeds up compilation
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)

# ── Post-build: copy .dll to your Skyrim SKSE plugins folder ────────────────
# Set SKYRIM_PATH in your environment or override here.
if(DEFINED ENV{SKYRIM_PATH})
    set(OUTPUT_DIR "$ENV{SKYRIM_PATH}/Data/SKSE/Plugins")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${PROJECT_NAME}>
            "${OUTPUT_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMENT "Copying plugin to ${OUTPUT_DIR}"
    )
endif()
