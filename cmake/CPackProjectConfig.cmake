# When generating DEB package, use dbg component instead of debug component.
# Both provides the same .debug file, but dbg use debian-preferred install
# location.

if(CPACK_GENERATOR STREQUAL DEB)
    set(CPACK_COMPONENTS_ALL main dbg)
    set(CPACK_COMPONENT_DBG_DEPENDS main)
endif()
