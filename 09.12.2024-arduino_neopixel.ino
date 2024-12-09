#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_NeoPixel.h>

// Configuration des LEDs NeoPixel
#define PIN_NEOPIXEL 18 // Choisissez une broche appropriée
#define NUMPIXELS 256   // Nombre de LEDs dans le ruban

Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
int animation_frame = 0;

// BLE variables
BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Variables de contrôle
std::string currentCommand = ""; // Commande active ("L", "R", "N", ou autre)
unsigned long lastActiveTime = 0;
const unsigned long autoOffInterval = 5000; // Temps d'inactivité avant arrêt (5 secondes)

// Fonction pour convertir une couleur hexadécimale (sous forme de chaîne) en uint32_t
uint32_t hexToColor(std::string hex) {
  if (hex.length() == 6) { // Ex: "FF0000" pour rouge
    long number = strtol(hex.c_str(), NULL, 16);
    return ((number >> 16) & 0xFF) << 16 | ((number >> 8) & 0xFF) << 8 | (number & 0xFF);
  }
  return 0;
}

// Effet de clignotement
void blinkEffect(int start, int end, uint32_t color, int cycles, int delayMs) {
  for (int k = 0; k < cycles; k++) {
    for (int i = start; i < end; i++) {
      pixels.setPixelColor(i, color); // Allumer la plage
    }
    pixels.show();
    delay(delayMs);
    for (int i = start; i < end; i++) {
      pixels.setPixelColor(i, 0); // Éteindre la plage
    }
    pixels.show();
    delay(delayMs);
  }
}

// Effet d'allumage progressif
void progressiveEffect(uint32_t color, int delayMs) {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, color); // Allumer progressivement
    pixels.show();
    delay(delayMs);
  }
}

// Callbacks BLE
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Device connected");
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      // Mise à jour de l'état actuel
      currentCommand = rxValue.substr(0, rxValue.find("#"));
      Serial.println(("New command received: " + currentCommand).c_str());
      lastActiveTime = millis(); // Réinitialisation du timer auto-off
    }
  }
};

void setup() {
  Serial.begin(115200);

  // Initialisation des LEDs NeoPixel
  pixels.begin();
  pixels.show(); // S'assurer que toutes les LEDs sont éteintes au départ

  // Initialisation du BLE
  BLEDevice::init("SafeDistanceDevice");

  // Création du serveur BLE
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Création du service BLE
  BLEService *pService = pServer->createService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");

  // Caractéristique TX
  pTxCharacteristic = pService->createCharacteristic(
      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E", BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  // Caractéristique RX
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E", BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Démarrer le service BLE
  pService->start();

  // Démarrer la publicité
  pServer->getAdvertising()->addServiceUUID(pService->getUUID());
  pServer->getAdvertising()->start();
  Serial.println("Waiting for a client connection...");
}

void setPixel(int row, int col, bool mirror){
  if (mirror) {
    if((row % 2) == 0){
      col = 15-col;
    }
  } else {
    if((row % 2) == 1){
      col = 15-col;
    }
  }
  
  pixels.setPixelColor(col + row * 16, pixels.Color(0, 190, 255));
}

void loop() {
  // Exécution des effets en fonction de la commande active
  /*if (currentCommand == "L") {
    // LEDs principales rouges avec clignotement vert
    pixels.clear();
    for (int i = 0; i < 96; i++) {
      pixels.setPixelColor(i, pixels.Color(120, 0, 0)); // Rouge
    }
    pixels.show();

    // Effet de clignotement progressif (96 à 128)
    blinkEffect(96, 128, pixels.Color(0, 120, 0), 1, 500);

  } else if (currentCommand == "R") {
    // LEDs principales bleues avec clignotement cyan
    pixels.clear();
    for (int i = 163; i < 256; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 120)); // Bleu
      
    }
    //setLED(0,0);
    //setLED(0,15);
    pixels.show();

    // Effet de clignotement progressif (225 à 256)
    blinkEffect(225, 256, pixels.Color(0, 120, 0), 1, 500);

  }/* else if (currentCommand == "N") {
    // Allumage progressif sur toute la bande
    progressiveEffect(pixels.Color(0, 255, 0), 50);

  }*/ /*else {
    // Pas de commande ou commande inconnue
    pixels.clear();
    pixels.show();
  }*/
  if (currentCommand == "NN") {
    pixels.clear(); 
    pixels.show();
    animation_frame = 0;
  } else if (currentCommand == "LN" || currentCommand == "RN"){
    bool is_right = false;
    if (currentCommand == "RN") {
      is_right = true;
    }
    animation_frame = animation_frame + 1;
    animation_frame = animation_frame % 16;
    pixels.clear();
    for (int j = 0; j < 8; j++){
      setPixel(j, (j+animation_frame)%16, is_right);
      setPixel(14-j, (j+animation_frame)%16, is_right);
      setPixel(j, (j+animation_frame + 8)%16, is_right);
      setPixel(14-j, (j+animation_frame + 8)%16, is_right);
    }
    pixels.show();
  }else if (currentCommand == "LB" || currentCommand == "RB"){
    bool is_right = false;
    if (currentCommand == "RB") {
      is_right = true;
    }
    animation_frame = animation_frame + 1;
    animation_frame = animation_frame % 16;
    pixels.clear();
    for (int j = 0; j < 8; j++){
      setPixel(j, (j+animation_frame)%16, is_right);
      setPixel(14-j, (j+animation_frame)%16, is_right);
      setPixel(j, (j+animation_frame + 8)%16, is_right);
      setPixel(14-j, (j+animation_frame + 8)%16, is_right);
    }
    for(int k = 0; k < 16; k++){
      pixels.setPixelColor(255-k, pixels.Color(120, 0, 0));
    }
    pixels.show();
  } else if (currentCommand == "NB") {
    pixels.clear(); 
    for(int k = 0; k < 16; k++){
      pixels.setPixelColor(255-k, pixels.Color(120, 0, 0));
    }
    pixels.show();
    animation_frame = 0;
  }
 

  

  /*pixels.clear();
  setPixel(0, 0, false);
  setPixel(0, 15, false);
  //pixels.setPixelColor(0, pixels.Color(0, 0, 120));
  pixels.show();
  delay(500);
  pixels.clear();
  setPixel(0, 0, true);
  setPixel(0, 15, true);
  //pixels.setPixelColor(0, pixels.Color(0, 0, 120));
  pixels.show();*/

  // Arrêt automatique après inactivité
  /*unsigned long currentTime = millis();
  if (currentTime - lastActiveTime > autoOffInterval) {
    pixels.clear();
    pixels.show();
    currentCommand = ""; // Réinitialiser l'état
  }*/

  // Gestion de la connexion BLE
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  } else if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Préparation de la pile Bluetooth
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }

  delay(50); // Petite pause pour éviter une boucle trop rapide
}

