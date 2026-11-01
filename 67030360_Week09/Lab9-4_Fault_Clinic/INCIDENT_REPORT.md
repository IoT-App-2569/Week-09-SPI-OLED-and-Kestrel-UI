# Engineering Forensic Incident Report - Lab 9.4
**ผู้รายงาน:** นายอิทธิพัทธิ์ สุทธินวล (67030360)
**กรณีศึกษา:** Case 2: Garbled Screen
**สาเหตุ:** Page Y-axis coordinate shift memory bounds corruption
**การแก้ไข:** page_index = (y / 8) * 128 + x; bit_mask = (1 << (y % 8));
