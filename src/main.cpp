#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <SD.h>
#include <SPI.h>
#include <time.h>


/*
 CIRCUIT CONFIGURATION
 */
const int photocellPin = A0;
File myFile;
/*
 WIFI CONFIGURATION
 */
 char SSID[] = "MARVIN-GUEST";
 char pwd[] = "B@bicKa281+";

 // char SSID[] = "FounHome";
 // char pwd[] = "Jounicek9";

/*
 SLACK CONFIGURATION
 */
const String slack_hook_url = "https://hooks.slack.com/services/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";
const String slack_icon_url = "<SLACK_ICON_URL>";
const String slack_message_free = ":free: :free: Volno! :free: :free:";
const String slack_message_busy = ":rotating_light: :hankey: Obsazeno! :hankey: :rotating_light: ";
const String slack_username = "HajzlGuard";
/*
 TIME SETTINGS
*/
int timezone = 3;
int dst = 0;

/*
 CONDITION SETTINGS
*/

int currentLightLevel = 0;
int previousLightLevel = 0;
int delta = 0;
int deltaValue = 10;
int lastMessageStatus = -1;


void setup()
{
  pinMode(photocellPin, INPUT);

  Serial.begin(9600);
  Serial.begin(115200);
  Serial.println();

  WiFi.begin(SSID, pwd);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  // OTA setup
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

//SD CARD

Serial.print("Initializing SD card...");

 if (!SD.begin(4)) {
   Serial.println("initialization failed!");
   while (1);
 }
 Serial.println("initialization done.");

 // open the file. note that only one file can be open at a time,
 // so you have to close this one before opening another.
 myFile = SD.open("error.log", FILE_WRITE);

 // if the file opened okay, write to it:
 if (myFile) {
   Serial.print("Writing to error.log...");
   myFile.println("Writing is succesfull!");
   // close the file:
   myFile.close();
   Serial.println("done.");
 } else {
   // if the file didn't open, print an error:
   Serial.println("error opening error.log");
 }

 // re-open the file for reading:
 myFile = SD.open("error.log");
 if (myFile) {
   Serial.println("error.log:");

   // read from the file until there's nothing else in it:
   while (myFile.available()) {
     Serial.write(myFile.read());
   }
   // close the file:
   myFile.close();
 } else {
   // if the file didn't open, print an error:
   Serial.println("error opening error.log");
 }


//Time code
Serial.setDebugOutput(true);

configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
Serial.println("\nWaiting for time");
while (!time(nullptr)) {
  Serial.print(".");
  delay(1000);
}
Serial.println("");

  ArduinoOTA.begin();
}



bool postMessageToSlack(String msg)
{
  const char* host = "hooks.slack.com";
  Serial.print("Connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClientSecure client;
  const int httpsPort = 443;
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed :-(");
    return false;
  }

  // We now create a URI for the request

  Serial.print("Posting to URL: ");
  Serial.println(slack_hook_url);

  String postData="payload={\"link_names\": 1, \"icon_url\": \"" + slack_icon_url + "\", \"username\": \"" + slack_username + "\", \"text\": \"" + msg + "\"}";

  // This will send the request to the server
  client.print(String("POST ") + slack_hook_url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Connection: close" + "\r\n" +
               "Content-Length:" + postData.length() + "\r\n" +
               "\r\n" + postData);
  Serial.println("Request sent");
  String line = client.readStringUntil('\n');
  Serial.printf("Response code was: ");
  Serial.println(line);
  if (line.startsWith("HTTP/1.1 200 OK")) {
    return true;
  } else {
    return false;
  }
}

void loop() {
  // TIME

  time_t now = time(nullptr);

  // Start handling OTA updates
  ArduinoOTA.handle();
  int currentLightLevel = analogRead(photocellPin); //read the sensor
      currentLightLevel = map(currentLightLevel, 0, 1023, 0, 255); // map the value
  myFile = SD.open("error.log", FILE_WRITE);
  myFile.println("***** LOG START *****");
  myFile.println(ctime(&now));
  myFile.print("Current light level:  ");
  myFile.println(currentLightLevel);
  myFile.print("Last message status:  ");
  myFile.println(lastMessageStatus);
  myFile.print("WiFi status:  ");
  myFile.println(WiFi.status());

      delta = abs(previousLightLevel - currentLightLevel); // calculate the absolute value of the difference btw privous and current light value
      Serial.println(currentLightLevel);
      if (delta >= deltaValue) { // if the difference is higher than a threshold
          Serial.print("currentLightLevel: ");
          Serial.println(currentLightLevel);

          if (currentLightLevel < 80 && lastMessageStatus != 0) {
              lastMessageStatus = 0;
              Serial.println("Volno");
              postMessageToSlack(slack_message_free);
              Serial.println(lastMessageStatus);
              myFile.println("Posted message:  Volno");

          } else if(currentLightLevel > 80 && lastMessageStatus != 1){
              lastMessageStatus = 1;
              Serial.println("Obsazeno");
              Serial.println(lastMessageStatus);
              postMessageToSlack(slack_message_busy);
              myFile.println("Posted message:  Obsazeno");

          }
      }

      myFile.println("----- LOG END -----");
      myFile.println(" ");
      myFile.close();
      previousLightLevel = currentLightLevel;
      delay(5000);
}
