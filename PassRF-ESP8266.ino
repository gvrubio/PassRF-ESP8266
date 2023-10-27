/*
TODO:
-WIFI QR Random password generation for initial setup - ✅
-Limit to one client - ✅
-Client automatic redirection using hotspot - Hard
-MAC Filter? - Hard
-Reset on client MAC address change
-Encription on http?
-Android KEEPASS Plugin
-PCB v2 - Module on PCB
-PCB v3 - Discrete components
-Separate functions on a different file and include it.
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include <Preferences.h>
String realSize = String(ESP.getFlashChipRealSize());
String ideSize = String(ESP.getFlashChipSize());
bool flashCorrectlyConfigured = realSize.equals(ideSize);

//////////////////////////////////////CONSTANTS
IPAddress apIP(192, 168, 4, 1);
const char* ssid = "PTYPER";
String WPAKeyVar = "";
const int channel = 1;
const int hidden_ssid = 0;
const int max_connections = 1;
const char* prefsNamespace = "wifi_settings";

//////////////////////////////////////OBJECT INITIALIZATION
ESP8266WebServer server(80);      // Port 80 for HTTP server
SoftwareSerial mySerial(D2, D3);  // RX, TX for SoftwareSerial

//////////////////////////////////////FUNCTIONS
String removeLineBreaks(String inputString) {
  String outputString = "";

  for (int i = 0; i < inputString.length(); i++) {
    char currentChar = inputString.charAt(i);

    // Check if the current character is not a newline or carriage return
    if (currentChar != '\n' && currentChar != '\r') {
      outputString += currentChar;
    }
  }

  return outputString;
}
// RANDOM PASSWORD GENERATOR
String getRandomPassword() {
  static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()_+-=[]{}|;:,.<>?";
  String password;
  for (int i = 0; i < 32; i++) {
    int index = random(0, sizeof(charset) - 1);
    password += charset[index];
  }
  return password;
}
String randomPassword = getRandomPassword();

// FIRST CONNECTION (/)
void handleFirstConnection() {
  String index_initialization = R"(
<!DOCTYPE html>
<html>

<head>
  <style>
    .code-container {
      position: relative;
    }

    #code {
      background-color: #f4f4f4;
      padding: 10px;
      border: 1px solid #ccc;
      border-radius: 4px;
      font-size: 14px;
    }

    #copy-button {
      position: absolute;
      top: 10px;
      right: 10px;
      background-color: #007bff;
      color: #fff;
      border: none;
      border-radius: 4px;
      padding: 5px 10px;
      cursor: pointer;
    }

    #copy-button:hover {
      background-color: #0056b3;
    }
  </style>
</head>

<body>
  <p>
    01 - COPY THE WIFI PASSWORD
  </p>
  <br>
  <div class="code-container">
    <pre id="code">%WPAKEY%</pre>
    <button id="copy-button">Copy Code</button>
  </div>
<br>
<form action='/submit' method='get'>
<input type='submit' value='Submit'>
  <script>
    document.getElementById('copy-button').addEventListener('click', function () {
      var codeElement = document.getElementById('code');
      var textArea = document.createElement('textarea');
      textArea.value = codeElement.textContent;
      document.body.appendChild(textArea);
      textArea.select();
      document.execCommand('copy');
      document.body.removeChild(textArea);
      this.textContent = 'Copied!';
      setTimeout(function () {
        document.getElementById('copy-button').textContent = 'Copy Code';
      }, 2000);
    });
  </script>
</body>

</html>
)";
  index_initialization.replace("%WPAKEY%", String(randomPassword));
  server.send(200, "text/html", index_initialization);
}
// ROOT (/)
void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
<meta http-equiv="Pragma" content="no-cache" />
<meta http-equiv="Expires" content="0" />
<head>
  <style>
    .code-container {
      position: relative;
    }

    #input {
      background-color: #f4f4f4;
      padding: 10px;
      border: 1px solid #ccc;
      border-radius: 4px;
      font-size: 14px;
    }

    #copy-button {
      top: 10px;
      right: 10px;
      background-color: #007bff;
      color: #fff;
      border: none;
      border-radius: 4px;
      padding: 5px 10px;
      cursor: pointer;
    }

    #copy-button:hover {
      background-color: #0056b3;
    }
  </style>
</head>

<body>
  <div class="code-container">
    <h1>Enter Password:</h1>
    <form action="/submit" method="get">
      <input id="input" type="password" name="password" autocomplete=current-password>
      <input id="copy-button" type="submit" value="Submit">
    </form>
  </div>
</body>

</html>

)";
  server.send(200, "text/html", html);
}
// PasswordManager Password (/)
void handlePassword() {
  String password = server.arg("password");
  mySerial.print(password + '\n');     // Send the password via SoftwareSerial
  server.sendHeader("Location", "/");  // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);
}

// WpaKey Password Save
void handleWpaKey() {
  Serial.println(randomPassword);
  SaveWpa(randomPassword);
  ESP.restart();
}
// PasswordManager API Key
void handleApikey() {
  server.send(200, "text/html", WPAKeyVar);
}

// WPAKey Save
void SaveWpa(const String& wifiPassword) {
  Preferences preferences;
  preferences.begin(prefsNamespace, false);  // Read-only mode
  preferences.putString("password", wifiPassword);
  preferences.end();
}

// WPAKey Read
String ReadWpa() {
  Preferences preferences;
  preferences.begin(prefsNamespace, false);  // Read-only mode
  String wifiPassword = preferences.getString("password", "");
  preferences.end();
  return wifiPassword;
}


//////////////////////////////////////SETUP
void setup() {
  // SERIAL
  Serial.begin(115200);
  mySerial.begin(115200);

  // ACCESS POINT
  WPAKeyVar = ReadWpa();
  Serial.println("Saved data is:" + WPAKeyVar);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  if (WPAKeyVar == "") {
    WiFi.softAP("FIRST", "drivesmenuts", channel, hidden_ssid, max_connections);
  } else {
    WiFi.softAP(ssid, WPAKeyVar, channel, hidden_ssid, max_connections);
  }
  // WEB SERVER
  if (WPAKeyVar == "") {
    server.on("/", HTTP_GET, handleFirstConnection);
    server.on("/submit", HTTP_GET, handleWpaKey);
  } else {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/submit", HTTP_GET, handlePassword);
    server.on("/apikey", HTTP_GET, handleApikey);
  }
  server.begin();
}

//////////////////////////////////////MAIN LOOP
void loop() {
  server.handleClient();
}
