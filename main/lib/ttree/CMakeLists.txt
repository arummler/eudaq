cmake_dependent_option(EUDAQ_LIBRARY_BUILD_TTREE "lib/ttree library (requires ROOT)" OFF
  "ROOT_FOUND" OFF)

if(NOT EUDAQ_LIBRARY_BUILD_TTREE)
    message(STATUS "lib/ttree is NOT to be built (EUDAQ_LIBRARY_BUILD_TTREE=OFF)")
  unset(EUDAQ_TTREE_LIBRARY CACHE)
  unset(TTREE_LIBRARIES CACHE)
  return()
endif()

get_filename_component(EUDAQ_COMPONENT ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_TTREE_LIBRARY ${EUDAQ_COMPONENT} CACHE INTERNAL "eudaq ttree library")

add_library(${EUDAQ_COMPONENT} SHARED 
	src/TTreeFileWriter.cc
	src/RawEvent2TTreeConverter.cc
	src/TTreeEventConverter.cc)


target_link_libraries(${EUDAQ_COMPONENT} ${EUDAQ_CORE_LIBRARY} ROOT::Core ROOT::Tree)
target_include_directories(${EUDAQ_COMPONENT} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

install(TARGETS ${EUDAQ_COMPONENT}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

file(GLOB INC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/eudaq/*.hh")
install(FILES ${INC_FILES} DESTINATION include/eudaq)
