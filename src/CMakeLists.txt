
# 'Macro' target
add_custom_target(unittests)
add_custom_target(integration_tests)

# Graph should put at the front since it's
# used by many other users
add_subdirectory(Graph)
add_subdirectory(Frontend)
add_subdirectory(CodeGen)
add_subdirectory(Driver)
add_subdirectory(integration_test)
