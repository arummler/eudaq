cmake_dependent_option(EUDAQ_BUILD_STDEVENT_MONITOR "monitor/StdEventMonitor executable (requires ROOT)" OFF
  "ROOT_FOUND" OFF)

if(NOT EUDAQ_BUILD_STDEVENT_MONITOR)
    message(STATUS "monitor/StdEventMonitor is NOT to be built (EUDAQ_BUILD_STDEVENT_MONITOR=OFF)")
  return()
endif()
message(STATUS "monitor/StdEventMonitor is to be built (EUDAQ_BUILD_STDEVENT_MONITOR=ON)")


set(THISMON StdEventMonitor)

aux_source_directory(src THISMON_SRC)
add_executable(${THISMON} ${THISMON_SRC})

root_generate_dictionary(${THISMON}_ROOT 
   include/OnlineMonWindow.hh
    MODULE
    ${THISMON}
   )

target_include_directories(${THISMON} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_BINARY_DIR}/include>)
target_link_libraries(${THISMON} ${EUDAQ_CORE_LIBRARY} ROOT::Gui ROOT::Core ROOT::Hist ROOT::RIO ROOT::Rint)

install(TARGETS ${THISMON}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
