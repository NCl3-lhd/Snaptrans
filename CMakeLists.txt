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
  src/settings_window.cpp
  src/hotkey_manager.cpp
  ${IMGUI_SOURCES}
)

# ==========================================================
# 🌟 7. 系统级打包：为不同系统准备资源文件 (图标注入)
# ==========================================================
if(WIN32)
  # Windows 平台：使用当前目录下的 app.rc 脚本
  set(APP_ICON_RESOURCE ${CMAKE_CURRENT_SOURCE_DIR}/app.rc)
elseif(APPLE)
  # macOS 平台：使用 .icns 图标文件，并将其塞进 App Bundle 的 Resources 目录
  set(APP_ICON_RESOURCE ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/tray.icns)

  if(EXISTS ${APP_ICON_RESOURCE})
    set_source_files_properties(${APP_ICON_RESOURCE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  endif()
else()
  # Linux: 不需要系统级打包图标文件
  set(APP_ICON_RESOURCE "")
endif()

# ==========================================================
# 🌟 8. 创建可执行文件 (动态区分打包模式)
# ==========================================================
if(APPLE)
  # Mac 必须加上 MACOSX_BUNDLE，才会生成 .app 应用程序
  add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_SOURCES} ${APP_ICON_RESOURCE})

  # 配置 Mac App Bundle 的 Info.plist 属性
  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_STRING "Snaptrans - Screenshot and Translation Tool"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.snaptrans.app"
    MACOSX_BUNDLE_BUNDLE_NAME "Snaptrans"
    MACOSX_BUNDLE_BUNDLE_VERSION "0.1.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
    MACOSX_BUNDLE_LONG_VERSION_STRING "0.1.0"
    MACOSX_BUNDLE_ICON_FILE "tray.icns" # 核心：告诉苹果系统主图标是谁
  )
else()
  # Windows (加上 WIN32 标志隐藏后台黑框框控制台) 和 Linux 的正常编译
  add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES} ${APP_ICON_RESOURCE})
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

# 11. 平台特定配置
if(APPLE)
  # macOS: 链接 Cocoa 和 CoreGraphics 框架
  target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa" "-framework CoreGraphics")
  target_compile_definitions(${PROJECT_NAME} PRIVATE TRAY_APPKIT=1)

  # 屏蔽 OpenGL 废弃警告
  target_compile_definitions(${PROJECT_NAME} PRIVATE GL_SILENCE_DEPRECATION)

  # macOS: 需要 Carbon 框架处理全局键盘拦截
  find_library(CARBON_FRAMEWORK Carbon)
  if(NOT CARBON_FRAMEWORK)
    message(FATAL_ERROR "Carbon framework not found! Apple environment required.")
  endif()
  target_link_libraries(${PROJECT_NAME} PRIVATE ${CARBON_FRAMEWORK})

elseif(WIN32)
  # Windows: 不需要额外库
  target_compile_definitions(${PROJECT_NAME} PRIVATE TRAY_WINAPI=1)

  # Windows: 需要 User32 处理 RegisterHotKey 和 GetMessage
  target_link_libraries(${PROJECT_NAME} PRIVATE user32)

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

# 13. 统一且清爽的跨平台资源复制逻辑
set(ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)

if(EXISTS ${ASSETS_DIR})
  if(APPLE)
    # macOS: 拷贝到 App Bundle 里
    add_custom_target(SyncAssets ALL
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${ASSETS_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources/assets
      COMMENT "Syncing assets to macOS App Bundle..."
    )
  else()
    # Win/Linux: 直接拷贝到当前的 Build 目录
    add_custom_target(SyncAssets ALL
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${ASSETS_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}/assets
      COMMENT "Syncing assets to build directory..."
    )
  endif()

  # 建立单向依赖：编译主程序之前，安全地完成同步任务
  add_dependencies(${PROJECT_NAME} SyncAssets)
endif()