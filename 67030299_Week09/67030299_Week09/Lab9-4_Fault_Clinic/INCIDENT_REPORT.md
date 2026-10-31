# Engineering Forensic Incident Report - Lab 9.4
**ผู้รายงาน:** นางสาวธนพร พิศมัย (67030299)
**กรณีศึกษา:** Case 3: Upside-down Display
**สาเหตุ:** SSD1306 hardware scan default COM0-COM[N-1] orientation
**การแก้ไข:** oled_send_cmd(0xA1); oled_send_cmd(0xC8);
