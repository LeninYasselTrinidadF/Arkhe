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
    ui/bubble_view.cpp
    ui/search_panel.cpp
    ui/info_panel.cpp
    search/loogle.cpp
)

target_include_directories(Arkhe PRIVATE ${CMAKE_SOURCE_DIR})

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
