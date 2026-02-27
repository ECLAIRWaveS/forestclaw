if(MSVC)
  add_compile_options(
  "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug,RelWithDebInfo>>:/W4>"
  )
else()
  add_compile_options(
    "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug,RelWithDebInfo>>:-Wall>"
  )
endif()

if (CMAKE_Fortran_COMPILER_ID MATCHES "Intel")
  add_compile_options(
    "$<$<AND:$<COMPILE_LANGUAGE:Fortran>,$<CONFIG:Debug,RelWithDebInfo>>:-warn>"
  )
elseif(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  add_compile_options(
    "$<$<AND:$<COMPILE_LANGUAGE:Fortran>,$<CONFIG:Debug,RelWithDebInfo>>:-Wall>"
    )
endif()

# preprocess all Fortran files
set(CMAKE_Fortran_PREPROCESS ON)
