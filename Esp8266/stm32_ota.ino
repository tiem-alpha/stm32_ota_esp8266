
#include <ESP8266WiFi.h>                 // thu vien mang cho esp8266 
#include <WiFiClient.h>                  // 
#include <ESP8266WebServer.h>            // webserver 
#include <ESP8266mDNS.h>               // thu vien dinh nghia ten mien
#include <ESP8266HTTPUpdateServer.h>  // thu vien update
#include<SoftwareSerial.h>
#include <FS.h>
#include"web_sup.h"
#include"crc.h"


enum STM32_state {
  SEND_RESET,
  WAIT_AC,
  SEND_INFORMATION,
  WAIT_ACK, //0x55
  SEND_FW,
  WAIT_FW_RES,
  FAIL_OTA,
  DONE_OTA,
};

enum command {
  RESET_CMD,
  NEW_FW,
  TRANSFER_FW,
};


//uint8_t resetCMD[] = { 0xAC, 0x02, RESET_CMD, 0x01, 0x37, 0x4D};


uint8_t volatile state = 0;



// SoftwareSerial s(4, 5);// D1 = tx, D2 =rx


const char* ssid     = "Tiem";
const char* password = "11111111";


const char* host = "home";
const char* updatePath = "/update";
const char* updateUsername = "";     //password
const char* updatePassword = "";      // password

ESP8266WebServer webServer(80);
ESP8266HTTPUpdateServer httpUpdater;

// Set your Static IP address
IPAddress local_IP(192, 168, 0, 184);
// Set your Gateway IP address
IPAddress gateway(192, 168, 0, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

uint8_t wifiStatus  = 0;

uint16_t fileSize = 0;

void handleFileUpload() {
  HTTPUpload& upload = webServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    //Serial.printf("Uploading: %s\n", upload.filename.c_str());
    SPIFFS.remove("/firmware.bin");
    File f = SPIFFS.open("/firmware.bin", "w");
    f.close();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    File f = SPIFFS.open("/firmware.bin", "a");
    if (f) f.write(upload.buf, upload.currentSize);
    f.close();
  } else if (upload.status == UPLOAD_FILE_END) {
    //Serial.printf("Upload completed: %s, Size: %d bytes\n", upload.filename.c_str(), upload.totalSize);
    webServer.send(200, "text/plain", "Upload Success! File size: " + String(upload.totalSize) + " bytes");
    fileSize = upload.totalSize;
  }
}

bool waitForResponse(uint8_t expectedByte, uint32_t timeout = 3000) {
  uint32_t startTime = millis();  // Lưu thời gian bắt đầu
  //Serial.println("Wait Bytes");

  while (millis() - startTime < timeout) {  // Kiểm tra timeout

    if (Serial.available()) {  // Nếu có dữ liệu trong buffer UART
      uint8_t receivedByte = Serial.read();  // Đọc byte từ UART
      //Serial.println(receivedByte, HEX);
      if (receivedByte == expectedByte) {
        //Serial.println("Bytes Expect");
        return true;  // Nhận đúng byte mong đợi
      }
    }
  }
  //Serial.println("Wrong Bytes");
  return false;  // Hết thời gian chờ nhưng không nhận được byte mong muốn
}

void sendFirmware() {
  File f = SPIFFS.open("/firmware.bin", "r");
  uint8_t buff[128];  // Mảng chứa dữ liệu đọc từ file
  uint16_t crc = 0;
  uint16_t offset = 0 ;
  uint16_t bytesRead = 0;
  uint8_t preState = 255;
  if (!f) {
    webServer.send(404, "text/plain", "File not found");
    return;
  }

  while (true) {
    switch (state) {
      case SEND_RESET:
        if (preState != state)
        {
          //Serial.println("Send reset");
          preState = state;
        }
        buff[0] = 0xAC;
        buff[1] = 0x02;
        buff[2] = RESET_CMD;
        buff[4] = 0x01;
        buff[5] = 0x37;
        buff[6] = 0x4D;
        for (int i = 0 ; i < 7 ; i++) {
          //Serial.print(buff[i], HEX);
          //Serial.print(" ");
          Serial.write(&buff[i], 1);
        }
        //Serial.println(" ");
        state = WAIT_AC;

        break;

      case WAIT_AC:
        if (preState != state)
        {
          //Serial.println("WAIT_AC");
          preState = state;
        }
        if (waitForResponse(0xAC, 10000)) {
          //Serial.println("receive AC");
          state = SEND_INFORMATION;
        } else {
          //Serial.println("AC Timeout");
          state = FAIL_OTA;
        }
        break;

      case SEND_INFORMATION:
        if (preState != state)
        {
          //Serial.println("SEND_INFORMATION");
          preState = state;
        }
        buff[0] = 0xAC;
        buff[1] = 6;
        buff[2] = NEW_FW;
        buff[3] = 0x01;
        buff[4] = 0x00;
        buff[5] = 0x01;
        buff[6] = (fileSize / 256) & 0xFF;
        buff[7] = fileSize & 0xFF;
        crc = crc_16((const unsigned char*)&buff[1], buff[1] + 1);
        buff[8] = (crc >> 8) & 0xff;
        buff[9] = crc & 0xff;

        for (int i = 0 ; i < buff[1] + 4 ; i++) {
          //Serial.print(buff[i], HEX);
          //Serial.print(" ");
          Serial.write(&buff[i], 1);
        }
        //Serial.println(" ");
        //        Serial.write(buff, buff[1] + 4); // start + length + crc[2]
        state =  WAIT_ACK;
        break;

      case WAIT_ACK:
        if (preState != state)
        {
          //Serial.println("SEND_INFORMATION");
          preState = state;
        }
        if (waitForResponse(0x55, 10000)) {
          //Serial.println("receive ACK");
          state = SEND_FW;
        } else {
          //Serial.println("Timeout");
          state = FAIL_OTA;
        }
        break;

      case SEND_FW:

        if (preState != state)
        {
          //Serial.println("SEND_FW");
          preState = state;
        }
        // ac _ length _ cmd _ byte read _ crc1 _ crc2;
        buff[0] = 0xAC;
        buff[2] = TRANSFER_FW;
        if ((bytesRead = f.read(&buff[3], 64)) > 0) {
          buff[1] = bytesRead + 1; // cmd + data
          crc = crc_16((const unsigned char*)&buff[1], buff[1] + 1); // include len;
          buff[3 + bytesRead] = (crc >> 8) & 0xFF;
          buff[4 + bytesRead] = (crc) & 0xFF;
            for (int i = 0 ; i <  5 + bytesRead ; i++) {
          //Serial.print(buff[i], HEX);
          //Serial.print(" ");
          Serial.write(&buff[i], 1);
        }
          state = WAIT_FW_RES;

        } else {
          //Serial.println("End of file");
          state = DONE_OTA;
        }

        break;


      case WAIT_FW_RES:
        if (waitForResponse(0x55, 10000)) {
          state = SEND_FW;
          offset += bytesRead;
          //Serial.println("Writing " + String(offset) + "/" + String(fileSize));
          if (fileSize <= offset) {
            //Serial.println("write File Done");
            state = DONE_OTA;
          }
        } else {
          state = FAIL_OTA;
        }
        break;


      case FAIL_OTA:
        //Serial.println("Update fail");
        crc = 0;
        offset = 0 ;
        bytesRead = 0;
        preState = 255;
        state = 0;
        f.close();

        return;
        break;


      case DONE_OTA:
        //Serial.println("Update Success");
        f.close();
        crc = 0;
        offset = 0 ;
        bytesRead = 0;
        preState = 255;
        state = 0;
        return;
        break;

      default:
        state = SEND_RESET;
        break;
    }
  }
}


void setup() {
  // initialize digital pin LED_BUILTIN as an output.

  Serial.begin(115200);
  // s.begin(115200);
  //  s.println("Hello");
  SPIFFS.begin();
  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    //Serial.println("STA Failed to configure");
  }

  // Connect to Wi-Fi network with SSID and password
  //Serial.print("Connecting to ");
  //Serial.println(ssid);
  WiFi.begin(ssid, password);
  uint8_t count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    count++;
    if (count >= 20) {
      break;
    }
    //Serial.print(".");
  }

  if (count >= 20) {
    wifiStatus = 0 ;
  } else {
    wifiStatus = 1;
  }

  if (wifiStatus == 1) {

    //Serial.print("IP address: ");
    //Serial.println(WiFi.localIP());
    MDNS.begin(host);     // multicast DNS
    MDNS.addService("http", "tcp", 80);

    httpUpdater.setup(&webServer, updatePath, updateUsername, updatePassword); //
    webServer.on("/", [] {
      String s = MainPage;
      webServer.send(200, "text/html", s);
    });


    webServer.on("/on", [] {
      String s = STM32Page;
      webServer.send(200, "text/html", s);

    });

    webServer.on("/upload", HTTP_POST, []() {}, handleFileUpload);
    webServer.on("/send", sendFirmware);

    webServer.begin();
    //Serial.println("Web Server is started!");
  }

}


// the loop function runs over and over again forever
void loop() {

  if (wifiStatus == 1) {
    MDNS.update();
    webServer.handleClient();
  }


}
