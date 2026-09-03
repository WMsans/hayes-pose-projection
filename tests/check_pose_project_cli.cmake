if(NOT DEFINED POSE_PROJECT OR NOT DEFINED CASE)
  message(FATAL_ERROR "POSE_PROJECT and CASE are required")
endif()

if(CASE STREQUAL "invalid-mode")
  set(args --mode invalid)
  set(expected "--mode must be lookat, gt or both")
elseif(CASE STREQUAL "missing-input")
  set(args --data "${CMAKE_BINARY_DIR}/missing-task-7-input" --mode lookat)
  set(expected "error:")
else()
  message(FATAL_ERROR "unknown CLI test case: ${CASE}")
endif()

execute_process(
  COMMAND "${POSE_PROJECT}" ${args}
  RESULT_VARIABLE result
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr)
set(output "${stdout}${stderr}")

if(result EQUAL 0)
  message(FATAL_ERROR "expected pose-project to fail, but it exited successfully")
endif()
if(NOT output MATCHES "${expected}")
  message(FATAL_ERROR "expected '${expected}' in CLI output, got:\n${output}")
endif()

message("observed expected CLI failure: ${output}")
