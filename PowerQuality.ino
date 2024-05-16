#include <PZEM004Tv30.h>

#if defined(ESP32)
PZEM004Tv30 pzem(Serial2, 16, 17); // RX2, TX2
#else
PZEM004Tv30 pzem(Serial2);
#endif

void setup() {
    Serial.begin(115200);
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
    } else {

        // Impresión de las variables
        Serial.print("Voltage: ");      Serial.print(voltage);      Serial.println("V");
        Serial.print("Current: ");      Serial.print(current);      Serial.println("A");
        Serial.print("Power: ");        Serial.print(power);        Serial.println("W");
        Serial.print("Energy: ");       Serial.print(energy,3);     Serial.println("kWh");
        Serial.print("Frequency: ");    Serial.print(frequency, 1); Serial.println("Hz");
        Serial.print("PF: ");           Serial.println(pf);

    }

    Serial.println();
    delay(2000); //Espera de 2 segundos para la siguiente lectura.
}
