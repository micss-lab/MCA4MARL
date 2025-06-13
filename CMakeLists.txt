cmake_minimum_required(VERSION 3.30)

project(MCA4MARL)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "~/Documents/MCA4MARL/")

add_executable(main
        # .h files
        src/astar.h
        src/constants.h
        src/experiments.h
        src/hashpair.h
        src/maze.h
        src/multiagent.h
        src/policyvisualizer.h
        src/singleagent.h
        src/startstats.h
        src/table.h
        src/testpolicy.h
        src/threadresult.h
        src/treenode.h
        src/treestrategy.h
        src/pathstate.h

        # .cpp files
        src/astar.cpp
        src/experiments.cpp
        src/hashpair.cpp
        src/maze.cpp
        src/multiagent.cpp
        src/policyvisualizer.cpp
        src/singleagent.cpp
        src/startstats.cpp
        src/table.cpp
        src/testpolicy.cpp
        src/treenode.cpp
        src/treestrategy.cpp
        src/main.cpp
)

find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)
target_link_libraries(main sfml-graphics sfml-window sfml-system)
