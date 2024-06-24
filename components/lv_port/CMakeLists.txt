if(ESP_PLATFORM)

file(GLOB_RECURSE LV_PORT_SOURCES lv_driver/*.c)

idf_component_register(
		SRCS 
			${LV_PORT_SOURCES}
        INCLUDE_DIRS
            "."
            "lv_driver"
        REQUIRES 
            esp_lcd driver lvgl fatfs )

else()
    message(FATAL_ERROR "LVGL LV examples: ESP_PLATFORM is not defined. Try reinstalling ESP-IDF.")
endif()
