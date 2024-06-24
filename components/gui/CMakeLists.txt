if(ESP_PLATFORM)

file(GLOB_RECURSE GUI_SOURCES images/*.c images/*/*.c ui/*.c)

idf_component_register(
		SRCS 
			${GUI_SOURCES}
        INCLUDE_DIRS
            "."
            "images"
            "ui"
        REQUIRES 
            lvgl mqtt_server)

else()
    message(FATAL_ERROR "LVGL LV examples: ESP_PLATFORM is not defined. Try reinstalling ESP-IDF.")
endif()