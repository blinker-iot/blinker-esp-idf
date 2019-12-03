
# componet standalone mode
if(NOT CONFIG_IOT_SOLUTION_EMBED)
    set(COMPONENT_SRCS "button/button.c"
                        "button/button_obj.cpp")

    set(COMPONENT_ADD_INCLUDEDIRS "button/include")
else()
    if(CONFIG_IOT_BUTTON_ENABLE)
        set(COMPONENT_SRCS "button/button.c"
                            "button/button_obj.cpp")

        set(COMPONENT_ADD_INCLUDEDIRS "button/include")
    else()
        set(COMPONENT_SRCS "")
        set(COMPONENT_ADD_INCLUDEDIRS "")
        message(STATUS "Building empty button component due to configuration")
    endif()
endif()

register_component()
