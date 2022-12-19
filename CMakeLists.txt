cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

project( "CoroutineSamples" LANGUAGES CXX )
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

set( MAIN_BINARY_DIR "${CMAKE_BINARY_DIR}/bin" CACHE INTERNAL "" FORCE )
set( CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "installation directory" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" CACHE INTERNAL "" FORCE )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" CACHE INTERNAL "" FORCE )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" CACHE INTERNAL "" FORCE )
	
file( GLOB SOURCES "*.h" "*.cpp" )
add_executable( "CoroutineSamples" ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( TARGET "CoroutineSamples" PROPERTY FOLDER "" )

set_target_properties( "CoroutineSamples" PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES )
target_compile_features( "CoroutineSamples" PUBLIC cxx_std_20 )
