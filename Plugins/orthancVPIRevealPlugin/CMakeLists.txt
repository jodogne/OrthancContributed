# CMakeLists.txt for Plugin
message(INFO "+++ CMakeLists.txt for Plugin")

cmake_minimum_required(VERSION 2.8)

project(orthancVPIRevealPlugin)

include(Common/CMakeLists.txt)

add_library(PluginTest SHARED Plugin/Plugin.cpp)
