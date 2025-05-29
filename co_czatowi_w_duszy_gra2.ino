#include <Servo.h>

// --- Stałe i zmienne globalne ---
Servo sailServo;
const int ENCODER_PIN_A = 2; // Pin A enkodera
const int ENCODER_PIN_B = 3; // Pin B enkodera
const int SERVO_PIN = 9;     // Pin serwa żagla

volatile int encoderPos = 0; // Aktualna pozycja enkodera (0-19)

// Zmienne do przechowywania kąta wiatru
int windDirectionDegrees = 0;   // 0-359 stopni (bezwzględny kierunek wiatru)
int normalizedWindAngle = 0;    // 0-180 stopni (kąt wiatru względem osi łodzi, normalizowany)

// --- Parametry PID dla żagla ---
float Kp = 1.0; // Przykładowe wartości, do kalibracji!
float Ki = 0.01;
float Kd = 0.5;

float previousError = 0;
float integral = 0;
unsigned long lastTime;

// --- Zmienne dla serwa żagla ---
int currentSailServoAngle = 90; // Początkowa pozycja serwa (środek)

// --- Stany żaglówki do zwrotów ---
enum BoatState {
  SAILING,
  TACKING,      // Zwrot przez sztag
  JIBING_PREP,  // Przygotowanie do zwrotu przez rufę (zaciąganie żagla)
  JIBING_EXEC   // Wykonywanie zwrotu przez rufę (przejście rufy przez wiatr)
};
BoatState currentState = SAILING;

// --- Funkcje obsługi przerwań (ISR) ---
void readEncoder() {
  int valA = digitalRead(ENCODER_PIN_A);
  int valB = digitalRead(ENCODER_PIN_B);

  // Logika odczytu kierunku obrotu (zakładamy kierunek obrotu w zależności od fazy)
  if (valA == LOW) {
    if (valB == LOW) { // Ruch do przodu
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

// --- Funkcje pomocnicze ---

// Oblicza optymalny kąt żagla w stopniach na podstawie kąta wiatru
int calculateOptimalSailAngle(int windAngle) {
  // windAngle to normalizedWindAngle (0-180 stopni)
  int optimalSailAngle = 0;

  if (windAngle >= 0 && windAngle < 45) {
    // Strefa martwa (wiatr od dziobu) - żagiel minimalnie zaciągnięty / luźny
    optimalSailAngle = 10; // Przykładowo, zaciągnięty na 10 stopni (serwo 10)
  } else if (windAngle >= 45 && windAngle < 90) {
    // Ostra żegluga - żagiel stopniowo otwierany
    // Mapowanie z zakresu (45, 90) na (10, 80) dla serwa
    optimalSailAngle = map(windAngle, 45, 90, 10, 80); 
  } else if (windAngle >= 90 && windAngle < 135) {
    // Wiatr boczny - żagiel coraz bardziej otwarty
    // Mapowanie z zakresu (90, 135) na (80, 140) dla serwa
    optimalSailAngle = map(windAngle, 90, 135, 80, 140);
  } else { // windAngle >= 135 && windAngle <= 180
    // Wiatr pełny / z rufy - żagiel maksymalnie otwarty
    // Mapowanie z zakresu (135, 180) na (140, 170) dla serwa
    optimalSailAngle = map(windAngle, 135, 180, 140, 170);
  }

  // Ogranicz do zakresu pracy serwa (0-180)
  return constrain(optimalSailAngle, 0, 180);
}

// --- Funkcja PID ---
float calculatePIDOutput(int setpoint, int processVariable) {
  unsigned long now = millis();
  float timeChange = (float)(now - lastTime) / 1000.0; // Czas w sekundach

  float error = setpoint - processVariable;
  integral += error * timeChange;
  float derivative = (error - previousError) / timeChange;

  float output = Kp * error + Ki * integral + Kd * derivative;

  previousError = error;
  lastTime = now;

  return output;
}

// --- Setup ---
void setup() {
  Serial.begin(9600);
  sailServo.attach(SERVO_PIN);

  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  // Przypnij przerwania do pinów enkodera
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), readEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), readEncoder, CHANGE);

  sailServo.write(currentSailServoAngle); // Ustaw serwo w początkowej pozycji
  lastTime = millis();
}

// --- Loop ---
void loop() {
  // Oblicz kierunek wiatru w stopniach (0-359)
  windDirectionDegrees = (encoderPos * 18) % 360;

  // Normalizuj kąt wiatru do zakresu 0-180 (symetria żaglówki)
  normalizedWindAngle = windDirectionDegrees;
  if (normalizedWindAngle > 180) {
    normalizedWindAngle = 360 - normalizedWindAngle;
  }
  
  // --- Logika maszyny stanów dla żaglówki ---
  switch (currentState) {
    case SAILING:
      // Jeśli wiatr wpadł w strefę martwą - przygotuj się do zwrotu przez sztag
      if (normalizedWindAngle < 45) { // Przykładowy próg
        currentState = TACKING;
        Serial.println("--- Entering TACKING state ---");
      }
      // Jeśli wiatr jest za rufą i chcemy wykonać zwrot przez rufę (poniżej 135 stopni)
      // Lub jeśli aktualny kąt jest blisko 0 i wiatr jest z rufy, przygotuj się do zwrotu przez rufę
      else if (normalizedWindAngle > 150) { // Przykładowy próg dla wiatru za rufą
        currentState = JIBING_PREP;
        Serial.println("--- Entering JIBING_PREP state ---");
      }
      else {
        // Normalna żegluga, sterowanie żaglem przez PID
        int setpointSailAngle = calculateOptimalSailAngle(normalizedWindAngle);
        float pidOutput = calculatePIDOutput(setpointSailAngle, currentSailServoAngle);
        
        currentSailServoAngle += (int)pidOutput; // Zastosuj wyjście PID do aktualnego kąta serwa
        currentSailServoAngle = constrain(currentSailServoAngle, 0, 180); // Ogranicz do zakresu serwa
        sailServo.write(currentSailServoAngle);
      }
      break;

    case TACKING:
      // Zwrot przez sztag (sterowanie tylko żaglem, ster pozostaje w rękach użytkownika)
      // W tym stanie żagiel powinien być poluzowany, aby ułatwić obrót
      sailServo.write(10); // Żagiel luźny/zaciągnięty minimalnie
      currentSailServoAngle = 10; // Zaktualizuj aktualną pozycję serwa

      // Czekaj, aż łódź obróci się i wiatr znajdzie się poza strefą martwą
      if (normalizedWindAngle > 55) { // Kiedy łódź opuści strefę martwą i wiatr będzie z boku
        currentState = SAILING;
        Serial.println("--- Exiting TACKING state, back to SAILING ---");
      }
      break;

    case JIBING_PREP:
      // Przygotowanie do zwrotu przez rufę - zaciągnij żagiel
      sailServo.write(10); // Zaciągnij żagiel maksymalnie
      currentSailServoAngle = 10;

      // Czekaj, aż wiatr przejdzie przez rufę i osiągnie odpowiedni kąt po zwrocie
      // Zakładamy, że ster kontroluje obrót łodzi
      // Gdy wiatr przejdzie przez rufę i będzie około 35 stopni po drugiej stronie
      // (np. z 150 na 180, a potem na 215 bezwzględnie, co da 145 znormalizowane)
      if (abs(windDirectionDegrees - (encoderPos * 18)) > 150 && normalizedWindAngle < 145) { // Przykładowa logika
          // Ta logika jest trudna bez czujnika kierunku łodzi (kompasu).
          // Musisz założyć, że łódź obraca się sterem.
          // Jeśli wiatr nadal wieje z rufy (normalizedWindAngle blisko 180), ale zmieniamy kurs
          // Gdy normalizedWindAngle zacznie spadać (np. poniżej 145), to znaczy, że rufa przeszła przez wiatr
          // i łódź obraca się w nowym kierunku.
          currentState = JIBING_EXEC;
          Serial.println("--- Entering JIBING_EXEC state ---");
      }
      break;

    case JIBING_EXEC:
      // Wykonywanie zwrotu przez rufę - płynne otwieranie żagla
      // Tutaj możemy zastosować płynne przejście lub proste ustawienie
      int targetJibeAngle = calculateOptimalSailAngle(normalizedWindAngle);
      if (currentSailServoAngle < targetJibeAngle) {
        currentSailServoAngle++; // Płynne otwieranie
      }
      sailServo.write(currentSailServoAngle);

      // Gdy żagiel osiągnie docelową pozycję lub wiatr ustabilizuje się
      if (abs(currentSailServoAngle - targetJibeAngle) < 5) { // Jeśli blisko celu
        currentState = SAILING;
        Serial.println("--- Exiting JIBING_EXEC state, back to SAILING ---");
      }
      break;
  }

  // --- Debugowanie ---
  Serial.print("Encoder: ");
  Serial.print(encoderPos);
  Serial.print(" Wind (Deg): ");
  Serial.print(windDirectionDegrees);
  Serial.print(" Norm Wind (Deg): ");
  Serial.print(normalizedWindAngle);
  Serial.print(" Servo Angle: ");
  Serial.print(currentSailServoAngle);
  Serial.print(" State: ");
  switch (currentState) {
    case SAILING: Serial.print("SAILING"); break;
    case TACKING: Serial.print("TACKING"); break;
    case JIBING_PREP: Serial.print("JIBING_PREP"); break;
    case JIBING_EXEC: Serial.print("JIBING_EXEC"); break;
  }
  Serial.println();

  delay(50); // Małe opóźnienie, aby nie obciążać procesora
}