# Engineering Forensic Incident Report - Lab 9.4
**ผู้รายงาน:** นายกร แดงทอง (67030268)
**กรณีศึกษา:** Case 2: Garbled Screen
**สาเหตุ:** Bitwise Framebuffer addressing bug index = x + (y / 8) * 128
**การแก้ไข:** int index = x + (y / 8) * 128; int bit = y % 8;
