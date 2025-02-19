#ifndef WEB_H
#define WEB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
 
const char STM32Page[] PROGMEM = R"=====(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>ESP8266 Firmware Upload</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                text-align: center;
                margin: 50px;
            }
            input, button {
                margin: 10px;
                padding: 10px;
                font-size: 16px;
            }
        </style>
    </head>
    <body>
        <h2>Upload Firmware to STM32</h2>
        <form id="uploadForm" enctype="multipart/form-data">
            <input type="file" id="fileInput" name="update" accept=".bin" required>
            <br>
            <button type="button" onclick="uploadFile()">Upload</button>
            <button type="button" onclick="controlPump('send')">Start Update</button>
            
        </form>
        <p id="status"></p>
    
        <script>
            function uploadFile() {
                var fileInput = document.getElementById("fileInput");
                if (fileInput.files.length === 0) {
                    alert("Please select a file!");
                    return;
                }
    
                var formData = new FormData();
                formData.append("update", fileInput.files[0]);
    
                var xhr = new XMLHttpRequest();
                xhr.open("POST", "/upload", true);
                
                xhr.onload = function () {
                    document.getElementById("status").innerText = xhr.responseText;
                };
    
                xhr.onerror = function () {
                    document.getElementById("status").innerText = "Upload failed!";
                };
    
                xhr.send(formData);
            }
      function controlPump(action) {
                // Gửi yêu cầu tới server khi nhấn nút
                fetch(`/${action}`)
                    .then(response => response.text())
                    .then(data => alert(data))
                    .catch(error => console.error('Error:', error));
            }
            
        </script>
    </body>
    </html>
    )=====";
    
    const char MainPage[] PROGMEM = R"=====(
      <!DOCTYPE html> 
      <html>
       <head> 
           <title>OTA-Nguyen Van Tiem</title> 
           <style>
         
            body {
                text-align: center;
                font-family: Arial, sans-serif;
            }
            button {
                padding: 10px 20px;
                font-size: 16px;
                margin: 10px;
                cursor: pointer;
            }
            .on {
                background-color: #4CAF50;
                color: white;
                border: none;
            }
            .off {
                background-color: #f44336;
                color: white;
                border: none;
            }
       
        <meta name="viewport" content="width=device-width, initial-scale=1.0"> 
              body{
                text-align: center;
              }
           </style>
           <meta name="viewport" content="width=device-width,user-scalable=0" charset="UTF-8">
       </head>
       <body> 
          <div>
            <button onclick="window.location.href='/update'">UPLOAD FIRMWARE ESP</button><br><br>
          </div>
          <div>
          <button class="on" onclick=" window.location.href = '/on'">Stm32 OTA</button>
      
        </div>
      
        <script>
            function controlPump(action) {
                // Gửi yêu cầu tới server khi nhấn nút
                fetch(`/${action}`)
                    .then(response => response.text())
                    .then(data => alert(data))
                    .catch(error => console.error('Error:', error));
            }
    
            
        </script>
       </body> 
      </html>
    )=====";
    
    

#ifdef __cplusplus
}
#endif

#endif//WEB_H