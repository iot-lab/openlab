if(${PLATFORM_HAS_CSMA})
add_executable(topology main network gossiping)
target_link_libraries(topology platform mac_csma)
endif(${PLATFORM_HAS_CSMA})