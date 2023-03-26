if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/async_nats-${PROJECT_VERSION}"
      CACHE PATH ""
  )
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package async_nats)

install(
    DIRECTORY
    include/
    "${PROJECT_BINARY_DIR}/export/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT async_nats_Development
)

install(
    TARGETS async_nats_async_nats nats_fabric
    EXPORT async_natsTargets
    RUNTIME #
    COMPONENT async_nats_Runtime
    LIBRARY #
    COMPONENT async_nats_Runtime
    NAMELINK_COMPONENT async_nats_Development
    ARCHIVE #
    COMPONENT async_nats_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    async_nats_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(async_nats_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${async_nats_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT async_nats_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${async_nats_INSTALL_CMAKEDIR}"
    COMPONENT async_nats_Development
)

install(
    EXPORT async_natsTargets
    NAMESPACE async_nats::
    DESTINATION "${async_nats_INSTALL_CMAKEDIR}"
    COMPONENT async_nats_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
