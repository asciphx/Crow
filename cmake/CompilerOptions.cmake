# Compiler options with hardening flags

if(MSVC)

  list(APPEND compiler_options 
    /W4
    /permissive-
    /utf-8
    $<$<CONFIG:RELEASE>:/O2 /Ob2>
    $<$<CONFIG:MINSIZEREL>:/O1 /Ob1>
    $<$<CONFIG:RELWITHDEBINFO>:/Zi /O2 /Ob1>
    $<$<CONFIG:DEBUG>:/Zi /Ob0 /Od /RTC1>)

else(MSVC)

  list(APPEND compiler_options 
    -g
    -m64
    -pg
    -fexec-charset=utf-8
    -finput-charset=utf-8
    $<$<CONFIG:RELEASE>:-O2>
    $<$<CONFIG:DEBUG>:-p>)
endif()
