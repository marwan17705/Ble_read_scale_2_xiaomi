/**
 * An ESP32 BLE client to retrieve data from the Weight Measurement characteristic  
 * of a Xiaomi Mi Smart weight scale
 * Author Pangodream
 * Date 2020.05.31
 */

#include "BLEDevice.h"
//Base UUIDs
//Weight Scale service
static BLEUUID srvUUID("0000181b-0000-1000-8000-00805f9b34fb");
//Weight Measurement characteristic
static BLEUUID chrUUID("00002a9c-0000-1000-8000-00805f9b34fb");
//static BLEUUID chrUUID("00002a2f-0000-3512-2118-0009af100700");
static bool notification = false;

static BLEAdvertisedDevice* scale;
static BLERemoteCharacteristic* remoteChr;
static boolean doConnect = false;
static boolean connected = false;
boolean BIA = false;
/**
 * Callback function for characteristic notify / indication
 */
static void chrCB(BLERemoteCharacteristic* remoteChr, uint8_t* pData, size_t length, bool isNotify) {
    //Console debugging
    Serial.print("Received data. Length = ");
    Serial.print(length);
    Serial.print(". - Data bytes: ");
    for(int i =0; i< length; i++){
      Serial.print(pData[i]);
      Serial.print("  ");
    }
    Serial.println(" ");
    //End of console debugging
    double weight = 0;
    weight = (pData[11] + pData[12] * 256) * 0.005;
    if (!BIA)
    { 
      Serial.printf("date time %d:%d:%d %d/%d/%d Weight: %f Kg (provisional) \n",pData[7],pData[6],pData[8],pData[5],pData[4],(pData[3]*256)+pData[2],weight);
    }
    else
    { 
      uint16_t BIA_wieght = 0 ;
      BIA_wieght = (pData[19] + pData[10] * 256);
      Serial.println("-----------Result-----------");
      Serial.printf("date time %d:%d:%d %d/%d/%d Weight: %f Kg BMA_independence: %d \n",pData[7],pData[6],pData[8],pData[5],pData[4],
                                                                   (pData[3]*256)+pData[2],weight,BIA_wieght);
    }
    if (pData[9]==254 &&pData[10]==255)
    {
      BIA = true;
    }
    else 
    {
      BIA = false;
    }
}

/**
 * Callback class for each advertised device during scan
 */
class deviceCB: public BLEAdvertisedDeviceCallbacks {
 //Called on each advertised device
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(srvUUID)) {
      if(advertisedDevice.getName() != "MIBFS"){
        Serial.print(".");
      } else {
        Serial.println("  Found!");
        BLEDevice::getScan()->stop();
        Serial.println("Stopping scan and connecting to scale");
        scale = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
      }
    } else {
      Serial.print(".");
    }
  } 
};
/**
 * Callback class for device events
 */
class ClientCB : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {

  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("Disconnected. Reconnecting...");
    connected = false;
  }
};

bool connectToScale() {
    Serial.println("Stablishing communications with scale:");
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println("    BLE client created");

    pClient->setClientCallbacks(new ClientCB());

    // Connect to the remove BLE Server.
    pClient->connect(scale);
    Serial.println("    Connected to scale");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(srvUUID);
    if (pRemoteService == nullptr) {
      Serial.println("    Error: Failed to find service");
      pClient->disconnect();
      return false;
    }
    Serial.println("    Service found");

    remoteChr = pRemoteService->getCharacteristic(chrUUID);
    if (remoteChr == nullptr) {
      Serial.print("    Failed to find characteristic");
      pClient->disconnect();
      return false;
    }
    Serial.println("    Characteristic found");
    Serial.println("    Setting callback for notify / indicate");
//    remoteChr->BLERemoteCharacteristic.charProp = remoteChr->BLERemoteCharacteristic.charProp|0x10;
    const uint8_t bothOff[]       = {0x0, 0x0};
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t indicationOn[]   = {0x2, 0x0};
const uint8_t bothOn[]         = {0x3, 0x0};
  remoteChr->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)bothOn, 2, true);
    
    remoteChr->registerForNotify(chrCB,true);

    return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Searching for MIBFS device");
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new deviceCB());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  //Set active scan
  pBLEScan->setActiveScan(true);
  //Scan during 5 seconds
  pBLEScan->start(5, false);
}
int value=0;
void loop() {
  if(doConnect && !connected){
    connected = connectToScale();
    
  }
  delay(1000);
}
