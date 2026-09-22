# หมายเหตุแก้ไข: ใบงานต้นฉบับระบุ SRCS เป็น "Lab9-3-ESP32-ClosedLoop.c" ซึ่งไม่ตรงกับ
# โครงสร้างโปรเจกต์ที่กำหนดไว้ในหัวข้อ 3.0 (ไฟล์จริงคือ main/main.c) จึงแก้ให้ตรงกันที่นี่
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        driver
                        esp_driver_spi
                        esp_driver_gpio
                        esp_driver_uart
                        esp_adc
                        esp_timer
)
