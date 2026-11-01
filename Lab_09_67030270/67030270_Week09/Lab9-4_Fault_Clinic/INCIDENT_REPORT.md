# Engineering Forensic Incident Report - Lab 9.4
**ผู้รายงาน:** นายกฤษฏิณัช สำราญกิจ (67030270)
**กรณีศึกษา:** Case 4: Telemetry Mismatch
**สาเหตุ:** Serial delimiter newline missing & baudrate 115200 synchronization
**การแก้ไข:** printf("ADC:%d\n", raw_val); fflush(stdout);
