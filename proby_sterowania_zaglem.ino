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

//zmienne do obsługi przerwań impulsatora
unsigned long time = 0;
long count = 0;
long num = 0;


volatile int encoderPos = 0; // Aktualna pozycja enkodera (0-19)
int prevEncoderPos = 0;

int windDirectionDegrees = 0; // 0-359 stopni

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

  while (num != count) {
    num = count;
    Serial.println(num);
  }

  unsigned long SW1_value = pulseIn(SW1, HIGH, 50000);  // Funkcja do odczytywania
  Serial.print(SW1_value);                                     // wypisuje wartość w mikrosekundach (1000–2000us)
  Serial.print("    ");
  delay(10);


  /*
  TODO:
  # ogarnij jakie pozycje switcha robią co
  # zaimplementuj zerowanie wiatrowskazu
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
    encoderPos = (encoderPos + 1) % 20;
  }
  time = millis();
}


int calculateSailAngle(int windAngle) {
  int servoAngle = 90;

  // Tutaj wstaw logikę z tabeli optymalnych kątów żagla
  if (windAngle >= 0 && windAngle < 35) {
    // Strefa martwa - żagiel luźno, lub zaciągnięty w celu przygotowania do zwrotu
    servoAngle = 90; // Przykładowo, minimalnie zaciągnięty
  } else if (windAngle >= 45 && windAngle < 90) {
    // Ostra żegluga - żagiel bardziej zaciągnięty
    servoAngle = map(windAngle, 45, 90, 45, 90); // Mapowanie kąta wiatru na kąt serwa
  } else if (windAngle >= 90 && windAngle < 135) {
    // Wiatr boczny - żagiel coraz bardziej otwarty
    servoAngle = map(windAngle, 90, 135, 90, 135);
  } else { // windAngle >= 135 && windAngle <= 180
    // Wiatr pełny / z rufy - żagiel maksymalnie otwarty
    servoAngle = map(windAngle, 135, 180, 135, 170); // Ustawienie nieco mniej niż 180, aby uniknąć blokady
  }

  // Zapewnij, że kąt serwa mieści się w zakresie 0-180
  servoAngle = constrain(servoAngle, 0, 180);
  return servoAngle;
}