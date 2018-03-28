/*
 * ESP8266 SPIFFS HTML Web Page with JPEG, PNG Image 
 *
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <time.h>
//#include <TM1637Display.h>
#include <FS.h>   //Include File System Headers
//#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <string.h>

const char* imagefile = "/image.png";
const char* htmlfile = "/index.html";

//ESP AP Mode configuration
const char *ssid = "Scoreboard-001";
const char *password = "scoreboard";

const int CLK = D5; //d4 beforeSet the CLK pin connection to the display
const int DIO = D7; //d3 beforeSet the DIO pin connection to the display
const byte DNS_PORT = 53;

bool    spiffsActive = false;

int pl1=0; //player 1
int pl2=0; // player2
int pg1=0;  // player 1 games
int pg2=0;
String player1, player2, gplayer1,gplayer2;
String leftplayer = "Home1"; //default
String rightplayer = "Away1"; //default
String fields[30][6];
String lcd1;
String lcd2;
String tape = "192.168.4.1";

uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
int pinCS = D4;  //d8 
int numberOfHorizontalDisplays = 6;
int numberOfVerticalDisplays = 1;
char time_value[20];
char score[6];
char sep[2];
char score1[2]; // score array for the display unit
char score2[2];
char score3[2];
char score4[2];
//TM1637Display display(CLK, DIO); //set up the 4-Digit Display.
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
int wait = 150; // In milliseconds
int spacer = 1;
int width = 5 + spacer; // The font width is 5 pixels

LiquidCrystal_I2C lcd(0x27,20,4);

ESP8266WebServer server(80);
//DNSServer dnsServer;

void handleRoot(){
  server.sendHeader("Location", "/index.html",true);   //Redirect to our html web page
  server.send(302, "text/plain","");
  displayit();
}

void handleWebRequests(){
  if(loadFromSpiffs(server.uri())) return;
 // Serial.println("not found");
  String message = "File Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.println(message);
}


//------------------------------------
void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();

  //Initialize File System
  SPIFFS.begin();
  Serial.println("File System Initialized");

  //Initialize AP Mode
  WiFi.softAP(ssid);  //Password not used
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Web Server IP:");
  Serial.println(myIP);
  if(MDNS.begin("score001")){
    Serial.println("mdns   ");
  }
   MDNS.addService("http", "tcp", 80);
 // dnsServer.start(DNS_PORT, "*", myIP); // reply with IP for all requests.
                                        // captive portal...
  //Initialize Webserver
  server.on("/",handleRoot);
  //server.on("/connecttest.txt", websendok);
  //server.on("/redirect", websendok);
  server.on("/report", createcard);
  server.on("/points", handlepoints);
  server.on("/savescore", savescore);
  server.on("/chooseplayer", chooseplayer);
  server.onNotFound(handleWebRequests); //Set setver all paths are not found so we can handle as per URI
  server.begin();  

// data[1] = 0b00000000;
//  display.encodeDigit(0);
//  data[0] = display.encodeDigit(0);
//  data[3] = 0b00000000;
//  data[2] = display.encodeDigit(0);


 
  matrix.fillScreen(LOW);
  time_t now = time(nullptr);
  String time = String(ctime(&now));
  time.trim();
  time.substring(11, 19).toCharArray(time_value, 10);

  matrix.setIntensity(3); // Use a value between 0 and 15 for brightness
  matrix.setRotation(0, 1); // The first display is position upside down
  matrix.setRotation(1, 1); // The first display is position upside down
  matrix.setRotation(2, 1); // The first display is position upside down
  matrix.setRotation(3, 1);
  matrix.setRotation(4,1);
  matrix.setRotation(5,1);
  
//Wire.begin(5,4);

  sprintf (score1, "%02i", pl1);
  sprintf (score2, "%02i",pl2);
  sprintf (sep, "%s", ":");
  sprintf (score3, "%02i", pg1);
  sprintf (score4, "%02i",pg2);
  
  matrix.drawChar(2, 0, score1[0], HIGH, LOW, 1); // H
  matrix.drawChar(8, 0, score1[1], HIGH, LOW, 1); // HH
  matrix.drawChar(14, 0, sep[0], HIGH, LOW, 1); // HH:
  matrix.drawChar(20, 0, score2[0], HIGH, LOW, 1); // HH:M
  matrix.drawChar(26, 0, score2[1], HIGH, LOW, 1); // HH:MM
  matrix.drawChar(33, 0, score4[1], HIGH, LOW, 1); // HH:M
  matrix.drawChar(42, 0, score3[1], HIGH, LOW, 1); // HH:MM
  matrix.write(); // Send bitmap to display  


//lcd.begin(16,2);
lcd.init();
lcd.backlight();
lcd.setCursor(7,0);
lcd.print("Points");
lcd.setCursor(0,1);
lcd1=lcd1+"Left:" + score1+"     Right:"+score3;
lcd.setCursor(7,2);
lcd.print("Games");
lcd.setCursor(0,3);
lcd2=lcd2+"Left:" + score2+"     Right:"+score4;
lcd.print(lcd1);
lcd.setCursor(0,1);
lcd.print(lcd2);
// delay(500);
// display.setBrightness(0x0e); //set the diplay to maximum brightness
// display.setSegments(data, 1, 1);
// display.setSegments(data+2, 1, 3);


}

void loop() {
// dnsServer.processNextRequest();
 server.handleClient();
}

//functions --------------------------------



void savescore(){

String buf;
String req = server.arg("score");

buf= leftplayer+","+pl1+","+pl2+","+rightplayer+","+pg1+","+pg2;
File f = SPIFFS.open("/scorecard.txt","a"); 
 
if (!f){
   Serial.println("Fileopenfailed");
}
//Serial.println("writing to file");
//f.println(buf);
//f.close();
//File g = SPIFFS.open("/card.txt","a");
//if (!g){
//   Serial.println("Fileopenfailed");
//}
Serial.println("writing to file");
String Home = "ome";
if (leftplayer.indexOf(Home)>=1){
// Serial.println("Home");
}else{
//  Serial.println("Left=away");
  buf= rightplayer+","+pl2+" ,"+pl1+","+leftplayer+","+pg2+","+pg1;
}
f.println(buf);
f.close();
server.send(200, "text/plain", "ok");  
}

//===============================================
void chooseplayer(){
String lreq = server.arg("left");
String rreq = server.arg("right");
//Serial.print("player");
//Serial.println(lreq);
//Serial.println(rreq);
if (lreq!="") leftplayer=lreq;
if (rreq!="") rightplayer=rreq;

server.send(200, "text/plain", "ok");   
}

//=====================================
void handlepoints() {
int spare;
String buf;
String tmp;
String req = server.arg("request");
//Serial.println(req);
if (req=="0")pl1=pl1+1; 
if (req=="1")pl1=pl1-1; 
if (req=="4")pl2=pl2+1; 
if (req=="5")pl2=pl2-1; 
if (pl1<=0) pl1=0;
if (pl2<=0) pl2=0;
if (req=="2") pg1=pg1+1;
if (req=="3") pg1=pg1-1;
if (req=="6") pg2=pg2+1;
if (req=="7") pg2=pg2-1;
if (req=="8"){ //clear points
    pl1=0;
    pl2=0;
}
if (req=="9" ){  // clear points and games
  pl1=0;
  pl2=0;
  pg1=0;
  pg2=0;
}
if (req=="a") {
  deletescorecard();
  pl1=0;
  pl2=0;
  pg1=0;
  pg2=0;
}

if (req=="r"){ //reverse the screen
 spare=pl1;
 pl1=pl2;
 pl2=spare;
 spare=pg1;
 pg1=pg2;
 pg2=spare;
 tmp=leftplayer;
 Serial.println("changeends");
 leftplayer=rightplayer;
 rightplayer=tmp;
}
if (pg1<=0) pg1=0; //make sure we atrent minus
if (pg2<=0) pg2=0;
player1=String (pl1);//make sure they are strings.
player2=String (pl2);
gplayer1=String (pg1);
gplayer2=String (pg2);

buf=player1+","+player2+","+gplayer1+","+gplayer2+",";
buf=buf+leftplayer+","+rightplayer;
Serial.print("Buf:");
Serial.println(buf);
server.send(200, "text/plain", buf);
displayit();
}

//========================================
void websendok(){
 server.send(200, "text/plain", "ok"); 
}

//==================================
void deletescorecard(){
File f = SPIFFS.open("/scorecard.txt","w"); 
if (!f){
  Serial.println("Fileopenfailed");
}
//Serial.println("writing to file");
//f.println("Scorecard:-");
//f.println("Player1 ,score, score,Player2,  P1leg, P2leg"); 
f.close();
  
}


bool loadFromSpiffs(String path){
  String dataType = "text/plain";
  if(path.endsWith("/")) path += "index.htm";

  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".html")) dataType = "text/html";
  else if(path.endsWith(".txt")) dataType = "text/plain";
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";
  File dataFile = SPIFFS.open(path.c_str(), "r");
  if (!dataFile) return false;
  if (server.hasArg("download")) dataType = "application/octet-stream";
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
//    Serial.println("that loop");
  }

  dataFile.close();
 // Serial.println(path);
 // Serial.println("end loop");
  
  return true;
}
void displayit(){
  matrix.fillScreen(LOW);
  sprintf (score2, "%02i", pl1);// reverse score!
  sprintf (score1, "%02i", pl2);
  sprintf (sep, "%s", ":");
  sprintf (score3, "%02i", pg1);
//  Serial.println(score3);
//  Serial.println(score4);
  sprintf (score4, "%02i", pg2);
  matrix.drawChar(2, 0, score1[0], HIGH, LOW, 1); // H
  matrix.drawChar(8, 0, score1[1], HIGH, LOW, 1); // HH
  matrix.drawChar(14, 0, sep[0], HIGH, LOW, 1); // HH:
  matrix.drawChar(20, 0, score2[0], HIGH, LOW, 1); // HH:M
  matrix.drawChar(26, 0, score2[1], HIGH, LOW, 1); // HH:MM
//  matrix.drawPixel(0,7,left);
  matrix.drawChar(33, 0, score4[1], HIGH, LOW,1);
  matrix.drawChar(42,0, score3[1] , HIGH, LOW,1);
//  matrix.drawPixel(31,7,right);
  matrix.write(); // Send bitmap to display
//Serial.println(lcd1);
//Serial.println(lcd2);

lcd.setCursor(7,0);
lcd.print("Points");
lcd.setCursor(0,1);
lcd1="Left:";
lcd2="Left:";
lcd1=lcd1 + score2+"     Right:"+score1;
lcd2=lcd2 + score3+"     Right:"+score4;
lcd.print(lcd1);
lcd.setCursor(7,2);
lcd.print("Games");
lcd.setCursor(0,3);
lcd.print(lcd2);

//Serial.print("display:");
//Serial.println(score1);
//Serial.println(score2);
//data[0] = display.encodeDigit(pg1);
//data[2] = display.encodeDigit(pg2);
//Serial.println("data:");
//Serial.print(pg1);
//Serial.println(pg2);
//display.setSegments(data,   1, 3); //third digit 
//display.setSegments(data+2, 1,1);  // firstdigit
//display.setSegments(data+1, 1,2);
//display.setSegments(data+3, 1,4);

//  digitalWrite(LED,LOW);
}

//=============================================
void createcard(){
  String req = server.arg("request");
  Serial.println(req);
  File f = SPIFFS.open("/scorecard.txt", "r");
  if (!f) {
        Serial.print("Unable To Open '");
        Serial.print("scorecard.txt");
        Serial.println("' for Reading");
        Serial.println();
  } else {
         String s;
         int leg=0;
         while (f.position()<f.size())
         {
            s=f.readStringUntil('\n');
            s.trim();
//          Serial.println(s);
            int l_start=0;
            int cnt=0;
            String field;
            while (l_start !=-1){
      //       Serial.println(
               fields[leg][cnt]=ENDF2(s,l_start, ',');
              cnt=cnt+1;
//              Serial.println(fields[leg][cnt-1]);
             }
          leg=leg+1;
         
         } 
         f.close();
      }
  // create scorecard
    int col=0;
    File g = SPIFFS.open("/matchcard.txt","w"); 
    if (!g){
    Serial.println("Fileopenfailed");
    }
//Serial.println("writing to file");
//f.println("Scorecard:-");
//f.println("Player1 ,score, score,Player2,  P1leg, P2leg"); 
  //  f.close();

     
    for (int row=0; row<=12; row++){
     g.print(fields[row][0]+" "+fields[row][3]+" "+fields[row][1]+"-"+fields[row][2]);
        while(fields[row][0]==fields[row+1][0]){
          row++;
          g.print(" "+ fields[row][1]+"-"+fields[row][2]);
        }
        g.println();         
     }
f.close();
g.close();
server.send(200, "text/plain", "ok"); 
}




String ENDF2(String &p_line, int &p_start, char p_delimiter) {
//EXTRACT NEXT DELIMITED FIELD VERSION 2
//Extract fields from a line one at a time based on a delimiter.
//Because the line remains intact we dont fragment heap memory
//p_start would normally start as 0
//p_start increments as we move along the line
//We return p_start = -1 with the last field

  //If we have already parsed the whole line then return null
  if (p_start == -1) {
    return "";
  }

  int l_start = p_start;
  int l_index = p_line.indexOf(p_delimiter,l_start);
  if (l_index == -1) { //last field of the data line
    p_start = l_index;
    return p_line.substring(l_start);
  }
  else { //take the next field off the data line
    p_start = l_index + 1;
    return p_line.substring(l_start,l_index); //Include, Exclude
  }
}


