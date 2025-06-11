cmake_minimum_required(VERSION 3.30)

project(MCA4MARL)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "~/Documents/MCA4MARL/")

add_executable(Maze-Ver6
        src/constants.h
        src/hashpair.h src/hashpair.cpp
        src/maze.h src/maze.cpp
        src/Maze-Ver6.cpp
        src/policyvisualizer.h src/policyvisualizer.cpp
        src/singleagent.h src/singleagent.cpp
        src/startstats.h src/startstats.cpp
        src/table.h src/table.cpp
        src/treenode.h src/treenode.cpp
        src/pathstate.h
)

find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)
target_link_libraries(Maze-Ver6 sfml-graphics sfml-window sfml-system)
