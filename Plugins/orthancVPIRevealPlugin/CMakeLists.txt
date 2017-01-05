# CMakeLists.txt for VPI Plugin
message(INFO "+++ CMakeLists.txt for VPI Plugin")

cmake_minimum_required(VERSION 2.8)

project(orthancVPIRevealPlugin)

include(Common/CMakeLists.txt)

add_library(VPI_Plugin SHARED Plugin/Plugin.cpp)
