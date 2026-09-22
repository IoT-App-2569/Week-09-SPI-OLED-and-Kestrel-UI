namespace ESP32.Kestrel.Webserver.Services;

// รับข้อมูล JSON จากหน้าเว็บ เช่น { "message": "TEST OK" }
// แล้วส่งต่อไปให้ CalibrationService.SetOledMessage() เพื่อพักคิวข้อความ
// รอให้ SerialBridgeService หยิบไปแนบไปกับแพ็กเก็ต "SET:<percent>:<message>\n"
public record DisplayMessageRequest(string Message);
