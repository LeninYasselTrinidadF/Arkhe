cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
    "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(Arkhe)
set(CMAKE_CXX_STANDARD 20)

# Resource file (set early so it's available when calling add_executable)
set(RES_FILE "")
if(WIN32)
    # Prefer a configured resources.rc.in (if present), otherwise use the
    # repository resources.rc directly so the ICON resource gets compiled.
    if(EXISTS ${CMAKE_SOURCE_DIR}/resources.rc.in)
        configure_file(${CMAKE_SOURCE_DIR}/resources.rc.in ${CMAKE_BINARY_DIR}/resources.rc @ONLY)
        set(RES_FILE ${CMAKE_BINARY_DIR}/resources.rc)
    elseif(EXISTS ${CMAKE_SOURCE_DIR}/resources.rc)
        set(RES_FILE ${CMAKE_SOURCE_DIR}/resources.rc)
    endif()
endif()

find_package(raylib        CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(Arkhe
    main.cpp
    app.hpp
    data/loaders/msc_loader.cpp
    data/loaders/mathlib_loader.cpp
    data/loaders/crossref_loader.cpp
    data/loaders/dep_graph.cpp
    data/gen/mathlib_gen.cpp
    data/state/nav_state.cpp
    data/connect/terminal_input.cpp
    data/connect/vscode_bridge.cpp
    data/search/loogle.cpp
    data/search/search_utils.cpp
    data/editor/editor_io.cpp
    data/editor/body_parser.cpp
    data/editor/equation_cache.cpp

    # ui/aesthetic
    ui/aesthetic/font_manager.cpp
    ui/aesthetic/nine_patch.cpp
    ui/aesthetic/skin.cpp
    ui/aesthetic/theme.cpp

    # ui/info
    ui/info/info_crossrefs.cpp
    ui/info/info_description.cpp
    ui/info/info_header.cpp
    ui/info/info_panel.cpp
    ui/info/info_resources.cpp

    # ui/toolbar
    ui/toolbar/toolbar.cpp
    ui/toolbar/config_panel.cpp
    ui/toolbar/docs_panel.cpp
    ui/toolbar/entry_editor.cpp
    ui/toolbar/ubicaciones_panel.cpp
    ui/toolbar/editor_panel/editor_sections.cpp

    # ui/search_panel
    ui/search_panel/search_panel.cpp
    ui/search_panel/search_widgets.cpp
    ui/search_panel/local_search_list.cpp
    ui/search_panel/loogle_list.cpp

    # ui/widgets
    ui/widgets/panel_widget.cpp

    # ui/views/bubble
    ui/views/bubble/bubble_stats.cpp
    ui/views/bubble/bubble_draw.cpp
    ui/views/bubble/bubble_label.cpp
    ui/views/bubble/bubble_layout.cpp

    # ui/views/controls
    ui/views/controls/mode_switcher.cpp
    ui/views/controls/zoom_controls.cpp
    ui/views/controls/ext_buttons.cpp
    ui/views/controls/canvas_btn.cpp
    ui/views/controls/canvas_buttons.cpp
    ui/views/controls/nav_sync.cpp

    # ui/key_controls
    ui/key_controls/keyboard_nav.cpp
    ui/key_controls/kbnav_info/kbnav_info.cpp
    ui/key_controls/kbnav_search/kbnav_search.cpp

    # ui/views/dep
    ui/views/dep/dep_draw.cpp
    ui/views/dep/dep_sim.cpp

    # ui/views
    ui/views/bubble_view.cpp
    ui/views/dep_view.cpp

    # platform
    platform_win_icon.cpp

    ${RES_FILE}
)

target_sources(Arkhe PRIVATE
    vendor/tinyfiledialogs.c
)

target_include_directories(Arkhe PRIVATE
    vendor/
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(Arkhe PRIVATE
    raylib
    nlohmann_json::nlohmann_json
)

# Consola visible + ventana raylib (necesario para terminal_input)
if(WIN32)
    target_link_options(Arkhe PRIVATE "/SUBSYSTEM:CONSOLE")
endif()

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