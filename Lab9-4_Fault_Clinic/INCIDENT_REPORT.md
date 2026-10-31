# Engineering Forensic Incident Report - Lab 9.4
**ผู้รายงาน:** นางสาวสุวิมล ศรีประไหม (67030249)
**กรณีศึกษา:** Case 1: Black Screen
**สาเหตุ:** Internal Charge Pump 0x8D, 0x10 (Disabled) & Hardware Reset timing
**การแก้ไข:** oled_send_cmd(0x8D); oled_send_cmd(0x14); oled_send_cmd(0xAF);
