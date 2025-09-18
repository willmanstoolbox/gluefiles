if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  message(FATAL_ERROR "CMAKE_INSTALL_PREFIX not set (expected CPack staging dir).")
endif()
set(_exe "${CMAKE_INSTALL_PREFIX}/gluefiles.exe")
if(NOT EXISTS "${_exe}")
  message(FATAL_ERROR "gluefiles.exe not found at: ${_exe}")
endif()
if(NOT DEFINED WINDEPLOYQT_EXE)
  find_program(WINDEPLOYQT_EXE NAMES windeployqt windeployqt.exe)
endif()

if(NOT WINDEPLOYQT_EXE)
  message(FATAL_ERROR
    "windeployqt not found. Provide it via PATH or -DWINDEPLOYQT_EXE=... when running CMake.")
endif()

message(STATUS "Running windeployqt: ${WINDEPLOYQT_EXE}")
message(STATUS "Staged install dir:  ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Target exe:          ${_exe}")
execute_process(
  COMMAND "${WINDEPLOYQT_EXE}"
          --release
          --compiler-runtime
          --no-translations
          --dir "${CMAKE_INSTALL_PREFIX}"
          "${_exe}"
  RESULT_VARIABLE _rc
  OUTPUT_VARIABLE _out
  ERROR_VARIABLE  _err
)
message(STATUS "windeployqt stdout:\n${_out}")
if(NOT _rc EQUAL 0)
  message(FATAL_ERROR "windeployqt failed with code ${_rc}:\n${_err}")
endif()
