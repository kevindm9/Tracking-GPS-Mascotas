#include <Adafruit_GPS.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <WiFiManager.h>
#include <ThingSpeak.h>

// Configuración de ThingSpeak
const long channelID = 2572994;         // Tu ID de canal de ThingSpeak
char *writeAPIKey = "4V32WLP6IDN7UE0W"; // Tu clave de API de escritura de ThingSpeak

// Pines de comunicación GPS
const int RXPin = 4; // GPIO4 (D2 en ESP8266, TX del módulo GPS)
const int TXPin = 5; // GPIO5 (D1 en ESP8266, RX del módulo GPS)

// Intervalo de envío de datos
const unsigned long postingInterval = 20L * 1000L; // Intervalo de envío en milisegundos
unsigned int dataFieldOne = 1;                     // Campo para escribir la latitud
unsigned int dataFieldTwo = 2;                     // Campo para escribir la longitud

SoftwareSerial SerialGPS(RXPin, TXPin); // Configurar SoftwareSerial
Adafruit_GPS GPS(&SerialGPS); // Crear una instancia de Adafruit_GPS

float Latitude;
float Longitude;
String DateString, TimeString, LatitudeString, LongitudeString;
WiFiClient cliente;
unsigned long lastPostTime = 0;

void setup() {
    Serial.begin(115200);     // Iniciar serial para depuración
    SerialGPS.begin(9600);    // Iniciar comunicación con GPS a 9600 baudios

    WiFiManager wifiManager;
    wifiManager.autoConnect("ESP8266Temp");
    Serial.println("¡Conexión exitosa a la red WiFi!");

    // Inicializar ThingSpeak
    ThingSpeak.begin(cliente);

    // Iniciar el GPS y configurar la salida NMEA
    GPS.begin(9600);
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); // Configurar para enviar solo RMC y GGA
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);    // Configurar la actualización a 1 Hz (una vez por segundo)

    // Imprimir un mensaje de inicio
    Serial.println("Iniciando GPS...");
}

void loop() {
    // Leer datos del GPS
    char c = GPS.read();
    
    // Imprimir datos en crudo del GPS para depuración
    
    if (c) {
        Serial.print(c);
    }

    // Verificar si hay una nueva línea de datos disponible
    if (GPS.newNMEAreceived()) {
        if (!GPS.parse(GPS.lastNMEA())) {
            return; // Si no se pudo analizar, salir
        }

        // Procesar los datos recibidos
        if (GPS.fix) {
            Latitude = GPS.latitudeDegrees;
            Longitude = GPS.longitudeDegrees;
            LatitudeString = String(Latitude, 6);
            LongitudeString = String(Longitude, 6);
        } else {
            LatitudeString = "INVALID";
            LongitudeString = "INVALID";
        }

        DateString = String(GPS.day) + "/" + String(GPS.month) + "/" + String(GPS.year);
        TimeString = String(GPS.hour) + ":" + String(GPS.minute) + ":" + String(GPS.seconds);

        // Imprimir datos de GPS para depuración
        Serial.print("Latitud: ");
        Serial.println(LatitudeString);
        Serial.print("Longitud: ");
        Serial.println(LongitudeString);
        Serial.print("Fecha: ");
        Serial.println(DateString);
        Serial.print("Hora: ");
        Serial.println(TimeString);
    }

    // Enviar datos a ThingSpeak si ha pasado el intervalo de envío
    unsigned long currentTime = millis();
    if (currentTime - lastPostTime >= postingInterval) {
        if (GPS.fix) {
            ThingSpeak.setField(dataFieldOne, Latitude);
            ThingSpeak.setField(dataFieldTwo, Longitude);

            int statusCode = ThingSpeak.writeFields(channelID, writeAPIKey);
            if (statusCode == 200) {
                Serial.println("Datos enviados a ThingSpeak correctamente.");
                 delay(1000); 
            } else {
                Serial.print("Error al enviar datos a ThingSpeak. Código de estado: ");
                Serial.println(statusCode);
            }
        } else {
            Serial.println("No se puede enviar a ThingSpeak porque no hay un fix GPS.");
        }
        lastPostTime = currentTime;
    }
}

