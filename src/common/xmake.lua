add_rules("mode.debug", "mode.release", "mode.releasedbg")

add_requires("sqlite3")

set_languages("c++17")

if is_host("windows") then
    add_cxxflags("cl::/utf-8")
end

target("myDebug")
    add_rules("qt.static")

    add_frameworks("QtCore", "QtGui", "QtWidgets")

    add_files("./myGlobalLog.cpp")
    add_files("./myGlobalLog.hpp")
target_end()

target("promise")
    add_rules("qt.static")

    add_frameworks("QtCore", "QtGui", "QtWidgets")

    add_deps("myDebug")

    add_files("./promise.cpp")
    add_files("./promise.hpp")
target_end()

target("sqlite")
    add_rules("qt.static")

    add_packages("sqlite3")

    add_frameworks("QtCore")
    add_deps("promise")

    add_files("./sqlite.cpp")
    add_files("./sqlite.hpp")
target_end()

target("common")
    add_rules("qt.static")

	add_frameworks("QtCore", "QtGui", "QtWidgets")

    add_deps("sqlite", "promise", "myDebug")
    add_deps("promise", "myDebug")

    add_headerfiles("./*.hpp")
    add_files("./configs.hpp")
    add_files("./danmaku.hpp")
    add_files("./stl.hpp")
    add_files("./configs.cpp")
    add_files("./danmaku.cpp")
target_end()