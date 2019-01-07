 /*
 * This is the Nodemcu Pins to arduino IDE pins
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;*/
#include <IRremoteESP8266.h>
#include <ESP8266WiFi.h>
#include <Wire.h> //Include the Arduino I2C Library</pre>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <time.h>
#define eeprom 0x50
int RECV_PIN = 5; //an IR detector/demodulatord is connected to GPIO pin D1
IRrecv irrecv(RECV_PIN);
decode_results results;
IRsend irsend(4); //an IR led is connected to GPIO pin 4
const char* passphrase = "Blastoff";
//192.168.4.1 = IP of Server  to select wifi gateway
ESP8266WebServer server(80);
String st;
String content;
int statusCode;
//IPAddress serveraddress(209,164,11,210);
IPAddress serveraddress(172,16,0,7);
IPAddress ip(192,168,2,3);  //static IP addition test this...

WiFiClient client;
const int FACTORY_PIN = 12;
boolean configured = false;


String macString = WiFi.macAddress(); //Device Serial #

boolean stringComplete = false;  // whether the string is complete
int currentMemoryAddress = 0;
byte mac[6]; 


//
//// the following variables are unsigned long's because the time, measured in miliseconds,
//// will quickly become a bigger number than can be stored in an int.
//unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
//unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
//unsigned long lastDebounceTime2 = 0;  // the last time the output pin was toggled
//unsigned long debounceDelay2 = 50;    // the debounce time; increase if the output flickers

void setup() {
    irrecv.enableIRIn(); // Start the receiver
      irsend.begin();

  Serial.begin(115200);
  Serial.println("WIFI IR BIAATCH!");
 
  pinMode(12, INPUT_PULLUP);
  EEPROM.begin(512);
    delay(10);
  boolean reset = (digitalRead(12)== LOW);
  if(reset) {
    for (int i = 0; i < 511; ++i) {
      EEPROM.write(i, 0x0);
    }
    EEPROM.commit();
    delay(10); 
   launchWeb(1);
  }
  Serial.println("Serial #");
  Serial.println();
  Serial.println(macString);
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 0; i < 32; ++i) {
    esid += char(EEPROM.read(i));
  }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i) {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);  
  esid.trim();
  WiFi.macAddress(mac);
  if(!reset) {
    WiFi.begin(esid.c_str(), epass.c_str());
    if (testWifi()) {
      Serial.println("WiFi connection successful - Local IP: " + WiFi.localIP());
      configured = true;
      WiFi.waitForConnectResult();
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      delay(3000); // delay to acquire time
      time_t t = time(NULL);
      Serial.println(ctime(&t));
      createWebServer(0);
      // Start the server
      server.begin();
      Serial.println("Server 2 started");
      return;
    }
    else {
      configured = false;
    }
  }  
  setupAP();
    Serial.print("AP Started... ");
}

bool testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");  
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) { return true; } 
    delay(500);
    Serial.print(WiFi.status());    
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
} 

void launchWeb(int webtype) {
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer(webtype);
  // Start the server
  server.begin();
  Serial.println("Server started"); 

  
}

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println(""); 
  st = "<ol>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += "<li>";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      st += "</li>";
    }
  st += "</ol>";
  delay(100);
  WiFi.softAP(getApName(), passphrase, 6);
  Serial.println("softap");
  launchWeb(1);
  Serial.println("over");
}

char* getApName() {

  // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd) to "WIFI IR Setup":
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  String AP_NameString = "WIFI IR Setup " + macID;

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);
  return AP_NameChar;
}

void createWebServer(int webtype)
{
  if ( webtype == 1 ) {
    server.on("/", []() {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
                content += ipStr;
        
         content = "<!DOCTYPE HTML>\r\n<html>Congratulations on your WIRELESS IR Device,First Point your IR remote to the Wifi IR Device, Press the Remote button you want to program, you should see the IR Recieved Field update.Next select the button ID you want to save the IR code to. Press Submit to save configuration to memory. Repeat process for all desired buttons. Then Save Wifi Credentials. *Note saving Wifi Credentials first will boot into Server mode and not allow for Button Configuration until Wifi is Cleared. in order to enable the Wifi IR Blaster to Communicate to the server please Select the Wifi Network you wish to connect to.  ";
         content += "<FORM><INPUT TYPE='button' onClick='history.go(0)' VALUE='Get latest IR CODE'></FORM>";
        if(results.value != 4294967295 ) {content += results.value;}  
        else{content+= "Try again";}
        content += "<!DOCTYPE html><html><body><form><select name='buttons'><option value='pwr'>Power</option><option value='input'>Input</option><option value='but0'>Button 0</option><option value='but1'>Button 1</option><option value='but2'>Button 2</option><option value='but3'>Button 3</option><option value='but4'>Button 4</option><option value='but5'>Button 5</option><option value='but6'>Button 6</option><option value='but7'>Button 7</option><option value='but8'>Button 8</option><option value='but9'>Button 9</option><option value='Vol+'>Volume Up</option><option value='Vol-'>Volume Down</option><option value='Chan+'>Channel Up</option><option value='Chan-'>Channel Down</option><option value='Selup'>Select UP</option><option value='Seldown'>Select Down</option><option value='SelLeft'>Select Left</option><option value='SelRight'>Select Right</option></select><br><br><input type='submit'></form></body></html>";
        content += st;
      
        
        content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
        content += "</html>";
        
        server.send(200, "text/html", content);  
    });
    server.on("/setting", []() {
        String qsid = server.arg("ssid");
        String qpass = server.arg("pass");
        if (qsid.length() > 0 && qpass.length() > 0) {
          Serial.println("clearing eeprom");
          for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
          Serial.println(qsid);
          Serial.println("");
          Serial.println(qpass);
          Serial.println("");
            
          Serial.println("writing eeprom ssid:");
          for (int i = 0; i < qsid.length(); ++i)
            {
              EEPROM.write(i, qsid[i]);
              Serial.print("Wrote: ");
              Serial.println(qsid[i]); 
            }
          Serial.println("writing eeprom pass:"); 
          for (int i = 0; i < qpass.length(); ++i)
            {
              EEPROM.write(32+i, qpass[i]);
              Serial.print("Wrote: ");
              Serial.println(qpass[i]); 
            }    
          EEPROM.commit();
          content = "{\"Success\":\"saved to Memory... Will reset to boot into new wifi\"}";
          statusCode = 200;
          digitalWrite(13, HIGH);
          ESP.restart();//to reset with new settings? issue with it using the config earlier after setting to correct wifi.
        } else {
          content = "{\"Error\":\"404 not found\"}";
          statusCode = 404;
          Serial.println("Sending 404");
        }
        server.send(statusCode, "application/json", content);
    });
  } else if (webtype == 0) {
    server.on("/", []() {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      server.send(200, "application/json", "{\"IP\":\"" + ipStr + "\"}");
    });
    server.on("/cleareeprom", []() {
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Clearing the EEPROM</p></html>";
      server.send(200, "text/html", content);
      Serial.println("clearing eeprom");
      for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
      EEPROM.commit();
    });
  }
}



void loop() {  if (irrecv.decode(&results)) {

Serial.println(results.value, HEX);
irrecv.resume(); // Receive the next value
  }
  delay(100);
 

  if (!configured){server.handleClient();}
}
