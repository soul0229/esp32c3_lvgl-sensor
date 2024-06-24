idf_component_register(SRCS "mqtt.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_wifi nvs_flash protocol_examples_common mqtt ath20 bmp280 bh1750)
