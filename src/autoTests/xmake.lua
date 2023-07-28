add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.asan", "mode.profile")

set_languages("c++17")

if is_host("windows") then
    add_cxxflags("cl::/utf-8")
    add_cxxflags("cl::/Zc:__cplusplus")
    add_cxxflags("cl::/permissive-")
end 

add_requires("gtest")
add_packages("gtest")

target("autoTests")
    set_kind("binary")

    add_files("main.cpp")
target_end()