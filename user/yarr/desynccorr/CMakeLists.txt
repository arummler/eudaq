find_package(ROOT REQUIRED COMPONENTS Core Hist)

ADD_EXECUTABLE("desynccorr" src/desynccorr.cxx)
#ADD_EXECUTABLE("yarr_resync" src/yarr_resync.cxx)

set(JSONCONS_BUILD_TESTS OFF CACHE INTERNAL "")
include(FetchContent)
FetchContent_Declare(extern_jsoncons
GIT_REPOSITORY https://github.com/danielaparker/jsoncons.git
GIT_TAG v0.176.0
GIT_SHALLOW 1
)
FetchContent_MakeAvailable(extern_jsoncons)

target_include_directories("desynccorr" PRIVATE include)
#target_include_directories("yarr_resync" PRIVATE include)

#target_link_libraries("yarr_resync" ${EUDAQ_CORE_LIBRARY} jsoncons)
target_link_libraries("desynccorr" ${EUDAQ_CORE_LIBRARY} ROOT::Core ROOT::Hist jsoncons)

#SET(name "desynccorr" "yarr_resync")
SET(name "desynccorr")
INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin COMPONENT MAIN_EXE
  LIBRARY DESTINATION lib COMPONENT MAIN_EXE
  ARCHIVE DESTINATION lib COMPONENT MAIN_EXE)
