#include <Adafruit_Fingerprint.h>

SoftwareSerial mySerial(2, 3); // TX/RX

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

volatile int finger_status = -1;
uint8_t id, option;
String result;

void setup()
{
  Serial.begin(9600);

  // set the data rate for the sensor serial port
  finger.begin(57600);

  // to test if there is any fingerprint sensor
  if (finger.verifyPassword()) {
    //Serial.println("Found fingerprint sensor!");
  } else {
    //Serial.println("ERROR: Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  // to verify how many fingerprints are stored in the sensor
  //finger.getTemplateCount();
  //Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
}

void loop()
{
  //Serial.println("------------------------------------------------------------------");
  //Serial.println("Send 1 to register a new fingerprint or 2 to verify a fingerprint");
  //Serial.println("------------------------------------------------------------------");
  result = "";
  option = readnumber();
  if(option == 1) { // 1 to Register a new fingerprint
    //Enroll mode
    mode_enrollFinger();
  }else if(option == 2){ //2 to Login and verify a fingerprint
    //Verify mode
    mode_verifyFinger();
  }else{
    //Serial.print("ERROR: option not valid");
  }
  Serial.println(result);
}

// AUXILIAR METHODS
// Main method to enroll a fingerprint - mode
void mode_enrollFinger(){
  //Serial.println("Ready to enroll a fingerprint!");
  //Serial.println("Please type in the ID # (from 1 to 127; 128 to exit) you want to save this finger as...");
  id = readnumber();
  if (id == 0 || id == 128) {// ID #0 not allowed, EXIT
     return;
  }
  //Serial.print("Enrolling ID #");
  //Serial.println(id);

  getFingerprintEnroll();
}

// Main method to verify a fingerprint - mode
void mode_verifyFinger(){
  Serial.println("STEP: Coloque su dedo para iniciar sesion");
  delay(5000); //Time for delay, otherwise, It wont detect any fingerprint and will go back to the menu
  finger_status = getFingerprintIDez();
  if (finger_status!=-1 and finger_status!=-2){
    //Serial.print("ID: ");
    //Serial.print(finger_status);
    //Serial.print("\n");
    result = "ID: " + String(finger_status);
    return;
  } else{
    result = "ERROR: Not Match";
  }
}

// Method to obtain the fingerprint image and store it
// Used in line 52
// If fails, it will return 'p' which is the status of what happened
uint8_t getFingerprintEnroll() {
  int p = -1;
  Serial.println("STEP: Ponga su dedo dentro de los siguientes 5 segundos");
  delay(5000); //Time for delay, otherwise, It wont detect any fingerprint and will go back to the menu
  p = finger.getImage();

  if(p != FINGERPRINT_NOFINGER){
    switch_captureFinger(p);

    // OK success!
    p = finger.image2Tz(1);
    switch_convertImage(p);

    Serial.println("STEP: Retire su dedo");
    delay(2000);
    p = 0;
    while (p != FINGERPRINT_NOFINGER) {
      p = finger.getImage();
    }
    //Serial.print("ID "); Serial.println(id);
    p = -1;
    Serial.println("STEP: Coloque nuevamente su dedo dentro de los siguientes 5 segundos");
    while (p != FINGERPRINT_OK) {
      p = finger.getImage();
      switch_captureFinger(p);
    }

    // OK success!
    p = finger.image2Tz(2);
    switch_convertImage(p);

    // OK converted!
    //Serial.print("Creating model for #");  Serial.println(id);

    p = finger.createModel();
    if (p == FINGERPRINT_OK) {
      //Serial.println("Prints matched!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      //Serial.println("ERROR: Communication error");
      result = "ERROR: Communication error, ";
      return p;
    } else if (p == FINGERPRINT_ENROLLMISMATCH) {
      //Serial.println("ERROR: Fingerprints did not match");
      result = "ERROR: Fingerprints did not match, ";
      return p;
    } else {
      //Serial.println("ERROR: Unknown error");
      result = "ERROR: Unknown error, ";
      return p;
    }

    // Store the fingerprint image
    //Serial.print("ID "); Serial.println(id);
    p = finger.storeModel(id);
    if (p == FINGERPRINT_OK) {
      //Serial.println("SUCCESS: Stored!");
      //Serial.print("ID: ");
      //Serial.print(id);
      //Serial.print("\n");
      result = "ID: " + String(id);
      return 1; //SUCCESS
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      //Serial.println("ERROR: Communication error");
      result = "ERROR: Communication error, ";
      return p;
    } else if (p == FINGERPRINT_BADLOCATION) {
      //Serial.println("ERROR: Could not store in that location");
      result = "ERROR: Could not store in that location, ";
      return p;
    } else if (p == FINGERPRINT_FLASHERR) {
      //Serial.println("ERROR: Error writing to flash");
      result = "ERROR: Error writing to flash, ";
      return p;
    } else {
      //Serial.println("ERROR: Unknown error");
      result = "ERROR: Unknown error, ";
      return p;
    }
  }else{
    //Serial.println("ERROR: No finger");
    result = "ERROR: No finger, ";
    return;
  }
}

// Method to verify if there is the fingerprint matches with one of the fingerprints stored in the sensor
// Used in line 57; returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();

  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -2;

  // found a match!
  //Serial.print("Found ID #"); Serial.print(finger.fingerID);
  //Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}

// Method to read a number from SerialPort
uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

// Method that contains a switch with possible end status while capturing the fingerprint image
// Used in lines 77 and 95
void switch_captureFinger(int p){
  switch (p) {
    case FINGERPRINT_OK:
      //Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("ERROR: Communication error");
      result = "ERROR: Communication error";
      break;
    case FINGERPRINT_IMAGEFAIL:
      //Serial.println("ERROR: Imaging error");
      result = "ERROR: Imaging error";
      break;
    default:
      //Serial.println("ERROR: Unknown error");
      result =  "ERROR: Unknown error";
      break;
    }
}

// Method that contains a switch with possible end status while converting the fingerprint image
// Used in lines 82 and 100
void switch_convertImage(int p){
  switch (p) {
    case FINGERPRINT_OK:
      //Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      //Serial.println("ERROR: Image too messy");
      result =  "ERROR: Image too messy";
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("ERROR: Communication error");
      result =  "ERROR: Communication error";
      return p;
    case FINGERPRINT_FEATUREFAIL:
      //Serial.println("ERROR: Could not find fingerprint features");
      result =  "ERROR: Could not find fingerprint features";
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      //Serial.println("ERROR: Could not find fingerprint features");
      result =  "ERROR: Could not find fingerprint features";
      return p;
    default:
      //Serial.println("ERROR: Unknown error");
      result =  "ERROR: Unknown error";
      return p;
  }
}
