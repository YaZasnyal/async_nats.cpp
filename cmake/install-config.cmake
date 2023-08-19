include(CMakeFindDependencyMacro)
find_dependency(Boost)

option(ASYNC_NATS_SHARED "Link shared library by default" ON)

include("${CMAKE_CURRENT_LIST_DIR}/async_natsTargets.cmake")
