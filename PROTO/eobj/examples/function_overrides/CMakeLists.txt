LIST(APPEND FUNCTION_OVERRIDES_CC_SOURCES
   main.c
   simple.c
   inherit.c
   inherit2.c
   inherit3.c
   )

include_directories(
   ${EINA_INCLUDE_DIRS}
   ${EVAS_INCLUDE_DIRS}
   ${CMAKE_SOURCE_DIR}/lib
   )

add_executable(function_overrides ${FUNCTION_OVERRIDES_CC_SOURCES})

get_target_property(eo_LIB_FILE eo LOCATION)
target_link_libraries(function_overrides
   ${EINA_LIBRARIES}
   ${EINA_LDFLAGS_OTHER}
   ${eo_LIB_FILE}
   )

add_dependencies(function_overrides eo)

add_test(Example_function_overrides function_overrides)
