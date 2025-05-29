#include <Servo.h>

Servo sailServo;
const int ENCODER_PIN_A = 2; // Pin A enkodera
const int ENCODER_PIN_B = 3; // Pin B enkodera
const int SERVO_PIN = 9;     // Pin serwa żagla

volatile int encoderPos = 0; // Aktualna pozycja enkodera (0-19)
int prevEncoderPos = 0;

// Zmienne do przechowywania kąta wiatru
int windDirectionDegrees = 0; // 0-359 stopni

void setup() {
  Serial.begin(9600);
  sailServo.attach(SERVO_PIN);

  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  // Przypnij przerwania do pinów enkodera
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), readEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), readEncoder, CHANGE);

  // Początkowe ustawienie żagla (np. na luźno)
  sailServo.write(90); // Ustaw serwo w pozycji środkowej
}

void loop() {
  // Oblicz kierunek wiatru w stopniach
  // Zakładamy, że 0 impulsów to wiatr prosto z dziobu (lub inny ustalony kierunek)
  windDirectionDegrees = (encoderPos * 18) % 360;

  // Normalizuj kąt wiatru do zakresu 0-179, ponieważ żaglówka jest symetryczna
  // Np. wiatr z 270 stopni (z lewej burty) jest odpowiednikiem wiatru z 90 stopni (z prawej burty)
  int normalizedWindAngle = windDirectionDegrees;
  if (normalizedWindAngle > 180) {
    normalizedWindAngle = 360 - normalizedWindAngle;
  }
  
  // Ustaw serwo na podstawie kąta wiatru
  // Tutaj zastosuj swoją logikę mapowania kąta wiatru na kąt serwa
  int servoAngle = calculateSailAngle(normalizedWindAngle);
  sailServo.write(servoAngle);

  Serial.print("Encoder Position: ");
  Serial.print(encoderPos);
  Serial.print(" Wind Direction: ");
  Serial.print(windDirectionDegrees);
  Serial.print(" degrees. Normalized Wind Angle: ");
  Serial.print(normalizedWindAngle);
  Serial.print(" degrees. Servo Angle: ");
  Serial.println(servoAngle);

  delay(100); // Małe opóźnienie dla stabilności
}

// Funkcja obsługi przerwania dla enkodera
void readEncoder() {
  int valA = digitalRead(ENCODER_PIN_A);
  int valB = digitalRead(ENCODER_PIN_B);

  // Logika odczytu kierunku obrotu
  if (valA == LOW) {
    if (valB == LOW) { // Ruch do przodu (zakładając, że B opóźnia A)
      encoderPos = (encoderPos + 1) % 20;
    } else { // Ruch do tyłu
      encoderPos = (encoderPos - 1 + 20) % 20;
    }
  } else {
    if (valB == HIGH) { // Ruch do przodu
      encoderPos = (encoderPos + 1) % 20;
    } else { // Ruch do tyłu
      encoderPos = (encoderPos - 1 + 20) % 20;
    }
  }
}

// Funkcja obliczająca kąt serwa na podstawie kąta wiatru
int calculateSailAngle(int windAngle) {
  int servoAngle = 0;

  // Tutaj wstaw logikę z tabeli optymalnych kątów żagla
  if (windAngle >= 0 && windAngle < 45) {
    // Strefa martwa - żagiel luźno, lub zaciągnięty w celu przygotowania do zwrotu
    servoAngle = 10; // Przykładowo, minimalnie zaciągnięty
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