idf_component_register(SRCS "blufi_init.c"
                            "blufi_security.c"
                            "blufi_main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_wifi nvs_flash bt mbedtls protocol_examples_common mqtt_server)
