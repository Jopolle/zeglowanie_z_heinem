#include <Servo.h>

/*
TODO:
-skalowanie i modulo kąta obrotu impulsatora
-regulator P dla kąta obortu
-Maszyna stanów
  - Sterowanie zwykle
  - rufa lewy hals
  - rufa prawy hals
-dodać poprawkę na ster (jeżeli jest wolny pin PWM)
*/


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

//zmienne do obsługi przerwań impulsatora
unsigned long time = 0;
long count = 0;
long num = 0;



int hals = 0;
/*
0 - prawy hals
1 - lewy hals

*/

int stan_sterowania = 0;
/*
0 - manualne sterowanie
1 - automatyczne sterowanie (bez zwrotu)
2 - zwrot przez rufę lewy hals
3 - zwrot przez rufę prawy hals
*/


volatile int encoderPos = 10; // Aktualna pozycja enkodera (0-19)
int prevEncoderPos = 10;

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



  attachInterrupt(0, obrot_lewo , LOW);
  attachInterrupt(1, obrot_prawo, LOW);

  time = millis();

  sailServo.attach(SAIL_PIN);
}

void loop() {


  unsigned long SW1_value = pulseIn(SW1, HIGH, 50000);  // Funkcja do odczytywania
  Serial.print(SW1_value);                                     // wypisuje wartość w mikrosekundach (1000–2000us)
  Serial.print("    ");
  delay(10);


  /*
  TODO:

  -ogarnij jakie pozycje switcha robią co
    -pozycja na silnik
    -pozycja na jalowy
    -pozycja zerująca wiatrowskaz

  
  */
  if(SW1_value > 1200 && SW1_value < 1500){ // załączenie silnika
    digitalWrite(RELAY_PIN, HIGH);
  }
  else if(SW1_value > 1500){
    digitalWrite(RELAY_PIN, LOW);
  }
  else{
    digitalWrite(RELAY_PIN, LOW);
  }

  unsigned long SW2_value = pulseIn(SW2, HIGH, 50000);  // Funkcja do odczytywania
  Serial.print(SW2_value);                                     // wypisuje wartość w mikrosekundach (1000–2000us)
  Serial.print("    ");
  delay(10);



  unsigned long joy2_us = pulseIn(JOY2, HIGH, 50000);

  if(SW2_value > 1200)  stan_sterowania = 0;
  else stan_sterowania = 1;

  switch(stan_sterowania){
    case 0:
      int angle2 = map(joy2_us, 1000, 2000, 0, 180);
      angle2 = constrain(angle2, 0, 180);
      sailServo.write(angle2); 
      Serial.println("Reczne sterowanie zaglem");
      //Serial.print("  JOY2 PWM: ");
      //Serial.print(joy2_us);
      //Serial.print(" -> Angle: ");
      //Serial.println(angle2);
      break;

    case 1:
      sailServo.write(calculateSailAngle());
      Serial.println("Automatyczne sterowanie zaglem");
    break;

    case 2:
    break;

    case 3:
    break;
  }



  delay(20);
  
  int angle2 = map(joy2_us, 1000, 2000, 0, 180);
  angle2 = constrain(angle2, 0, 180);
  sailServo.write(angle2); 

  
  delay(20);  
}

void obrot_lewo() {
  if ((millis() - time) > 30){
    count++;
    encoderPos = (encoderPos + 1) % 20;
  }
  
  time = millis();
}

void obrot_prawo() {
  if ((millis() - time) > 30){
    count--;
    encoderPos = (encoderPos - 1 + 20) % 20;
  }
  time = millis();
}


int calculateSailAngle() {
  int servoAngle = map(encoderPos, 0, 19, 0, 180);
  servoAngle = constrain(servoAngle, 0, 180);
  return servoAngle;
}