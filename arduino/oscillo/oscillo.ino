/*
 * Oscilloscope GPA787
 * 
 * Guy Gauthier (c) 2020
 */
 
#include <TimerOne.h>
#include <MCP492X.h> // Include the library

#define PIN_SPI_CHIP_SELECT_DAC 8 // Or any pin you'd like to use

MCP492X myDac(PIN_SPI_CHIP_SELECT_DAC);

int i=0,j=0,onde=0, Decal = 512;
int periode = 500; // usec.
double x=0.0, oldx = 0.0;
int oldX = 0, X = 0;
double t=0.0;
double dt = 0.00050;
double freq = 16;
double ampli = 2.0, decal = 2.5;
unsigned char entree0 = 14;
unsigned char entree1 = 15;
unsigned char entree2 = 16;
unsigned char entree3 = 17;
int readings[101], genera[101];
unsigned char readingPtr = 0;
unsigned char writingPtr = 0;
bool timeoutStatus;
bool triggered = false;
int trigSource = 0;
String serialRead;
int rempli = 0;
int timeK = 16;

void setup()
{
  Serial.begin(250000);//115200);
  Serial.setTimeout(20);
  
  myDac.begin();
  pinMode(entree0, INPUT);
  pinMode(entree1, INPUT);
  pinMode(entree2, INPUT);
  pinMode(entree3, INPUT);
  pinMode(10, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(2,LOW);
  Timer1.initialize(periode);         // initialize timer1, and set a 1/2 second period
  Timer1.pwm(9, 512);                // setup pwm on pin 9, 50% duty cycle
  Timer1.attachInterrupt(callback);  // attaches callback() as a timer overflow interrupt
}
 
void callback()
{
  // Générateur de fonction
  digitalWrite(10, digitalRead(10) ^ 1);
  oldx = x;
  if (onde<2) x = ampli*sin(2*PI*freq*t)+decal;
  if (onde==1){
    if (x>decal) x = ampli+decal;
    else x = decal-ampli;
  }
  if (onde==2){
    x = (-ampli)*(4.0*freq*(t-1.0/(2.0*freq)*floor(t*2.0*freq+0.5)))*pow(-1.0,floor(t*2.0*freq-0.5))+decal;
  }
  if (x>5.0) x = 5.0;
  if (x<0.0) x = 0.0;
  i = (int) (x/5.0*4096);
  // Write any value from 0-4095
  myDac.analogWrite(i);
  t += dt;
  
  // Oscilloscope

  oldX = X;      // Vérification du trigger.
  X = analogRead(entree0);
  if (triggered == false){
    j = 0;
    if ((oldX<Decal && X>Decal && trigSource==1) || (oldx<decal && x>decal && trigSource==0)) {
      triggered = true;
      digitalWrite(2,HIGH); 
      j = timeK;
    }
  }
  if (triggered == true){
    j++;
    if (j>=timeK){     // On échantillonne qu'au moment opportun
      if (rempli==0) { // Vérifions si on est en cours d'acquisition ou pas
        digitalWrite(3, digitalRead(3) ^ 1);
        readings[readingPtr] = X;
        genera[readingPtr] = i;
        readingPtr++;
        if (readingPtr>=101){
          rempli = 1;
          readingPtr = 0;
        }
      }
    j = 0;
    }
  }
    //  ampli = 5.0*analogRead(entree2)/1024.0/2.0;
    //Decal = analogRead(entree3);
    //decal = 5.0*Decal/1024.0;
}
 
void loop()
{
  if (rempli==1){ 
    timeoutStatus = true;

    while(timeoutStatus == true)
    {
      Serial.println("R?");
      serialRead = Serial.readString();
      if(serialRead == "K")
      {
        timeoutStatus = false;
      }
    }
    for (writingPtr = 0; writingPtr<101; writingPtr++)
    {
      Serial.write(highByte(readings[writingPtr]<<6));
    }
    for (writingPtr = 0; writingPtr<101; writingPtr++)
    {
      Serial.write(highByte(genera[writingPtr]<<4));
    }
    serialRead = Serial.readString();
    if((serialRead == "U") && (timeK<200))
    {
      timeK +=1;
    }
    if((serialRead == "D") && (timeK>1))
    {
      timeK -= 1;
    }
    if(serialRead == "1")
    {
      timeK =1;
    }
    if(serialRead == "2")
    {
      timeK = 2;
    }
    if(serialRead == "3")
    {
      timeK = 4;
    }
    if(serialRead == "4")
    {
      timeK = 8;
    }
    if(serialRead == "5")
    {
      timeK = 16;
    }
    if(serialRead == "6")
    {
      timeK = 32;
    }
    if(serialRead == "7")
    {
      timeK = 64;
    }
    if(serialRead == "8")
    {
      timeK = 128;
    }
    if(serialRead == "S")
    {
      freq *= 2;
      if (freq>256) freq=256;
    }
    if(serialRead == "A")
    {
      freq = freq / 2;
      if (freq<0.25) freq=0.25;
    }
    if(serialRead == "W")
    {
      onde=onde+1;
      if (onde>1) onde = 0;
    }
    if(serialRead == "E")
    {
      ampli-=0.25;
      if (ampli<0.25) ampli = 0.25;
    }
    if(serialRead == "R")
    {
      ampli+=0.25;
      if (ampli>2.5) ampli = 2.5;
    }
    if(serialRead == "T")
    {
      if (trigSource==0) trigSource = 1;
      else trigSource = 0;
    }
    //ampli = 5.0*analogRead(entree2)/1024.0/2.0;
    //Decal = analogRead(entree3);
    //decal = 5.0*Decal/1024.0;
    rempli = 0;
    triggered = false;
    digitalWrite(2,LOW);
  }
}
