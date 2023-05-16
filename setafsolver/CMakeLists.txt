cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lstdc++fs -Wall -Wextra")
add_executable (solver "src/main.cpp" "header/main.hpp"  "src/datamodel/Argument.cpp" "src/datamodel/Clause.cpp" "src/parsing/ParserSimpleFormat.cpp" "header/parsing/Parser.hpp" "header/datamodel/Misc.hpp" "src/datamodel/Instance.cpp" "src/tools/Helper.cpp" "header/tools/Helper.hpp" "header/datamodel/Argument.hpp" "header/datamodel/Clause.hpp" "header/datamodel/Instance.hpp" "header/datamodel/Heuristics.hpp" "header/datamodel/Semantics.hpp"  "src/datamodel/Semantics.cpp" "src/datamodel/Heuristics.cpp" "header/solver.hpp"  "header/tools/IDTrie.hpp" "src/tools/IDTrie.cpp" "src/solver.cpp")
find_package(fmt)
target_link_libraries(solver -static stdc++fs fmt::fmt-header-only)