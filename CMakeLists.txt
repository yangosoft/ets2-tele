# CMakeList.txt: proyecto de CMake para ets2tele, incluya el origen y defina
# la lógica específica del proyecto aquí.
#
cmake_minimum_required (VERSION 3.8)

set(SCS_SDK_PATH "D:\\proj\\scs_sdk_1_14\\include" CACHE STRING "Path to SCS SDK include folder")
set(WEBSOCKETPP_PATH "D:/proj/websocketpp/out/install/x64-Debug/include/" CACHE STRING "Path to WEBSOCKETPP include folder")
set(BOOST_PATH "D:/proj/boost_1_85_0/" CACHE STRING "Path to BOOST folder")
SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "D:/proj/boost_1_85_0/")
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "D:/proj/boost_1_85_0/stage/lib")
set(JSON_PATH "D:/proj/json/single_include/" CACHE STRING "Path to nlohmann JSON single_include folder")


# Habilite Recarga activa para los compiladores de MSVC si se admiten.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("ets2tele")

# Agregue un origen al ejecutable de este proyecto.
add_executable (ets2tele "ets2tele.cpp" "ets2tele.h")

target_include_directories(ets2tele PRIVATE ${SCS_SDK_PATH} ${WEBSOCKETPP_PATH} ${BOOST_PATH} ${JSON_PATH})
#target_link_libraries(ets2tele PUBLIC boost_system)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ets2tele PROPERTY CXX_STANDARD 20)
endif()


