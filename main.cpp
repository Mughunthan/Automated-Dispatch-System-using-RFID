#include <SPI.h>
#include <MFRC522.h>

#define RST 9
#define SS 10
#define BUZZER 8

MFRC522 rfid(SS, RST);
const unsigned long cooldown = 180000;    // 3 min
const int maxtags = 10;

struct tag {
  String uid;
  unsigned long lastSeen;
  bool inRoomA;  };
tag tags[maxtags];
int tagcount = 0;
int serialno = 1;

String getUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += (rfid.uid.uidByte[i] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;  }

int findtag(String uid) {
  for (int i = 0; i < tagcount; i++) {
    if (tags[i].uid == uid) return i;
  }
  return -1;  }

void beep() {
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW); }

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  Serial.println("Scan your RFID card."); }

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String uid = getUID();
  unsigned long now = millis();
  int index = findtag(uid);

  if (index == -1 && tagcount < maxtags) {
    index = tagcount++;
    tags[index] = {uid, 0, true};
  }

tag &currentTag = tags[index];
  if (currentTag.lastSeen == 0 || now - currentTag.lastSeen >= cooldown) {
    beep();
    String roomA = currentTag.inRoomA ? "Entered" : "";
    String roomB = !currentTag.inRoomA ? "Entered" : "";
    Serial.println(String(serialno++) + "," + uid + ",T+" + String(now / 1000) + "s," + roomA + "," + roomB);
    currentTag.lastSeen = now;
    currentTag.inRoomA = !currentTag.inRoomA;
}

  delay(1000);
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();  }

  //END OF CODE//
  ---------------------------------------------------------------------
  // Python script to log RFID scans to an Excel file under this(this was executed in thonny IDE not a part of the above code)
import serial
from datetime import datetime
from openpyxl import Workbook, load_workbook

excel = "rfid_log.xlsx"
try:
    workbook = load_workbook(excel)
    sheet = workbook.active
except FileNotFoundError:
    workbook = Workbook()
    sheet = workbook.active
    sheet.append(["S.No", "RFID", "Scan Time", "Room-A", "Room-B"])
    workbook.save(excel)

cooldown = 180 
tag_lastscanned = {}

# serial commu
ser = serial.Serial('COM3', 9600, timeout=1)
print("Listening on COM3")

sno = sheet.max_row 

try:
    while True:
        line = ser.readline().decode().strip()
        if line.count(',') == 4:
            _, uid, _, room_a, room_b = line.split(',')
            now = datetime.now()

            if uid in tag_lastscanned:
                if (now - tag_lastscanned[uid]).total_seconds() < cooldown:
                    continue

            tag_lastscanned[uid] = now
            sno += 1
            scan_time = now.strftime("%Y-%m-%d %H:%M:%S")
            sheet.append([sno, uid, scan_time, room_a, room_b])
            workbook.save(excel)

            print(f">>> {sno},{uid},{scan_time},{room_a},{room_b}")

except KeyboardInterrupt:
    print("stopped by user.")
finally:
    ser.close()

