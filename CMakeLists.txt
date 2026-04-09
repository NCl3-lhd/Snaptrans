cmake_minimum_required(VERSION 3.14)
project(Snaptrans)

# 使用 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置默认构建类型为 Debug（用于 VS Code IntelliSense）
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# 强制 Windows MSVC 使用 UTF-8 编码（解决中文乱码问题）
if(MSVC)
  add_compile_options(/utf-8)
endif()

# 🔧 优化：禁用 FetchContent 自动更新（加速 cmake 配置）
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)

# 1. 自动下载并配置 GLFW
include(FetchContent)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
  GIT_SHALLOW TRUE
)

# 关闭 GLFW 不必要的测试和文档编译，加快速度
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# 2. 自动下载并配置 ImGui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking # 使用 docking 分支（更现代）
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

# 3. 自动下载并配置 nlohmann/json（重型依赖，使用 FetchContent）
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(json)

# 4. 查找操作系统自带的 OpenGL
find_package(OpenGL REQUIRED)

# 5. 收集 ImGui 的源文件
set(IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# 6. 项目源文件
set(PROJECT_SOURCES
  src/main.cpp
  src/tray_manager.cpp
  src/i18n_manager.cpp
  ${IMGUI_SOURCES}
)

# 7. 创建我们的可执行文件 Snaptrans
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# 8. macOS: 创建 App Bundle (清理了原先冗余的局部复制逻辑)
if(APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_STRING "Snaptrans - Screenshot and Translation Tool"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.snaptrans.app"
    MACOSX_BUNDLE_BUNDLE_NAME "Snaptrans"
    MACOSX_BUNDLE_BUNDLE_VERSION "0.1.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
    MACOSX_BUNDLE_LONG_VERSION_STRING "0.1.0"
  )

  # 将图标文件注册到 bundle 的 Resources 目录
  set(ICON_FILE trayTemplate.png)
  set(ICON_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/${ICON_FILE})

  if(EXISTS ${ICON_SOURCE})
    set_source_files_properties(${ICON_SOURCE} PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(${PROJECT_NAME} PRIVATE ${ICON_SOURCE})
  endif()
endif()

# 9. 告诉编译器去哪里找头文件 (#include)
target_include_directories(${PROJECT_NAME} PRIVATE
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 10. 配置 tray 库支持
set(TRAY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tray)
target_include_directories(${PROJECT_NAME} PRIVATE ${TRAY_DIR})

# 11. 平台特定配置 (重点修复区)
if(APPLE)
  # macOS: 链接 Cocoa 和 CoreGraphics 框架（修复：补充屏幕抓取底层依赖）
  target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa" "-framework CoreGraphics")
  target_compile_definitions(${PROJECT_NAME} PRIVATE TRAY_APPKIT=1)

  # 屏蔽 OpenGL 废弃警告
  target_compile_definitions(${PROJECT_NAME} PRIVATE GL_SILENCE_DEPRECATION)
elseif(WIN32)
  # Windows: 不需要额外库
  target_compile_definitions(${PROJECT_NAME} PRIVATE TRAY_WINAPI=1)
elseif(UNIX AND NOT APPLE)
  # Linux: 需要 GTK3 和 AppIndicator
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
  pkg_check_modules(APPINDICATOR REQUIRED appindicator3-0.1)

  target_include_directories(${PROJECT_NAME} PRIVATE ${GTK3_INCLUDE_DIRS} ${APPINDICATOR_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} PRIVATE ${GTK3_LIBRARIES} ${APPINDICATOR_LIBRARIES})
  target_compile_definitions(${PROJECT_NAME} PRIVATE TRAY_APPINDICATOR=1)
endif()

# 12. 链接通用库
target_link_libraries(${PROJECT_NAME} PRIVATE
  glfw
  OpenGL::GL
  nlohmann_json::nlohmann_json
)

# 13. 统一且清爽的跨平台资源复制逻辑 (修复：防止资源多重拷贝)
set(ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)

if(EXISTS ${ASSETS_DIR})
  if(APPLE)
    # macOS: 将整个 assets 目录统一复制到 App Bundle 的 Resources 中
    # 结果路径: Snaptrans.app/Contents/Resources/assets/
    file(COPY ${ASSETS_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources)
  else()
    # Win/Linux: 复制到与可执行文件同级的构建目录
    # 结果路径: build/assets/
    file(COPY ${ASSETS_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  endif()
endif()