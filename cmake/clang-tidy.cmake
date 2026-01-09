#----------------------------------
# STATIC ANALYSIS
#----------------------------------

if(SDBUSCPP_CLANG_TIDY)
    message(STATUS "Building with static analysis")
    find_program(CLANG_TIDY NAMES clang-tidy)
    if(NOT CLANG_TIDY)
        message(WARNING "clang-tidy not found")
    else()
        message(STATUS "clang-tidy found: ${CLANG_TIDY}")
        set(DO_CLANG_TIDY "${CLANG_TIDY}")
        #set(DO_CLANG_TIDY "${CLANG_TIDY}" "-fix")
        set(CMAKE_CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    endif()
endif()
