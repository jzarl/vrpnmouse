###############################################################################
# Project Information:
###############################################################################
project ( VRPNMOUSE )

###############################################################################
# Set Policies:
###############################################################################
cmake_minimum_required (VERSION 2.8.9)

###############################################################################
# enable rules to make ctest-targets:
###############################################################################
option (VRPNMOUSE_ENABLE_TESTING "Enable ctest test-suite" ON )
if ( VRPNMOUSE_ENABLE_TESTING )
	enable_testing()
endif ( VRPNMOUSE_ENABLE_TESTING )

###############################################################################
# Configure install prefixes:
###############################################################################
# As long as these paths are not absolute, they are relative to
# CMAKE_INSTALL_PREFIX:
set (INSTALL_BIN_DIR bin CACHE STRING "Installation subdirectory for executables")
set (INSTALL_LIB_DIR lib CACHE STRING "Installation subdirectory for libraries")
set (INSTALL_INCLUDE_DIR include CACHE STRING "Installation subdirectory for header files")
set (INSTALL_DOC_DIR share/doc/vrpnmouse CACHE STRING "Installation subdirectory for documentation")

# to make install statements shorter:
set( DEFAULT_INSTALL_STANZA
	RUNTIME
		DESTINATION "${INSTALL_BIN_DIR}"
		COMPONENT runtime
	LIBRARY
		DESTINATION "${INSTALL_LIB_DIR}"
		COMPONENT runtime
	ARCHIVE
		DESTINATION "${INSTALL_LIB_DIR}"
		COMPONENT dev
	PUBLIC_HEADER
		DESTINATION "${INSTALL_INCLUDE_DIR}"
		COMPONENT dev
   )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )

###############################################################################
# Provide Uninstall-target:
###############################################################################
configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
		IMMEDIATE @ONLY
		)

add_custom_target(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake")

###############################################################################
# General install-targets:
###############################################################################
install( FILES README.md
    DESTINATION ${INSTALL_DOC_DIR} )

install(
	FILES scripts/vrpnmouse-curvedscreen.sh
	DESTINATION ${INSTALL_BIN_DIR}
	PERMISSIONS
		OWNER_READ OWNER_WRITE OWNER_EXECUTE
		GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE
	)

###############################################################################
# Locate dependencies (libraries etc.):
###############################################################################
# enable built-in FindPACKAGE.cmake scripts:
list ( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" )

###############################################################################
# Subdirectories in which CMakeLists.txt are located:
###############################################################################
add_subdirectory (src)

