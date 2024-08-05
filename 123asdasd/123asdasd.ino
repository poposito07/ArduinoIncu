#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <time.h>

#define DHT_PIN 4 

const char* ssid = "Honor";
const char* password = "12345678";

const char* mqtt_server = "34.226.127.27";  
const int mqtt_port = 1883;  
const char* mqtt_user = "angel"; 
const char* mqtt_password = "angelgabriel"; 

WiFiClient espClient;
PubSubClient client(espClient);

Servo miServo;  
LiquidCrystal lcd(14, 17, 5, 22, 23, 21); 
DHT dht(DHT_PIN, DHT11);  

const int pinServo = 2;  
const int ldrPin = 32; 

#define TRIGGER_PIN 18
#define ECHO_PIN 19

const char* topic = "esp32.mqtt";

String scrollText;
int scrollIndex = 0;
unsigned long lastScrollTime = 0;
unsigned long scrollInterval = 1000;
const int ldrThreshold = 30;

void setup() {
  Serial.begin(9600);

  miServo.attach(pinServo);

  lcd.begin(16, 2);

  dht.begin();
  
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);

  configTime(-5 * 3600, 0, "pool.ntp.org");

  delay(2000);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("Conectado");
    } else {
      Serial.print("Error, rc=");
      Serial.print(client.state());
      Serial.println(" Intenta de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

float medirDistancia() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  long duracion = pulseIn(ECHO_PIN, HIGH);

  float distancia = (duracion * 0.0343) / 2;

  return distancia;
}

void mostrarFechaHora() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  
  char timeStringBuff[20];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%d/%m %H:%M:%S", &timeinfo);

  lcd.setCursor(0, 0);
  lcd.print(timeStringBuff);
}

void actualizarScrollText(float t, float h, float d, String diaNoche) {
  scrollText = "Temp: " + String(t) + "C Hum: " + String(h) + "% Dist: " + String(d) + "cm " + diaNoche;
}

void mostrarScrollText() {
  if (millis() - lastScrollTime >= scrollInterval) {
    lcd.setCursor(0, 1);
    for (int i = 0; i < 16; i++) {
      int charIndex = (scrollIndex + i) % scrollText.length();
      lcd.print(scrollText[charIndex]);
    }
    scrollIndex = (scrollIndex + 1) % scrollText.length();
    lastScrollTime = millis();
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float d = medirDistancia(); 
  int ldrValue = analogRead(ldrPin); 

  Serial.print("Valor LDR: ");
  Serial.println(ldrValue);

  String diaNoche = (ldrValue > ldrThreshold) ? "Dia" : "Noche";

  if (isnan(h) || isnan(t) || isnan(d)) {
    lcd.setCursor(0, 1);
    lcd.print("Error en sensor");
    Serial.println("Error al leer el sensor!");
  } else {

    actualizarScrollText(t, h, d, diaNoche);

    Serial.print("Temperatura: ");
    Serial.print(t);
    Serial.print(" °C\t");
    Serial.print("Humedad: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Distancia: ");
    Serial.print(d);
    Serial.print(" cm\t");
    Serial.print("Estado: ");
    Serial.println(diaNoche);


    String payload = "Temp:" + String(t) + ", Hum:" + String(h) + ", Dist:" + String(d) + ", Estado:" + diaNoche;
    if (client.publish(topic, payload.c_str())) {
      Serial.println("Mensaje enviado con éxito");
    } else {
      Serial.println("Error al enviar el mensaje");
    }
  }


  for (int angulo = 0; angulo <= 180; angulo += 1) {
    miServo.write(angulo);
    delay(15);
  }

  for (int angulo = 180; angulo >= 0; angulo -= 1) {
    miServo.write(angulo);
    delay(15);
  }

  mostrarFechaHora();  

  mostrarScrollText();
  
  delay(2000);
}


