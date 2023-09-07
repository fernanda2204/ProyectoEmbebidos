#include <EEPROM.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
//#include <Arduino.h>
#include <ESP32Servo.h>

/*-------------------------------------------------------------- ENCODER ROTATIVO---------------------------------------------*/
#define DT  36     // Pin DT 
#define CLK  39     // Pin CLK 
#define SW 26     // Botón del encoder  
/*-------------------------------------------------------------- SERVOMOTORES---------------------------------------------*/
#define servoPin1 27        // pastillas largas
#define servoPin2 14        // pastillas redondas
#define anguloInicial1 91     //posicion donde cae la pastilla hacia el recipiente (NO MODIFICAR!!)
#define anguloInicial2 67     
#define anguloFinal 0      //posicion donde cae la pastilla de LOS contenedor (NO MODIFICAR!!!)
Servo servo1;
Servo servo2;

/*------------------------------------------------------ MOTORES A PASOS--------------------------------------------------*/
#define inA 19      //MOTOR DE BANDA
#define inB 18
#define inC 5
#define inD 17

#define in1 16     //MOTOR SELECTOR DE VASOS
#define in2 4
#define in3 2
#define in4 15
#define pasosM1 512      //define el numero de pasos para que el motor de una vuelta completa  (MODIFICAR !!)
#define tiempopasos 10  // tiempo de cada paso, minimo para no perder fuerza = 10
#define tiempopasosM2 5  // tiempo de cada paso, minimo para no perder fuerza = 10
/*------------------------------------------------ELECTROBOMBAS------------------------------------------------------------*/
#define bomba1Pin 23
#define bomba2Pin 32
unsigned long tempoAgua = 12000;       //tiempo de activacion de la bomba de agua  (MODIFICAR !!)
unsigned long tempoJarabe = 16000;     //teimpo de activacion de la bomba de jarabe  (MODIFICAR !!)
/*---------------------------------------------- SENSOR INFRORROJO-------------------------------------------------------*/
#define detectorPin 25
#define buzzerPin 13
/*---------------------------------------------- FINAL DE CARRERA-------------------------------------------------------*/
#define FC1 33
#define FC2 35
#define countAlarm 5

/*---------------------------------------------- DECLARACION DE VARIABLES GLOBALES----------------------------------------*/
#define EEPROM_SIZE 30 // 30 bytes
#define RXD 3
#define TXD 1
//VARIABLES DE AJUSTE DEL SISTEMA
int dosis1, dosis2, dosis3;
int Hora1,Minuto1,Hora2,Minuto2,Hora3,Minuto3,Frec_Hora1,Frec_Minuto1,Frec_Hora2,Frec_Minuto2,Frec_Hora3,Frec_Minuto3;
int dosis[3] = {0 , 0, 0}; //P1, P2, J
int Horarios[3][2] = {
  {Hora1, Minuto1}, //HORA DE LA PASTILLA 1
  {Hora2, Minuto2}, //HORA DE LA PASTILLA 2
  {Hora3, Minuto3}  //HORA DEL JARABE
};
int Frecuencia[3][2] = {
  {Frec_Hora1, Frec_Minuto1}, //frec DE LA PASTILLA 1
  {Frec_Hora2, Frec_Minuto2}, //frec DE LA PASTILLA 2
  {Frec_Hora3, Frec_Minuto3}  //frec DEL JARABE
};
//variables para el encoder rotativo
volatile int posicion_encoder = 0;  // Posicion del encoder
int ant_posicion = 0;               // Valor anterior de la posicion del encoder
int Sensibilidad_rotativo = 180;    // menor valor , mas rapido, menos preciso
int Sensibilidad_pulsador = 80;     // mayor valor, presionar mas fuerte
int nivel_Menu = 1;                 // cuenta el nivel del menu en el que se esta
int Selec_encoder = 0;              // Para escoger el rango en que puede variar el encoder 

char *Medicamentos[] = {"Pastilla 1", "Pastilla 2", "Jarabe    ", "Atras         "}; // Arreglo del menu de medicamentos
char *Med_Opciones[] = {"Dosis      ", "Horario   ","Frecuencia" ,"Atras         "}; // Arreglo del menu de opciones para cada medicamento
String med,medida,b,c;                                                               // Para desplegar en el LCD
bool ActualizarDatos = true;

int CLAVE[3] = {0,0,0};            // almacena en un array 4 digitos ingresados
int CLAVE_MAESTRA[3] = {1,2,3};    // almacena en un array la contraseña inicial
int INDICE = 0;                    // indice del array CLAVE[]
int Indice_Medicamento;            
int aux_Horario = 0;               // para seleccionar hora/minuto

//variables para el LCD
LiquidCrystal_I2C lcd(0x27,16,2);
int FlechaD = 0, FlechaI = 1, FlechaAb = 2, FlechaAr = 3;  //indices para guardar los caracteres creados

byte FlechaDerecha[8] = { B01000,B01100,B01110,B01111,B01110,B01100,B01000,B00000};
byte FlechaIzquierda[8] = { B00010,B00110,B01110,B11110,B01110,B00110,B00010,B00000};
byte FlechaAbajo[8] = {0b00000,0b00000,0b00000,0b11111,0b01110,0b00100,0b00000,0b00000};
byte FlechaArriba[8] = {0b00000,0b00000,0b00100,0b01110,0b11111,0b00000,0b00000,0b00000};

//variables para el RTC
RTC_DS1307 rtc;


  int baudRate = 230400;
  bool HMI = true;
  bool signupOK = false;

void setup() {
  Serial.begin(baudRate);
  configPines();
  rescatar_EEPROM();
  ReemplazarDatos();
}

void loop() {
  ReemplazarDatos();
  SendSerial();
  Recibir_Serial2();
  //imprimirDatosSerial();
  guardar_EEPROM();
  DateTime now = rtc.now(); //almacena el tiempo actual en 'now'
  if (nivel_Menu == 1) {Selec_encoder = 0;menu_inicial(); navegacion1();} 
  if (nivel_Menu == 2) {Selec_encoder = 1;menu_password();navegacion2();}
  if (nivel_Menu == 3) {Selec_encoder = 2;menu_password2();navegacion3();} 
  if (nivel_Menu == 4) {menu_Clave_Incorrecta();} 
  if (nivel_Menu == 5) {Selec_encoder = 3;menu_medicamentos();navegacion5();}
  if (nivel_Menu == 6) {Selec_encoder = 4;menu_Med(med);navegacion6(); }
  if (nivel_Menu == 7) {Selec_encoder = 4;menu_Med(med);navegacion6(); }
  if (nivel_Menu == 8) {Selec_encoder = 4;menu_Med(med);navegacion6(); }
  if (nivel_Menu == 9) {if(Indice_Medicamento<2){Selec_encoder = 5;}else{Selec_encoder = 6;}menu_Dosis(medida);navegacion7();}
  if (nivel_Menu == 10){if(aux_Horario<1){Selec_encoder = 7;}else{Selec_encoder = 8;} menu_Horario(Horarios,b,c);navegacion8(Horarios);}
  if (nivel_Menu == 11){if(aux_Horario<1){Selec_encoder = 7;}else{Selec_encoder = 8;} menu_Horario(Frecuencia,b,c);navegacion8(Frecuencia);}
  if (ActualizarDatos) {
    if ((now.hour() == Horarios[0][0]) && (now.minute() == Horarios[0][1]) &&
      (now.hour() == Horarios[1][0]) && (now.minute() == Horarios[1][1]) &&
      (now.hour() == Horarios[2][0]) && (now.minute() == Horarios[2][1])) {
    Actualizar_Horario1();
    Actualizar_Horario2();
    Actualizar_Horario3();
    processInit(dosis1, dosis2, dosis3, 1, 1);

  } else if ((now.hour() == Horarios[0][0]) && (now.minute() == Horarios[0][1]) &&
             (now.hour() == Horarios[1][0]) && (now.minute() == Horarios[1][1])) {
    Actualizar_Horario1();
    Actualizar_Horario2();           
    processInit(dosis1, dosis2, 0, 1, 1);
    
  } else if ((now.hour() == Horarios[0][0]) && (now.minute() == Horarios[0][1]) &&
             (now.hour() == Horarios[2][0]) && (now.minute() == Horarios[2][1])) {
   Actualizar_Horario1();
   Actualizar_Horario3();
    processInit(dosis1, 0, dosis3, 1, 1);
    
  } else if ((now.hour() == Horarios[1][0]) && (now.minute() == Horarios[1][1]) &&
             (now.hour() == Horarios[2][0]) && (now.minute() == Horarios[2][1])) {
   Actualizar_Horario2();
   Actualizar_Horario3();           
    processInit(0, dosis2, dosis3, 1, 1);
  
  } else if ((now.hour() == Horarios[0][0]) && (now.minute() == Horarios[0][1])) {
    Actualizar_Horario1();
    processInit(dosis1, 0, 0, 1, 1);
  } else if ((now.hour() == Horarios[1][0]) && (now.minute() == Horarios[1][1])) {
    Actualizar_Horario2();
    processInit(0, dosis2, 0, 1, 1);
  } else if ((now.hour() == Horarios[2][0]) && (now.minute() == Horarios[2][1])) {
    Actualizar_Horario3();
    processInit(0, 0, dosis3, 1, 1);
  }
 }
}

void configPines () {
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  servo1.setPeriodHertz(50);
  servo2.setPeriodHertz(50);
  servo1.attach(servoPin1, 1000, 2000);
  servo2.attach(servoPin2, 1000, 2000);
  servo1.write(anguloInicial1);
  servo2.write(anguloInicial2);

  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(inA, OUTPUT);
  pinMode(inB, OUTPUT);
  pinMode(inC, OUTPUT);
  pinMode(inD, OUTPUT);
  pinMode(FC1, INPUT);
  pinMode(FC2, INPUT);
  pinMode(bomba1Pin, OUTPUT);
  pinMode(bomba2Pin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(detectorPin, INPUT);
  pinMode(DT, INPUT);   
  pinMode(CLK, INPUT);    
  pinMode(SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(DT), encoder, CHANGE); 
  //lcd
  lcd.init();
  lcd.backlight();
  lcd.createChar(FlechaD, FlechaDerecha);
  lcd.createChar(FlechaI, FlechaIzquierda);
  lcd.createChar(FlechaAb, FlechaAbajo);
  lcd.createChar(FlechaAr, FlechaArriba);
  //rtc
  iniciarReloj();
  //EEPROM
   EEPROM.begin(EEPROM_SIZE);
}

void iniciarReloj(){
  if (! rtc.begin()) 
  {
    lcd.print("Couldn't find RTC");
    while (1);
  }
  if (! rtc.isrunning()) 
  {
    lcd.print("RTC is NOT running!");
  }
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  //actualiza el tiempo de la PC
    //rtc.adjust(DateTime(2023, 12, 21, 23, 58, 0));  // Setear tiempo manualmente
}

void encoder()  {                               //Funcion que determina el sentido del giro del encoder
  static unsigned long ultimaInterrupcion = 0;  // variable static con ultimo valor de tiempo de interrupcion
  unsigned long tiempoInterrupcion = millis();  
  if(Selec_encoder==0){
    return;
  }
  if (tiempoInterrupcion - ultimaInterrupcion > Sensibilidad_rotativo) {  // No lee posibles rebotes en pulsos menores a X mseg
    if (digitalRead(CLK) == HIGH)                                         // si CLK es HIGH, rotacion a la derecha
    {
      posicion_encoder++ ;        
    }
    else {                                                                // si CLK es LOW, rotacion a la izquierda
      posicion_encoder--  ;        
    }
    
  switch (Selec_encoder) {
    case 1:
     if (posicion_encoder < 0) {posicion_encoder = 0;} 
     else if (posicion_encoder > 1) {posicion_encoder =1;} 
      break;
    case 2:
    if (posicion_encoder < 0) {posicion_encoder = 0;} 
     else if (posicion_encoder > 9) {posicion_encoder =9;}  
      break;
    case 3:
    if (posicion_encoder < 0) {posicion_encoder = 3;} 
     else if (posicion_encoder > 3) {posicion_encoder =0;}    
      break;
    case 4:
    if (posicion_encoder < 0) {posicion_encoder = 3;} 
     else if (posicion_encoder > 3) {posicion_encoder =0;} 
    case 5:
    if (posicion_encoder < 0) {posicion_encoder = 0;} 
     else if (posicion_encoder > 9) {posicion_encoder = 9;} 
      break;
    case 6:
    if (posicion_encoder < 0) {posicion_encoder = 0;} 
     else if (posicion_encoder > 6) {posicion_encoder = 6;} 
      break;
     case 7:
    if (posicion_encoder < 0) {posicion_encoder = 23;} 
     else if (posicion_encoder > 23) {posicion_encoder = 0;} 
       break;   
    case 8:
    if (posicion_encoder < 0) {posicion_encoder = 59;} 
     else if (posicion_encoder > 59) {posicion_encoder = 0;} 
       break;
    default:
      break;
  }
  ultimaInterrupcion = tiempoInterrupcion;
  }           
}

void menu_inicial(){
    ActualizarDatos = true;
    DateTime now = rtc.now();
    lcd.setCursor(0, 1);
    lcd.print("HORA");
    lcd.print(" ");
    if(now.hour()<10){lcd.print('0');}
    lcd.print(now.hour());
    lcd.print(':');
    if(now.minute()<10){lcd.print('0');}
    lcd.print(now.minute());
    lcd.print(':');
    if(now.second()<10){lcd.print('0');}
    lcd.print(now.second());
    lcd.print("  ");
    lcd.write(FlechaD);

    lcd.setCursor(0, 0);
    lcd.print("FECHA");
    lcd.print(" ");
    if(now.day()<10){lcd.print('0');}
    lcd.print(now.day());
    lcd.print('/');
    if(now.month()<10){lcd.print('0');}
    lcd.print(now.month());
    lcd.print('/');
    lcd.print(now.year());
    lcd.print("  ");
}

void navegacion1(){
  if (digitalRead(SW) == 0){
     posicion_encoder = 0; 
     nivel_Menu = 2; 
     //nivel_Menu =5; 
     lcd.clear();
     delay(Sensibilidad_pulsador);
  }
}

void menu_password() {
  lcd.setCursor(1, 0);
  lcd.print("CLAVE: ");
  lcd.setCursor(0, posicion_encoder);
  lcd.write(FlechaD);
  lcd.setCursor(1, 1);
  lcd.print("Volver");
  //lcd.write(FlechaAr);
  for(int k =0;k<3;k++){
  lcd.setCursor(10+2*k, 0);
  lcd.print(CLAVE[k]);
  }
 
}

void navegacion2(){
  if (posicion_encoder != ant_posicion){
    lcd.setCursor(0,0);
    lcd.print(" ");
    lcd.setCursor(0,1);
    lcd.print(" ");
    menu_password();
    ant_posicion = posicion_encoder;
  }
  if(digitalRead(SW) == 0){
      if(posicion_encoder==0){nivel_Menu = 3;}
      else{nivel_Menu = 1;}
      delay(Sensibilidad_pulsador);
    }
}
void menu_password2() {
  lcd.setCursor(1, 0);
  lcd.print("CLAVE: ");
  lcd.setCursor(0,0);
  lcd.write(FlechaD);
  lcd.setCursor(0, 1);
  lcd.print("       ");
  lcd.setCursor(10+INDICE*2, 1);
  lcd.write(FlechaAr);
  for(int k =0;k<3;k++){
  lcd.setCursor(10+2*k, 0);
  lcd.print(CLAVE[k]);
  }
}
void navegacion3(){
    if (posicion_encoder != ant_posicion){
      CLAVE[INDICE] = posicion_encoder;
      ant_posicion = posicion_encoder;
    }
      if(digitalRead(SW) == 0){
       if(INDICE<4){
       INDICE ++;
       posicion_encoder = 0;
       lcd.setCursor(0,1);
       lcd.print("                ");
       }
      if(INDICE == 3){
        //Serial.println('INDICE = 3');
        bool claveCorrecta = true;
        for(int i; i<3;i++){
          if(CLAVE[i] != CLAVE_MAESTRA[i]){
            claveCorrecta = 0;
          }
        }
        if(claveCorrecta){
        nivel_Menu = 5;
        for(int i = 0;i<3;i++){
        CLAVE[i] = 0;
        INDICE = 0;      
        }
        posicion_encoder = 0;
        lcd.clear();
        }
        else{
        nivel_Menu = 4;
        for(int i = 0;i<3;i++){
        CLAVE[i] = 0;      
        }
        INDICE = 0;
        }
       }
      delay(Sensibilidad_pulsador);
      }

 }

void menu_Clave_Incorrecta(){
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("CLAVE INCORRECTA");
    delay(1000);
    lcd.clear();
    posicion_encoder = 0;
    nivel_Menu = 2;
 }

void menu_medicamentos(){
  ActualizarDatos = false;
  lcd.setCursor(1, 0);
  lcd.print("MEDICAMENTOS");
  lcd.setCursor(1, 1);
  lcd.write(FlechaD);
  lcd.print(Medicamentos[posicion_encoder]);
 }

void navegacion5(){
  if (posicion_encoder != ant_posicion){
    lcd.setCursor(2,1);
    lcd.print(Medicamentos[posicion_encoder]);
    ant_posicion = posicion_encoder;
  }
  if(digitalRead(SW) == 0){
        switch(posicion_encoder){
          case 0:
          nivel_Menu = 6; //OPCIONES DE LA PASTILLA 1
          posicion_encoder = 0;
          lcd.clear();
          medida = "pastillas"; 
          med = "PASTILLA 1"; 
          Indice_Medicamento = 0;
          break;
          case 1:
          nivel_Menu = 7; //OPCIONES DE LA PASTILLA 2
          posicion_encoder = 0;
          lcd.clear();
          medida = "pastillas"; 
          med = "PASTILLA 2";
          Indice_Medicamento = 1;
         
          break;
          case 2:
          nivel_Menu = 8; //OPCIONES DEL JARABE
          posicion_encoder = 0;
          lcd.clear();
          medida = "onzas"; 
          med = "JARABE";
          Indice_Medicamento = 2;
          break;
          case 3:
          nivel_Menu = 1; //volver al menu inicial
          posicion_encoder = 0;
          SendSerial();
          lcd.clear();
          break;
        }
      delay(Sensibilidad_pulsador);
    }
}
void menu_Med(String medi){
  lcd.setCursor(1, 0);
  lcd.print(medi);
  lcd.setCursor(1, 1);
  lcd.write(FlechaD);
  lcd.print(Med_Opciones[posicion_encoder]);
}

void navegacion6(){
  if (posicion_encoder != ant_posicion){
    lcd.setCursor(2,1);
    lcd.print(Med_Opciones[posicion_encoder]);
    ant_posicion = posicion_encoder;
  }
  if(digitalRead(SW) == 0){
        switch(posicion_encoder){
          case 0:
          nivel_Menu = 9; //MENU DOSIS
          posicion_encoder = dosis[Indice_Medicamento];
          lcd.clear();
          break;
          case 1:
          nivel_Menu = 10;  //MENU HORARIO
          posicion_encoder = Horarios[Indice_Medicamento][0];
          aux_Horario = 0; 
          b = "ELIJA LA |"; 
          c = "  HORA   | ";
          lcd.clear();
          break;
          case 2:
          nivel_Menu = 11; //MENU FRECUENCIA
          posicion_encoder = Frecuencia[Indice_Medicamento][0];
          aux_Horario = 0; 
          b = "  TOMAR  |";
          c = "  CADA   | ";
          lcd.clear();
          break;
          case 3:
          nivel_Menu = 5;
          posicion_encoder = Indice_Medicamento;
          lcd.clear();
          break;
        }
      delay(Sensibilidad_pulsador);
    }
}
void menu_Dosis(String medida){
  lcd.setCursor(1, 0);
  lcd.print("ELEGIR DOSIS");
  lcd.setCursor(1, 1);
  lcd.write(FlechaD);
  lcd.print(" ");
  lcd.print(posicion_encoder);
  lcd.setCursor(5, 1);
  lcd.print(medida);
}

void navegacion7(){
  if (posicion_encoder != ant_posicion){
    dosis[Indice_Medicamento] = posicion_encoder;
    lcd.setCursor(3,1);
    lcd.print(dosis[Indice_Medicamento]);
    ant_posicion = posicion_encoder;
  }
  if(digitalRead(SW) == 0){
      nivel_Menu = 6+Indice_Medicamento;
      posicion_encoder = 0;
      lcd.clear(); 
      delay(Sensibilidad_pulsador);
      }
}

void menu_Horario(int a[3][2],String b, String c){
  lcd.setCursor(0, 0);
  //lcd.print("ELIJA LA |");
  lcd.print(b);
   lcd.setCursor(12+3*(aux_Horario), 0);
  lcd.write(FlechaAb);
  lcd.setCursor(0, 1);
  //lcd.print("  HORA   | ");
  lcd.print(c);
  if(a[Indice_Medicamento][0]<10){
  lcd.print('0');  
  }
  lcd.print(a[Indice_Medicamento][0]);
  lcd.print(":");
  if(a[Indice_Medicamento][1]<10){
  lcd.print('0');  
  }
  lcd.print(a[Indice_Medicamento][1]);
}

void navegacion8(int a[3][2]){
  if (posicion_encoder != ant_posicion){
    if(aux_Horario == 0){a[Indice_Medicamento][0] = posicion_encoder;}
    if(aux_Horario == 1){a[Indice_Medicamento][1] = posicion_encoder;}
    ant_posicion = posicion_encoder;
  }
 if(digitalRead(SW) == 0){
      if(aux_Horario == 0){
        posicion_encoder = a[Indice_Medicamento][1];
        aux_Horario = 1;
      }
      else if(aux_Horario == 1){
        nivel_Menu = 6+Indice_Medicamento;
        posicion_encoder = (c == "  HORA   | " ? 1 : 2);
      }
      lcd.clear(); 
      delay(Sensibilidad_pulsador);
}
}

void imprimirDatosSerial(){
  Serial.print(dosis1);
  Serial.print(dosis2);
  Serial.print(dosis3);
  Serial.print("          ");
  for (int i = 0; i < 3; i++) {
    int hora = Horarios[i][0];
    int minuto = Horarios[i][1];
    
    if (i > 0) {
      Serial.print(" | "); 
    }

    Serial.print("Horario ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(hora < 10 ? "0" : "");
    Serial.print(hora);
    Serial.print(":");
    Serial.print(minuto < 10 ? "0" : "");
    Serial.print(minuto);
  }

  Serial.print("   "); 

 for (int j = 0; j < 3; j++) {
    int hora2 = Frecuencia[j][0];
    int minuto2 = Frecuencia[j][1];
    
    if (j > 0) {
      Serial.print(" | "); 
    }

    Serial.print("Frecuencia ");
    Serial.print(j + 1);
    Serial.print(": ");
    Serial.print(hora2 < 10 ? "0" : "");
    Serial.print(hora2);
    Serial.print(":");
    Serial.print(minuto2 < 10 ? "0" : "");
    Serial.print(minuto2);
  }
  Serial.println(); 
}
void guardar_EEPROM(){
  EEPROM.write(0,dosis1);
  EEPROM.commit();
  EEPROM.write(1,dosis2);
  EEPROM.commit();
  EEPROM.write(2,dosis3);
  EEPROM.commit();
  EEPROM.write(3,Hora1);
  EEPROM.commit();
  EEPROM.write(4,Minuto1);
  EEPROM.commit();
  EEPROM.write(5,Hora2);
  EEPROM.commit();
  EEPROM.write(6,Minuto2);
  EEPROM.commit();
  EEPROM.write(7,Hora3);
  EEPROM.commit();
  EEPROM.write(8,Minuto3);
  EEPROM.commit();
  EEPROM.write(9,Frec_Hora1);
  EEPROM.commit();
  EEPROM.write(10,Frec_Minuto1);
  EEPROM.commit();
  EEPROM.write(11,Frec_Hora2);
  EEPROM.commit();
  EEPROM.write(12,Frec_Minuto2);
  EEPROM.commit();
  EEPROM.write(13,Frec_Hora3);
  EEPROM.commit();
  EEPROM.write(14,Frec_Minuto3);
  EEPROM.commit();
}
void ReemplazarDatos(){
  dosis1 = dosis[0];
  dosis2 = dosis[1];
  dosis3 = dosis[2];

  Hora1 = Horarios[0][0];
  Minuto1 = Horarios[0][1];
  Hora2 = Horarios[1][0];
  Minuto2 = Horarios[1][1];
  Hora3 = Horarios[2][0];
  Minuto3 = Horarios[2][1];

  Frec_Hora1 = Frecuencia[0][0];
  Frec_Minuto1 = Frecuencia[0][1];
  Frec_Hora2 = Frecuencia[1][0];
  Frec_Minuto2 = Frecuencia[1][1];
  Frec_Hora3 = Frecuencia[2][0];
  Frec_Minuto3 = Frecuencia[2][1];

}
void rescatar_EEPROM(){
  dosis[0] = EEPROM.read(0);
  dosis[1] = EEPROM.read(1);
  dosis[2] = EEPROM.read(2);
  Horarios[0][0] = EEPROM.read(3);
  Horarios[0][1] = EEPROM.read(4);
  Horarios[1][0] = EEPROM.read(5);
  Horarios[1][1] = EEPROM.read(6);
  Horarios[2][0] = EEPROM.read(7);
  Horarios[2][1] = EEPROM.read(8);
  Frecuencia[0][0] = EEPROM.read(9);
  Frecuencia[0][1] = EEPROM.read(10);
  Frecuencia[1][0] = EEPROM.read(11);
  Frecuencia[1][1] = EEPROM.read(12);
  Frecuencia[2][0] = EEPROM.read(13);
  Frecuencia[2][1] = EEPROM.read(14);
}

void SendSerial(){
    Serial.println(dosis1);
    Serial.println(dosis2);
    Serial.println(dosis3);
    Serial.println(Hora1);
    Serial.println(Minuto1);
    Serial.println(Hora2);
    Serial.println(Minuto2);
    Serial.println(Hora3);
    Serial.println(Minuto3);
    Serial.println(Frec_Hora1);
    Serial.println(Frec_Minuto1);
    Serial.println(Frec_Hora2);
    Serial.println(Frec_Minuto2);
    Serial.println(Frec_Hora3);
    Serial.println(Frec_Minuto3);
    Serial.println(HMI);
}
void Recibir_Serial2(){
  if (Serial.available()>14) {
    dosis[0] = Serial.parseInt();
    dosis[1] = Serial.parseInt();
    dosis[2] = Serial.parseInt();
    Horarios[0][0] = Serial.parseInt();
    Horarios[0][1] = Serial.parseInt();
    Horarios[1][0] = Serial.parseInt();
    Horarios[1][1] = Serial.parseInt();
    Horarios[2][0] = Serial.parseInt();
    Horarios[2][1] = Serial.parseInt();
    Frecuencia[0][0] = Serial.parseInt();
    Frecuencia[0][1] = Serial.parseInt();
    Frecuencia[1][0] = Serial.parseInt();
    Frecuencia[1][1] = Serial.parseInt();
    Frecuencia[2][0] = Serial.parseInt();
    Frecuencia[2][1] = Serial.parseInt();  
    Serial.end();
    Serial.begin(baudRate, SERIAL_8N1, RXD, TXD);
    }
  }

void beep1 (void) {
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);
  delay(50);
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);
  delay(50);
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);
  delay(50);
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);
  delay(50);
}

void selectVaso (void) {
  int j= 0;
  char secuencia[4][4] = {
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
  };
  // eempieza a girar el motor seleccionador de vasos una vuelta completa
  while (j < pasosM1) {
    for (int i = 0; i <= 3; i++) {
      digitalWrite(in1, secuencia [i][0]);
      digitalWrite(in2, secuencia [i][1]);
      digitalWrite(in3, secuencia [i][2]);
      digitalWrite(in4, secuencia [i][3]);
      delay(tiempopasos);
      }
    j ++;  
  }
  digitalWrite(in1, 0);
  digitalWrite(in2, 0);
  digitalWrite(in3, 0);
  digitalWrite(in4, 0);

}

void moverBase (void) {
  char secuencia[4][4] = {
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
  };
  delay(500);
  while (digitalRead(FC2) != 0) {
    for (int i = 0; i <= 3; i++) {
      digitalWrite(inA, secuencia [i][0]);
      digitalWrite(inB, secuencia [i][1]);
      digitalWrite(inC, secuencia [i][2]);
      digitalWrite(inD, secuencia [i][3]);
      delay(tiempopasos);
      } 
      //Serial.println("dentro del bucle 1");
  }
  digitalWrite(inA, 0);
  digitalWrite(inB, 0);
  digitalWrite(inC, 0);
  digitalWrite(inD, 0);
}

void regresarBase (void) {
  char secuencia[4][4] = {
    {0, 0, 0, 1},
    {0, 0, 1, 0},
    {0, 1, 0, 0},
    {1, 0, 0, 0}
  };
  delay(500);
  while (digitalRead(FC1) != 0) {
    for (int i = 0; i <= 3; i++) {
      digitalWrite(inA, secuencia [i][0]);
      digitalWrite(inB, secuencia [i][1]);
      digitalWrite(inC, secuencia [i][2]);
      digitalWrite(inD, secuencia [i][3]);
      delay(tiempopasosM2);
      }  
      //Serial.println("dentro del bucle 2");
  }
  digitalWrite(inA, 0);
  digitalWrite(inB, 0);
  digitalWrite(inC, 0);
  digitalWrite(inD, 0);
}

void dispensarAgua (void) {
  unsigned char alarma = 0;
  unsigned long tempo1ANT = 0;

  selectVaso(); // eempieza a girar el motor seleccionador de vasos una vuelta completa
  delay(1000);
  moverBase();
  // la banda transportadora lleva el vaso de la posicion inicial a la final
  tempo1ANT = millis();
  digitalWrite(bomba1Pin, HIGH);
  while ((millis() - tempo1ANT) < tempoAgua) ;
  digitalWrite(bomba1Pin, LOW);
  beep1();

  // espera a que el sensor infrarrojo detecte que el vaso ha sido tomado
  tempo1ANT = millis();
  while (digitalRead(detectorPin) == 0)
  {
    delay(1);
    if ((millis() - tempo1ANT) > 10000)
    {
      tempo1ANT = millis();
      alarma++;
      beep1();
      if (alarma > countAlarm)
      {
        // Serial.println("Medicinas no tomadas");
      }
    }
  }
  regresarBase();
  delay(500);
}

void dispensarJarabe(int nivelJarabe) {
  unsigned long jarabe = 0, tempo1ANT = 0;
  unsigned char alarma = 0;
  selectVaso(); // eempieza a girar el motor seleccionador de vasos una vuelta completa
  delay(1000);
  moverBase();  // la banda transportadora lleva el vaso de la posicion inicial a la final
  delay(1000);
  // inicia el proceso de llenado de jarabe

  jarabe = map(nivelJarabe, 0, 6, 0, 10000);

  digitalWrite(bomba2Pin, HIGH);
  tempo1ANT = millis();
  while ((millis() - tempo1ANT) < tempoJarabe)
    ;
  digitalWrite(bomba2Pin, LOW);
  delay(100);

  // espera a que el sensor infrarrojo detecte que el vaso ha sido tomado
  // tempo1ANT = millis();
  while (digitalRead(detectorPin) == 0)
  {
    // Serial.println(digitalRead(detectorPin));
    if ((millis() - tempo1ANT) > 10000)
    {
      tempo1ANT = millis();
      alarma++;
      beep1();
      if (alarma > countAlarm)
      {
        // Serial.println("Medicinas no tomadas");
      }
    }
    /// en este apartado se envia una alerta al hmi de la red indicando que el vaso no ha sido tomado
  }
  regresarBase(); // la banda transportadora regresa a su posicion iniciar
  delay(500);
}

void dispensarPastilla (int pastilla1, int pastilla2) {
  servo1.write(anguloInicial1);
  servo2.write(anguloInicial2);
  regresarBase();
  delay(500);
  // despliega el numero de pastillas redondas
  while (pastilla1 > 0) {
    servo1.write(anguloFinal);
    delay(1000);
    servo1.write(anguloInicial1);
    delay(1000);
    pastilla1 --;
  }

  // despliega el numero de pastilla rectangulares
  while (pastilla2 > 0) {
    servo2.write(anguloFinal);
    delay(1000);
    servo2.write(anguloInicial2);
    delay(1000);
    pastilla2 --;
  }
}

void processInit (int past1, int past2, int nivelJarabe, bool liqDisponible, bool pastDisponibles) {
  //lcd.clear();
  //lcd.print("DISPENSANDO . . .");
  /*if (liqDisponible == 0) {
    Serial.println("No hay suficiente liquido en el tanque");
    goto final;
  }
  if (pastDisponibles == 0) {
    Serial.println("No hay suficientes pastillas");
    goto final;
  }
  if ((past1 == 0) || (past2 == 0)) {
    Serial.println("No hay suficientes pastillas");
    goto final;
  }*/
  if ((past1 > 0) || (past2 > 0)) {
    dispensarPastilla(past1, past2);
    dispensarAgua();
  }
  if (nivelJarabe > 0){
    dispensarJarabe(nivelJarabe);
  }
  final:
  servo1.attach(servoPin1, 1000, 2000);
  servo2.attach(servoPin2, 1000, 2000);
  //Serial.println("final del programa");
  //lcd.clear();
}

void Actualizar_Horario1(){
    Horarios[0][1] = Horarios[0][1]  + Frecuencia[0][1];
    if(Horarios[0][1]>59){
       Horarios[0][1] = Horarios[0][1] - 60; 
       Horarios[0][0] = Horarios[0][0] + 1;
       }
    else{
       Horarios[0][0] = Horarios[0][0]+Frecuencia[0][0]; 
    }       
    if(Horarios[0][0]>23){
      Horarios[0][0] = Horarios[0][0] -24;
    } 
}

void Actualizar_Horario2(){
    Horarios[1][1] = Horarios[1][1]  + Frecuencia[1][1];
    if(Horarios[1][1]>59){
       Horarios[1][1] = Horarios[1][1] - 60; 
       Horarios[1][0] = Horarios[1][0] + 1;
       }
    else{
       Horarios[1][0] = Horarios[1][0]+Frecuencia[1][0]; 
    }       
    if(Horarios[1][0]>23){
      Horarios[1][0] = Horarios[1][0] -24;
    } 
}

void Actualizar_Horario3(){
    Horarios[2][1] = Horarios[2][1]  + Frecuencia[2][1];
    if(Horarios[2][1]>59){
       Horarios[2][1] = Horarios[2][1] - 60; 
       Horarios[2][0] = Horarios[2][0] + 1;
       }
    else{
       Horarios[2][0] = Horarios[2][0]+Frecuencia[2][0]; 
    }       
    if(Horarios[2][0]>23){
      Horarios[2][0] = Horarios[2][0] -24;
    } 
}
       
       


