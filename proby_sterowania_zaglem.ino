#include <Servo.h>

//To są piny do przerwań
#define IMP_INT1 2
#define IMP_INT2 3
//musi byc na tych IMP_INT1ch bo arduino na innych nie obsluguje

//Piny wejsciowe do PWM z pilota
#define SW1 5
#define SW2 6
#define JOY1 9
#define JOY2 10

//Pin do wysylania PWM na serwo 
#define SAIL_PIN 11

//Pin do wysterowania przekaźnika załączającego silnik 
#define RELAY_PIN 7

long sail_trim = 0;
unsigned long time = 0;
long count = 0;
long num = 0;
Servo sailServo;

void setup() {
  Serial.begin(9600);

  pinMode(IMP_INT1, INPUT);
  pinMode(IMP_INT2, INPUT);
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(JOY1, INPUT);
  pinMode(JOY2, INPUT);
  pinMode(SAIL_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);



  attachInterrupt(0, blinkA, LOW);
  attachInterrupt(1, blinkB, LOW);

  time = millis();

  sailServo.attach(SAIL_PIN);
}

void loop() {

  while (num != count) {
    num = count;
    Serial.println(num);
  }

  unsigned long SW1_value = pulseIn(SW1, HIGH, 50000);  // Funkcja do odczytywania
  Serial.print(SW1_value);                                     // wypisuje wartość w mikrosekundach (1000–2000us)
  Serial.print("    ");
  delay(10);

  if(SW1_value > 1200){
    digitalWrite(RELAY_PIN, HIGH);

  }
  else{
    digitalWrite(RELAY_PIN, LOW);
  }

  unsigned long SW2_value = pulseIn(SW2, HIGH, 50000);  // Funkcja do odczytywania
  Serial.print(SW2_value);                                     // wypisuje wartość w mikrosekundach (1000–2000us)
  Serial.print("    ");
  delay(10);



  unsigned long joy2_us = pulseIn(JOY2, HIGH, 50000);
  delay(20);
  
    int angle2 = map(joy2_us, 1000, 2000, 0, 180);
    angle2 = constrain(angle2, 0, 180);
    sailServo.write(angle2); 
    Serial.print("  JOY2 PWM: ");
    Serial.print(joy2_us);
    Serial.print(" -> Angle: ");
    Serial.println(angle2);
  
  delay(20);  
}

void blinkA() {
  if ((millis() - time) > 30){
    count++;
    
  }
  time = millis();
}

void blinkB() {
  if ((millis() - time) > 30)
    count--;
  time = millis();
}
