cmake_minimum_required(VERSION 3.14)
project(SnapTrans)

# 使用 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 自动下载并配置 GLFW (极简魔法)
include(FetchContent)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
)

# 关闭 GLFW 不必要的测试和文档编译，加快速度
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# 2. 查找操作系统自带的 OpenGL
find_package(OpenGL REQUIRED)

# 3. 收集 ImGui 的源文件
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui)
set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# 4. 创建我们的可执行文件 SnapTrans
add_executable(${PROJECT_NAME} src/main.cpp ${IMGUI_SOURCES})

# 5. 告诉编译器去哪里找头文件 (#include)
target_include_directories(${PROJECT_NAME} PRIVATE
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

# 6. 将 GLFW 和 OpenGL 链接到我们的程序
target_link_libraries(${PROJECT_NAME} PRIVATE glfw OpenGL::GL)