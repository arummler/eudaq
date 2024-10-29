# GET_FILENAME_COMPONENT(OBJNAME ${CMAKE_CURRENT_LIST_DIR} NAME)

add_definitions("-DEUDAQ_CORE_EXPORTS")

find_package(Filesystem)
message("file system variable: ")
message(${CXX_FILESYSTEM_HAVE_FS})
# looks strange but BOOL value can be true/TRUE/on/ON/1/.... and the preprocessor does accept only a subset where 1/0 is the most cross platform
if (${CXX_FILESYSTEM_HAVE_FS})
    set(CXX_FILESYSTEM_HAVE_FS 1)
else()
    set(CXX_FILESYSTEM_HAVE_FS 0)
endif()


configure_file(src/ModuleManager.cc.in ModuleManager.cc @ONLY)

add_library(${EUDAQ_CORE_LIBRARY} SHARED
	src/BufferSerializer.cc
	src/CommandReceiver.cc
	src/Configuration.cc
	src/DataCollector.cc
	src/DataReceiver.cc
	src/DataSender.cc
	src/Deserializer.cc
	src/Event.cc
	src/Exception.cc
	src/FileDeserializer.cc
	src/FileLogCollector.cc
	src/FileNamer.cc
	src/FileReader.cc
	src/FileSerializer.cc
	src/FileWriter.cc
	src/LinkDef.h
	src/LogCollector.cc
	src/LogMessage.cc
	src/LogSender.cc
	src/Logger.cc
	${CMAKE_CURRENT_BINARY_DIR}/ModuleManager.cc
	src/Monitor.cc
	src/NativeFileReader.cc
	src/NativeFileWriter.cc
	src/OptionParser.cc
	src/Processor.cc
	src/Producer.cc
	src/RawEvent.cc
	src/RawEvent2StdEventConverter.cc
	src/RunControl.cc
	src/Serializable.cc
	src/Serializer.cc
	src/StandardEvent.cc
	src/StandardPlane.cc
	src/Status.cc
	src/StdEventConverter.cc
	src/Time.cc
	src/TransportBase.cc
	src/TransportClient.cc
	src/TransportNULL.cc
	src/TransportServer.cc
	src/TransportTCP.cc
	src/Utils.cc
)

# generate the dictionary source code if ROOT is available
FIND_PACKAGE(root QUIET)
IF(ROOT_FOUND)
  ROOT_GENERATE_DICTIONARY(eudaqCoreDict
    include/eudaq/Exception.hh
    LINKDEF
    src/LinkDef.h
    OPTIONS
    -inlineInputHeader
    -I${CMAKE_CURRENT_SOURCE_DIR}
    -I${CMAKE_CURRENT_SOURCE_DIR}/src
    MODULE
    ${EUDAQ_CORE_LIBRARY}
  )
  SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/eudaqCoreDict.cxx
    PROPERTIES
    GENERATED TRUE

    COMPILE_FLAGS  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:"-Wno-unused-function -Wno-overlength-strings -Wno-zero-as-null-pointer-constant -w -I${CMAKE_CURRENT_SOURCE_DIR}">
    )

  TARGET_LINK_LIBRARIES(${EUDAQ_CORE_LIBRARY} ROOT::Core)

  # Also install the dictionary objects
  INSTALL(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}${EUDAQ_CORE_LIBRARY}_rdict.pcm
      ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}${EUDAQ_CORE_LIBRARY}.rootmap
      DESTINATION lib)
ENDIF()

set_target_properties(${EUDAQ_CORE_LIBRARY} PROPERTIES DEFINE_SYMBOL "core_EXPORTS" )
set_target_properties(${EUDAQ_CORE_LIBRARY} PROPERTIES VERSION ${EUDAQ_VERSION_MAJOR}.${EUDAQ_VERSION_MINOR})
set_target_properties(${EUDAQ_CORE_LIBRARY} PROPERTIES EXPORT_NAME ${EUDAQ_TARGET})

if(CMAKE_CXX_STANDARD>=17 AND CXX_FILESYSTEM_HAVE_FS)
  list(APPEND ADDITIONAL_LIBRARIES std::filesystem)
endif()

list(APPEND ADDITIONAL_LIBRARIES ${CMAKE_DL_LIBS})
target_link_libraries(${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})
target_include_directories(${EUDAQ_CORE_LIBRARY} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

install(TARGETS ${EUDAQ_CORE_LIBRARY}
  EXPORT eudaqTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  # PUBLIC_HEADER DESTINATION include/eudaq
  )

file(GLOB INC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/eudaq/*.hh")
install(FILES ${INC_FILES} DESTINATION include/eudaq)
