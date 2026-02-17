#include <Arduino.h>

// Пины подключения
const int joystickXPin = A1;
const int potPin = A0;
const int in1Pin = 9;
const int in2Pin = 10;
const int buttonPin = 2;
const int pin3 = 3;
const int pin4 = 4;
const int ledPin6 = 6;
const int ledPin7 = 7;
const int ledPin8 = 8;

// Фиксированные положения актуатора
const int fixedPosition1 = 497; // Нейтральное
const int fixedPosition2 = 422; // Подъём
const int fixedPosition3 = 580; // Опускание
const int fixedPosition4 = 620; // Плавающее
const int tolerance = 5;

// Параметры П-регулятора
const float Kp = 1;
const int minPower = 30;

// Мёртвая зона джойстика
const int joystickCenter = 515;
const int joystickDeadzone = 20;

// Переменные для обработки кнопки
bool lastButtonState = HIGH;
bool currentButtonState;
bool buttonPressed = false;
bool actuatorPosition4 = false;

// Текущее положение актуатора
int currentActuatorPosition = fixedPosition1;

void setup() {
  pinMode(in1Pin, OUTPUT);
  pinMode(in2Pin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pin3, INPUT);
  pinMode(pin4, INPUT);
  pinMode(ledPin6, OUTPUT);
  pinMode(ledPin7, OUTPUT);
  pinMode(ledPin8, OUTPUT);
  Serial.begin(9600);
}

void controlActuator(int targetPosition, int potNormalized, int maxPower, bool proportional = true) {
  int error = targetPosition - potNormalized;

  if (abs(error) <= tolerance) {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, LOW);
    return;
  }

  int power;
  if (proportional) {
    power = (int)(Kp * abs(error));
    power = constrain(power, minPower, maxPower);
  } else {
    power = maxPower;
  }

  if (error > 0) {
    digitalWrite(in1Pin, LOW);
    analogWrite(in2Pin, power);
  } else {
    analogWrite(in1Pin, power);
    digitalWrite(in2Pin, LOW);
  }

  currentActuatorPosition = targetPosition;
}

void controlLEDs(int potValue) {
  if (potValue >= fixedPosition1 - tolerance && potValue <= fixedPosition1 + tolerance) {
    // Нейтральное — все выключены
    digitalWrite(ledPin6, LOW);
    digitalWrite(ledPin7, LOW);
    digitalWrite(ledPin8, LOW);
  } else if (potValue >= fixedPosition2 && potValue < fixedPosition1 - tolerance) {
    // Подъём — ledPin6
    digitalWrite(ledPin6, HIGH);
    digitalWrite(ledPin7, LOW);
    digitalWrite(ledPin8, LOW);
  } else if (potValue > fixedPosition1 + (4 * tolerance) && potValue <= fixedPosition3 + (4 * tolerance)) {
    // Опускание — ledPin7
    digitalWrite(ledPin6, LOW);
    digitalWrite(ledPin7, HIGH);
    digitalWrite(ledPin8, LOW);
  } else if (potValue > fixedPosition3 + (4 * tolerance) && potValue <= fixedPosition4 + (4 * tolerance)) {
    // Плавающее — ledPin8
    digitalWrite(ledPin6, LOW);
    digitalWrite(ledPin7, LOW);
    digitalWrite(ledPin8, HIGH);
  } else {
    digitalWrite(ledPin6, LOW);
    digitalWrite(ledPin7, LOW);
    digitalWrite(ledPin8, LOW);
  }
}

void loop() {
  int joystickXValue = analogRead(joystickXPin);
  int potValue = analogRead(potPin);
  int potNormalized = map(potValue, 0, 1023, 0, 1000);

  // Считываем текущее состояние кнопки
  currentButtonState = digitalRead(buttonPin);

  // Проверка нажатия кнопки (переход с HIGH на LOW)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    buttonPressed = !buttonPressed;
    actuatorPosition4 = !actuatorPosition4; // Переключаем флаг положения актуатора
  }

  Serial.print("Флаг кнопки: ");
  Serial.println(buttonPressed ? "Нажата" : "Не нажата");

  
  if (actuatorPosition4) {
    // Едем в плавающее
    controlActuator(fixedPosition4, potNormalized, 255, false);
    if (digitalRead(pin3) == HIGH) {
      // АОГ подъём — выходим из плавающего
      actuatorPosition4 = false;
    }
  } else if (digitalRead(pin3) == HIGH) {
    // АОГ подъём → позиция 2
    controlActuator(fixedPosition2, potNormalized, 255, false);
  } else if (digitalRead(pin4) == HIGH) {
    // АОГ опускание → позиция 3, затем сразу в плавающее
    controlActuator(fixedPosition3, potNormalized, 255, false);
    if (buttonPressed) { // если флаг кнопки активен
       actuatorPosition4 = true ; // актуатор движется в позицию 4
    } 
  } else {  
  int deflection = abs(joystickXValue - joystickCenter);
    if (deflection < joystickDeadzone) {
    // Джойстик в нейтрали -> быстро возвращаемся в нейтраль
    controlActuatorNoSlow(fixedPosition1, potNormalized, 255);
    } else {
    // Целевая позиция пропорциональна джойстику
    int targetPosition = map(joystickXValue, 0, 1023, fixedPosition2, fixedPosition3);
    // Скорость (мощность) пропорциональна отклонению джойстика
    int power = map(deflection, joystickDeadzone, 512, minPower, 255);
    power = constrain(power, minPower, 255);
    controlActuatorNoSlow(targetPosition, potNormalized, power);
    }
  } 
  controlLEDs(potValue);
  // Обновляем предыдущее состояние кнопки
  lastButtonState = currentButtonState;
  // Отладочная информация
  Serial.print("Joystick X: ");
  Serial.print(joystickXValue);
  Serial.print(" | Potentiometer: ");
  Serial.print(potValue);
  Serial.print(" | PotNorm: ");
  Serial.print(potNormalized);
  Serial.print(" | pin3: ");
  Serial.print(digitalRead(pin3));
  Serial.print(" | pin4: ");
  Serial.print(digitalRead(pin4));
  Serial.print(" | actuatorPosition4: ");
  Serial.println(actuatorPosition4 ? "true" : "false");

  delay(100);
}
void controlActuatorNoSlow(int targetPosition, int potNormalized, int power)
{
  int error = targetPosition - potNormalized;

  if (abs(error) <= tolerance) {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, LOW);
    return;
  }

  power = constrain(power, 0, 255);

  if (error > 0) {
    digitalWrite(in1Pin, LOW);
    analogWrite(in2Pin, power);
  } else {
    analogWrite(in1Pin, power);
    digitalWrite(in2Pin, LOW);
  }
}