# รายงานผลการทดลองที่ 9.1 (Lab 9.1)
### การประกอบสร้างตัวขับจอแสดงผล SSD1306 ทีละชิ้นส่วน (Deconstructed Bring-up) สู่ Hello World และการตรวจสอบความจำภาพเชิงนิติวิทยาศาสตร์ (Framebuffer Forensics)

---

## 3. ขั้นตอนการทดลองแบบแยกส่วนประกอบ (Deconstructed Steps)

### กิจกรรมที่ 1.2 ปลุกจอให้ตื่นด้วย Magic Sequence (Proof-of-Life)
**รูปภาพผลการทดลอง**

<img width="884" height="688" alt="9 1 2" src="https://github.com/user-attachments/assets/002408a0-7879-4e0b-bc54-648d6b2fd1c8" />

#### Log ผลการทดลอง 1.2
```text
I (27) boot: ESP-IDF v6.0.2 2nd stage bootloader
I (27) boot: compile time Sep 14 2026 10:07:44
I (28) boot: Multicore bootloader
I (29) boot: chip revision: v3.1
I (32) boot.esp32: SPI Speed      : 40MHz
I (35) boot.esp32: SPI Mode       : DIO
I (39) boot.esp32: SPI Flash Size : 2MB
I (42) boot: Enabling RNG early entropy source...
I (47) boot: Partition Table:
I (49) boot: ## Label            Usage          Type ST Offset   Length
I (56) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (62) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (69) boot:  2 factory          factory app      00 00 00010000 00100000
I (75) boot: End of partition table
I (79) esp_image: segment 0: paddr=00010020 vaddr=3f400020 size=0a608h ( 42504) map
I (101) esp_image: segment 1: paddr=0001a630 vaddr=3ffb0000 size=02c3ch ( 11324) load
I (106) esp_image: segment 2: paddr=0001d274 vaddr=40080000 size=02da4h ( 11684) load
I (111) esp_image: segment 3: paddr=00020020 vaddr=400d0020 size=0fe38h ( 65080) map
I (136) esp_image: segment 4: paddr=0002fe60 vaddr=40082da4 size=0a690h ( 42640) load
I (154) esp_image: segment 5: paddr=0003a4f8 vaddr=50000000 size=00028h (    40) load
I (161) boot: Loaded app from partition at offset 0x10000
I (161) boot: Disabling RNG early entropy source...
I (172) cpu_start: Multicore app
I (180) cpu_start: GPIO 3 and 1 are used as console UART I/O pins
I (180) cpu_start: Pro cpu start user code
I (180) cpu_start: cpu freq: 160000000 Hz
I (182) app_init: Application information:
I (186) app_init: Project name:     Lab9-1_OLED_BringUp
I (191) app_init: App version:      1
I (194) app_init: Compile time:     Sep 14 2026 10:07:16
I (186) app_init: Project name:     Lab9-1_OLED_BringUp
I (191) app_init: App version:      1
I (194) app_init: Compile time:     Sep 14 2026 10:07:16
I (194) app_init: Compile time:     Sep 14 2026 10:07:16
I (199) app_init: ELF file SHA256:  5cd2b1bb3...
I (204) app_init: ESP-IDF:          v6.0.2
I (207) efuse_init: Min chip rev:     v0.0
I (211) efuse_init: Max chip rev:     v3.99
I (215) efuse_init: Chip rev:         v3.1
I (219) heap_init: Initializing. RAM available for dynamic allocation:
I (226) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (231) heap_init: At 3FFB3660 len 0002C9A0 (178 KiB): DRAM
I (236) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (241) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (247) heap_init: At 4008D434 len 00012BCC (74 KiB): IRAM
I (253) spi_flash: detected chip: generic
I (256) spi_flash: flash io: dio
W (259) spi_flash: Detected size(4096k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (272) main_task: Started on CPU0
I (272) main_task: Calling app_main()
I (272) OLED_LAB9_1: Starting Lab 9.1 - Activity 1.1 & 1.2: Proof-of-Life Full Screen Test
I (302) OLED_LAB9_1: Full screen 0xFF written. Display should now be completely lit.
I (302) main_task: Returned from app_main()
```

### กิจกรรมที่ 1.3 การเขียนเอนจินพิกเซลบน 1KB Framebuffer (Bitwise Canvas)
**รูปภาพผลการทดลอง**

<img width="942" height="722" alt="9 1 3" src="https://github.com/user-attachments/assets/80101b7a-1186-4577-805f-4d43b7a63b3c" />

#### Log ผลการทดลอง 1.3
```text
I (27) boot: ESP-IDF v6.0.2 2nd stage bootloader
I (27) boot: compile time Sep 14 2026 10:07:44
I (28) boot: Multicore bootloader
I (29) boot: chip revision: v3.1
I (32) boot.esp32: SPI Speed      : 40MHz
I (35) boot.esp32: SPI Mode       : DIO
I (39) boot.esp32: SPI Flash Size : 2MB
I (42) boot: Enabling RNG early entropy source...
I (47) boot: Partition Table:
I (49) boot: ## Label            Usage          Type ST Offset   Length
I (56) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (62) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (69) boot:  2 factory          factory app      00 00 00010000 00100000
I (75) boot: End of partition table
I (79) esp_image: segment 0: paddr=00010020 vaddr=3f400020 size=0a648h ( 42568) map
I (101) esp_image: segment 1: paddr=0001a670 vaddr=3ffb0000 size=02c3ch ( 11324) load
I (106) esp_image: segment 2: paddr=0001d2b4 vaddr=40080000 size=02d64h ( 11620) load
I (111) esp_image: segment 3: paddr=00020020 vaddr=400d0020 size=0ff48h ( 65352) map
I (136) esp_image: segment 4: paddr=0002ff70 vaddr=40082d64 size=0a6d0h ( 42704) load
I (154) esp_image: segment 5: paddr=0003a648 vaddr=50000000 size=00028h (    40) load
I (161) boot: Loaded app from partition at offset 0x10000
I (161) boot: Disabling RNG early entropy source...
I (172) cpu_start: Multicore app
I (180) cpu_start: GPIO 3 and 1 are used as console UART I/O pins
I (181) cpu_start: Pro cpu start user code
I (181) cpu_start: cpu freq: 160000000 Hz
I (182) app_init: Application information:
I (186) app_init: Project name:     Lab9-1_OLED_BringUp
I (191) app_init: App version:      1
I (195) app_init: Compile time:     Sep 14 2026 10:07:16
I (200) app_init: ELF file SHA256:  3c4c03364...
I (204) app_init: ESP-IDF:          v6.0.2
I (208) efuse_init: Min chip rev:     v0.0
I (212) efuse_init: Max chip rev:     v3.99
I (216) efuse_init: Chip rev:         v3.1
I (220) heap_init: Initializing. RAM available for dynamic allocation:
I (226) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (231) heap_init: At 3FFB3A60 len 0002C5A0 (177 KiB): DRAM
I (236) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (241) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (247) heap_init: At 4008D434 len 00012BCC (74 KiB): IRAM
I (254) spi_flash: detected chip: generic
I (256) spi_flash: flash io: dio
W (259) spi_flash: Detected size(4096k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (272) main_task: Started on CPU0
I (272) main_task: Calling app_main()
I (272) OLED_LAB9_1: Starting Lab 9.1 - Activities 1.1, 1.2, 1.3
I (302) OLED_LAB9_1: Full screen test: 1.5 seconds delay...
I (1802) OLED_LAB9_1: Activity 1.3: Corner Pixels Test
I (1802) OLED_LAB9_1: Corner pixels drawn. 4 corner dots should now be visible.
I (1802) main_task: Returned from app_main()
```

### กิจกรรมที่ 1.4 สร้างตัวอักษรและพิมพ์ "Hello World"
**รูปภาพผลการทดลอง**

<img width="2925" height="1950" alt="9 1 4" src="https://github.com/user-attachments/assets/d281f76c-350f-42b7-85ec-3a3190cff12a" />

#### Log ผลการทดลอง 1.4
```text
I (27) boot: ESP-IDF v6.0.2 2nd stage bootloader
I (27) boot: compile time Sep 14 2026 10:07:44
I (28) boot: Multicore bootloader
I (29) boot: chip revision: v3.1
I (32) boot.esp32: SPI Speed      : 40MHz
I (35) boot.esp32: SPI Mode       : DIO
I (39) boot.esp32: SPI Flash Size : 2MB
I (42) boot: Enabling RNG early entropy source...
I (47) boot: Partition Table:
I (49) boot: ## Label            Usage          Type ST Offset   Length
I (56) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (62) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (69) boot:  2 factory          factory app      00 00 00010000 00100000
I (75) boot: End of partition table
I (79) esp_image: segment 0: paddr=00010020 vaddr=3f400020 size=0a7f8h ( 43000) map
I (102) esp_image: segment 1: paddr=0001a820 vaddr=3ffb0000 size=02c3ch ( 11324) load
I (106) esp_image: segment 2: paddr=0001d464 vaddr=40080000 size=02bb4h ( 11188) load
I (111) esp_image: segment 3: paddr=00020020 vaddr=400d0020 size=10010h ( 65552) map
I (136) esp_image: segment 4: paddr=00030038 vaddr=40082bb4 size=0a880h ( 43136) load
I (154) esp_image: segment 5: paddr=0003a8c0 vaddr=50000000 size=00028h (    40) load
I (161) boot: Loaded app from partition at offset 0x10000
I (161) boot: Disabling RNG early entropy source...
I (172) cpu_start: Multicore app
I (181) cpu_start: GPIO 3 and 1 are used as console UART I/O pins
I (181) cpu_start: Pro cpu start user code
I (181) cpu_start: cpu freq: 160000000 Hz
I (183) app_init: Application information:
I (186) app_init: Project name:     Lab9-1_OLED_BringUp
I (191) app_init: App version:      1
I (195) app_init: Compile time:     Sep 14 2026 10:07:16
I (200) app_init: ELF file SHA256:  f1328cf6d...
I (204) app_init: ESP-IDF:          v6.0.2
I (208) efuse_init: Min chip rev:     v0.0
I (212) efuse_init: Max chip rev:     v3.99
I (216) efuse_init: Chip rev:         v3.1
I (220) heap_init: Initializing. RAM available for dynamic allocation:
I (226) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (231) heap_init: At 3FFB3A60 len 0002C5A0 (177 KiB): DRAM
I (236) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (242) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (247) heap_init: At 4008D434 len 00012BCC (74 KiB): IRAM
I (254) spi_flash: detected chip: generic
I (256) spi_flash: flash io: dio
W (259) spi_flash: Detected size(4096k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (273) main_task: Started on CPU0
I (273) main_task: Calling app_main()
I (273) OLED_LAB9_1: Starting Lab 9.1 - Activities 1.1 to 1.4
I (3303) OLED_LAB9_1: Activity 1.4: Printing text on OLED
I (3303) OLED_LAB9_1: Text rendered successfully.
I (3303) main_task: Returned from app_main()
```

#### ภารกิจสังเกตการณ์เชิงลึก (Forensic Visual Observation Challenge)
1. **แถบสีของจอภาพ (Dual-Color Zone)** 
> แถบสีไม่ได้สลับตำแหน่งกัน แต่พิกัดเพี้ยน ทำให้ข้อความโดนเลื่อนลงมาแสดงในโซนสีฟ้าแทน
2. **ทิศทางของตัวอักษร** 
> ตัวอักษรไม่ได้กลับหัว แต่อยู่ผิดตำแหน่งเพราะโค้ดยังไม่มีคำสั่งกำหนดทิศทางหน้าจอ

---

## 4. ขั้นตอนการตรวจสอบเชิงนิติวิทยาศาสตร์ (Framebuffer Forensics)

### กิจกรรมนิติวิทยาศาสตร์ 1.1 Hex Dump Memory Inspection

#### Log ผลการทดลอง 1.1 (นิติวิทยาศาสตร์)
```text
I (27) boot: ESP-IDF v6.0.2 2nd stage bootloader
I (27) boot: compile time Sep 14 2026 10:07:44
I (28) boot: Multicore bootloader
I (29) boot: chip revision: v3.1
I (32) boot.esp32: SPI Speed      : 40MHz
I (35) boot.esp32: SPI Mode       : DIO
I (39) boot.esp32: SPI Flash Size : 2MB
I (42) boot: Enabling RNG early entropy source...
I (47) boot: Partition Table:
I (49) boot: ## Label            Usage          Type ST Offset   Length
I (56) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (62) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (69) boot:  2 factory          factory app      00 00 00010000 00100000
I (75) boot: End of partition table
I (79) esp_image: segment 0: paddr=00010020 vaddr=3f400020 size=0a7e8h ( 42984) map
I (102) esp_image: segment 1: paddr=0001a810 vaddr=3ffb0000 size=02c3ch ( 11324) load
I (106) esp_image: segment 2: paddr=0001d454 vaddr=40080000 size=02bc4h ( 11204) load
I (111) esp_image: segment 3: paddr=00020020 vaddr=400d0020 size=10098h ( 65688) map
I (136) esp_image: segment 4: paddr=000300c0 vaddr=40082bc4 size=0a870h ( 43120) load
I (154) esp_image: segment 5: paddr=0003a938 vaddr=50000000 size=00028h (    40) load
I (161) boot: Loaded app from partition at offset 0x10000
I (161) boot: Disabling RNG early entropy source...
I (172) cpu_start: Multicore app
I (181) cpu_start: GPIO 3 and 1 are used as console UART I/O pins
I (181) cpu_start: Pro cpu start user code
I (181) cpu_start: cpu freq: 160000000 Hz
I (183) app_init: Application information:
I (186) app_init: Project name:     Lab9-1_OLED_BringUp
I (191) app_init: App version:      1
I (195) app_init: Compile time:     Sep 14 2026 10:07:16
I (200) app_init: ELF file SHA256:  8e4f74c75...
I (204) app_init: ESP-IDF:          v6.0.2
I (208) efuse_init: Min chip rev:     v0.0
I (212) efuse_init: Max chip rev:     v3.99
I (216) efuse_init: Chip rev:         v3.1
I (220) heap_init: Initializing. RAM available for dynamic allocation:
I (226) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (231) heap_init: At 3FFB3A60 len 0002C5A0 (177 KiB): DRAM
I (236) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (242) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (247) heap_init: At 4008D434 len 00012BCC (74 KiB): IRAM
I (254) spi_flash: detected chip: generic
I (256) spi_flash: flash io: dio
W (259) spi_flash: Detected size(4096k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (273) main_task: Started on CPU0
I (273) main_task: Calling app_main()
I (5293) FORENSIC: === DUMPING FRAMEBUFFER PAGE 0 (First 16 Bytes) ===
Byte[ 0] (Col  0): 0x7F  [Binary: 01111111]
Byte[ 1] (Col  1): 0x08  [Binary: 00001000]
Byte[ 2] (Col  2): 0x08  [Binary: 00001000]
Byte[ 3] (Col  3): 0x08  [Binary: 00001000]
Byte[ 4] (Col  4): 0x7F  [Binary: 01111111]
Byte[ 5] (Col  5): 0x00  [Binary: 00000000]
Byte[ 6] (Col  6): 0x7F  [Binary: 01111111]
Byte[ 7] (Col  7): 0x49  [Binary: 01001001]
Byte[ 8] (Col  8): 0x49  [Binary: 01001001]
Byte[ 9] (Col  9): 0x49  [Binary: 01001001]
Byte[10] (Col 10): 0x41  [Binary: 01000001]
Byte[11] (Col 11): 0x00  [Binary: 00000000]
Byte[12] (Col 12): 0x7F  [Binary: 01111111]
Byte[13] (Col 13): 0x40  [Binary: 01000000]
Byte[14] (Col 14): 0x40  [Binary: 01000000]
Byte[15] (Col 15): 0x40  [Binary: 01000000]
I (5343) main_task: Returned from app_main()
```

### กิจกรรมนิติวิทยาศาสตร์ 1.2 Bit-to-Pixel Forensic Reconstruction
**ตารางวิเคราะห์**
```text
          Col 0  Col 1  Col 2  Col 3  Col 4
          0x7F   0x08   0x08   0x08   0x7F
        ------------------------------------
Bit 0:      █      .      .      .      █ 
Bit 1:      █      .      .      .      █ 
Bit 2:      █      .      .      .      █ 
Bit 3:      █      █      █      █      █   <-- เส้นคานแนวนอน (Bit 3)
Bit 4:      █      .      .      .      █ 
Bit 5:      █      .      .      .      █ 
Bit 6:      █      .      .      .      █ 
Bit 7:      .      .      .      .      . 
```
*(เมื่อจำลองการจัดเรียงบิตในแนวตั้ง จะเห็นเป็นรูปทรงของตัวอักษร 'H' อย่างชัดเจน)*

---

## 5. คำถามท้ายการทดลองเพื่อการประเมินผล (Review Questions)

1. จากการทำ Hex Dump ในกิจกรรมนิติวิทยาศาสตร์ จงอธิบายว่าทำไมตัวอักษร 'H' จึงใช้ข้อมูลจำนวน 5 ไบต์ และแต่ละไบต์ทำหน้าที่ควบคุมพิกเซลในทิศทางใด?
> เพราะใช้ฟอนต์ขนาด 5x7 พิกเซล (กว้าง 5 คอลัมน์) โดยแต่ละไบต์จะคุมพิกเซลแนวตั้ง 1 คอลัมน์ (บิต 0 = ด้านบนสุด, บิต 7 = ด้านล่างสุด)

2. หากเราสลับสายไฟระหว่างขา D0 และ D1 จะเกิดผลอย่างไรกับสัญญาณ SPI และหน้าจอจะติดหรือไม่?
> หน้าจอจะไม่ติดเลย เพราะสัญญาณ Clock (D0) และ Data (D1) สลับกัน ทำให้จอรับข้อมูลไม่ได้

3. เหตุใดการแก้ไขพิกัด (x, y) บน s_oled_buffer จึงไม่ทำให้ภาพบนหน้าจอจริงเปลี่ยนทันที จนกว่าจะมีการเรียกคำสั่ง oled_flush()?
> เพราะคำสั่งวาดเป็นการแก้ข้อมูลใน RAM ของ ESP32 เท่านั้น ต้องเรียก `oled_flush()` เพื่อดันข้อมูลทั้งหมดผ่าน SPI ไปแสดงที่หน้าจอ
