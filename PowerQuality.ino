#include <PZEM004Tv30.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Credenciales para conexión WiFi
const char *ssid = "ProyectosElectronica"; // Nombre de la red WiFi.
const char *password = "proyectos";  // Contraseña de la red.

// Credenciales para la conexión MQTT
const char *mqtt_broker = "broker.emqx.io"; // Broker EMQX.
const char *topic = "esp32/recepcion"; // Tópico para recepción de datos en el broker.
const char *mqtt_username = "emqx"; // Nombre de usuario para acceder al broker.
const char *mqtt_password = "public"; // Contraseña para acceder al broker.
const int mqtt_port = 1883; // Puerto de comunicación.

// Otras variables
float pot_aparente;
float pot_reactiva;

#if defined(ESP32)
PZEM004Tv30 pzem(Serial2, 16, 17); RX2, TX2
#else
PZEM004Tv30 pzem(Serial2);
#endif

// Variables para el manejo de formato JSON.
String jsonSerial = ""; 
JsonDocument myjson; //Se genera un objeto JSON.
char msg;

// Clientes para publicación a MQTT.
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password); // Se inicia la conexión WiFi.

  // Se revisa el estado de la conexión y no se avanza hasta que sea exitoso.
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the Wi-Fi network");
  
  // Conexión al broker MQTT
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  // Se revisa el estado de la conexión MQTT hasta que se logre la comunicación con el broker.
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf(" The client %s connects to the public MQTT broker\n", client_id.c_str());

    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Public EMQX MQTT broker connected");
    } 
    else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

// Función para procesar los mensajes recibidos en un tópico MQTT.
void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}

void loop() {
        
  Serial.print("Custom Address:");
  Serial.println(pzem.readAddress(), HEX);

  // Se almacenan los datos obtenidos por el sensor
  float voltage = pzem.voltage(); // Se obtiene el voltaje - V
  float current = pzem.current(); // Se obtiene la corriente - A
  float power = pzem.power(); // Se obtiene la potencia (activa) - W
  float energy = pzem.energy(); // Se obtiene la energía - kWh
  float frequency = pzem.frequency(); // Se obtiene la frecuencia - Hz
  float pf = pzem.pf(); // Se obtiene el factor de potencia

  // Se obtienen variables faltantes que no se extraen directamente desde el sensor
  pot_aparente = abs(voltage)*abs(current); // Se obtiene la potencia aparente - VA
  pot_reactiva = pot_aparente*sin(acos(pf)); // Se obtiene la potencia reactiva - VAr

  // Se verifica si se pudo obtener todas las variables, en caso de que no, mostrará un error de lectura.
  if(isnan(voltage)){
      Serial.println("Error reading voltage");
  } else if (isnan(current)) {
      Serial.println("Error reading current");
  } else if (isnan(power)) {
      Serial.println("Error reading power");
  } else if (isnan(energy)) {
      Serial.println("Error reading energy");
  } else if (isnan(frequency)) {
      Serial.println("Error reading frequency");
  } else if (isnan(pf)) {
      Serial.println("Error reading power factor");
  } 
  else {

    // Impresión de las variables
    Serial.print("Voltage: ");           Serial.print(voltage);             Serial.println("V");
    Serial.print("Current: ");           Serial.print(current);             Serial.println("A");
    Serial.print("Power: ");             Serial.print(power);               Serial.println("W");
    Serial.print("Energy: ");            Serial.print(energy,3);            Serial.println("kWh");
    Serial.print("Frequency: ");         Serial.print(frequency, 1);        Serial.println("Hz");
    Serial.print("PF: ");                Serial.println(pf);
    Serial.print("Reactive power: ");    Serial.print(pot_reactiva, 1);     Serial.println("VAr");
    Serial.print("Apparent power: ");    Serial.print(pot_aparente, 1);     Serial.println("VA");

    // Se almacenan las variables en un objeto JSON de forma llave-valor.
    myjson["Voltage"] = voltage;
    myjson["Current"] = current;
    myjson["Power"] = power;
    myjson["Energy"] = energy;
    myjson["Frequency"] = frequency;
    myjson["PF"] = pf;
    myjson["Reactive"] = pot_reactiva;
    myjson["Aparent"] = pot_aparente;

  }

  serializeJson(myjson, jsonSerial); // Se serializa (conversión a string) el contenido del JSON para facilitar su transmisión.
  Serial.println();
  Serial.println(jsonSerial);
  Serial.println();

  // Conversión de string a char para realizar la publicación al tópico MQTT.
  char avr[jsonSerial.length()+1];
  jsonSerial.toCharArray(avr, jsonSerial.length()+1);

  client.loop(); // Revisa constantemente si hay nuevos mensajes.
  client.publish(topic, avr); // Se publica el JSON serializado al tópico MQTT.
  delay(500);
}
