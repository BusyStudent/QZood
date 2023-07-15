add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_languages("c++17")

if is_host("windows") then
    add_cxxflags("cl::/utf-8")
end

target("common")
    add_rules("qt.static")
    add_frameworks("QtCore")
    add_files("./*.hpp")
    add_files("./*.cpp")
target_end()