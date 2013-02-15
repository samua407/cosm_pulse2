#include <SPI.h>
#include <Ethernet.h>
#include <HttpClient.h>
#include <Cosm.h>

/*
>> Pulse Sensor Amped 1.1 <<
 This code is for Pulse Sensor Amped by Joel Murphy and Yury Gitman
 www.pulsesensor.com 
 >>> Pulse Sensor purple wire goes to Analog Pin 0 <<<
 Pulse Sensor sample aquisition and processing happens in the background via Timer 2 interrupt. 2mS sample rate.
 PWM on pins 3 and 11 will not work when using this code, because we are using Timer 2!
 The following variables are automatically updated:
 Signal :    int that holds the analog signal data straight from the sensor. updated every 2mS.
 IBI  :      int that holds the time interval between beats. 2mS resolution.
 BPM  :      int that holds the heart rate value, derived every beat, from averaging previous 10 IBI values.
 QS  :       boolean that is made true whenever Pulse is found and BPM is updated. User must reset.
 Pulse :     boolean that is true when a heartbeat is sensed then false in time with pin13 LED going out.
 
 This code is designed with output serial data to Processing sketch "PulseSensorAmped_Processing-xx"
 The Processing sketch is a simple data visualizer. 
 All the work to find the heartbeat and determine the heartrate happens in the code below.
 Pin 13 LED will blink with heartbeat.
 If you want to use pin 13 for something else, adjust the interrupt handler
 It will also fade an LED on pin fadePin with every beat. Put an LED and series resistor from fadePin to GND.
 Check here for detailed code walkthrough:
 http://pulsesensor.myshopify.com/pages/pulse-sensor-amped-arduino-v1dot1
 
 Code Version 02 by Joel Murphy & Yury Gitman  Fall 2012
 This update changes the HRV variable name to IBI, which stands for Inter-Beat Interval, for clarity.
 Switched the interrupt to Timer2.  500Hz sample rate, 2mS resolution IBI value.
 Fade LED pin moved to pin 5 (use of Timer2 disables PWM on pins 3 & 11).
 Tidied up inefficiencies since the last version. 
 */


//  VARIABLES
int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 9;                // pin to blink led at each beat
int fadePin = 5;                  // pin to do fancy classy fading blink at each beat
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin
int numberOfBeats = 0;
int averageBPM = 0;
int averageNumber = 10;
int edCounter = 0;
long previousUploadTime = 0;
long uploadInterval = 5000;

// these variables are volatile because they are used during the interrupt service routine!
volatile int BPM;                   // used to hold the pulse rate
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // holds the time between beats, the Inter-Beat Interval
volatile boolean Pulse = false;     // true when pulse wave is high, false when it's low
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

// MAC address for your Ethernet shield
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x0D, 0x94, 0xA3 };

// Your Cosm key to let you upload data
char cosmKey[] = "3Icd6QSp5qc6dLfNHsezRpobxyaSAKxKeUFjT1h4K3Ywdz0g";

// Analog pin which we're monitoring (0 and 1 are used by the Ethernet shield)
int sensorPin = 2;

// Define the strings for our datastream IDs
char sensorId[] = "group_four";
CosmDatastream datastreams[] = {
  CosmDatastream(sensorId, strlen(sensorId), DATASTREAM_INT),
};
// Finally, wrap the datastreams into a feed
CosmFeed feed(104792, datastreams, 1 /* number of datastreams */);

EthernetClient client;
CosmClient cosmclient(client);

void setup() {
  // put your setup code here, to run once:
  pinMode(blinkPin,OUTPUT);         // pin that will blink to your heartbeat!
  pinMode(fadePin,OUTPUT);          // pin that will fade to your heartbeat!
  Serial.begin(115200);             // we agree to talk fast!
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 

  Serial.println("Starting single datastream upload to Cosm...");
  Serial.println();

  while (Ethernet.begin(mac) != 1)
  {
    Serial.println("Error getting IP address via DHCP, trying again...");
    delay(15000);
  }
}

void loop() {
  unsigned long currentMillis = millis();
  //sendDataToProcessing('S', Signal);     // send Processing the raw Pulse Sensor data
  if (QS == true){                       // Quantified Self flag is true when arduino finds a heartbeat
    fadeRate = 255;                  // Set 'fadeRate' Variable to 255 to fade LED with pulse
    //sendDataToProcessing('B',BPM);   // send heart rate with a 'B' prefix
    //sendDataToProcessing('Q',IBI);   // send time between beats with a 'Q' prefix
    //numberOfBeats++;
    //averageBPM += BPM;
   // QS = false;                      // reset the Quantified Self flag for next time    
  }
 // if(currentMillis - previousUploadTime > uploadInterval) {
   if(QS == true){    //int lastAverage = averageBPM/numberOfBeats;
    datastreams[0].setInt(edCounter);
    Serial.println("Uploading it to Cosm");
    int ret = cosmclient.put(feed, cosmKey);
    Serial.print("cosmclient.put returned ");
    Serial.println(ret);
    
    Serial.println();
    previousUploadTime = millis();
    edCounter++;
  }

  //ledFadeToBeat();

  delay(20);                             //  take a break

  //  int sensorValue = analogRead(sensorPin);
  //  datastreams[0].setFloat(sensorValue);
  //
  //  Serial.print("Read sensor value ");
  //  Serial.println(datastreams[0].getFloat());
  //
  //  Serial.println("Uploading it to Cosm");
  //  int ret = cosmclient.put(feed, cosmKey);
  //  Serial.print("cosmclient.put returned ");
  //  Serial.println(ret);
  //
  //  Serial.println();
  //  delay(15000);
}
void ledFadeToBeat(){
  fadeRate -= 15;                         //  set LED fade value
  fadeRate = constrain(fadeRate,0,255);   //  keep LED fade value from going into negative numbers!
  analogWrite(fadePin,fadeRate);          //  fade LED
}


void sendDataToProcessing(char symbol, int data ){
  Serial.print(symbol);                // symbol prefix tells Processing what type of data is coming
  Serial.println(data);                // the data to send culminating in a carriage return
}

