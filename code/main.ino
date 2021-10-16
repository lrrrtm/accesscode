// Main executable file of the ACCCESCODE project – v.1.0
 
//----------------------------------------LIBRARIES----------------------------------------------------
  #include <Adafruit_Fingerprint.h>
  #include <SoftwareSerial.h>
  #include <Keypad.h>
  #include <Servo.h>
  #include <SPI.h>
  #include ”I2Cdev.h”
  #include ”MPU6050.h”
  #include ”RedMP3.h”
  #include <MFRC522.h>
 
//----------------------------------------DEFINES------------------------------------------------------
  #define mySerial Serial1
  #define TIME_OUT 20
  #define MP3_RX 38 // pin RX player
  #define MP3_TX 39 // pin TX player
  #define RST_PIN 5 // pin RST RFID scanner
  #define SS_PIN 53 // pin SS RFID scanner
  #define PIN_TRIG 31 // TRIG pin of the distance sensor
  #define PIN_ECHO 30 // ECHO pin of the distance sensor
  #define PIN_POWER_SUPPLY 4 // logic pin of the power supply relay
  #define PIN_LOCKS 2 //logic pin of the locks relay
  #define RESET_PIN 3
 
//----------------------------------------SOUNDS-------------------------------------------------------
 
//sounds
  #define BLOCK_SOUND 0x03 // lock sound
  #define UNLOCK_SOUND 0x03 // unlock sound
  #define TRUE_INPUT_SOUND 0x01 // sound of correct login (for RFID, Fingerpint)
  #define FALSE_INPUT_SOUND 0x05 // sound of incorrect login (for RFID, Fingerpint)
  #define KEYBOARD_SOUND 0x01 // sound of pressing the keyboard
  #define DESTRUCTION_SOUND 0x02 // self-destruction sound
  #define TRUE_INPUT_SOUND_2 0x06 // sound of correct login
  #define DOOR_SOUND 0x07 // very special sound
 
//volume
  #define BLOCK_VOLUME 0x1c // lock sound volume
  #define UNLOCK_VOLUME 0x1a // unlock sound volume
  #define TRUE_INPUT_VOLUME 0x1a // sound volume of correct login
  #define FALSE_INPUT_VOLUME 0x16 // sound volume of incorrect login
  #define KEYBOARD_VOLUME 0x16 // volume of the sound of pressing the keyboard
  #define DESTRUCTION_VOLUME 0x16 // self destruct sound volume
 
//time
  #define BLOCK_TIME 1500 // duration of the lock sound (мс)
  #define UNLOCK_TIME 1500 // duration of the unlock sound (мс)
  #define TRUE_INPUT_TIME 1500 // sound duration of correct login (мс)
  #define FALSE_INPUT_TIME 1000 // sound duration of incorrect login (мс)
  #define KEYBOARD_TIME 50 // duration of sound of pressing the keyboard (мс)
  #define DESTRUCTION_TIME 19000 // self-destruction sound duration (мс)
 
//----------------------------------------NOTIFICATIONS------------------------------------------------
 
  #define PHONE_NUMBER ”+79000000000”
  #define UNLOCK_NOTIFY “Your system was unlocked\nACCESSCODE” // message text for unlock process
  #define DOORS_NOTIFY “Side doors on your system were unlocked\nACCESSCODE” // message text for opening doors
  #define DESTRUCTION_NOTIFY “ !!! SELF-DESTRUCTION PROCESS WAS STARTED !!!\nACCESSCODE” // message text for destruction process
  #define WELCOME_NOTIFY “Your protection system is up\nACCESSCODE”
  #define SETUP_NOTIFY “Your protection system is turned on in setup mode: the side doors are open\nACCESSCODE”
 
//----------------------------------------CONSTANTS----------------------------------------------------
 
  String MASTER_PASSWORD = ”000000”; // will remove
 
  String RECIEVED_PASSWORD;
  String TRUE_RFID_ID_1 = “1111111111”; //ID of RFID carf
  String TRUE_RFID_ID_2 = ”0000000000”;
  String TRUE_PASSWORD_UNLOCK = “000000”; //sms login password
  String TRUE_PASSWORD_DESTRUCT = “333333”; //self-destruction password
  int COUNTER_LIMIT = 5; // delay in seconds for resetting the event counter
  int COUNTER_MAX_EVENTS = 5; // number of gyro position changes before starting self-destruction
  int MAX_LENGTH_SONAR = 10; // distance between distance sensor and side cover
 
  String SELF_DESTRUCTION_SMS = ”79000000000111111”; // +79000000000, 111111
  String UNLOCK_SMS = ”79000000000222222”; // +79000000000, 222222
  String LOCK_SMS = ”79000000000333333”; // +79000000000, 333333
 
//----------------------------------------OTHER_INITIALISATIONS----------------------------------------
 
  MPU6050 accgyro;
  Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
  Servo myservo;
  MFRC522 mfrc522(SS_PIN, RST_PIN); 
  MP3 mp3(MP3_RX, MP3_TX);
  unsigned long int t1;
  const byte ROWS = 4; // number of lines in the keyboard
  const byte COLS = 4; // the number of columns in the keyboard
  char keys[ROWS][COLS] = {
    {‘1’,’2’,’3’,’A’},
    {‘4’,’5’,’6’,’B’},
    {‘7’,’8’,’9’,’C’},
    {‘*’,’0’,’#’,’D’}
    };
  byte rowPins[ROWS] = {9,8,7,6}; // rows connection pins
  byte colPins[COLS] = {13,12,11,10}; // collumns connection pins
  Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
 
  int pos = 0; // home position of the servo in degrees
 
  long DURATION_SONAR, LENGTH_SONAR; 
  int low_x, low_y, low_z;
  int high_x, high_y, high_z;
  int COUNTER_EVENTS = 0;
  int COUNTER = 0;
  int fingerIDS = 0;
 
  char key;
  String notify;
 
  String messageCheck;
 
//----------------------------------------BOOLS--------------------------------------------------------
 
  bool isProtectionOn = true; // current position of the protection function
  bool isRFIDChecked = false; // RFID tag check position
  bool isFingerChecked = false; // scanned print check position
  bool isPinSent = false; // verification of sending a message with an unlock code
 
//-----------------------------------------------------------------------------------------------------
 
void rfidInitialization(){ // RFID module initialization
  while (!Serial);    
  SPI.begin();
  mfrc522.PCD_Init();
  delay(4);
  mfrc522.PCD_DumpVersionToSerial();
}
 
void playSound(int8_t index, int8_t volume, int delayTime){ // music playback (sound index, volume, duration)
  delay(100);
  mp3.playWithVolume(index,volume);
  delay(delayTime);
}
 
//-----------------------------------------------------------------------------------------------------
void setup(){
  digitalWrite(RESET_PIN, HIGH);
  pinMode(RESET_PIN, OUTPUT);
 
  Serial.begin(9600);
  Serial2.begin(9600);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  long int t = millis();
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  DURATION_SONAR = pulseIn(PIN_ECHO, HIGH);
  LENGTH_SONAR = (DURATION_SONAR / 2) / 29.1;
 
 
  if (LENGTH_SONAR > 3){
    playSound(TRUE_INPUT_SOUND_2, TRUE_INPUT_VOLUME, TRUE_INPUT_TIME);
    sendSms(SETUP_NOTIFY);
  while (key != ‘C’){
     key = keypad.getKey();
     if (key != NO_KEY){
       playSound(KEYBOARD_SOUND, KEYBOARD_VOLUME, KEYBOARD_TIME);
       Serial.println(key);
     }   
  }
}
 
  finger.begin(57600);
  rfidInitialization();
  accgyro.initialize();
  pinMode(PIN_LOCKS, OUTPUT); //relay 1
  pinMode(PIN_POWER_SUPPLY, OUTPUT); //relay 2
 
  digitalWrite(PIN_POWER_SUPPLY, HIGH);
  //digitalWrite(PIN_LOCKS, HIGH);
 
  finger.getParameters();
  finger.getTemplateCount();
 
  int ax, ay, az, gx, gy, gz;
  accgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  low_x = ax - 1000;
  low_y = ay - 1000;
  low_z = az - 1000;
  high_x = ax + 1000; 
  high_y = ay + 1000;
  high_z = az + 1000;
  delay(100);
 
  Serial.println(”INITIALIZATION COMPLETED”);
  delay(500);
  playSound(BLOCK_SOUND, BLOCK_VOLUME, BLOCK_TIME);
  sendSms(WELCOME_NOTIFY);  
  }
 
  void resetFunc(){
    digitalWrite(RESET_PIN, LOW);
  }
 
  bool checkFingerprint(int current){ // checking the scanned fingerprint for presence in the database
    if (current != 0){
      playSound(TRUE_INPUT_SOUND, TRUE_INPUT_VOLUME, TRUE_INPUT_TIME);
      return true;
      }
    else{
      return false;
      }
  }
  bool recieveFingerprint() { // obtaining a print using a scanner
    protection();
    uint8_t p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        break;
      case FINGERPRINT_NOFINGER:
        return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        return p;
      case FINGERPRINT_IMAGEFAIL:
        return p;
      default:
        return p;
      }
    p = finger.image2Tz();
    switch (p) {
      case FINGERPRINT_OK:
        break;
      case FINGERPRINT_IMAGEMESS:
        return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        return p;
      case FINGERPRINT_FEATUREFAIL:
        return p;
      case FINGERPRINT_INVALIDIMAGE:
        return p;
      default:
      return p;
    }
 
    p = finger.fingerSearch();
    if (p == FINGERPRINT_OK){
      fingerIDS = finger.fingerID;
      } 
    else if (p == FINGERPRINT_PACKETRECIEVEERR){
      return p;
      } 
    else if (p == FINGERPRINT_NOTFOUND){
      return p;
      } 
    else{
      return p;
      }
  }
 
  void restInPeace(){ // shutdown cycle after self-destruction
    while (true){
      playSound(DESTRUCTION_SOUND, DESTRUCTION_VOLUME, DESTRUCTION_TIME);
      delay(2400);
      }
  }
 
  void sendSms(String current_data) {
      delay(1000);
      Serial2.println(”AT”);
      delay(100);
      Serial2.println(”AT+CMGF=1”);
      delay(200);
      Serial2.print(”AT+CMGS=\””);
      Serial2.print(PHONE_NUMBER); // mobile number with country code
      Serial2.println(”\””);
      delay(500);
      Serial2.println(current_data); // Type in your text message here
      delay(500);
      Serial2.println((char)26); // This is to terminate the message
      delay(1000);
   }
 
  void sendPin(){
    randomSeed(micros());
    Serial.println(“SENDING PIN CODE”);
    TRUE_PASSWORD_UNLOCK = random(100000, 1000000);
    notify = “Your verification code is “ + TRUE_PASSWORD_UNLOCK + “ \nACCESSCODE”;
    Serial.println(notify);
    sendSms(notify);
    isPinSent = true;
  }
 
  void destruction(){ // self-destruction function
    if (isProtectionOn == false){
      digitalWrite(PIN_POWER_SUPPLY, HIGH);
    }
    sendSms(DESTRUCTION_NOTIFY);
    myservo.attach(45);
 
    Serial.println(“STARTING SELF-DESTRUCTION”);
    for (pos = 0; pos <= 180; pos += 1) {
      myservo.write(pos);
      delay(10);
      }
 
    restInPeace();
  }
 
  void protection(){ // sensor polling protection function
    Serial.println(“PROTECTION IS RUNNING”);
    long int t = millis();
    int ax, ay, az, gx, gy, gz;
    accgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(5);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
    DURATION_SONAR = pulseIn(PIN_ECHO, HIGH);
    LENGTH_SONAR = (DURATION_SONAR / 2) / 29.1;
    if (ax > high_x or ax < low_x or ay > high_y or ay < low_y or az > high_z or az < low_z or LENGTH_SONAR > MAX_LENGTH_SONAR){
      COUNTER_EVENTS++;
      Serial.println(”TRACKING SENSORS”);
      }
    if (COUNTER_EVENTS >= COUNTER_MAX_EVENTS){
      destruction();
      }
    COUNTER++;
    if (COUNTER >= COUNTER_LIMIT){
      COUNTER = 0;
      COUNTER_EVENTS = 0;
      }
    delay(50); 
 
  }
 
  String passwordInput(){ // function of entering 6-digit codes from the keyboard
    playSound(TRUE_INPUT_SOUND_2, TRUE_INPUT_VOLUME, TRUE_INPUT_TIME);
    String passkeys;
    for (int i = 0; i < 6; i++){
      char key = keypad.getKey();
      while (key == NO_KEY){
        if (isProtectionOn){
          protection();
          }
        key = keypad.getKey();
      }
      playSound(KEYBOARD_SOUND, KEYBOARD_VOLUME, KEYBOARD_TIME);
      passkeys.concat(key);
    }
    return passkeys;
  }
 
  int checkPassword(char input_type, String input_password){ // checking the correctness of the entered code and processing it
    if ((input_type == ‘A’ && input_password == TRUE_PASSWORD_UNLOCK) or (input_password == MASTER_PASSWORD)){
      return 1; // return 1 if the login password was checked and the password was entered correctly
      }
    else if (input_type == ‘D’ && input_password == TRUE_PASSWORD_DESTRUCT){
      return 2; // return 2 if the self-destruction password was checked and the password was entered correctly
      }
    else {
      return 3;
      }
  }
 
  bool checkRFID(){ // verification of the correctness of the presented RFID key
    if ( ! mfrc522.PICC_IsNewCardPresent()) {
      return false;
      }
    if ( ! mfrc522.PICC_ReadCardSerial()) {
      return false;    
      }
 
    unsigned long UID_unsigned;
    UID_unsigned =  mfrc522.uid.uidByte[0] << 24;
    UID_unsigned += mfrc522.uid.uidByte[1] << 16;
    UID_unsigned += mfrc522.uid.uidByte[2] <<  8;
    UID_unsigned += mfrc522.uid.uidByte[3];
    String UID_string = (String)UID_unsigned;
 
    Serial.println(UID_string);
    if (UID_string == TRUE_RFID_ID_1 or UID_string == TRUE_RFID_ID_2){
      playSound(TRUE_INPUT_SOUND, TRUE_INPUT_VOLUME, TRUE_INPUT_TIME);
      return true;
      }
    else {
      playSound(FALSE_INPUT_SOUND, FALSE_INPUT_VOLUME, FALSE_INPUT_TIME);
      return false;      
      }    
  }
 
String updateGsm(){
  char data;
  String result;
  String info;
  String returnedPassword;
  while(Serial2.available()) 
  {
    delay(10);
    data = Serial2.read();
    result.concat(data);
  }
 
  if (result[0] = ”+”){
    info = result;
  }
 
  if (info != ””){
    for (int i = 10; i < 21; i++){
    returnedPassword.concat(info[i]);
    }
 
    for (int i = 50; i < 56; i++){
    returnedPassword.concat(info[i]);
    }
  }
 
  return returnedPassword;
 
}
 
  void zeroBools(){
    fingerIDS = 0;
    isRFIDChecked = false;
    isFingerChecked = false;
    isPinSent = false;
  }
 
  void blockSystem(){ // lock function
    isProtectionOn = true;
    //digitalWrite(PIN_LOCKS, HIGH);
    digitalWrite(PIN_POWER_SUPPLY, HIGH);
    Serial.print(”SYSTEM LOCKED”);
    zeroBools();
    //playSound(BLOCK_SOUND, BLOCK_VOLUME, BLOCK_TIME);
    //sendSms(WELCOME_NOTIFY);  
    resetFunc();
    delay(150);
 
  }
 
  void openDoors(){ // function of disabling door locks
    Serial.print(“DOOR OPENED, LOCKS ARE DISABLED”);
    sendSms(DOORS_NOTIFY);
    //digitalWrite(PIN_LOCKS, LOW);
    for (int i = 0; i < 12; i++){
      playSound(KEYBOARD_SOUND, KEYBOARD_VOLUME, KEYBOARD_TIME); // Можно заменить на DOOR_SOUND
      delay(1000);      
    }
    for (int i = 0; i < 3; i++){
      playSound(KEYBOARD_SOUND, KEYBOARD_VOLUME, KEYBOARD_TIME); // Можно заменить на DOOR_SOUND
      delay(50);      
    }    
    //digitalWrite(PIN_LOCKS, HIGH);
  }
 
  void unlockSystem(){
    isProtectionOn = false;
    zeroBools();
    digitalWrite(PIN_POWER_SUPPLY, LOW);
    playSound(UNLOCK_SOUND, UNLOCK_VOLUME, UNLOCK_TIME);
    Serial.println(”SYSTEM UNLOCKED”);
    sendSms(UNLOCK_NOTIFY);
  }
 
  void loop(){
    if (isProtectionOn){
      protection();
  }
 
  if (isRFIDChecked != true && isProtectionOn){
    isRFIDChecked = checkRFID();
    }
 
  if (isRFIDChecked && isProtectionOn){
      if (isFingerChecked != true){
        recieveFingerprint();
        isFingerChecked = checkFingerprint(fingerIDS);
        Serial.print(isFingerChecked);
 
      key = keypad.getKey();
      if (key != NO_KEY){
        playSound(KEYBOARD_SOUND, KEYBOARD_VOLUME, KEYBOARD_TIME);   
        Serial.println(key);
        if (key == ‘*’){
          delay(100);
          playSound(KEYBOARD_SOUND, KEYBOARD_VOLUME, KEYBOARD_TIME); 
          zeroBools();
        }
      }
        }       
      else {
        if (isPinSent == false){
          sendPin(); // генерирует pin, отправляет pin, назначает pin
          }
        else {
          key = keypad.getKey();
          if (key != NO_KEY){
          playSound(KEYBOARD_SOUND, KEYBOARD_VOLUME, KEYBOARD_TIME);
            Serial.println(key);
            if (key == ‘A’ or key == ‘D’){
              Serial.println(“PASS IS BEING ENTERED”);
              String enteredPassword = passwordInput();
              Serial.println(enteredPassword);
              int checkedTypeOfPassword = checkPassword(key, enteredPassword);
                if (checkedTypeOfPassword == 1){
                  unlockSystem();
                  }
                else if (checkedTypeOfPassword == 2){
                  destruction();
                  }
                else if (checkedTypeOfPassword == 3){
                  playSound(FALSE_INPUT_SOUND, FALSE_INPUT_VOLUME, FALSE_INPUT_TIME);
                  }
          }
            else if (key == ‘*’){
              delay(100);
              playSound(KEYBOARD_SOUND, KEYBOARD_VOLUME, KEYBOARD_TIME);  
              zeroBools();
            }
        }
      }
    }
    }
 
    else {
      key = keypad.getKey();
      if (key != NO_KEY){ 
        playSound(KEYBOARD_SOUND, KEYBOARD_VOLUME, KEYBOARD_TIME);   
        Serial.println(key);
        if (key == ‘D’){
          Serial.println(“PASS IS ENTERING”);
          String enteredPassword = passwordInput();
          Serial.println(enteredPassword);
          int checkedTypeOfPassword = checkPassword(key, enteredPassword);
            if (checkedTypeOfPassword == 2){
                destruction();
                }
            else {
                playSound(FALSE_INPUT_SOUND, FALSE_INPUT_VOLUME, FALSE_INPUT_TIME);
                }
           }
        if (key == ‘B’ && isProtectionOn == false){
          blockSystem();
          }
        if (key == ‘C’ && isProtectionOn == false){
          openDoors();
          }  
       }
     }
 
 
     messageCheck = updateGsm();
 
     if (messageCheck.equals(SELF_DESTRUCTION_SMS)){
      destruction();
     }
     if (messageCheck.equals(UNLOCK_SMS) && isProtectionOn){
      unlockSystem();
     }        
     if (messageCheck.equals(LOCK_SMS) && isProtectionOn == false){
      blockSystem();
     }
   }
