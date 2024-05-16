#include <PZEM004Tv30.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

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

// Variables para extraer datos de fecha y tiempo.
String date;
String Time;
String dateTime;


#if defined(ESP32)
PZEM004Tv30 pzem(Serial2, 16, 17); // RX2, TX2
#else
PZEM004Tv30 pzem(Serial2);
#endif

// Variables para el manejo de formato JSON.
String jsonSerial = "";
JsonDocument myjson; //Se genera un objeto JSON.
char msg;

unsigned long previousTime = 0; // Variable para contar segundos.

// Clientes para publicación a MQTT.
WiFiClient espClient;
PubSubClient client(espClient);

// Objetos para acceder al servidor NTP y obtener la fecha y hora.
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

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
  timeClient.begin(); // Se da inicio al cliente NTP.
  timeClient.setTimeOffset(-18000); // Se establece un desfase -18000 segundos para sincronizar con la ubicación geográfica de Colombia.
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

  unsigned long currentTime = millis(); // Se obtienen los segundos que han pasado.

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

  impresionSerial(voltage, current, power, energy, pf, frequency, pot_aparente, pot_reactiva); // Impresión de las variables.
  crearJson(voltage, current, power, energy, pf, frequency, pot_aparente, pot_reactiva); // Creación del JSON.
  

  serializeJson(myjson, jsonSerial); // Se serializa (conversión a string) el contenido del JSON para facilitar su transmisión.

  // Conversión de string a char para realizar la publicación al tópico MQTT.
  char json_serial[jsonSerial.length()+1];
  jsonSerial.toCharArray(json_serial, jsonSerial.length()+1);

  // Cada 10 segundos, se obtiene la fecha y se imprime el CSV.
  if(currentTime - previousTime >= 10000){
    previousTime = currentTime;

    dateTime = getDatetime();
    serialCSV(voltage, current, power, energy, pf, frequency, pot_aparente, pot_reactiva, dateTime);
  }

  client.loop(); // Revisa constantemente si hay nuevos mensajes.
  client.publish(topic, json_serial); // Se publica el JSON serializado al tópico MQTT.

  delay(1000);
}

// Función para imprimir las variables eléctricas en el monitor serial.
void impresionSerial(float voltage, float current, float power, float energy, float pf, float frequency, float pot_aparente, float pot_reactiva){

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
  }
}

// Función para realizar el empaquetado JSON.
void crearJson(float voltage, float current, float power, float energy, float pf, float frequency, float pot_aparente, float pot_reactiva){

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

// Función para extraer la fecha y hora actual.
String getDatetime(){
  timeClient.update(); // Se actualiza el cliente al momento actual.
  
  date = timeClient.getFormattedDate(); // Se extrae la fecha.
  Time = timeClient.getFormattedTime(); // Se extrae la hora.
  int split = date.indexOf("T"); // Se obtiene el índice donde termina la fecha.

  date = date.substring(0, split); // Se extrae únicamente la fecha en formato YYYY-MM-DD
  
  String datetime = date + " " + Time; // Se concatena la fecha y hora, obtieniendo un datetime con formato YYYY-MM-DD HH-MM-SS
  
  return datetime;
}

// Función para una impresión serial de las variables en un formato CSV, esto para realizar la extracción y almacenamiento a un archivo de texto.
void serialCSV(float voltage, float current, float power, float energy, float pf, float frequency, float pot_aparente, float pot_reactiva, String date){
  Serial.print(date);
  Serial.print(",");
  Serial.print(voltage);
  Serial.print(",");
  Serial.print(current);
  Serial.print(",");
  Serial.print(power);
  Serial.print(",");
  Serial.print(energy);
  Serial.print(",");
  Serial.print(frequency);
  Serial.print(",");
  Serial.print(pf);
  Serial.print(",");
  Serial.print(pot_reactiva);
  Serial.print(",");
  Serial.print(pot_aparente);
  Serial.print("\n");
}
