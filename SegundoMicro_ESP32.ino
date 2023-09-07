#include <iostream>
#include <string>
#include <sstream>
//#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Insert your network credentials
  #define WIFI_SSID "CAMPUS_EPN"
  #define WIFI_PASSWORD "politecnica**"
 // #define WIFI_SSID "proyecto"
  //#define WIFI_PASSWORD "soypobre" 
 // #define WIFI_SSID "CELERITY_JOHAN"
 // #define WIFI_PASSWORD "JSav2711"
//#define WIFI_SSID "FIBRAMAX_PEDRO"
 //#define WIFI_PASSWORD "poch1715134373"
    
    //#define WIFI_SSID "Spider-man"
    //#define WIFI_PASSWORD "759153aquaparkBlackSpider188HDsanake188bombastico"

#define API_KEY "AIzaSyAYgWfoFyqvcjlaHWH_Ohn13RekR3LUCYs"
#define DATABASE_URL "https://test-dispensador-default-rtdb.firebaseio.com/" 
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

int dosis1, dosis2, dosis3;
int Hora1,Minuto1,Hora2,Minuto2,Hora3,Minuto3,Frec_Hora1,Frec_Minuto1,Frec_Hora2,Frec_Minuto2,Frec_Hora3,Frec_Minuto3;

std::string Horario1,Horario2,Horario3,Frecuencia1,Frecuencia2,Frecuencia3;
bool signupOK = false;

//SERIAL 2
#define RXD2 16
#define TXD2 17
int baudRate = 230400;

void setup(){
  Serial.begin(baudRate);
  Serial2.begin(baudRate, SERIAL_8N1, RXD2, TXD2);
  Recibir_Serial();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  /* Assign the api key (required) */
  config.api_key = API_KEY;
  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

bool HMI = true;
void loop(){
  HMI = VerificarHMI();
  Serial2.print(HMI);
  if(HMI){
  Recibir_Serial();
  EnviarFireBase();
  }
  else{
  Recibir_Firebase();
  EnviarSerial();
  }
    
Serial.println("dosis1: " + String(dosis1));
Serial.println("dosis2: " + String(dosis2));
Serial.println("dosis3: " + String(dosis3));
Serial.println("Hora1: " + String(Hora1));
Serial.println("Minuto1: " + String(Minuto1));
Serial.println("Hora2: " + String(Hora2));
Serial.println("Minuto2: " + String(Minuto2));
Serial.println("Hora3: " + String(Hora3));
Serial.println("Minuto3: " + String(Minuto3));
Serial.println("Frec_Hora1: " + String(Frec_Hora1));
Serial.println("Frec_Minuto1: " + String(Frec_Minuto1));
Serial.println("Frec_Hora2: " + String(Frec_Hora2));
Serial.println("Frec_Minuto2: " + String(Frec_Minuto2));
Serial.println("Frec_Hora3: " + String(Frec_Hora3));
Serial.println("Frec_Minuto3: " + String(Frec_Minuto3));
Serial.println("HMI: " + String(HMI));
}

void Recibir_Serial(){
  if (Serial2.available()>15) {
    
    dosis1 = Serial2.parseInt();
    dosis2 = Serial2.parseInt();
    dosis3 = Serial2.parseInt();
    Hora1 = Serial2.parseInt();
    Minuto1 = Serial2.parseInt();
    Hora2 = Serial2.parseInt();
    Minuto2 = Serial2.parseInt();
    Hora3 = Serial2.parseInt();
    Minuto3 = Serial2.parseInt();
    Frec_Hora1 = Serial2.parseInt();
    Frec_Minuto1 = Serial2.parseInt();
    Frec_Hora2 = Serial2.parseInt();
    Frec_Minuto2 = Serial2.parseInt();
    Frec_Hora3 = Serial2.parseInt();
    Frec_Minuto3 = Serial2.parseInt();  
    HMI = Serial2.read();  
    Serial2.end();
    Serial2.begin(baudRate, SERIAL_8N1, RXD2, TXD2);
    }
    Horario1 = TratarDatos(Hora1,Minuto1);
    Horario2 = TratarDatos(Hora2,Minuto2);  
    Horario3 = TratarDatos(Hora3,Minuto3); 
    Frecuencia1 = TratarDatos(Frec_Hora1,Frec_Minuto1); 
    Frecuencia2 = TratarDatos(Frec_Hora2,Frec_Minuto2); 
    Frecuencia3 = TratarDatos(Frec_Hora3,Frec_Minuto3);
 }


std::string TratarDatos(int a,int b){
  std::string c;
  if(a<10){
    if(b<10){
      c = "0"+ std::to_string(a) + "h" + "0"+ std::to_string(b);
    }
    else{
      c = "0"+ std::to_string(a) + "h" + std::to_string(b);  
    }
  }
  else{
  c = std::to_string(a) + "h" + std::to_string(b);  
  }
  return c;
}
void EnviarSerial(){
    Serial2.println(dosis1);
    Serial2.println(dosis2);
    Serial2.println(dosis3);
    Serial2.println(Hora1);
    Serial2.println(Minuto1);
    Serial2.println(Hora2);
    Serial2.println(Minuto2);
    Serial2.println(Hora3);
    Serial2.println(Minuto3);
    Serial2.println(Frec_Hora1);
    Serial2.println(Frec_Minuto1);
    Serial2.println(Frec_Hora2);
    Serial2.println(Frec_Minuto2);
    Serial2.println(Frec_Hora3);
    Serial2.println(Frec_Minuto3);
}
void EnviarFireBase(){
  if (Firebase.ready() && signupOK ){
    if (Firebase.RTDB.setString(&fbdo, "test/dosis1", dosis1)){
    }
    if (Firebase.RTDB.setString(&fbdo, "test/dosis2", dosis2)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/dosis3", dosis3)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Horario1",Horario1)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Horario2", Horario2)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Horario3", Horario3)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Frecuencia1", Frecuencia1)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Frecuencia2", Frecuencia2)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Frecuencia3", Frecuencia3)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Hora1", Hora1)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Hora2", Hora2)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Hora3", Hora3)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Minuto1", Minuto1)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Minuto2", Minuto2)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Minuto3", Minuto3)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Frec_Hora1", Frec_Hora1)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Frec_Hora2", Frec_Hora2)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Frec_Hora3", Frec_Hora3)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Frec_Minuto1", Frec_Minuto1)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Frec_Minuto2", Frec_Minuto2)){
    }
     if (Firebase.RTDB.setString(&fbdo, "test/Frec_Minuto3", Frec_Minuto3)){
    }
  } 
}

void Recibir_Firebase(){
  if (Firebase.ready() && signupOK) {
    dosis1 = RecibirDataFB("dosis1");
    dosis2 = RecibirDataFB("dosis2");
    dosis3 = RecibirDataFB("dosis3");
    Hora1 = RecibirDataFB("Hora1");
    Minuto1 = RecibirDataFB("Minuto1");
    Hora2 = RecibirDataFB("Hora2");
    Minuto2 = RecibirDataFB("Minuto2");
    Hora3 = RecibirDataFB("Hora3");
    Minuto3 = RecibirDataFB("Minuto3");
    Frec_Hora1 = RecibirDataFB("Frec_Hora1");
    Frec_Minuto1 = RecibirDataFB("Frec_Minuto1");
    Frec_Hora2 = RecibirDataFB("Frec_Hora2");
    Frec_Minuto2 = RecibirDataFB("Frec_Minuto2");
    Frec_Hora3 = RecibirDataFB("Frec_Hora3");
    Frec_Minuto3 = RecibirDataFB("Frec_Minuto3");
  }
}

bool VerificarHMI(){
  std::string a;
  bool HMI;
   if (Firebase.RTDB.getString(&fbdo, "/test/boton")) {
      if (fbdo.dataType() == "string") {
        String st =  fbdo.stringData();
        a = std::string(st.c_str());
        }
        if( a == "true"){
          HMI = true;
        }
        else{
          HMI = false;
        }
      }
      return HMI;
}
  
int RecibirDataFB(String label){
  std::string b;
  int numero;
  if (Firebase.RTDB.getString(&fbdo, "/test/"+label)) {
      if (fbdo.dataType() == "string") {
        String st = fbdo.stringData();
        b = std::string(st.c_str());
        //std::istringstream(cadenaSinComillas) >> numero;
        numero = std::stoi(b.c_str());
      }
      /*else   if (fbdo.dataType() == "int") {
        numero = fbdo.intData();
      }*/
  }
   return numero;
}
