# CloudCompare example for standard plugins

# Add an option to CMake to control whether we build this plugin or not
option( PLUGIN_STANDARD_QSEGMENTER "Install qSegmenter plugin" OFF )

if ( PLUGIN_STANDARD_QSEGMENTER )
	project( qSegmenter )
	 
	AddPlugin( NAME ${PROJECT_NAME} )
		
	add_subdirectory( include )
	add_subdirectory( src )
	
	# set dependencies to necessary libraries
	# target_link_libraries( ${PROJECT_NAME} LIB1 )
endif()
