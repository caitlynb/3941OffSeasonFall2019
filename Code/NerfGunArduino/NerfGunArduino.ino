#include <Adafruit_DotStar.h>
#include <SPI.h>

const bool enablemotors = true;

#define NUMPIXELS 14 // Number of LEDs in strip

Adafruit_DotStar ledstrip = Adafruit_DotStar(NUMPIXELS, DOTSTAR_BGR);

#define spinmotor1pin 9 // off = high
#define spinmotor2pin 8 // on = low
#define feedmotorpin 7 

#define triggerpin 5 // all tied high (pullup)
#define readypin 3
#define magazinepin 4

#define riospinuppin 2 // floating
#define riofirepin 6

long motorspinuptime = 0;
long motorfiretime = 0;
long cooldowntimer = 0;
long timenow = 0;
const long motorspinupmaxtime = 10000;
const long motorfiremaxtime = 5000;
const long motorcooldowntime = 4000;

bool overheat = false;

void setup() {

  ledstrip.begin(); // Initialize pins for output
  ledstrip.show();  // Turn all LEDs off ASAP

  pinMode(spinmotor1pin, OUTPUT);
  pinMode(spinmotor2pin, OUTPUT);
  pinMode(feedmotorpin, OUTPUT);
  pinMode(triggerpin, INPUT);
  pinMode(readypin, INPUT);
  pinMode(magazinepin, INPUT);
  pinMode(riospinuppin, INPUT);
  pinMode(riofirepin, INPUT);

  digitalWrite(spinmotor1pin, HIGH);
  digitalWrite(spinmotor2pin, HIGH);
  digitalWrite(feedmotorpin, HIGH);

  
}

const byte seq_booting = 255;
const byte seq_safe = 0;
const byte seq_spinningup = 1;
const byte seq_spinningready = 2;
const byte seq_firing = 3;
const byte seq_spinningdown = 4;
const byte seqcooldown = 5;

const byte maxsequenceval = 4;

uint32_t safecolor = ledstrip.Color(0,255,0);
uint32_t spinningreadycolor = ledstrip.Color(0,125,255);
uint32_t firingcolor = ledstrip.Color(255,255,0);


byte spinseqstep = 0;

byte sequence = seq_safe;

//long lastseqchangetime =0;



void cooldown(){
  safesystem();
  int n=0;
  if(overheat == true){
    for(n=0; n<NUMPIXELS; n++){
      if(n%2==0){
        ledstrip.setPixelColor(n, 255,0,0);
      } else {
        ledstrip.setPixelColor(n,255,255,255);
      }
    }
  }
  ledstrip.show();
  delay(motorcooldowntime);
//  timenow = millis();
  motorspinuptime = 0;
  motorfiretime = 0;
  overheat = false;
  sequence = seq_safe;
}

void show_safe(){
  safesystem();
  int n=0;
  if(overheat == true){
    for(n=0; n<NUMPIXELS; n++){
      ledstrip.setPixelColor(n, 255,255,255);
    }
  } else {
    for(n=0; n<NUMPIXELS; n++){
      ledstrip.setPixelColor(n, safecolor);
    }
  }
  ledstrip.show();
  timenow = millis();
  motorspinuptime = 0;
  motorfiretime = 0;
}

void show_spinningup(){
  byte i;
  timenow = millis();
  if(issafetospin() && timenow - motorspinuptime < motorspinupmaxtime){
    if(enablemotors){
      spinup(); // turn on spin motors
    }
    for(i = spinseqstep; i < 250; i+= 5){
      spinseqstep = i;
      if(!issafetospin()){
        // abort
        sequence = seq_spinningdown;
        safesystem();
        break;
      }
      for(int n=0; n<NUMPIXELS; n++){
        ledstrip.setPixelColor(n, 255-i, 0, i);
      }
      ledstrip.show();
      delay(15);
    }
    if(spinseqstep >= 245){
      sequence = seq_spinningready;
    }
  } else { // spin up time check
    overheat = true;
    safesystem(); // spin down
    spinseqstep = 250;
    sequence = seq_spinningdown;
  }
}

void show_spinningready(){
  timenow = millis();
  if(!issafetospin()){
    // abort
    sequence = seq_spinningdown;
    safesystem();
  }
  if( timenow - motorspinuptime > motorspinupmaxtime){
    overheat = true;
    sequence = seq_spinningdown;
    spinseqstep = 250;
    safesystem();
  }
  for(int n=0; n<NUMPIXELS; n++){
    ledstrip.setPixelColor(n, spinningreadycolor);
  }
  ledstrip.show();
  //delay(1000);
}

void show_firing(){
  
  if(!issafetospin()){
    // abort
    spinseqstep = 250;
    sequence = seq_spinningdown;
    safesystem();
  } else if(millis() - motorfiretime > motorfiremaxtime){
    overheat = true;
    spinseqstep = 250;
    sequence = seq_spinningdown;
    safesystem();
  } else if(digitalRead(riofirepin) == LOW) {
    sequence = seq_spinningready;
    digitalWrite(feedmotorpin, HIGH);
  } else {
    if(enablemotors){
      digitalWrite(feedmotorpin, LOW);
    }
    for(int n=0; n<NUMPIXELS; n++){
      ledstrip.setPixelColor(n, firingcolor);
    }
    ledstrip.show();
  }
}

void show_spinningdown(){
  byte i;
  safesystem(); // turnoff motors
  for(i = spinseqstep; i > 0; i-= 5){
    spinseqstep = i;
//    if(issafetospin() && overheat== false){
//      // resume spinning
////      spinseqstep=0;
//      sequence = seq_spinningup;
////      safesystem();
//      break;
//    }
    for(int n=0; n<NUMPIXELS; n++){
      ledstrip.setPixelColor(n, 255-i, 0, i);
    }
    ledstrip.show();
    delay(30);
  }
  sequence = seq_safe;
  spinseqstep = 0;
  if(overheat == true){
    sequence = seqcooldown;
    cooldowntimer = motorcooldowntime;
  }
}

bool issafetospin(){
  bool ret = true;
  if(digitalRead(magazinepin) == HIGH){
    ret = false;
  }
  if(digitalRead(riospinuppin) == LOW){
    ret = false;
  }
  return ret;
}

void safesystem(){
  digitalWrite(feedmotorpin, HIGH);
  digitalWrite(spinmotor1pin, HIGH);
  digitalWrite(spinmotor2pin, HIGH);
}

void spinup(){
  digitalWrite(spinmotor1pin, LOW);
  digitalWrite(spinmotor2pin, LOW);
}

void loop() {

  switch(sequence){
    case seq_safe:
      show_safe();
      break;
    case seq_spinningup:
      show_spinningup();
      break;
    case seq_spinningready:
      show_spinningready();
      break;
    case seq_firing:
      show_firing();
      break;
    case seq_spinningdown:
      show_spinningdown();
      break;
    case seqcooldown:
      cooldown();
      break;
    default:
      show_safe();
  }
  delay(10);

    if(digitalRead(riospinuppin) == HIGH && sequence == seq_safe){
      if(issafetospin()){
        motorspinuptime = millis();
        sequence = seq_spinningup;
      }
    }
  
  //  if(digitalRead(riospinuppin) == HIGH && sequence == seq_spinningdown){
  //    if(millis() - motorspinuptime > 
  //  }
  
    if(digitalRead(riofirepin) == HIGH && sequence == seq_spinningready){
      if(issafetospin()){
        motorfiretime = millis();
        sequence = seq_firing;
      }
    }
    
    
}

