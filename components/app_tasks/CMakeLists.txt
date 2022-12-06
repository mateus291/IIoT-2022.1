idf_component_register(SRCS "mqtt_task.c" "oled_task.c" "button_task.c" "temp_task.c" "accel_task.c"
                    INCLUDE_DIRS "include"
                    REQUIRES mpu6050
                             utils
                             driver
                             onewire_bus
                             ds18b20
                             ssd1306
                             esp32-wifi-manager
                             mqtt)
