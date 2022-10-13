# CMakeList.txt: QuantMiner 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)
add_compile_options("/std:c++20")

project ("QuantMiner")

add_executable (QuantMiner "QuantMiner.cpp" "QuantMiner.h" "Config.h" "Config.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET QuantMiner PROPERTY CXX_STANDARD 20)
endif()

find_package(jsoncpp CONFIG REQUIRED)
target_link_libraries(QuantMiner PRIVATE jsoncpp_lib jsoncpp_object JsonCpp::JsonCpp)

find_package(unofficial-sqlite3 CONFIG REQUIRED)
target_link_libraries(QuantMiner PRIVATE unofficial::sqlite3::sqlite3)

find_package(glog CONFIG REQUIRED)
target_link_libraries(QuantMiner PRIVATE glog::glog)