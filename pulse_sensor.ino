#define pulsePin A0
#include <ESP8266WiFi.h>
#include "FirebaseESP8266.h"
#define FIREBASE_HOST "beat-beat-beep.firebaseio.com"
#define FIREBASE_AUTH "TY1aDav6Cwhvdbq9aVgS18yv6a8Z85p39G40Wp9b"
const char* ssid = "UIT Public";
const char* password = "";
FirebaseData firebaseData;
int rate[10];
unsigned long sampleCounter = 0;
unsigned long lastBeatTime = 0;
unsigned long lastTime = 0, N;
int BPM = 0;
int IBI = 0;
int P = 512;
int T = 512;
int thresh = 550;
int amp = 100;
int Signal;
boolean Pulse = false;
boolean firstBeat = true;
boolean secondBeat = true;
boolean QS = false;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Wfif connected!");
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Serial.println("Firebase connected");
  delay(10);
  Firebase.reconnectWiFi(true);
}

void loop() {

  if (QS == true) {
    Serial.println("BPM: " + String(BPM));
    UploadtoFirebase(BPM);
    QS = false;
  } else if (millis() >= (lastTime + 2)) {
    readPulse();
    lastTime = millis();
  }
}

void readPulse() {

  Signal = analogRead(pulsePin);
  sampleCounter += 2;
  int N = sampleCounter - lastBeatTime;

  detectSetHighLow();

  if (N > 250) {
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI / 5) * 3) )
      pulseDetected();
  }

  if (Signal < thresh && Pulse == true) {
    Pulse = false;
    amp = P - T;
    thresh = amp / 2 + T;
    P = thresh;
    T = thresh;
  }

  if (N > 2500) {
    thresh = 512;
    P = 512;
    T = 512;
    lastBeatTime = sampleCounter;
    firstBeat = true;
    secondBeat = true;
  }

}

void detectSetHighLow() {

  if (Signal < thresh && N > (IBI / 5) * 3) {
    if (Signal < T) {
      T = Signal;
    }
  }

  if (Signal > thresh && Signal > P) {
    P = Signal;
  }

}

void pulseDetected() {
  Pulse = true;
  IBI = sampleCounter - lastBeatTime;
  lastBeatTime = sampleCounter;

  if (firstBeat) {
    firstBeat = false;
    return;
  }
  if (secondBeat) {
    secondBeat = false;
    for (int i = 0; i <= 9; i++) {
      rate[i] = IBI;
    }
  }

  word runningTotal = 0;

  for (int i = 0; i <= 8; i++) {
    rate[i] = rate[i + 1];
    runningTotal += rate[i];
  }

  rate[9] = IBI;
  runningTotal += rate[9];
  runningTotal /= 10;
  BPM = 60000 / runningTotal;
  QS = true;
}
void UploadtoFirebase(int &bpm)
{
  if (Firebase.pushInt(firebaseData, "/bpm", bpm))
  {
    //Success
    Serial.println("Data uploaded");

  } else {
    //Failed?, get the error reason from firebaseData

    Serial.print("Error while uploading, ");
    Serial.println(firebaseData.errorReason());
  }


}
