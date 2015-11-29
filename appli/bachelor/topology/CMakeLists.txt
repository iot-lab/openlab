include_directories(${PROJECT_SOURCE_DIR}/appli/iotlab/lib)

if(${PLATFORM_HAS_CSMA})
add_executable(topology main network gossiping)
target_link_libraries(topology platform mac_csma iotlab_uid_num_table)
endif(${PLATFORM_HAS_CSMA})