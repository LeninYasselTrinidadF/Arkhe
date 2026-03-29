cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
    "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(Arkhe)
set(CMAKE_CXX_STANDARD 20)

find_package(raylib        CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(Arkhe
    main.cpp
    data/msc_loader.cpp
    data/mathlib_loader.cpp
    data/crossref_loader.cpp

    # ui/core
    ui/core/font_manager.cpp
    ui/core/nine_patch.cpp
    ui/core/skin.cpp
    ui/core/theme.cpp

    # ui/info
    ui/info/info_crossrefs.cpp
    ui/info/info_description.cpp
    ui/info/info_header.cpp
    ui/info/info_panel.cpp
    ui/info/info_resources.cpp

    # ui/panels
    ui/panels/config_panel.cpp
    ui/panels/docs_panel.cpp
    ui/panels/entry_editor.cpp
    ui/panels/search_panel.cpp
    ui/panels/toolbar.cpp
    ui/panels/ubicaciones_panel.cpp

    # ui/widgets
    ui/widgets/panel_widget.cpp

    # ui raíz
    ui/bubble_stats.cpp
    ui/bubble_view.cpp
    # búsqueda
    search/loogle.cpp
)


target_sources(Arkhe PRIVATE
    vendor/tinyfiledialogs.c
)
target_include_directories(Arkhe PRIVATE
    vendor/
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/ui
)
target_link_libraries(Arkhe PRIVATE
    raylib
    nlohmann_json::nlohmann_json
)

add_custom_command(TARGET Arkhe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Arkhe>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Users/USUARIO/Documents/vcpkg/installed/x64-windows/bin/raylib.dll"
        $<TARGET_FILE_DIR:Arkhe>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Users/USUARIO/Documents/vcpkg/installed/x64-windows/bin/glfw3.dll"
        $<TARGET_FILE_DIR:Arkhe>
)
