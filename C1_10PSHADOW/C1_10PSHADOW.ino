// =================================================================================================
//   SHADOW :  Small Handheld Arduino Droid Operating Wand
// =================================================================================================
//   Originally Written By KnightShade
//   Heavily Modified By Ian Martin
//   Inspired by the PADAWAN by danf
//   Totally hacked by Eebel 22 Aug 2019 - Added Tabs, NeoPixel Dome Lighting, and MP3 Trigger
//    Adding ServoEasing library for real time servo control
//      I also cleaned up some compiler Warnings due to different number type being compared like unsigned long to millis()
//
//   PS3 Bluetooth library - developed by Kristian Lauszus
//
//   Sabertooth (Foot Drive): Set Sabertooth 2x32 or 2x25 Dip Switches: 1 and 2 Down, All Others Up
//   SyRen 10 Dome Drive: For SyRen packetized Serial Set Switches: 1, 2 and 4 Down, All Others Up
//   Serial1 - MiniMaestro Body
//   Serial2 - Sabertooth and Syren
//	 Serial3 - MP3 Trigger
//   Serial1-maestrohead, Serial2 - Sabertooth and       , Serial3-maestro (body)
//=================================================================================================


// Satisfy IDE, which only needs to see the include statement in the ino.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif

// --------------------------------------------------------
//                      User Settings
// --------------------------------------------------------
//#define PrimaryPS3Controller
//#include <Arduino.h>

#ifdef PrimaryPS3Controller
//Chopper Controller number1
   String PS3MoveNavigatonPrimaryMAC = "04:76:6E:99:01:44"; //controller 1. If using multiple controllers, designate a primary (MIKES: 00:07:04:EF:56:80
#else
   String PS3MoveNavigatonPrimaryMAC = "04:76:6E:99:01:44"; //controller 2.
#endif

//String PS3MoveNavigatonPrimaryMAC = "00:07:04:BA:6F:DF";//Controller 2

byte drivespeed1 = 50; //was 50;set these 3 to whatever speeds work for you. 0-stop, 127-full speed.
byte drivespeed2 = 50; //was 80.  Recommend beginner: 50 to 75

byte turnspeed = 75; //was 58.  50 - the higher this number the faster it will spin in place, lower - easier to control.

byte domespeedfast = 127;//127; // Use a number up to 127
byte domespeedslow = 80; // Use a number up to 127
byte domespeed = domespeedfast; // Use a number up to 127 was domespeedslow...I like it faster

byte ramping = 5; //was2  - Ramping, the lower this number the longer R2 will take to speedup or slow down,

byte joystickFootDeadZoneRange = 18;  //15 For controllers that centering problems, use the lowest number with no drift
byte joystickDomeDeadZoneRange = 12;  //10 For controllers that centering problems, use the lowest number with nfo drift
byte driveDeadBandRange = 10; // Used to set the Sabertooth DeadZone for foot motors

int invertTurnDirection = -1;   //This may need to be set to 1 for some configurations

#define SHADOW_DEBUG       //uncomment this for console DEBUG output
#define EEBEL_TEST    //Uncomment to Change things back from my test code
//#define SERVO_EASING //Comment out to use Adafruit PWM code instead.
//#define SOFTWARE_SERIAL //Uncomment out to use software serial

#define NEOPIXEL_TEST //Uncomment to not use test NEOPIXEL test code

//Body Sequence Numbers
#define ALL_CLOSED 0
#define ALL_TOOLS_OUT_UP 1
#define BODY_ALL_WAVE 2
#define BODY_WAVE_LEFT 3
#define BREADPANS_CLOSED 4
#define BREADPANS_OPEN 5
#define CARD_DISPENSE 6
#define HEAD_WOBBLE_LEFT 7
#define HEAD_WOBBLE_RIGHT 8
#define INTERFACE_BOINK_BOINK 9
#define INTERFACE_CLOSE 10
#define INTERFACE_OPEN 11
#define INTERFACE_SEQUENCE 12
#define MINISAW_PULSE 13
#define MINISAW_SEQUENCE 14
#define MINISAW_CLOSE 15
#define MINISAW_OPEN 16
#define UTILITYARM_CLOSE 17
#define UTILITYARM_OPEN_CLOSE 18
#define UTILITYARM_OPEN 19

// --------------------------------------------------------
//                Drive Controller Settings
// --------------------------------------------------------

int motorControllerBaudRate = 9600; // Set the baud rate for the Syren motor controller. For packetized options are: 2400, 9600, 19200 and 38400

#define SYREN_ADDR         129      // Serial Address for Dome Syren
#define SABERTOOTH_ADDR    128      // Serial Address for Foot Sabertooth
//#define ENABLE_UHS_DEBUGGING 1

#define smDirectionPin  2 // Zapper Direction pin
#define smStepPin  3 //Zapper Stepper pin
#define smEnablePin  4 //Zapper Enable pin
#define PIN_DOMENEOPIXELS 6
#define PIN_UTILITYNEOPIXELS 53
#define PIN_ZAPPER 7
#define PIN_BUZZSAW 8
#define PIN_BADMOTIVATOR 9
#define PIN_DOMECENTER 49
#define PIN_BUZZSLIDELIMIT 24

//#define SHADOW_VERBOSE     //uncomment this for console VERBOSE output


// --------------------------------------------------------
//                       Libraries
// --------------------------------------------------------

#include <Arduino.h>
#include <PS3BT.h>  //PS3 Bluetooth Files
#include <SPP.h>
#include <usbhub.h>
#include <Sabertooth.h>
#include <Servo.h>
#include <Wire.h>
#ifdef SOFTWARE_SERIAL
	#include <SoftwareSerial.h>
#endif

//#include <Adafruit_PWMServoDriver.h> // I think I don't need this anymore due to ServoEasing library
//#include <Adafruit_Soundboard.h>

#include <NeoPatterns.h>
#include <MP3Trigger.h>
#include <PololuMaestro.h> // added the Maestro libray

#ifdef SOFTWARE_SERIAL
	SoftwareSerial maestroSerial(10, 11); //tx pin 11
#endif
/*
 * !!! Uncomment use PCA9865 in ServoEasing.h to make the expander work !!!
 * Otherwise you will see errors like: "PCA9685_Expander:44:46: error: 'Wire' was not declared in this scope"
 */


/*
 Works with the following versions
 NeoPatterns 2.0.0
 ServoEasing 1.4.1
 Servo 1.1.1
 Adafruit NeoPixel 1.3.2
 Adafruit PWM ServoDriver Libray 2.3.0
 USB HOST SHIELD version 1.3.1
 */



//Declarations-----------------------------------------------
void swapPS3NavControllers(void);
void moveUtilArms(int arm, int direction, int position, int setposition);
//void allPatterns(NeoPatterns * aLedsPtr);

//Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, NULL);

// --------------------------------------------------------
//                        Variables
// --------------------------------------------------------

unsigned long currentMillis = millis();
unsigned long flutterMillis = 0;
unsigned long previousDomeMillis = millis();
unsigned long previousFootMillis = millis();
unsigned long stoppedDomeMillis = 0;
unsigned long startDomeMillis = 0;
boolean domeSpinning = false;
int lastDomeRotationSpeed = 0;
int startDomeRotationSpeed = 0;
int autoDomeSetSpeed = 0;
unsigned long autoDomeSetTime = 0;
boolean head_right = true;
boolean flutterActive = false;
int lastSoundPlayed = 0;
int lastLastSoundPlayed = 0; // I'm an idiot, I know.
boolean cardLightsOn = false;//Card dispenser lights are off

//int serialLatency = 25;   //This is a delay factor in ms to prevent queuing of the Serial data. 25ms seems appropriate for HardwareSerial
unsigned long serialLatency = 25; //removes compiler warning for comparisons of different number types set it to 30..Does this make for more reliable PS3 connections?
unsigned long motivatorMillis = millis();
unsigned long smokeMillis = millis();
unsigned long blinkMillis = millis();
unsigned long lastBlinkMillis;
boolean smokeTriggered = false;
boolean motivatorTriggered = false;

int centerDomeSpeed = 0;
int centerDomeHighCount = 0;
int centerDomeLowCount = 0;
int centerDomeRotationCount = 0;
boolean domeCentered = false;
boolean domeCentering = false;

boolean droidFreakOut = false;
boolean droidFreakingOut = false;
boolean droidFreakedOut = false;
boolean droidFreakOutComplete = false;
boolean droidScream = false;
boolean InterfaceArmActivated = false;
boolean miniSawActivated = false;
boolean interfaceOut = false;
boolean utilArmClosed = true;
boolean drawerActivated = false;

boolean piePanelsOpen = false;
boolean sidePanelsOpen = false;
boolean midPanelsOpen = false;
boolean buzzDoorOpen = false;
boolean utilityGrab = false;
boolean allToolsOut = false;
//boolean lastHeadWobbleLeft = false;

unsigned long freakOutMillis = millis();

boolean lifeformActivated = false;
unsigned long lifeformMillis = millis();
unsigned long lifeformLEDMillis = millis();
int lifeformPosition = 0;
int lifeformDir = 0;
int lifeformLED = 0;

int lastUtilStickPos = 127;
unsigned long lastUtilArmMillis = millis();



boolean buzzActivated = false;
boolean buzzSpinning = false;
unsigned long buzzSawMillis = millis();
unsigned long buzzSawSlideInMillis = millis();
boolean buzzSawSliding = false;

/*
 * I Moved these variables to #define statements so they would all be in the same place
int smDirectionPin = 2; // Zapper Direction pin
int smStepPin = 3; //Zapper Stepper pin
int smEnablePin = 4; //Zapper Enable pin
 */
boolean zapperActivated = false;

boolean isStickEnabled = true;
byte isAutomateDomeOn = true;

unsigned long automateSoundMillis = 0;
unsigned long automateDomeMillis = 0;
byte automateAction = 0;

//Maestro declarations
#ifdef SOFTWARE_SERIAL
	MiniMaestro maestrobody(maestroSerial); //create "maestrobody as the maestro object for the body on Serial 3
#else
	//MiniMaestro maestrobody(Serial1);
	MiniMaestro maestrobody(Serial1, 255,12);
#endif
	//MiniMaestro maestrohead(Serial1); //create "maestrobody as the maestro object for the head on Software Serial
	MiniMaestro maestrohead(Serial1, 255, 13);

Sabertooth *ST=new Sabertooth(SABERTOOTH_ADDR, Serial2);
Sabertooth *SyR=new Sabertooth(SYREN_ADDR, Serial2);

const int HOLO_DELAY = 5000; //up to 20 second delay

uint32_t holoFrontRandomTime = 0;
uint32_t holoFrontLastTime = 0;



/////// Setup for USB and Bluetooth Devices

USB Usb;
BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so
PS3BT *PS3Nav=new PS3BT(&Btd);
PS3BT *PS3Nav2=new PS3BT(&Btd);

//Used for PS3 Fault Detection
uint32_t msgLagTime = 0;
uint32_t lastMsgTime = 0;
uint32_t currentTime = 0;
uint32_t lastLoopTime = 0;
int badPS3Data = 0;

//SPP SerialBT(&Btd,"Astromech:R5","1977"); // Create a BT Serial device(defaults: "Arduino" and the pin to "0000" if not set)
//boolean firstMessage = true;
String output = "";

boolean isFootMotorStopped = true;
boolean isDomeMotorStopped = true;

boolean isPS3NavigatonInitialized = false;
boolean isSecondaryPS3NavigatonInitialized = false;



//------------------------NeoPixel Setup-------------------------------------
#ifdef NEOPIXEL_TEST
  void ownPatterns(NeoPatterns * aLedsPtr);
  NeoPatterns bar16 = NeoPatterns(9, PIN_UTILITYNEOPIXELS, NEO_GRB + NEO_KHZ800, &ownPatterns);
#else
  //onComplete callback functions
  void allPatterns(NeoPatterns * aLedsPtr);
  NeoPatterns bar16 = NeoPatterns(9, PIN_UTILITYNEOPIXELS, NEO_GRB + NEO_KHZ800, &allPatterns);
#endif


//----------------MP3 Trigger Sound------------------------------------------
MP3Trigger trigger;//make trigger object
int droidSoundLevel = 40; //60Amount of attenuation  higher=LOWER volume..ten is pretty loud 0-127

///////////////////////////////////////////////////////////////================================
void WobbleHead(){
	boolean WobbleLeft=random(0,2);
	Serial.print(WobbleLeft);
	if (WobbleLeft){
		maestrobody.restartScript(HEAD_WOBBLE_LEFT);
	}
	else{
		maestrobody.restartScript(HEAD_WOBBLE_RIGHT);
	}

}
void silenceMiddleServos(){

}
void silenceServos(){

}



///////////////////////////////////////////////////////////////================================================================
// =======================================================================================
// //////////////////////////Process PS3 Controller Fault Detection///////////////////////
// =======================================================================================
boolean criticalFaultDetect(){
    if (PS3Nav->PS3NavigationConnected || PS3Nav->PS3Connected){
        lastMsgTime = PS3Nav->getLastMessageTime();
        currentTime = millis();
        if ( currentTime >= lastMsgTime){
          msgLagTime = currentTime - lastMsgTime;
        }else{
             #ifdef SHADOW_DEBUG
               output += "Waiting for PS3Nav Controller Data\r\n";
             #endif
             badPS3Data++;
             msgLagTime = 0;
        }

        if (msgLagTime > 100 && !isFootMotorStopped){
            #ifdef SHADOW_DEBUG
              output += "It has been 100ms since we heard from the PS3 Controller\r\n";
              output += "Shut downing motors, and watching for a new PS3 message\r\n";
            #endif
            ST->stop();
            SyR->stop();
            isFootMotorStopped = true;
            return true;
        }
        if ( msgLagTime > 30000 ){
            //was 30000 I set to 3
            #ifdef SHADOW_DEBUG
              output += "It has been 30s since we heard from the PS3 Controller\r\n";
              output += "msgLagTime:";
              output += msgLagTime;
              output += "  lastMsgTime:";
              output += lastMsgTime;
              output += "  millis:";
              output += millis();
              output += "\r\nDisconnecting the controller.\r\n";
            #endif
            PS3Nav->disconnect();
        }

        //Check PS3 Signal Data
        if(!PS3Nav->getStatus(Plugged) && !PS3Nav->getStatus(Unplugged)){
            // We don't have good data from the controller.
            //Wait 10ms, Update USB, and try again
            delay(10);
            Usb.Task();
            if(!PS3Nav->getStatus(Plugged) && !PS3Nav->getStatus(Unplugged)){
                badPS3Data++;
                #ifdef SHADOW_DEBUG
                    output += "\r\nInvalid data from PS3 Controller.";
                #endif
                return true;
            }
        }else if (badPS3Data > 0){
            //output += "\r\nPS3 Controller  - Recovered from noisy connection after: ";
            //output += badPS3Data;
            badPS3Data = 0;
        }

        if ( badPS3Data > 10 ){
            #ifdef SHADOW_DEBUG
                output += "Too much bad data coming fromo the PS3 Controller\r\n";
                output += "Disconnecting the controller.\r\n";
            #endif
            PS3Nav->disconnect();
        }
    }else if (!isFootMotorStopped){
        #ifdef SHADOW_DEBUG
            output += "No Connected Controllers were found\r\n";
            output += "Shutting down motors, and watching for a new PS3 message\r\n";
        #endif
        ST->stop();
        SyR->stop();
        isFootMotorStopped = true;
        return true;
    }
    return false;
}
// =======================================================================================
// //////////////////////////END of PS3 Controller Fault Detection///////////////////////
// =======================================================================================




boolean readUSB(){


    Usb.Task();
    #ifdef EEBEL_TEST
    //Serial.print(F("\r\nEebelUSB8"));
          //The more devices we have connected to the USB or BlueTooth, the more often Usb.Task need to be called to eliminate latency.
          if (PS3Nav->PS3NavigationConnected)
          {
              if (criticalFaultDetect())
              {
                  //We have a fault condition that we want to ensure that we do NOT process any controller data!!!
                  //printOutput();
                  return false;
              }

          } else if (!isFootMotorStopped)
          {
              #ifdef SHADOW_DEBUG
                  output += "No foot controller was found\r\n";
                  output += "Shutting down motors, and watching for a new PS3 foot message\r\n";
              #endif
              ST->stop();
              isFootMotorStopped = true;
              drivespeed1 = 0;
              //WaitingforReconnect = true;
          }

//          if (PS3Nav2->PS3NavigationConnected)
//          {
//
//              if (criticalFaultDetectDome())
//              {
//                 //We have a fault condition that we want to ensure that we do NOT process any controller data!!!
//                 printOutput();
//                 return false;
//              }
//          }

          return true;
  #else
        //Old R5 code
        Serial.println("\n\rRead USB");
        //The more devices we have connected to the USB or BlueTooth, the more often Usb.Task need to be called to eliminate latency.
        Usb.Task();
        if (PS3Nav->PS3NavigationConnected) Usb.Task();
        if (PS3Nav2->PS3NavigationConnected) Usb.Task();
        if (criticalFaultDetect()){
          //We have a fault condition that we want to ensure that we do NOT process any controller data!!!
          flushAndroidTerminal();
          Serial.print(F("\r\nPS3 Controller Fault!"));
          return false;
        }
        return true;

    #endif
}









// ==============================================================
//                          SOUND STUFF
// ==============================================================

void processSoundCommand(int soundCommand, int soundCommand2 = 0){
  // Takes two track numbers and selects a random track.
  //Then, it builds a string to call for that track name.
    if (soundCommand2 != 0){ //If it is == 0, then there is only one track in the list
      soundCommand = random(soundCommand,soundCommand2);
      if (soundCommand == lastSoundPlayed || soundCommand2 == lastLastSoundPlayed) soundCommand = random(soundCommand,soundCommand2);
      //No idea why this is here three times.  Also, why compares to the same thing on either side of the or.
      //if (soundCommand == lastSoundPlayed || soundCommand == lastLastSoundPlayed) soundCommand = random(soundCommand,soundCommand2);
      //if (soundCommand == lastSoundPlayed || soundCommand == lastLastSoundPlayed) soundCommand = random(soundCommand,soundCommand2);
      if (soundCommand != lastSoundPlayed && soundCommand2 != lastLastSoundPlayed) lastLastSoundPlayed = soundCommand;
    }
    trigger.trigger(soundCommand); //play the track
    lastSoundPlayed = soundCommand;
}

void ps3soundControl(PS3BT* myPS3 = PS3Nav, int controllerNumber = 1){
    if (!(myPS3->getButtonPress(L1)||myPS3->getButtonPress(L2)||myPS3->getButtonPress(PS))){
      if (myPS3->getButtonClick(UP)) {
        processSoundCommand(30,33);
        Serial.println("\n\rSoundCommand Annoyed");//;  // Annoyed
      }
      else if (myPS3->getButtonClick(RIGHT)){
        processSoundCommand(1,12);  // CHAT
        Serial.println("\n\rSoundCommand Chat");
      }
      else if (myPS3->getButtonClick(DOWN)){
        processSoundCommand(50,54); // Sad
        Serial.println("\n\rSoundCommand Sad");
      }
      else if (myPS3->getButtonClick(LEFT)) {
        processSoundCommand(40,47); // Razz
        Serial.println("\n\rSoundCommand Razz");
      }
    }else if (myPS3->getButtonPress(L2)&&!myPS3->getButtonPress(L1)){
      if (myPS3->getButtonClick(RIGHT))  processSoundCommand(7);         //Yes
      else if (myPS3->getButtonClick(DOWN))   processSoundCommand(3);    //Hello
      else if (myPS3->getButtonClick(LEFT))   processSoundCommand(6);    //No
    }else if (myPS3->getButtonPress(L1)&&!myPS3->getButtonPress(L2)){
      if (myPS3->getButtonClick(UP))          processSoundCommand(1,26);  // Random/Chat Auto Sounds
      else if (myPS3->getButtonClick(DOWN))   processSoundCommand(21);  // Hum
      else if (myPS3->getButtonClick(LEFT))   processSoundCommand(99);  // None
      else if (myPS3->getButtonClick(RIGHT))  processSoundCommand(11); // Patrol
    }
}

void soundControl(){
   if (PS3Nav->PS3NavigationConnected) ps3soundControl(PS3Nav,1);
   if (PS3Nav2->PS3NavigationConnected) ps3soundControl(PS3Nav2,2);
}

// ==============================================================
//                         DOME FUNCTIONS
// ==============================================================

void pieOpenAll(){

    }

void pieCloseAll(){

}

void openAll(boolean motivator = false, boolean openUtil = false){

}

void closeAll(){
    maestrobody.restartScript(0);//Close Body Stuff
}




void moveUtilArms(int arm, int direction, int position, int setposition){

}

void freakOut(){
  if (droidFreakOut){
    if (!droidFreakingOut){

        processSoundCommand(1);
        centerDomeRotationCount = 1;
        domeCentered = false;
        centerDomeSpeed = 75;
        droidFreakingOut = true;
    }else if (droidFreakingOut && domeCentered && centerDomeRotationCount == 0){
        openAll(true);
        smokeTriggered = true;
        digitalWrite(PIN_BADMOTIVATOR, HIGH);   //Bad Motivator
        smokeMillis = millis() + 3000;
        droidFreakOut = false;
        droidFreakingOut = false;
        droidFreakedOut = true;
        droidFreakOutComplete = true;
    }
  }
}

void centerDomeLoop(){
    if (digitalRead(PIN_DOMECENTER) == 0 && !domeCentered){
      if (centerDomeHighCount >= 200 && !droidFreakingOut){
        centerDomeHighCount = 200;
        if (centerDomeRotationCount == 0){
          centerDomeSpeed = 0;
          domeCentered = true;
          domeCentering = false;
        }
      }else{
        centerDomeHighCount++;
      }
      if (centerDomeHighCount >= 10 && centerDomeLowCount == 30){
        centerDomeHighCount = 0;
        centerDomeLowCount = 0;

        if (centerDomeRotationCount == 0){
          centerDomeSpeed = 0;
          SyR->stop();
          domeCentered = true;
          domeCentering = false;
        }else{
          centerDomeRotationCount = centerDomeRotationCount - 1;
        }
      }
    }else{
      if (centerDomeLowCount >= 30){
        centerDomeLowCount = 30;
      }else{
        centerDomeLowCount++;
        centerDomeHighCount = 0;
      }
      SyR->motor(centerDomeSpeed);
      domeCentered = false;
      domeCentering = true;
    }
}

void blinkEyes(){

}

void badMotivatorLoop(){
     if (millis() > smokeMillis && smokeTriggered){
      digitalWrite(PIN_BADMOTIVATOR, LOW);
      smokeTriggered = false;
    }
    if (millis() > motivatorMillis && motivatorTriggered){
        #ifdef SERVO_EASING
        //TODO: Set bad Motivator!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
          //pwm2_Servo2.startEaseTo(pwm2_2Info.maxDeg,30);
        #else
          pwm2.setPWM(2, 0, pwm2_2_max); // MOTIVATOR
        #endif
        delay(100);
        //sfx.playTrack("T05     OGG");
        trigger.trigger(63);
        #ifdef SERVO_EASING
          //TODO: Set Bad Motivator !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
          //pwm2_Servo2.startEaseTo(pwm2_2Info.minDeg,30);
        #else
          pwm2.setPWM(2, 0, pwm2_2_min); // MOTIVATOR
        #endif
        silenceServos();
        blinkEyes();
        motivatorTriggered = false;
    }
}

int easingMotor(float time, float startValue, float change, float duration) {
    time /= duration / 2;
    if (time < 1)  {
         return change / 2 * time * time + startValue;
    }
    time--;
    return -change / 2 * (time * (time - 2) - 1) + startValue;
};


void rotateDome(int domeRotationSpeed, int domeRotationLength = 0){
    currentMillis = millis();
    if (((currentMillis - stoppedDomeMillis) < 2000) || ((!isDomeMotorStopped || domeRotationSpeed != 0) && ((currentMillis - previousDomeMillis) > (2*serialLatency)))){

      if (centerDomeSpeed == 0){
        if (domeRotationLength > 0){
           autoDomeSetSpeed = domeRotationSpeed;
           autoDomeSetTime = currentMillis+domeRotationLength;
        }
        if (currentMillis < autoDomeSetTime){
          domeRotationSpeed = autoDomeSetSpeed;
        }
      }

      if (domeRotationSpeed != 0){
        isDomeMotorStopped = false;
      }else{
        isDomeMotorStopped = true;
      }
      previousDomeMillis = currentMillis;

      if (domeSpinning && domeRotationSpeed == 0){
        stoppedDomeMillis = millis();
        domeSpinning = false;
      }else if (!domeSpinning && (domeRotationSpeed > 0 || domeRotationSpeed < 0)){
        domeSpinning = true;
        domeCentered = false;
      }

      if ((currentMillis - stoppedDomeMillis) < 400){
         domeRotationSpeed = easingMotor((currentMillis - stoppedDomeMillis), lastDomeRotationSpeed,(lastDomeRotationSpeed*-1), 400);
      }else{
         lastDomeRotationSpeed = domeRotationSpeed;
      }

      SyR->motor(domeRotationSpeed);
    }
}

void randomDomeMovement(){
  if (!droidFreakOut && !miniSawActivated){
    currentMillis = millis();
    if (currentMillis - automateDomeMillis > 3000){
      automateDomeMillis = millis();
      automateAction = random(6,35);
      if (automateAction < 20 && automateAction > 6 && !domeSpinning && !domeCentering){
        int randomDomeLength = random(500,800);
        centerDomeSpeed = 0;
        if (head_right){
            rotateDome(-automateAction,randomDomeLength);
            head_right = false;
        }else{
            rotateDome(automateAction,randomDomeLength);
            head_right = true;
        }
      }else if (automateAction <= 18 && !domeSpinning && !domeCentered && !domeCentering){
        centerDomeRotationCount = 0;
        if (head_right){
            centerDomeSpeed = -25;
            head_right = false;
        }else{
            centerDomeSpeed = 25;
            head_right = true;
        }
      }
    }
  }
}

void DrivingSoundHead(){
  if (isAutomateDomeOn){
    randomDomeMovement();
    currentMillis = millis();
    if (currentMillis - automateSoundMillis > 3500){
      automateSoundMillis = millis();
      automateAction = random(1,100);
      if (automateAction < 36) processSoundCommand(40,68);
    }
  }
}

// ==============================================================
//                       MOTOR FUNCTIONS
// ==============================================================

boolean ps3FootMotorDrive(PS3BT* myPS3 = PS3Nav){
  //Serial.println("\r\nMotor Funtion");
  int footDriveSpeed = 0;
  int stickSpeed = 0;
  int turnnum = 0;

  if (isPS3NavigatonInitialized) {
      if (!isStickEnabled){ // Additional fault control.  Do NOT send additional commands to Sabertooth if no controllers have initialized.
          if (abs(myPS3->getAnalogHat(LeftHatY)-128) > joystickFootDeadZoneRange) output += "Drive Stick is disabled\r\n";
          ST->stop();
          isFootMotorStopped = true;
      }else if (!myPS3->PS3NavigationConnected){
          ST->stop();
          isFootMotorStopped = true;
      }else if (myPS3->getButtonPress(L1)){
          ST->stop();
          isFootMotorStopped = true;
      }else{
          int joystickPosition = myPS3->getAnalogHat(LeftHatY);
          isFootMotorStopped = false;
          if (myPS3->getButtonPress(L2)){
            int throttle = 0;
            if (joystickPosition < 127){
                throttle = joystickPosition - myPS3->getAnalogButton(L2);
            }else{
                throttle = joystickPosition + myPS3->getAnalogButton(L2);
            }
            stickSpeed = (map(throttle, -255, 510, -drivespeed2, drivespeed2));
          }else{
            stickSpeed = (map(joystickPosition, 0, 255, -drivespeed1, drivespeed1));
          }

          if (abs(joystickPosition-128) < joystickFootDeadZoneRange){
              footDriveSpeed = 0;
          }else if (footDriveSpeed < stickSpeed){
              if (stickSpeed-footDriveSpeed<(ramping+1)){
                  footDriveSpeed+=ramping;
              }else{
                  footDriveSpeed = stickSpeed;
              }
          }else if (footDriveSpeed > stickSpeed){
              if (footDriveSpeed-stickSpeed<(ramping+1)){
                  footDriveSpeed-=ramping;
              }else{
                  footDriveSpeed = stickSpeed;
              }
          }

          turnnum = (myPS3->getAnalogHat(LeftHatX));

          //TODO:  Is there a better algorithm here?
          if (abs(footDriveSpeed) > 50){
              turnnum = (map(myPS3->getAnalogHat(LeftHatX), 54, 200, -(turnspeed/4), (turnspeed/4)));
          }else if (turnnum <= 200 && turnnum >= 54){
              turnnum = (map(myPS3->getAnalogHat(LeftHatX), 54, 200, -(turnspeed/3), (turnspeed/3)));
          }else if (turnnum > 200){
              turnnum = (map(myPS3->getAnalogHat(LeftHatX), 201, 255, turnspeed/3, turnspeed));
          }else if (turnnum < 54){
              turnnum = (map(myPS3->getAnalogHat(LeftHatX), 0, 53, -turnspeed, -(turnspeed/3)));
          }

          currentMillis = millis();
          if ((currentMillis - previousFootMillis) > serialLatency){
            if (footDriveSpeed < -driveDeadBandRange || footDriveSpeed > driveDeadBandRange){
              DrivingSoundHead();
            }

            ST->turn(turnnum * invertTurnDirection);
            ST->drive(footDriveSpeed);
            previousFootMillis = currentMillis;
            return true; //we sent a foot command
          }
      }
  }
  return false;
}

void footMotorDrive(){
  if ((millis() - previousFootMillis) < serialLatency) return;
  if (PS3Nav->PS3NavigationConnected) ps3FootMotorDrive(PS3Nav);
}

int ps3DomeDrive(PS3BT* myPS3 = PS3Nav, int controllerNumber = 1){
    int domeRotationSpeed = 0;
    int joystickPosition = myPS3->getAnalogHat(LeftHatX);
    if (((controllerNumber==1 && myPS3->getButtonPress(L1))&&!myPS3->getButtonPress(L2)) || (controllerNumber==1 && myPS3->getButtonPress(L3)) || ( controllerNumber==2 && !myPS3->getButtonPress(L1) && !myPS3->getButtonPress(L2))){
        if (controllerNumber==1 && myPS3->getButtonPress(L3)&&!myPS3->getButtonPress(L1)){
          domespeed = domespeedfast;
          domeRotationSpeed = (map(joystickPosition, 255, 0, -domespeed, domespeed));
        }else if (controllerNumber==1 && myPS3->getButtonPress(L3)&&myPS3->getButtonPress(L1)&&!myPS3->getButtonPress(L2)){
          domespeed = domespeedfast;
          domeRotationSpeed = (map(joystickPosition, 0, 255, -domespeed, domespeed));
        }else{
          domespeed = domespeedslow;
          domeRotationSpeed = (map(joystickPosition, 0, 255, -domespeed, domespeed));
        }
        if (abs(joystickPosition-128) < joystickDomeDeadZoneRange){
          domeRotationSpeed = 0;
        }
    }else{
      domespeed = domespeedslow;
    }

    if (domeRotationSpeed > 0 || domeRotationSpeed < 0){
      domeSpinning = true;
    }
    return domeRotationSpeed;
}





void domeDrive(){
  if ((millis() - previousDomeMillis) < (2*serialLatency) ) return;

  int domeRotationSpeed = 0;
  int ps3NavControlSpeed = 0;
  int ps3Nav2ControlSpeed = 0;
  if (PS3Nav->PS3NavigationConnected) ps3NavControlSpeed = ps3DomeDrive(PS3Nav,1);
  if (PS3Nav2->PS3NavigationConnected) ps3Nav2ControlSpeed = ps3DomeDrive(PS3Nav2,2);

  //In a two controller system, give dome priority to the secondary controller.
  //Only allow the "Primary" controller dome control if the Secondary is NOT spinnning it
  if ( abs(ps3Nav2ControlSpeed) > 0 ){
    domeRotationSpeed = ps3Nav2ControlSpeed;
  }else{
    domeRotationSpeed = ps3NavControlSpeed;
  }
  rotateDome(domeRotationSpeed);
}








// ==============================================================
//                        PS REMOTE COMMANDS
// ==============================================================
void ps3ProcessCommands(PS3BT* myPS3 = PS3Nav){

//All Closed
   if (!myPS3->getButtonPress(L2)&&!myPS3->getButtonPress(L1)){
	   if (myPS3->getButtonClick(X)){
		   cardLightsOn = false;
		   bar16.ColorWipe(COLOR32(0,0,0),0,REVERSE);//All lights off
		   maestrobody.restartScript(ALL_CLOSED);
	   }
   }
  // CardDispense
   if (!myPS3->getButtonPress(L2)&&myPS3->getButtonPress(L1)){
	   if (myPS3->getButtonClick(UP)){
		   cardLightsOn = true;
		   //bar16.ColorWipe(COLOR32(0,0,0),0,REVERSE);//All lights off
		   maestrobody.restartScript(CARD_DISPENSE);
	   }
   }
   // BodyWave Left
    if (!myPS3->getButtonPress(L2)&&myPS3->getButtonPress(L1)){
 	   if (myPS3->getButtonClick(DOWN)){
 		   maestrobody.restartScript(BODY_WAVE_LEFT);
 	   }
    }
    //HeadWoggle
     if (!myPS3->getButtonPress(L2)&&myPS3->getButtonPress(L1)){
  	   if (myPS3->getButtonClick(LEFT)){
  		 WobbleHead();
  	   }
     }




//****************** mark L2 and Arrow Command

	//AllToolsUp/Out

	//L2 and Down
	//All tools out
   if (myPS3->getButtonPress(L2)&&!myPS3->getButtonPress(L1)){
	   if (myPS3->getButtonClick(DOWN)){
		 if (allToolsOut){
			 maestrobody.restartScript(ALL_CLOSED);
			 maestrohead.restartScript(1);
			 allToolsOut = false;
		 }else{
			 maestrobody.restartScript(ALL_TOOLS_OUT_UP);
			 trigger.trigger(20);
		   //openAll();
		   allToolsOut = true;
		 }
	   }
   }


   //L2 UP
   // -BodyALL Wave
   if (myPS3->getButtonPress(L2)&&!myPS3->getButtonPress(L1)){
       if (myPS3->getButtonClick(UP)){
    	   maestrobody.restartScript(BODY_ALL_WAVE);
    	   trigger.trigger(19);
//         if (droidScream){
//
//           //closeAll();
//           droidScream = false;
//         }else{
//           //processSoundCommand(1);
//           trigger.trigger(60);
//           //openAll();
//           droidScream = true;
//         }
       }
#pragma mark -
#pragma mark BuzzDoorOnly
            //BuzzDoorOnly
      if (myPS3->getButtonClick(LEFT)){

      }
#pragma mark - Utility Grab
      if (myPS3->getButtonClick(RIGHT)){
        if (!utilityGrab){

        	maestrobody.restartScript(UTILITYARM_OPEN);
           utilityGrab = true;
        }else{

        	maestrobody.restartScript(UTILITYARM_CLOSE);
           utilityGrab = false;
        }
      }

   }


    #pragma mark -
    #pragma mark    Bad Motivator
   // BAD MOTIVATOR
   if (myPS3->getButtonPress(PS)){
        if(myPS3->getButtonClick(UP)){
            motivatorTriggered = true;
            smokeTriggered = true;
            digitalWrite(PIN_BADMOTIVATOR, HIGH);
            motivatorMillis = millis() + 1000;
            smokeMillis = millis() + 3000; // was5000;
            silenceServos();
          }
        if(myPS3->getButtonClick(RIGHT)){
            smokeTriggered = true;
            digitalWrite(PIN_BADMOTIVATOR, HIGH);
            smokeMillis = millis() + 3000;
            //my code
            output = "PS + Right Button";
            //printOutput();
          }
        // Rear Bolt Door (Previously Freak Out)
        if(myPS3->getButtonClick(LEFT)){

          processSoundCommand(80,88);

        }
    }



    // Lifeform Scanner


       if(myPS3->getButtonPress(L2)&&myPS3->getButtonPress(L1)&&myPS3->getButtonClick(L3)){

       }


     // Open Pie Panels
     if(myPS3->getButtonPress(L2)&&myPS3->getButtonPress(L1)&&myPS3->getButtonClick(UP)){
        Serial.println("\n\rOpen Pie Panels");
        Serial.print("piePanelsOpen = ");
        Serial.println(piePanelsOpen);
       if (piePanelsOpen){
          piePanelsOpen = false;
          pieCloseAll();
       }else{
          piePanelsOpen = true;
          pieOpenAll();
       }
     }

     // Close All Panels
     if(myPS3->getButtonPress(L2)&&myPS3->getButtonPress(L1)&&myPS3->getButtonClick(DOWN)){

      #ifdef SERVO_EASING
        //pwm3_Servo4.startEaseTo(pwm3_4_max,30); DataPanel
        //TODO: Head blinky thing?
        //Don't need anything here but a light blinky thing?

      #else
        pwm3.begin();
        pwm3.setPWMFreq(62);  // This is the maximum PWM frequency
        pwm3.setPWM(4,0,pwm3_4_max);
        pwm3.setPWM(10,0,pwm3_10_max);//UNUSED HEAD LEDs
        pwm3.setPWM(11,0,pwm3_11_max);
        pwm3.setPWM(12,0,pwm3_12_max);
        pwm3.setPWM(13,0,pwm3_13_max);
        pwm3.setPWM(14,0,pwm3_14_max);
       #endif
        //closeAll();
        maestrobody.restartScript(ALL_CLOSED); //Close All
     }

#pragma mark -
#pragma mark Side Panels
     // Open Side Pannels (Or if Interface Arm is Out, Grip it)///////////////////////////

      if(myPS3->getButtonPress(L2)&&myPS3->getButtonPress(L1)&&myPS3->getButtonClick(RIGHT)){
      if (!InterfaceArmActivated && !miniSawActivated){
        if (!sidePanelsOpen){
          Serial.println("Open BreadPan Doors");

          maestrobody.restartScript(BREADPANS_OPEN);
           sidePanelsOpen = true;
        }else{
          Serial.println("Close BreadPan Doors");

          maestrobody.restartScript(BREADPANS_CLOSED);
           sidePanelsOpen = false;
        }
      }else if (InterfaceArmActivated){

          maestrobody.restartScript(INTERFACE_BOINK_BOINK);


      }
      delay(300);
      silenceServos();
     }


    #pragma mark -
    #pragma mark    Middle Panels
     // Open Middle Pannels (Or if MiniSaw is Out, Spin it)//////////////////////
       if(myPS3->getButtonPress(L2)&&myPS3->getButtonPress(L1)&&myPS3->getButtonClick(LEFT)){
        if (!InterfaceArmActivated && !miniSawActivated){
          if (!midPanelsOpen){

             midPanelsOpen = true;
             Serial.println("midPanelsOpen = True");

          }else{
             //midPanelsOpen = false;
             Serial.println("midPanelsOpen = false");

             midPanelsOpen = false;
          }
          //delay(400);
          //silenceServos();
        }else if (miniSawActivated){
        	maestrobody.restartScript(MINISAW_PULSE);
        }
       }



    // Panel Drawer
      if(myPS3->getButtonPress(PS)&&myPS3->getButtonClick(L1)){
          if (!drawerActivated){

            drawerActivated = true;
          }else{

              drawerActivated = false;
          }
      }


    // Lego

  if(myPS3->getButtonPress(PS)&&myPS3->getButtonClick(DOWN)){
        if (zapperActivated){
           digitalWrite(PIN_ZAPPER,HIGH);
          // delay(1000);
           digitalWrite(PIN_ZAPPER,LOW);
        }else{


        }
      }


    // Card Dispenser

      if(myPS3->getButtonPress(L1)&&myPS3->getButtonClick(L3)){

    }


    #pragma mark -
    #pragma mark    Volume
    // Volume Up
    if(myPS3->getButtonPress(L1)&&!myPS3->getButtonPress(L2)&&myPS3->getButtonClick(CROSS)){
      droidSoundLevel -= 10;
      if (droidSoundLevel < 0) droidSoundLevel = 0;
      trigger.setVolume(droidSoundLevel);
      trigger.trigger(1);
    }
    // Volume Down
    if(!myPS3->getButtonPress(L1)&&myPS3->getButtonPress(L2)&&myPS3->getButtonClick(CROSS)){
      droidSoundLevel += 10;
      if (droidSoundLevel >100) droidSoundLevel = 100;
      trigger.setVolume(droidSoundLevel);
      trigger.trigger(1);
    }

#pragma mark -
#pragma mark Lower UtilityArm

        // Lower Utility Arm

   if(myPS3->getButtonPress(L1) && !myPS3->getButtonPress(L2)){
/*     int joystickPositionY = myPS3->getAnalogHat(LeftHatY);
     if (joystickPositionY < 95 && !myPS3->getButtonPress(L3)){
       int utilArmPos1 = (map(joystickPositionY, 0, 125, utilArmMax1, utilArmMin1)); //87 -310
        pwm2_Servo8.easeTo(utilArmPos1, 200);
       if (lastUtilArmPos1 > utilArmPos1){
         //moveUtilArms(1,0,joystickPositionY,utilArmPos1);
       }else{
         //moveUtilArms(1,0,joystickPositionY,lastUtilArmPos1);
       }
      }else if (joystickPositionY > 145 && !myPS3->getButtonPress(L3)){
       int utilArmPos1 = (map(joystickPositionY, 127, 225, (lastUtilArmPos1-10), utilArmMin1));
       if (lastUtilArmPos1 < utilArmPos1){
         //moveUtilArms(1,1,joystickPositionY,utilArmPos1);
       }else{
         //moveUtilArms(1,1,joystickPositionY,lastUtilArmPos1);
       }
      }else{
          pwm2_Servo8.detach();
     }*/
   }







     // Upper Utility Arm
     if(myPS3->getButtonPress(L2) && !myPS3->getButtonPress(L1)){
//       int joystickPositionY = myPS3->getAnalogHat(LeftHatY);
//       if (joystickPositionY < 95 && !myPS3->getButtonPress(L3)){
//         int utilArmPos0 = (map(joystickPositionY, 0, 125, utilArmMax0, utilArmMin0)); //87 -310
////        Serial.print("\n\rLastArmPos0= ");
////        Serial.print(lastUtilArmPos0);
////        Serial.print("\n\rutilArmPos0= ");
////        Serial.print(utilArmPos0);
//         if (lastUtilArmPos0 < utilArmPos0){
////           Serial.print("\n\rUpper Utility Arm Y = ");
////           Serial.print(utilArmPos0);
//           moveUtilArms(0,0,joystickPositionY,utilArmPos0);
//         }else{
//           moveUtilArms(0,0,joystickPositionY,lastUtilArmPos0);
//         }
//        }else if (joystickPositionY > 145 && !myPS3->getButtonPress(L3)){
//         int utilArmPos0 = (map(joystickPositionY, 127, 225, (lastUtilArmPos0+10), utilArmMin0));
//         if (lastUtilArmPos0 > utilArmPos0){
//           moveUtilArms(0,1,joystickPositionY,utilArmPos0);
//         }else{
//           moveUtilArms(0,1,joystickPositionY,lastUtilArmPos0);
//         }
//        }else{
//          #ifdef SERVO_EASING
//            //pwm2_Servo9.detach();
//          #else
//            pwm2.setPWM(9, 0, 0);
//          #endif
//       }
      }

     // UTILITY ARMS
     if(myPS3->getButtonPress(L2)&&myPS3->getButtonPress(L1)){
       randomDomeMovement();
//       int joystickPositionY = myPS3->getAnalogHat(LeftHatY);
//       if (joystickPositionY < 110 && !myPS3->getButtonPress(L3)){
//         int utilArmPos0 = (map(joystickPositionY, 0, 125, utilArmMax0, utilArmMin0)); //533? - 300
//         int utilArmPos1 = (map(joystickPositionY, 0, 125, utilArmMax1, utilArmMin1)); //87 -310
//         if (lastUtilArmPos1 > utilArmPos1){
//           moveUtilArms(0,0,joystickPositionY,utilArmPos0);
//           moveUtilArms(1,0,joystickPositionY,utilArmPos1);
//         }else{
//           moveUtilArms(0,0,joystickPositionY,lastUtilArmPos0);
//           moveUtilArms(1,0,joystickPositionY,lastUtilArmPos1);
//         }
//        }else if (joystickPositionY > 125 && !myPS3->getButtonPress(L3)){
//         int utilArmPos0 = (map(joystickPositionY, 127, 225, utilArmMin0, (lastUtilArmPos0+10)));
//         int utilArmPos1 = (map(joystickPositionY, 127, 225, (lastUtilArmPos1-10), utilArmMin1));
//         if (lastUtilArmPos1 < utilArmPos1){
//           moveUtilArms(0,1,joystickPositionY,utilArmPos0);
//           moveUtilArms(1,1,joystickPositionY,utilArmPos1);
//         }else{
//           moveUtilArms(0,1,joystickPositionY,lastUtilArmPos0);
//           moveUtilArms(1,1,joystickPositionY,lastUtilArmPos1);
//         }
//        }else{
//          #ifdef SERVO_EASING
//            //pwm2_Servo8.detach();
//            //pwm2_Servo9.detach();
//          #else
//            pwm2.setPWM(8, 0, 0);
//            pwm2.setPWM(9, 0, 0);
//          #endif
//       }
//     }else if (!myPS3->getButtonPress(L1)){
//          #ifdef SERVO_EASING
//            //pwm2_Servo8.detach();
//            //pwm2_Servo9.detach();
//          #else
//            pwm2.setPWM(8, 0, 0);
//            pwm2.setPWM(9, 0, 0);
//          #endif
     }

    #pragma mark -
    #pragma mark    Buzz Saw

    // BUZZ SAW

      if(myPS3->getButtonPress(PS)&&myPS3->getButtonClick(CIRCLE)){

      }

    // CENTER DOME
    if (!myPS3->getButtonPress(L2)&&!myPS3->getButtonPress(L1)&&!myPS3->getButtonPress(PS)){
      if (myPS3->getButtonClick(CIRCLE)){
        centerDomeSpeed = 50;
      }else if (myPS3->getButtonClick(CROSS)){
        centerDomeSpeed = -50;
      }
    }

#pragma mark -
#pragma mark InterfaceArm
    // Chopper Interface Arm/////////////////////////////////////////////

      if(myPS3->getButtonPress(L2)&&myPS3->getButtonPress(L1)&&myPS3->getButtonClick(CIRCLE)){

        if (!InterfaceArmActivated){
          Serial.println("Interface Activated");

            InterfaceArmActivated = true;

            maestrobody.restartScript(INTERFACE_SEQUENCE);
        }else{
          Serial.println("Interface Arm DeActivated");

          maestrobody.restartScript(INTERFACE_CLOSE);
          InterfaceArmActivated = false;
        }

      }


#pragma mark  -
#pragma mark MiniSaw

    // MiniSaw Arm

      if(myPS3->getButtonPress(L2)&&myPS3->getButtonPress(L1)&&myPS3->getButtonClick(CROSS)){
      //if(myPS3->getButtonClick(CROSS)){
        Serial.println("MiniSaw Arm");
        if (!miniSawActivated){

        	maestrobody.restartScript(MINISAW_SEQUENCE);
          miniSawActivated = true;

        }else{

        	maestrobody.restartScript(MINISAW_PULSE);
        	maestrobody.restartScript(MINISAW_CLOSE);
          miniSawActivated = false;
        }
      }

    // ZAPPER

      if(myPS3->getButtonPress(PS)&&myPS3->getButtonClick(CROSS)){
         if (!zapperActivated){

            zapperActivated = true;
         }else if (zapperActivated){
            zapperActivated = false;

          }
       }


    // Extinguisher
    if(myPS3->getButtonPress(PS)&&myPS3->getButtonClick(L3)){

    }

    // Disiabe the DriveStick
    if(myPS3->getButtonPress(L2)&&!myPS3->getButtonPress(L1)&&myPS3->getButtonClick(CROSS)){
      isStickEnabled = false;
      //sfx.playTrack("T07     OGG");
    }

    // Enable the DriveStick
    if(myPS3->getButtonPress(L2)&&!myPS3->getButtonPress(L1)&&myPS3->getButtonClick(CIRCLE)){
      isStickEnabled = true;
      //sfx.playTrack("T08     OGG");;
    }

    // Auto Dome Off
    if(myPS3->getButtonPress(L1)&&!myPS3->getButtonPress(L2)&&myPS3->getButtonClick(CROSS)){
      isAutomateDomeOn = false;
      automateAction = 0;
    }

    // Auto Dome On
    if(myPS3->getButtonPress(L1)&&!myPS3->getButtonPress(L2)&&myPS3->getButtonClick(CIRCLE)){
      isAutomateDomeOn = true;
    }
}

void processCommands(){
   if (PS3Nav->PS3NavigationConnected) ps3ProcessCommands(PS3Nav);
   if (PS3Nav2->PS3NavigationConnected) ps3ProcessCommands(PS3Nav2);
}


// ==============================================================
//             START UP AND OTHER IMPORTANT FUNCTIONS
// ==============================================================


String getLastConnectedBtMAC(){
    String btAddress = "";
    for(int8_t i = 5; i > 0; i--){
        if (Btd.disc_bdaddr[i]<0x10){
            btAddress +="0";
        }
        btAddress += String(Btd.disc_bdaddr[i], HEX);
        btAddress +=(":");
    }
    btAddress += String(Btd.disc_bdaddr[0], HEX);
    btAddress.toUpperCase();
    return btAddress;
}




void onInitPS3(){
    Serial.print(F("\r\nGot to onInitPS3"));
    String btAddress = getLastConnectedBtMAC();
    PS3Nav->setLedOn(LED1);
    isPS3NavigatonInitialized = true;
    badPS3Data = 0;
    trigger.trigger(23);
    //maestrobody.restartScript(3);
    #ifdef SHADOW_DEBUG
      output += "\r\nBT Address of Last connected Device when Primary PS3 Connected: ";
      output += btAddress;
      if (btAddress == PS3MoveNavigatonPrimaryMAC){
          output += "\r\nWe have our primary controller connected.\r\n";

      }else{
          output += "\r\nWe have a controller connected, but it is not designated as \"primary\".\r\n";
      }
    #endif
}




void onInitPS3Nav2(){
    String btAddress = getLastConnectedBtMAC();
    PS3Nav2->setLedOn(LED1);
    isSecondaryPS3NavigatonInitialized = true;
    badPS3Data = 0;
    //sfx.playTrack("T12     OGG");

    //TODO:Fix the error when uncommenting line below
    if (btAddress == PS3MoveNavigatonPrimaryMAC) swapPS3NavControllers();
    #ifdef SHADOW_DEBUG
      output += "\r\nBT Address of Last connected Device when Secondary PS3 Connected: ";
      output += btAddress;
      if (btAddress == PS3MoveNavigatonPrimaryMAC){
          output += "\r\nWe have our primary controller connecting out of order.  Swapping locations\r\n";
      }else{
          output += "\r\nWe have a secondary controller connected.\r\n";
      }
    #endif
}

void swapPS3NavControllers(){
    PS3BT* temp = PS3Nav;
    PS3Nav = PS3Nav2;
    PS3Nav2 = temp;
    //Correct the status for Initialization
    boolean tempStatus = isPS3NavigatonInitialized;
    isPS3NavigatonInitialized = isSecondaryPS3NavigatonInitialized;
    isSecondaryPS3NavigatonInitialized = tempStatus;
    //Must relink the correct onInit calls
    PS3Nav->attachOnInit(onInitPS3);
    PS3Nav2->attachOnInit(onInitPS3Nav2);
}


/*
void initAndroidTerminal(){
    //Setup for Bluetooth Serial Monitoring
    if (SerialBT.connected){
        if (firstMessage){
            firstMessage = false;
            SerialBT.println(F("Hello from S.H.A.D.O.W.")); // Send welcome message
        }
    }else{
        firstMessage = true;
    }
}

void flushAndroidTerminal(){
    if (output != ""){
        if (Serial) Serial.println(output);
        if (SerialBT.connected){
            SerialBT.println(output);
            SerialBT.send();
        output = ""; // Reset output string
        }
    }
}
*/
int get_int_len (int value){
  int l=1;
  while(value>9){ l++; value/=10; }
  return l;
}



// =======================================================================================
//          Print Output Function
// =======================================================================================

void printOutput()
{
    if (output != "")
    {
        if (Serial) Serial.println(output);
        output = ""; // Reset output string
    }
}
//+++++++++++++++++++++++++++++++++NEOPIXEL SETUP++++++++++++++++++
void allPatterns(NeoPatterns * aLedsPtr) {
      static int8_t sState = 1; //was 0, but Case 0 is busted

      uint8_t tDuration = random(40, 81);
      uint8_t tColor = random(255);

      switch (sState) {
      case 0:
          // simple scanner
          aLedsPtr->clear();
          aLedsPtr->ScannerExtended(NeoPatterns::Wheel(tColor), 5, tDuration, 2, FLAG_SCANNER_EXT_CYLON);
          break;
      case 1:
          // rocket and falling star - 2 times bouncing
          aLedsPtr->ScannerExtended(NeoPatterns::Wheel(tColor), 7, tDuration, 2,
          FLAG_SCANNER_EXT_ROCKET | FLAG_SCANNER_EXT_START_AT_BOTH_ENDS, (tDuration & DIRECTION_DOWN));
          break;
      case 2:
          // 1 times rocket or falling star
          aLedsPtr->clear();
          aLedsPtr->ScannerExtended(COLOR32_WHITE_HALF, 7, tDuration / 2, 0, FLAG_SCANNER_EXT_VANISH_COMPLETE,
                  (tDuration & DIRECTION_DOWN));
          break;
      case 3:
          aLedsPtr->RainbowCycle(20);
          break;
      case 4:
          aLedsPtr->Stripes(COLOR32_WHITE_HALF, 5, NeoPatterns::Wheel(tColor), 3, tDuration, 2 * aLedsPtr->numPixels(),
                (tDuration & DIRECTION_DOWN));
          break;
      case 5:
          // old TheaterChase
          aLedsPtr->Stripes(NeoPatterns::Wheel(tColor), 1, NeoPatterns::Wheel(tColor + 0x80), 2, tDuration / 2,
                2 * aLedsPtr->numPixels(), (tDuration & DIRECTION_DOWN));
          break;
      case 6:
          aLedsPtr->Fade(COLOR32_RED, COLOR32_BLUE, 32, tDuration);
          break;
      case 7:
          aLedsPtr->ColorWipe(NeoPatterns::Wheel(tColor), tDuration);
          break;
      case 8:
          // clear pattern
          aLedsPtr->ColorWipe(COLOR32_BLACK, tDuration, FLAG_DO_NOT_CLEAR, DIRECTION_DOWN);
          break;
      case 9:
          // Multiple falling star
          initMultipleFallingStars(aLedsPtr, COLOR32_WHITE_HALF, 7, tDuration / 2, 3, &allPatterns);
          break;
      case 10:
          if ((aLedsPtr->PixelFlags & PIXEL_FLAG_GEOMETRY_CIRCLE) == 0) {
              //Fire
              aLedsPtr->Fire(tDuration * 2, tDuration / 2);
          } else {
              // start at both end
              aLedsPtr->ScannerExtended(NeoPatterns::Wheel(tColor), 5, tDuration, 0,
              FLAG_SCANNER_EXT_START_AT_BOTH_ENDS | FLAG_SCANNER_EXT_VANISH_COMPLETE);
          }

          sState = 1;  //Case 0 is busted so skip it-1; // Start from beginning
          break;
      default:
          Serial.println("ERROR");
          break;
      }
      sState++;
}

#ifdef NEOPIXEL_TEST
 /************************************************************************************************************
   * Put your own pattern code here
   * Provided are sample implementation not supporting initial direction DIRECTION_DOWN
   * Full implementation of this functions can be found in the NeoPatterns.cpp file of the NeoPatterns library
   ************************************************************************************************************/

  /*
   * set all pixel to aColor1 and let a pixel of color2 move through
   * Starts with all pixel aColor1 and also ends with it.
   */
  void UserPattern1(NeoPatterns * aNeoPatterns, color32_t aPixelColor, color32_t aBackgroundColor, uint16_t aIntervalMillis,
          uint8_t aDirection) {
      /*
       * Sample implementation not supporting DIRECTION_DOWN
       */
      aNeoPatterns->ActivePattern = PATTERN_USER_PATTERN1;
      aNeoPatterns->Interval = aIntervalMillis;
      aNeoPatterns->Color1 = aPixelColor;
      aNeoPatterns->LongValue1.BackgroundColor = aBackgroundColor;
      aNeoPatterns->Direction = aDirection;
      aNeoPatterns->TotalStepCounter = aNeoPatterns->numPixels() + 1;
      aNeoPatterns->ColorSet(aBackgroundColor);
      aNeoPatterns->show();
      aNeoPatterns->lastUpdate = millis();
  }

  /*
   * @return - true if pattern has ended, false if pattern has NOT ended
   */
  bool UserPattern1Update(NeoPatterns * aNeoPatterns, bool aDoUpdate) {
      /*
       * Sample implementation not supporting initial direction DIRECTION_DOWN
       */
      if (aDoUpdate) {
          if (aNeoPatterns->decrementTotalStepCounterAndSetNextIndex()) {
              return true;
          }
      }

      for (uint16_t i = 0; i < aNeoPatterns->numPixels(); i++) {
          if (i == aNeoPatterns->Index) {
              aNeoPatterns->setPixelColor(i, aNeoPatterns->Color1);
          } else {
              aNeoPatterns->setPixelColor(i, aNeoPatterns->LongValue1.BackgroundColor);
          }
      }

      return false;
  }

  /*
   * let a pixel of aColor move up and down
   * starts and ends with all pixel cleared
   */
  void UserPattern2(NeoPatterns * aNeoPatterns, color32_t aColor, uint16_t aIntervalMillis, uint16_t aRepetitions,
          uint8_t aDirection) {
      /*
       * Sample implementation not supporting DIRECTION_DOWN
       */
      aNeoPatterns->ActivePattern = PATTERN_USER_PATTERN2;
      aNeoPatterns->Interval = aIntervalMillis;
      aNeoPatterns->Color1 = aColor;
      aNeoPatterns->Direction = aDirection;
      aNeoPatterns->Index = 0;
      // *2 for up and down. (aNeoPatterns->numPixels() - 1) do not use end pixel twice.
      // +1 for the initial pattern with end pixel. + 2 for the first and last clear pattern.
      aNeoPatterns->TotalStepCounter = ((aRepetitions + 1) * 2 * (aNeoPatterns->numPixels() - 1)) + 1 + 2;
      aNeoPatterns->clear();
      aNeoPatterns->show();
      aNeoPatterns->lastUpdate = millis();
  }

  /*
   * @return - true if pattern has ended, false if pattern has NOT ended
   */
  bool UserPattern2Update(NeoPatterns * aNeoPatterns, bool aDoUpdate) {
      /*
       * Sample implementation
       */
      if (aDoUpdate) {
          // clear old pixel
         //aNeoPatterns->setPixelColor(aNeoPatterns->Index, COLOR32_BLACK);
         aNeoPatterns->setPixelColor(aNeoPatterns->Index, COLOR_YELLOW);

          if (aNeoPatterns->decrementTotalStepCounterAndSetNextIndex()) {
              return true;
          }
          /*
           * Next index
           */
          if (aNeoPatterns->Direction == DIRECTION_UP) {
              // do not use top pixel twice
              if (aNeoPatterns->Index == (aNeoPatterns->numPixels() - 1)) {
                  aNeoPatterns->Direction = DIRECTION_DOWN;
              }
          } else {
              // do not use bottom pixel twice
              if (aNeoPatterns->Index == 0) {
                  aNeoPatterns->Direction = DIRECTION_UP;
              }
          }
      }
      /*
       * Refresh pattern
       */
      if (aNeoPatterns->TotalStepCounter != 1) {
          // last pattern is clear
          aNeoPatterns->setPixelColor(aNeoPatterns->Index, aNeoPatterns->Color1);
      }
      return false;
  }


  void UserPatternOFF(NeoPatterns * aNeoPatterns, color32_t aColor, uint16_t aIntervalMillis, uint16_t aRepetitions,
          uint8_t aDirection) {
      /*
       * Sample implementation not supporting DIRECTION_DOWN
       */
      aNeoPatterns->ActivePattern = PATTERN_NONE;
      aNeoPatterns->Interval = aIntervalMillis;
      aNeoPatterns->Color1 = aColor;
      aNeoPatterns->Direction = aDirection;
      aNeoPatterns->Index = 0;
      // *2 for up and down. (aNeoPatterns->numPixels() - 1) do not use end pixel twice.
      // +1 for the initial pattern with end pixel. + 2 for the first and last clear pattern.
      aNeoPatterns->TotalStepCounter = ((aRepetitions + 1) * 2 * (aNeoPatterns->numPixels() - 1)) + 1 + 2;
      aNeoPatterns->clear();
      aNeoPatterns->show();
      aNeoPatterns->lastUpdate = millis();
  }

  /*
   * @return - true if pattern has ended, false if pattern has NOT ended
   */
  bool UserPatternOFFUpdate(NeoPatterns * aNeoPatterns, bool aDoUpdate) {
      /*
       * Sample implementation
       */
      if (aDoUpdate) {
          // clear old pixel
         //aNeoPatterns->setPixelColor(aNeoPatterns->Index, COLOR32_BLACK);
         aNeoPatterns->setPixelColor(aNeoPatterns->Index, COLOR_YELLOW);

          if (aNeoPatterns->decrementTotalStepCounterAndSetNextIndex()) {
              return true;
          }
          /*
           * Next index
           */
          if (aNeoPatterns->Direction == DIRECTION_UP) {
              // do not use top pixel twice
              if (aNeoPatterns->Index == (aNeoPatterns->numPixels() - 1)) {
                  aNeoPatterns->Direction = DIRECTION_DOWN;
              }
          } else {
              // do not use bottom pixel twice
              if (aNeoPatterns->Index == 0) {
                  aNeoPatterns->Direction = DIRECTION_UP;
              }
          }
      }
      /*
       * Refresh pattern
       */
      if (aNeoPatterns->TotalStepCounter != 1) {
          // last pattern is clear
          aNeoPatterns->setPixelColor(aNeoPatterns->Index, aNeoPatterns->Color1);
      }
      return false;
  }
  /*
   * Handler for testing your own patterns
   */
  void ownPatterns(NeoPatterns * aLedsPtr) {
      static int8_t sState = 0;

      uint8_t tDuration = random(20, 120);
      uint8_t tColor = random(255);
      uint8_t tRepetitions = random(2);

      switch (sState) {
      case 0:
          UserPattern1(aLedsPtr, COLOR32_RED_HALF, NeoPatterns::Wheel(tColor), tDuration, FORWARD);
          break;

      case 1:
          UserPattern2(aLedsPtr, NeoPatterns::Wheel(tColor), tDuration, tRepetitions, FORWARD);
          sState = -1; // Start from beginning
          break;

      default:
          Serial.println("ERROR");
          break;
      }

      sState++;
  }
  //*/
//#else
#endif

//void pixelsOff(NeoPatterns * aLedsPtr){
//	UserPattern2(aLedsPtr, NeoPatterns::Wheel(tColor), tDuration, tRepetitions, FORWARD);
//
//}

//+++++++++++++++++++++END NEOPIXEL++++++++++++++++++++++++++++++

// ==============================================================
//                      START UP SEQUENCE
// ==============================================================
void setup(){
    //Debug Serial for use with USB Debugging
    Serial.begin(115200);
    while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
    if (Usb.Init() == -1){
        Serial.print(F("\r\nOSC did not start"));
        while (1); //halt
    }
    Serial.print(F("\r\nBluetooth Library Started"));
    output.reserve(200); // Reserve 200 bytes for the output string

    //Setup for PS3
    PS3Nav->attachOnInit(onInitPS3); // onInit() is called upon a new connection - you can call the function whatever you like
    PS3Nav2->attachOnInit(onInitPS3Nav2);
    Serial1.begin(9600);
    Serial.print(F("\r\nEnd PS3 Connection"));

    //Setup for Serial2:: Motor Controllers - Syren (Dome) and Sabertooth (Feet)
    Serial2.begin(motorControllerBaudRate);

    //SyR->setRamping(1980);
	#ifdef SOFTWARE_SERIAL
		maestroSerial.begin(9600); //Set virtual port up for maestrobody
	#endif

    #ifdef EEBEL_TEST
    //Trying to see if R2-D2 SHADOW-MD code helps.  R5 tends to crash and not connect sometimes
    //Setup for Serial2:: Motor Controllers - Sabertooth (Feet)
      ST->autobaud();          // Send the autobaud command to the Sabertooth controller(s).
      ST->setTimeout(10);      //DMB:  How low can we go for safety reasons?  multiples of 100ms
      ST->setDeadband(driveDeadBandRange);
      ST->stop();
      Serial.print(F("\r\nEnd Eebel SaberTooth SetUp"));
      SyR->autobaud();
      SyR->setTimeout(20);      //DMB:  How low can we go for safety reasons?  multiples of 100ms
      SyR->stop();
      Serial.print(F("\r\nEnd Eebel Syren SetUp"));
      Serial3.begin(9600); //
    #else
      //Original R5 code
      SyR->autobaud();
      //SyR->setRamping(1980);
      SyR->setTimeout(300);      //DMB:  How low can we go for safety reasons?  multiples of 100ms
      Serial.print(F("\r\nEnd Syren SetUp"));

      Serial3.begin(9600); //

      //Setup for Sabertooth / Foot Motors
      ST->autobaud();          // Send the autobaud command to the Sabertooth controller(s).
      ST->setTimeout(300);      //DMB:  How low can we go for safety reasons?  multiples of 100ms
      ST->setDeadband(driveDeadBandRange);
      ST->stop();
      Serial.print(F("\r\nEnd SaberTooth SetUp"));
    #endif



    // ZAPPER and BUZZSAW SETUP
    pinMode(smDirectionPin, OUTPUT);
    pinMode(smStepPin, OUTPUT);
    pinMode(smEnablePin, OUTPUT);
    digitalWrite(smEnablePin, HIGH);


    pinMode(PIN_ZAPPER,OUTPUT); // Zapper Relay

    pinMode(PIN_BUZZSAW, OUTPUT); // BUZZ SAW PIN
    digitalWrite(PIN_BUZZSAW, LOW);
    pinMode(PIN_BUZZSLIDELIMIT, INPUT_PULLUP); //BuzzSaw Inner Limit switch

    pinMode(PIN_BADMOTIVATOR, OUTPUT); // MOTIVATOR PIN
    digitalWrite(PIN_BADMOTIVATOR, LOW);

    // DOME CENTER SWITCH
    pinMode(PIN_DOMECENTER, INPUT_PULLUP); // DOME CENTER PIN

    centerDomeLoop();
    closeAll();






    //PLay MP3 Trigger sound
    //MP3Trigger trigger;//make trigger object
    // Start serial communication with the trigger (over Serial)
    trigger.setup(&Serial3);
    trigger.setVolume(droidSoundLevel);//Amount of attenuation  higher=LOWER volume..ten is pretty loud 0-127
    trigger.trigger(255);
    //delay(50);
    //trigger.setVolume(10);//Amount of attenuation  higher=LOWER volume..ten is pretty loud
    //sfx.playTrack("T06     OGG");


    //------------NeoPixel Setup--------------------
    bar16.begin(); // This initializes the NeoPixel library.
    //bar16.ColorWipe(COLOR32(0, 0, 02), 50, REVERSE); // light Blue
    bar16.ColorWipe(0, 0, 0);
    Serial.print(F("\r\nEnd NeoPixel SetUp"));

}




// ==============================================================
//                          MAIN PROGRAM
// ==============================================================
void loop(){

    if (!droidFreakOut){
      if (!readUSB()) return; //Check if we have a fault condition. If we do, we want to ensure that we do NOT process any controller data!!!
      footMotorDrive();
      if (!readUSB()) return; // Not sure why he has this twice, and in this order, but better safe then sorry?
      domeDrive();
      //Serial.println(domeSpinning);
      processCommands();
    }

    //badMotivatorLoop();

    //freakOut();
    soundControl();
    if (cardLightsOn){
    bar16.update();
    }


}



