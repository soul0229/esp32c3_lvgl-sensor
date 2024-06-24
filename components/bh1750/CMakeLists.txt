idf_component_register(SRCS "bh1750.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES i2c_common esp_driver_i2c)
