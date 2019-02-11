
# 'Macro' target
add_custom_target(unittests)
add_custom_target(integration_tests)

add_subdirectory(Graph)
add_subdirectory(Frontend)
add_subdirectory(integration_test)
