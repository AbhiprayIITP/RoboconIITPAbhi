#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include<Wire.h>

#define FIREBASE_HOST "nodemcu-ac.firebaseio.com"
#define FIREBASE_AUTH "9HZs1YLQcQuZkvgTc1TuhSNOa8fUKAZZK4VMMuJq"
#define WIFI_SSID "PREDATOR_ZATCH"
#define WIFI_PASSWORD "Abhipray4"

long long int UPDATE_DELAY = 10000;
long long int NOTIF_DELAY = 30000;
long long int THRES = 25;

bool exited_last = false;

void setup() {
  //randomSeed(50);

  //I2C serial commmunication setup
  Serial.begin(9600); /* begin serial for debug */
  Wire.begin(D1, D2); /* join i2c bus with SDA=D1 and SCL=D2 of NodeMCU */
  
  //Serial.begin(115200);                                  //Serial connection
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);   //WiFi connection
 
  while (WiFi.status() != WL_CONNECTED) {  //Wait for the WiFI connection completion
 
    delay(500);
    Serial.println("Waiting for connection");
 
  }

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}
 
void loop() {
 if(WiFi.status()== WL_CONNECTED) {   //Check WiFi connection status
   
   UPDATE_DELAY = Firebase.getInt("UPDATE_DELAY");
   Serial.println((int)UPDATE_DELAY);
   UPDATE_DELAY = constrain(UPDATE_DELAY,10000,30000);

   NOTIF_DELAY = Firebase.getInt("NOTIF_DELAY");
   Serial.println((int)NOTIF_DELAY);
   NOTIF_DELAY = constrain(NOTIF_DELAY,30000,1800000);
   
   THRES = Firebase.getInt("THRES");
   Serial.println((int)THRES);
   THRES = constrain(THRES,20,30);

   Wire.beginTransmission(8);
   Wire.write((uint8_t)THRES >>8);
   Wire.write((uint8_t)THRES & 255);
   Wire.endTransmission();
   
   int temp;
   int Status;
   int receivedValue;

   long long int i=0;

   for (i=0; i<(NOTIF_DELAY/UPDATE_DELAY); i++) {


    //to recieve data from Arduino uno///////////////////////////////////////////////////////////////
    Wire.requestFrom(8, 2); /* request & read data of size 13 from slave */
    while(0<Wire.available()){
    
    receivedValue = Wire.read() << 8;
    receivedValue |= Wire.read();
    //Serial.println(receivedValue);
    //Serial.println("InsideWhile");
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////////////

    temp = receivedValue/10;
    Status = receivedValue%10;
    Serial.println(temp);
    
    
    Firebase.setInt("temp",temp);
   // if (Firebase.failed()) {Serial.println("Setting temp failed");}
    Firebase.setBool("ac_on",Status?true:false);
    //if (Firebase.failed()) {Serial.println("Setting ac_on failed");}
    Firebase.setBool("status",temp<=THRES?true:false);
    //if (Firebase.failed()) {Serial.println("Setting status failed");}

    if (temp>THRES && !exited_last) {
      break;  
    }
    
    delay(UPDATE_DELAY);
   }

   if (i<(NOTIF_DELAY/UPDATE_DELAY))
    exited_last = true;
   else
    exited_last = false;

   if (temp>THRES && exited_last) {
   
     HTTPClient http;    //Declare object of class HTTPClient
  
     http.begin("http://fcm.googleapis.com/fcm/send");      //Specify request destination
     http.addHeader("Content-Type","application/json");  //Specify content-type header
     http.addHeader("Authorization","key=AAAAcTXXS3g:APA91bFgMf5_NbURK3jJSJmfOiSw0Afjg851qmiyObQx3ntuNLnoZ_gzlKwypwYgwbitOK2SgXB_HnnqWB5qE2A7IyOVXy1neYs84imTWc7VmNuGuAqzCwlvWqcDl0jETLlE9bM1pEv-");  //Specify content-type header
  
     String data = "{\n";
     data += "\"notification\": {\n";
     data += "\"title\" : \"HIGH TEMPERATURE\",\n";
     data += "\"body\": \"WARNING: Server room temperature is "+String(temp)+"°C.\",\n";
     data += "\"sound\" : \"default\"\n";
     data += "}\n";
     data += ",\n";
     data += "\"to\" : \"/topics/ac_status\"\n";
     data += "}\"\n";
  
     Serial.println(data);
     
     int httpCode = http.POST(data);   //Send the request
     String payload = http.getString();                  //Get the response payload
   
     Serial.println(httpCode);   //Print HTTP return code
     Serial.println(payload);    //Print request response payload
     http.end();
   }
   
 } else {
    Serial.println("Error in WiFi connection");   
 }
}

int getTemp() {
 return random(20,30); 
}
