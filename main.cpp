// incluir librerias de sensores
#include <Arduino.h>
#include <ESP32WebServer.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h> 
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

//____________________________________________________________________________________________________________
// Sensor Pulso Cardiaco [codigo variables]
float factor = 0.75; //ponderación, 75%
float maximo = 0.0; //referencia inicial
int minimoEntreLatidos = 300; //300ms entre cada latidos (180 LPM), ritmo cardiaco mayor a 180 no será valido
float valorAnterior = 1950; //aprox mitad del valor máximo de 4095
int latidos = 0;
// Sensor Temp & Hum DHT11 [codigo variables]
int DHT_PIN = 15;
DHT dht(DHT_PIN, DHT11);
// Pantalla lcd con I2C [codigo variables]
LiquidCrystal_I2C lcd(0x27, 20, 4);
// Modulo GPS NEO6MV2 [codigo variables]
#define rxD2 16
#define txD2 17
SoftwareSerial mygps(rxD2,txD2);
void print_speed();
TinyGPSPlus gps;
int pulso = 2;
// Sensor ultrasónico HC-SR04 [codigo variables]
const int Trigger = 12;
const int Echo = 13;
int Buzzer = 14;
int Duracion;
int Distancia;
// Conexión WiFi
const char* ssid = "servidor_ESP32GPG"; //nombre WiFi
const char* password = "claveGPG"; //contraseña WiFi
String html;
ESP32WebServer server(80);
void code_html();
void respuesta();

//____________________________________________________________________________________________________________
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid,password);
// Sensor Cardiaco [codigo de setup]
  pinMode(2,OUTPUT);
  Serial.begin(115200);
  Serial.println("Iniciado mediciones");
// Sensor Temp & Hum DHT11[codigo de setup]
  dht.begin(); 
// Pantalla lcd con I2C[codigo de setup]
  lcd.init();
  lcd.backlight(); 
  lcd.begin(20, 4);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Bienvenido");
//Modulo GPS NEO6MV2[codigo de setup]
  Serial.begin(9600);
  mygps.begin(9600);
  pinMode(pulso,OUTPUT);      
  delay(3000);
  Serial.println("Velocimetro");
// Sensor ultrasónico HC-SR04 y Buzzer[codigo de setup]
  pinMode(Trigger, OUTPUT);
  pinMode(Echo, INPUT);
  pinMode(Buzzer, OUTPUT);
// Conexión WiFi[codigo de setup]
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print("Conectando a WiFi...");
    }
  Serial.println("Conexión WiFi establecida. Dirección IP: ");
  Serial.println(WiFi.localIP());
  server.on("/", respuesta);
  server.begin();
}

//____________________________________________________________________________________________________________
void loop() {
// Sensor Cardiaco [codigo de loop]
  static unsigned long tiempoLPM = millis();
  static unsigned long entreLatidos = millis();

  int valorLeido = analogRead(4);

  float valorFiltrado = factor * valorAnterior + (1-factor) * valorLeido;
  float cambio = valorFiltrado - valorAnterior;
  valorAnterior = valorFiltrado;

  if((cambio >= maximo) && (millis() > entreLatidos + minimoEntreLatidos)){ //Verdadero, se encuentra un pulso maximo
    maximo = cambio;
    digitalWrite(2,HIGH);
    entreLatidos = millis();
    latidos++;
  }
  else { //Cuando no se encuentra un máximo
    digitalWrite(2, LOW);
  }

  maximo = maximo * 0.97; //Por si la siguiente lectura es un poco menor que el valor maximo

  if(millis() >= tiempoLPM + 5000){ //Latidos en 5 segundos * 12 = 1 minuto
    Serial.print("Latidos por minuto: ");
    Serial.println(latidos*12);
    latidos = 0;
    tiempoLPM = millis();
  }

delay(50);
// Sensor Temp & Hum DHT11 [codigo de loop]
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  Serial.println(temp); 
  Serial.println(hum); 
// Pantalla lcd con I2C[codigo de loop]
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(hum);
  lcd.print("%");
  lcd.setCursor(0, 2);
  lcd.print("Vel: ");
  lcd.print(gps.speed.kmph);
  lcd.print("km/h");
  lcd.setCursor(0, 3);
  lcd.print("LPM: ");
  lcd.print(latidos*12);
// Modulo GPS NEO6MV2
boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (mygps.available())
    {
      if (gps.encode(mygps.read()))
      {
        newData = true;
      }
    }
  }
  //If newData is true
  if(newData == true)
  {
    newData = false;
    print_speed();
  }
  else
  {
    Serial.print("No hay datos");
  }
// Sensor ultrasónico HC-SR04[codigo de loop]
  digitalWrite(Trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trigger, LOW);
  Duracion = pulseIn(Echo, HIGH);
  Distancia = Duracion / 58.2;
  Serial.println(Distancia);
  delay(200);
//Buzzer
  if (Distancia <= 20 && Distancia >= 0) {
    digitalWrite(Buzzer, HIGH);
    delay(Distancia * 10);
    digitalWrite(Buzzer, LOW);
// Conexión WiFi[codigo de loop]
server.handleClient();
}
void print_speed()
{     
  if (gps.location.isValid() == 1)
  {
      
    Serial.print(gps.speed.kmph());
    Serial.println("km/h");
    Serial.print("SAT:");
    Serial.println(gps.satellites.value());
    Serial.print("ALT:");
    Serial.println(gps.altitude.meters(), 0);
  }
}
void code_html(){
  html= "<html> <head> <title> Modulo ESP32 Equipo 1</title> </head> <meta http-equiv=refresh content = 3>";
  html+="<body> <font color = 'black'> <div align = 'center'> <h1> Equipo de Ejercicio Tipo Kart </h1> <br> <h2> Estadísticas del usuario </h2> </div> </font>";
  html+="<hr align='center' size='2' width=90'%' color='blue'> <br>"; 
  html+="<table align = 'center' border = '2' width = 50'%' cellpadding = '10'> <tr> <th>Parámetro</th> <th>Medición</th> </tr>";
  html+="<tr> <td align = 'center'> Temperatura </td> <td align = 'center'>" + String(temp) + "C </td> </tr>";
  html+="<tr> <td align = 'center'> Humedad </td> <td align = 'center'>" + String(hum) + "% </td> </tr>";
  html+="<tr> <td align = 'center'> Distancia de obstáculo </td> <td align = 'center'>" + String(distance) + "cm </td> </tr> </table>";
  html+="<br> <div align = 'center'> <h3> Datos del GPS </h3> <br> <p>" + String(gpsData) + "<p> </div> </body> </html>";
}
void respuesta(){
  code_html();
  server.send(200, "text/html", html);
}
}

