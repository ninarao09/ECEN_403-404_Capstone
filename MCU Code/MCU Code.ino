//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial
// Modified by ECEN 404-Team 47-Jason Dimelow, Nina Roa, Annie Rizvi...
#include <Wire.h>
#include <BluetoothSerial.h>
//#include <BTAddress.h>
//#include <BTAdvertisedDevice.h>
//#include <BTScan.h>
#include <Dictionary.h>
#include "esp_log.h"
#include "BluetoothSerial.h"
#include <SparkFunBQ27441.h>


#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define _DICT_CRC 32
#define I2C_SDA         21
#define I2C_SCL         22
#define I2C_MASTER_NUM  0 
#define I2C_FREQ        100000
#define I2C_TX_BUFF_DISABLE 0
#define I2C_RX_BUFF_DISABLE 0
#define I2C_TIMEOUT_MS      1000
#define I2C_SLAVE_ADDR      0x48


BluetoothSerial SerialBT;
Dictionary *notes = new Dictionary();
Dictionary *chords = new Dictionary();

int received_input = 0;
int newBatteryLevel = 100;

// Set BATTERY_CAPACITY to the design capacity of your battery.
const unsigned int BATTERY_CAPACITY = 1500; // e.g. 850mAh battery

const byte SOCI_SET = 15; // Interrupt set threshold at 20%
const byte SOCI_CLR = 20; // Interrupt clear threshold at 25%
const byte SOCF_SET = 5; // Final threshold set at 5%
const byte SOCF_CLR = 10; // Final threshold clear at 10%

// Arduino pin connected to BQ27441's GPOUT pin
const int GPOUT_PIN = 2;

void setupBQ27441(void)
{
  pinMode(GPOUT_PIN, INPUT_PULLUP); // Set the GPOUT pin as an input w/ pullup
  
  // Use lipo.begin() to initialize the BQ27441-G1A and confirm that it's
  // connected and communicating.
  if (!lipo.begin()) // begin() will return true if communication is successful
  {
  // If communication fails, print an error message and loop forever.
    Serial.println("Error: Unable to communicate with BQ27441.");
    Serial.println("  Check wiring and try again.");
    Serial.println("  (Battery must be plugged into Battery Babysitter!)");
    while (1) ;
  }
  Serial.println("Connected to BQ27441!");

  // In this example, we'll manually enter and exit config mode. By controlling
  // config mode manually, you can set the chip up faster -- completing all of
  // the set up in a single config mode sweep.
  lipo.enterConfig(); // To configure the values below, you must be in config mode
  lipo.setCapacity(BATTERY_CAPACITY); // Set the battery capacity
  lipo.setGPOUTPolarity(LOW); // Set GPOUT to active-high
  lipo.setGPOUTFunction(BAT_LOW); // Set GPOUT to BAT_LOW mode
  lipo.setSOC1Thresholds(SOCI_SET, SOCI_CLR); // Set SOCI set and clear thresholds
  lipo.setSOCFThresholds(SOCF_SET, SOCF_CLR); // Set SOCF set and clear thresholds
  lipo.exitConfig();

  // Use lipo.GPOUTPolarity to read from the chip and confirm the changes
  if (lipo.GPOUTPolarity())
    Serial.println("GPOUT set to active-HIGH");
  else
    Serial.println("GPOUT set to active-LOW");

  // Use lipo.GPOUTFunction to confirm the functionality of GPOUT
  if (lipo.GPOUTFunction())
    Serial.println("GPOUT function set to BAT_LOW");
  else
    Serial.println("GPOUT function set to SOC_INT");

  // Read the set and clear thresholds to make sure they were set correctly
  Serial.println("SOC1 Set Threshold: " + String(lipo.SOC1SetThreshold()));
  Serial.println("SOC1 Clear Threshold: " + String(lipo.SOC1ClearThreshold()));
  Serial.println("SOCF Set Threshold: " + String(lipo.SOCFSetThreshold()));
  Serial.println("SOCF Clear Threshold: " + String(lipo.SOCFClearThreshold()));
}

void printBatteryStats()
{
  // Read battery stats from the BQ27441-G1A
  unsigned int soc = lipo.soc();  // Read state-of-charge (%)
  unsigned int volts = lipo.voltage(); // Read battery voltage (mV)
  int current = lipo.current(AVG); // Read average current (mA)
  unsigned int fullCapacity = lipo.capacity(FULL); // Read full capacity (mAh)
  unsigned int capacity = lipo.capacity(REMAIN); // Read remaining capacity (mAh)
  int power = lipo.power(); // Read average power draw (mW)
  int health = lipo.soh(); // Read state-of-health (%)

  // Assemble a string to print
  double ratio = (double)volts/4200;
  ratio= ratio*100.0;
  newBatteryLevel = (int)ratio;
  String toPrint = String(soc) + "% | ";
  toPrint += String(volts) + " mV | ";
  toPrint += String(current) + " mA | ";
  toPrint += String(capacity) + " / ";
  toPrint += String(fullCapacity) + " mAh | ";
  toPrint += String(power) + " mW | ";
  toPrint += String(health) + "%";
  toPrint += " | 0x" + String(lipo.flags(), HEX);
  
  // Print the string
  Serial.println(toPrint);
   Serial.println(newBatteryLevel);

}




void setup() {
  Serial.begin(115200);
  setupBQ27441();
  SerialBT.begin("AudioHarmonizer"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
  notes->insert("C", "0");
  notes->insert("C#/Eb", "1");
  notes->insert("D", "2");
  notes->insert("D#/Eb", "3");
  notes->insert("E", "4");
  notes->insert("F", "5");
  notes->insert("F#/Gb", "6");
  notes->insert("G", "7");
  notes->insert("G#/Ab", "8");
  notes->insert("A", "9");
  notes->insert("A#/Bb", "10");
  notes->insert("B", "11");
  notes->insert("n/a", "12");

  chords->insert("Cmaj", "0");
  chords->insert("Cmin", "1");
  chords->insert("C#/Dbmaj", "2");
  chords->insert("C#/Dbmin", "3");
  chords->insert("Dmaj", "4");
  chords->insert("Dmin", "5");
  chords->insert("D#/Ebmaj", "6");
  chords->insert("D#/Ebmin", "7");
  chords->insert("Emaj", "8");
  chords->insert("Emin", "9");
  chords->insert("Fmaj" ,"10");
  chords->insert("Fmin", "11");
  chords->insert("F#/Gbmaj", "12");
  chords->insert("F#/Gbmin", "13");
  chords->insert("Gmaj", "14");
  chords->insert("Gmin", "15");
  chords->insert("G#/Abmaj", "16");
  chords->insert("G#/Abmin", "17");
  chords->insert("Amaj", "18");
  chords->insert("Amin", "19");
  chords->insert("A#/Bbmaj", "20");
  chords->insert("A#/Bbmin", "21");
  chords->insert("Bmaj", "22");
  chords->insert("Bmin", "23");
  chords->insert("n/a", "24");

  Wire.begin();
  
  /*
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = GPIO_NUM_21;
  conf.scl_io_num = GPIO_NUM_22;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = I2C_FREQ;
  i2c_param_config(I2C_NUM_0, &conf);
  esp_err_t install = i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
  */

  
}

void loop() { 

  printBatteryStats();
  
  // If the GPOUT interrupt is active (low)
  if (!digitalRead(GPOUT_PIN))
  {
    // Check which of the flags triggered the interrupt
    if (lipo.socfFlag()) 
      Serial.println("<!-- WARNING: Battery Dangerously low -->");
    else if (lipo.socFlag())
      Serial.println("<!-- WARNING: Battery Low -->");
  }
  
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }
    
  if (SerialBT.available()) {
    //Serial.println(SerialBT.readString());
    //Serial.println("");
    
    String input = SerialBT.readString();
    if(received_input == 0)
    {
      Wire.begin();
      received_input = 1;
      Serial.println("recieving input");
      char buf[input.length()];
      input.toCharArray(buf, sizeof(buf));
      Serial.println(buf);
      char *p = buf;
      char *str;
      int index=0;
      String introInput[4];
      while (index < 4 && (str = strtok_r(p, ";", &p)) != NULL ){
        introInput[index]=str;
        index++;
        //Serial.println(str);
      }
      Serial.println(p);
      uint8_t data = 0;
      uint8_t beats_per_measure = introInput[1].toInt(); 
      uint8_t beats_per_min = introInput[2].toInt();
  
      data = beats_per_measure;
      sendDataOverIC(data);
      data = beats_per_min;
      sendDataOverIC(data);

      if(introInput[3] == "Automatic"){
        Serial.println("In automatic mode");
        data = 1;
        sendDataOverIC(data);
        String autoInput[5];
        int autoIndex = 0;
        char *str;
        while ((str = strtok_r(p, ";", &p)) != NULL){
          autoInput[autoIndex]=str;
          autoIndex++;
        }
        uint8_t num_chords = 0;
        for(int i=1; i<5;i++)
        {
          Serial.println(autoInput[i]);
          if(autoInput[i] != "n/a")
          {
            num_chords++;
          }
        }
        data = (uint8_t)autoInput[0].toInt(); //number of harmonies
        sendDataOverIC(data);
        data = num_chords;
        sendDataOverIC(data);
        for(int i=1; i<5; i++)
        {
          if(autoInput[i] != "n/a")
          {
            String chord_num = chords->search(autoInput[i]);
            uint8_t ch = (uint8_t)chord_num.toInt();
            data = ch;
            sendDataOverIC(data);
          }
        }
        
        //Serial.println(str); 
      }
      else{
        Serial.println("In manual mode");
        data = 0;
        sendDataOverIC(data);
        String mlength = String(strtok_r(p, ";", &p)); // 3*notes*harmonies
        String harmonies = String(strtok_r(p, ";", &p));
        Serial.println(harmonies);
        String noteNumber = String(strtok_r(p, ";", &p));
        String manualInput[mlength.toInt()];
        int manualIndex=0;
        char *str;
        Serial.println("about to parse");
        while ((str = strtok_r(p, ";", &p)) != NULL){
          manualInput[manualIndex]=str;
          manualIndex++;
          //Serial.println(manualIndex);
        }
  
        for(int i=0;i<mlength.toInt();i++)
        {
          Serial.println(manualInput[i]);
        }
  
        int note_num = mlength.toInt()/(3*harmonies.toInt());
        uint8_t h_notes[harmonies.toInt()][note_num];
        uint8_t octaves[harmonies.toInt()][note_num];
        double lengths[harmonies.toInt()][note_num];
  
        uint8_t notes_h2[note_num];
        int count = 0;
        for(int i=0; i<harmonies.toInt(); i++)
        {
          for(int j=0; j<note_num; j++)
          {
            String note_num = notes->search(manualInput[count]);
            int note = note_num.toInt();
            h_notes[i][j] = note;
            count++;
            octaves[i][j] = manualInput[count].toInt();
            count++;
            lengths[i][j] = manualInput[count].toDouble();
            count++;        
          }
        }
  
        data = mlength.toInt();
        sendDataOverIC(data);
        data = harmonies.toInt();
        sendDataOverIC(data);
        data = note_num;
        sendDataOverIC(data);
        
        for(int i=0; i<harmonies.toInt(); i++)
        {
          for(int j=0; j<note_num; j++)
          {
            data = h_notes[i][j];
            sendDataOverIC(data);
            data = octaves[i][j];
            sendDataOverIC(data);
            data = (int)(lengths[i][j]*2);
            sendDataOverIC(data);
          }
        }
      } 
    }
    else
    {
      Serial.println(input);
      if(input.indexOf("start") != -1)
      {
        Serial.println("starting");
        uint8_t data = {1};
        sendDataOverIC(data);
        
      }
      else if (input.indexOf("stop") != -1)
      {
        uint8_t data = {0};
        sendDataOverIC(data);
        
      }
      Serial.println("shouldn't crash");
    }
    
    
    
    
     
  }
  // simulate battery level testing 
  int batteryLevel = newBatteryLevel;
  Serial.println(batteryLevel);
  int oldBatteryLevel = 90;
  batteryMonitor(batteryLevel, oldBatteryLevel);
  // testing code for battery fuel guage 
//  int batteryLevel = 57;
//  int oldBatteryLevel = 0;
//  for(int i = 0; i < 1; i++) {
//    oldBatteryLevel = batteryLevel;
//    batteryLevel = random(0, 100);
//    batteryMonitor(batteryLevel, oldBatteryLevel);
//    //Serial.println(oldBatteryLevel);
//    Serial.println(batteryLevel);
//  }

  delay(500);
}

 void batteryMonitor(int batteryLevel, int oldBatteryLEvel) {
   if(batteryLevel != oldBatteryLEvel){
    int splitArray[3] = {0, 0, 0};
    splitInteger(splitArray, batteryLevel);
    convertAscii(splitArray);
  }
}

void splitInteger(int splitArray[3], int batteryLevel){
  for(int i=2; i>=0; i--){
    splitArray[i] = (batteryLevel%10);
    batteryLevel /= 10;
  }
}

void convertAscii(int splitArray[3]){
  int batteryLevel[3];
  // convert to decimal
  for(int i=0; i<3; i++){
    if(splitArray[i] == 0){
      batteryLevel[i] = 48;
    }else if(splitArray[i] == 1){
      batteryLevel[i] = 49;
    }else if(splitArray[i] == 2){
      batteryLevel[i] = 50;
    }else if(splitArray[i] == 3){
      batteryLevel[i] = 51;
    }else if(splitArray[i] == 4){
      batteryLevel[i] = 52;
    }else if(splitArray[i] == 5){
      batteryLevel[i] = 53;
    }else if(splitArray[i] == 6){
      batteryLevel[i] = 54;
    }else if(splitArray[i] == 7){
      batteryLevel[i] = 55;
    }else if(splitArray[i] == 8){
      batteryLevel[i] = 56;
    }else if(splitArray[i] == 9){
      batteryLevel[i] = 57;
    }
  }

  
  //write to terminal
  //SerialBT.write('Battery Level:');
  for(int i=0; i<3; i++){
    SerialBT.write(batteryLevel[i]);
    
    delay(500);
    
  }
  SerialBT.write(100);
}

void sendDataOverIC(uint8_t data)
{
    Wire.beginTransmission(I2C_SLAVE_ADDR);
    Wire.write(data);
    Wire.endTransmission();
    Serial.println("sending data");
  
}
