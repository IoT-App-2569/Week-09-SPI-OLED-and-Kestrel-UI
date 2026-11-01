# Engineering Forensic Incident Report - Lab 9.4
**ผู้รายงาน:** นายสุวิจักขณ์ ลาวัง (67030248)
**กรณีศึกษา:** Case 3: Upside-down Display
**สาเหตุ:** Segment Remap & COM Scan Direction 180-degree inverted
**การแก้ไข:** oled_send_cmd(0xA1); oled_send_cmd(0xC8);
