# ผลลัพธ์การทดลองของใบงานการทดลองที่ 9.1 (Lab 9.1)
## การประกอบสร้างตัวขับจอแสดงผล SSD1306 ทีละชิ้นส่วน (Deconstructed Bring-up) สู่ Hello World และการตรวจสอบความจำภาพเชิงนิติวิทยาศาสตร์ (Framebuffer Forensics)
### รูปผลการทดลอง
<img width="1108" height="1477" alt="8595" src="https://github.com/user-attachments/assets/655d67ba-7d81-42cf-9113-805b897208ca" />

---

### รูปผลการทดลองมีจุดพิกเซลสว่างขึ้นเพียง 4 จุดตรงมุมจอทั้งสี่พอดี
<img width="1477" height="1108" alt="S__5472260_0" src="https://github.com/user-attachments/assets/e4ac7dbb-18ec-45ca-ab21-3d8a03dc2a4d" />

---
### รูปผลการทดลองแสดงข้อความ Hello World และ รหัสนักศึกษา
<img width="1477" height="1108" alt="S__5472261_0" src="https://github.com/user-attachments/assets/82de3cfb-faca-4cdf-8bc6-c2be3e022993" />
<img width="2024" height="808" alt="S__5472263" src="https://github.com/user-attachments/assets/e1f568c3-02b6-4e37-92ec-c05410f6b91d" />

---

> [!IMPORTANT]
> **ภารกิจสังเกตการณ์เชิงลึก (Forensic Visual Observation Challenge)**
> หลังจากรันโค้ดและข้อความแสดงขึ้นมาบนจอภาพ ให้นักศึกษาหยุดสังเกตความผิดปกติทางกายภาพอย่างละเอียด
> 1. **แถบสีของจอภาพ (Dual-Color Zone)** โครงสร้างโมดูล 0.96" OLED ชนิด 2 สี มีแถบสีเหลือง (16 แถวบน) และแถบสีฟ้า (48 แถวล่าง) แต่บนจอจริงแถบสีเหลืองไปปรากฏอยู่ *ด้านล่าง* หรือไม่?
> 2. **ทิศทางของตัวอักษร** ข้อความ `"HELLO WORLD"` และ `"ID: 65012345"` แสดงผลกลับหัว 180 องศา (Upside-down) หรือไม่?
>
> 🔍 **ปริศนานิติวิทยาศาสตร์** ทำไมหน้าจอถึงแสดงผลกลับหัว ทั้ง ๆ ที่ในโค้ดเราสั่งวาดพิกัด $(0,0)$ ที่มุมบนซ้ายอย่างถูกต้องแล้ว? มีคำสั่งใดในชุด Magic Initialization Sequence ที่ฮาร์ดแวร์ SSD1306 เริ่มต้นสแกนพิกเซลสลับทิศทางหรือไม่?
>
> 💡 **คำชี้แจง** ในใบงานย่อยที่ 9.1 นี้ **ขอให้นักศึกษาคงสภาพโค้ดที่แสดงผลกลับหัวนี้ไว้ก่อน** และบันทึกสิ่งที่สังเกตเห็นลงในแบบฟอร์มรายงานผล เราจะนำปรากฏการณ์นี้ไปร่วมกันวิเคราะห์ ผ่าชันสูตรคำสั่งรีจิสเตอร์ของคอนโทรลเลอร์ (`0xA1` Segment Remap & `0xC8` COM Scan) และแก้ไขให้ถูกต้องอย่างเป็นระบบใน **ใบงานย่อยที่ 9.4 (กรณีศึกษาที่ 3: Mirrored & Upside-down Display Forensics)**

---

### กิจกรรมนิติวิทยาศาสตร์ 1.2 Bit-to-Pixel Forensic Reconstruction
ให้นักศึกษานำค่า Binary ของไบต์จาก Serial Monitor มาเขียนลงในตารางรายงานผลการทดลอง
- ถอดรหัสว่าในแต่ละคอลัมน์ บิตใดเป็น `1` บ้าง
- พิสูจน์ว่ารูปแบบของบิต `1` ตรงกับรูปร่างของตัวอักษร `'H'` บนหน้าจอ OLED จริงหรือไม่!

### log output 
```
rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0030,len:6380
ho 0 tail 12 room 4
load:0x40078000,len:15916
load:0x40080400,len:3860
--- 0x40080400: _invalid_pc_placeholder at C:/Users/Rusneeda/esp/v5.5.1/esp-idf/components/xtensa/xtensa_vectors.S:2235
entry 0x40080638
--- 0x40080638: call_start_cpu0 at C:/Users/Rusneeda/esp/v5.5.1/esp-idf/components/bootloader/subproject/main/bootloader_start.c:25
I (29) boot: ESP-IDF v5.5.1 2nd stage bootloader
I (29) boot: compile time Sep 14 2026 10:09:01
I (29) boot: Multicore bootloader
I (31) boot: chip revision: v3.1
I (33) boot.esp32: SPI Speed      : 40MHz
I (37) boot.esp32: SPI Mode       : DIO
I (41) boot.esp32: SPI Flash Size : 2MB
I (44) boot: Enabling RNG early entropy source...
I (49) boot: Partition Table:
I (51) boot: ## Label            Usage          Type ST Offset   Length
I (58) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (64) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (71) boot:  2 factory          factory app      00 00 00010000 00100000
I (77) boot: End of partition table
I (81) esp_image: segment 0: paddr=00010020 vaddr=3f400020 size=0bac8h ( 47816) map
I (104) esp_image: segment 1: paddr=0001baf0 vaddr=3ffb0000 size=0256ch (  9580) load
I (108) esp_image: segment 2: paddr=0001e064 vaddr=40080000 size=01fb4h (  8116) load
I (112) esp_image: segment 3: paddr=00020020 vaddr=400d0020 size=1698ch ( 92556) map
I (147) esp_image: segment 4: paddr=000369b4 vaddr=40081fb4 size=0d0c4h ( 53444) load
I (168) esp_image: segment 5: paddr=00043a80 vaddr=50000000 size=00020h (    32) load
I (176) boot: Loaded app from partition at offset 0x10000
I (176) boot: Disabling RNG early entropy source...
I (189) cpu_start: Multicore app
I (197) cpu_start: Pro cpu start user code
I (197) cpu_start: cpu freq: 160000000 Hz
I (197) app_init: Application information:
I (197) app_init: Project name:     Lab9-1_OLED_BringUp
I (202) app_init: App version:      1
I (206) app_init: Compile time:     Sep 14 2026 10:08:18
I (211) app_init: ELF file SHA256:  e8eb14b3c...
I (215) app_init: ESP-IDF:          v5.5.1
I (219) efuse_init: Min chip rev:     v0.0
I (223) efuse_init: Max chip rev:     v3.99 
I (227) efuse_init: Chip rev:         v3.1
I (231) heap_init: Initializing. RAM available for dynamic allocation:
I (237) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (242) heap_init: At 3FFB3270 len 0002CD90 (179 KiB): DRAM
I (247) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (252) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (258) heap_init: At 4008F078 len 00010F88 (67 KiB): IRAM
W (265) spi_flash: Detected boya flash chip but using generic driver. For optimal functionality, enable `SPI_FLASH_SUPPORT_BOYA_CHIP` in menuconfig
I (276) spi_flash: detected chip: generic
I (280) spi_flash: flash io: dio
W (283) spi_flash: Detected size(4096k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (296) main_task: Started on CPU0
I (306) main_task: Calling app_main()
I (3326) FORENSIC: === DUMPING FRAMEBUFFER (Text Area) ===
Byte[30] (Col 30): 0xF0  [Binary: 11110000]
Byte[31] (Col 31): 0x80  [Binary: 10000000]
Byte[32] (Col 32): 0x80  [Binary: 10000000]
Byte[33] (Col 33): 0x80  [Binary: 10000000]
Byte[34] (Col 34): 0xF0  [Binary: 11110000]
Byte[35] (Col 35): 0x00  [Binary: 00000000]
Byte[36] (Col 36): 0xF0  [Binary: 11110000]
Byte[37] (Col 37): 0x90  [Binary: 10010000]
Byte[38] (Col 38): 0x90  [Binary: 10010000]
Byte[39] (Col 39): 0x90  [Binary: 10010000]
Byte[40] (Col 40): 0x10  [Binary: 00010000]
Byte[41] (Col 41): 0x00  [Binary: 00000000]
Byte[42] (Col 42): 0xF0  [Binary: 11110000]
Byte[43] (Col 43): 0x00  [Binary: 00000000]
Byte[44] (Col 44): 0x00  [Binary: 00000000]
Byte[45] (Col 45): 0x00  [Binary: 00000000]
I (3376) main_task: Returned from app_main()
```
### ผลการทดลอง
| ตำแหน่งพิกัดแกน Y | Col 30 (`0xF0`) | Col 31 (`0x80`) | Col 32 (`0x80`) | Col 33 (`0x80`) | Col 34 (`0xF0`) | Col 35 (`0x00`) |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **y=0 (Bit 0)** | 0 | 0 | 0 | 0 | 0 | 0 |
| **y=1 (Bit 1)** | 0 | 0 | 0 | 0 | 0 | 0 |
| **y=2 (Bit 2)** | 0 | 0 | 0 | 0 | 0 | 0 |
| **y=3 (Bit 3)** | 0 | 0 | 0 | 0 | 0 | 0 |
| **y=4 (Bit 4)** | **1** | 0 | 0 | 0 | **1** | 0 |
| **y=5 (Bit 5)** | **1** | 0 | 0 | 0 | **1** | 0 |
| **y=6 (Bit 6)** | **1** | 0 | 0 | 0 | **1** | 0 |
| **y=7 (Bit 7)** | **1** | **1** | **1** | **1** | **1** | 0 |

## 5. คำถามท้ายการทดลองเพื่อการประเมินผล (Review Questions)
1. จากการทำ Hex Dump ในกิจกรรมนิติวิทยาศาสตร์ จงอธิบายว่าทำไมตัวอักษร `'H'` จึงใช้ข้อมูลจำนวน 5 ไบต์ และแต่ละไบต์ทำหน้าที่ควบคุมพิกเซลในทิศทางใด?

   **คำตอบ** เนื่องจากตัวอักษรถูกสร้างขึ้นมาจากตารางฟอนต์ขนาด 5x7 (font5x7.h) ซึ่งมีความกว้าง 5 คอลัมน์ จึงต้องใช้ข้อมูลจำนวน 5 ไบต์ในการประกอบเป็นรูปร่าง 1 ตัวอักษร โดยข้อมูลแต่ละไบต์ (8 บิต) จะทำหน้าที่ควบคุมการเปิด-ปิดของพิกเซลใน แนวตั้ง (แกน Y) บนเพจของหน้าจอ OLED

3. หากเราสลับสายไฟระหว่างขา **D0** และ **D1** จะเกิดผลอย่างไรกับสัญญาณ SPI และหน้าจอจะติดหรือไม่?

   **คำตอบ** ขา D0 ทำหน้าที่เป็นสายสัญญาณนาฬิกา ส่วนขา D1 ทำหน้าที่เป็นสายส่งข้อมูล หากต่อสลับกัน ชิป SSD1306 จะได้รับสัญญาณนาฬิกาผิดจังหวะ และไม่สามารถถอดรหัสบิตข้อมูลได้ ทำให้การสื่อสารผ่านบัส SPI ล้มเหลวโดยสมบูรณ์ และส่งผลให้ หน้าจอไม่ติด (จอดำ)
5. เหตุใดการแก้ไขพิกัด $(x, y)$ บน `s_oled_buffer` จึงไม่ทำให้ภาพบนหน้าจอจริงเปลี่ยนทันที จนกว่าจะมีการเรียกคำสั่ง `oled_flush()`?

   **คำตอบ** ตัวแปร s_oled_buffer ทำหน้าที่เป็นเพียงหน่วยความจำพักข้อมูล (Back Buffer) ที่อยู่ภายในแรมของไมโครคอนโทรลเลอร์ ESP32 เท่านั้น การแก้ไขข้อมูลจึงเกิดขึ้นแค่ในตัวชิป ESP32 การจะทำให้ภาพบนหน้าจอจริงเปลี่ยนไปได้ จะต้องเรียกฟังก์ชัน oled_flush() เพื่อทำหน้าที่ส่งถ่ายข้อมูลในบัฟเฟอร์ทั้ง 1,024 ไบต์ ผ่านบัส SPI ไปอัปเดตลงในหน่วยความจำกราฟิก (GRAM) ของหน้าจอ OLED จริงๆ ภาพถึงจะแสดงผลออกมา
