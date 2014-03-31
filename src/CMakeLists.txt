find_package( VRPN REQUIRED )
find_package( X11 REQUIRED )
if ( NOT X11_XTest_FOUND )
	message( SEND_ERROR "XTest extension not found!")
endif()
#find_package( GMTL REQUIRED )

include_directories ( ${VRPN_INCLUDE_DIRS} ${X11_X11_INCLUDE_PATH} ${X11_XTest_INCLUDE_PATH} )

add_executable ( vrpnmouse vrpnmouse.cc)
target_link_libraries ( vrpnmouse ${VRPN_LIBRARIES} ${X11_X11_LIB} ${X11_XTest_LIB} )

install ( TARGETS vrpnmouse ${DEFAULT_INSTALL_STANZA} )
