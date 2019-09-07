#include <Adafruit_DotStar.h>
#include <SPI.h>


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

const byte maxsequenceval = 4;

uint32_t safecolor = ledstrip.Color(0,255,0);
uint32_t spinningreadycolor = ledstrip.Color(0,0,255);
uint32_t firingcolor = ledstrip.Color(255,255,0);


byte spinseqstep = 0;

byte sequence = seq_spinningup;

long lastseqchangetime =0;

void loop() {
//  long timenow = millis();
//  if(timenow > lastseqchangetime+5000){
//    lastseqchangetime = timenow;
//    sequence += 1;
//    if(sequence > maxsequenceval)
//      sequence = 0;
//  }

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
    default:
      show_safe();
  }
  delay(10);

  if(digitalRead(riospinuppin) == HIGH && (sequence == seq_safe || sequence == seq_spinningdown)){
    if(issafetospin())
      sequence = seq_spinningup;
  }

  if(digitalRead(riofirepin) == HIGH && sequence == seq_spinningready){
//    for(int n=0; n<NUMPIXELS; n++){
//      ledstrip.setPixelColor(n,255,0,255);
//    }
//    ledstrip.show();
//    delay(100);
    if(issafetospin())
      sequence = seq_firing;
  }
  
}

void show_safe(){
  safesystem();
  int n=0;
  for(n=0; n<NUMPIXELS; n++){
    ledstrip.setPixelColor(n, safecolor);
  }
  ledstrip.show();
}

void show_spinningup(){
  byte i;
  spinup(); // turn on spin motors
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
    delay(10);
  }
  if(spinseqstep >= 250){
    sequence = seq_spinningready;
  }
}

void show_spinningready(){
  if(!issafetospin()){
    // abort
    sequence = seq_spinningdown;
    safesystem();
  }
  for(int n=0; n<NUMPIXELS; n++){
    ledstrip.setPixelColor(n, spinningreadycolor);
  }
  ledstrip.show();
}

void show_firing(){
  if(!issafetospin()){
    // abort
    sequence = seq_spinningdown;
    safesystem();
  } else if(digitalRead(riofirepin) == LOW) {
    sequence = seq_spinningready;
    digitalWrite(feedmotorpin, HIGH);
  } else {
    digitalWrite(feedmotorpin, LOW);
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
    if(issafetospin()){
      // resume spinning
      sequence = seq_spinningup;
      safesystem();
      break;
    }
    for(int n=0; n<NUMPIXELS; n++){
      ledstrip.setPixelColor(n, 255-i, 0, i);
    }
    ledstrip.show();
    delay(20);
  }
  if(spinseqstep <= 5){
    sequence = seq_safe;
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
