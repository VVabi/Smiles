if(NOT DEFINED TEST_EXECUTABLE)
    message(FATAL_ERROR "TEST_EXECUTABLE is required")
endif()

if(NOT DEFINED TEST_INPUT)
    message(FATAL_ERROR "TEST_INPUT is required")
endif()

execute_process(
    COMMAND "${TEST_EXECUTABLE}" -e preprocessor "${TEST_INPUT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE actual_output
    ERROR_VARIABLE actual_error
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "smiles exited with code ${result}\nstdout:\n${actual_output}\nstderr:\n${actual_error}")
endif()

string(ASCII 27 esc)
set(expected_output "Smiles\nPreprocessed file: ${TEST_INPUT}\nSkipped characters: ${esc}[31m52${esc}[0m\nInclude paths (1):\n  - lalalala\nUsing namespaces (0):\nOutput:\n${esc}[31m---SKIPPED 33 tokens---${esc}[0m\nprintln(7)${esc}[31m---SKIPPED 19 tokens---${esc}[0m\nprintln(3)\n")

if(NOT actual_error STREQUAL "")
    message(FATAL_ERROR "Expected empty stderr but got:\n${actual_error}")
endif()

if(NOT actual_output STREQUAL expected_output)
    message(FATAL_ERROR "Unexpected stdout.\nExpected:\n${expected_output}\nActual:\n${actual_output}")
endif()
