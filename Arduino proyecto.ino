/**
 * @file Arduino proyecto.ino
 * @brief Jukebox de 5 Canciones con Selección por 5 Botones:
 *        - Botón 1 (GPIO 18): Noche de Paz 🎄
 *        - Botón 2 (GPIO 13): Super Mario Bros 🍄
 *        - Botón 3 (GPIO 12): Star Wars (Marcha Imperial) ⚔️
 *        - Botón 4 (GPIO 14): Piratas del Caribe 🏴‍☠️
 *        - Botón 5 (GPIO 27): Tetris (Korobeiniki) 🕹️
 *        Pantalla LCD 1602 I2C (D21/D22), 4 Servomotores y 6 Foquitos LED (PCA9685 D25/D26).
 */

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_PWMServoDriver.h>

struct NoteStep {
  int freq;
  int durationMs;
  const char* l1;
  const char* l2;
};

// Configuración de Pines
constexpr uint8_t PIN_BUZZER_D4   = 4;   // Pin D4 para el Zumbador
constexpr uint8_t PIN_LED_ONBOARD = 2;   // LED Azul indicador en placa ESP32

// 5 Pines de Entrada para los 5 Pulsadores (INPUT_PULLUP interno, van a GND)
constexpr uint8_t PIN_BTN_NOCHE   = 18;  // Botón 1: Noche de Paz 🎄
constexpr uint8_t PIN_BTN_MARIO   = 13;  // Botón 2: Super Mario Bros 🍄
constexpr uint8_t PIN_BTN_SW      = 12;  // Botón 3: Star Wars ⚔️
constexpr uint8_t PIN_BTN_PIRATES = 14;  // Botón 4: Piratas del Caribe 🏴‍☠️
constexpr uint8_t PIN_BTN_TETRIS  = 27;  // Botón 5: Tetris 🕹️

// BUS I2C 1 (Exclusivo para Pantalla LCD)
constexpr uint8_t PIN_LCD_SDA     = 21;  // SDA Exclusivo Pantalla LCD
constexpr uint8_t PIN_LCD_SCL     = 22;  // SCL Exclusivo Pantalla LCD

// BUS I2C 2 (Totalmente separado y Exclusivo para PCA9685)
constexpr uint8_t PIN_PCA_SDA     = 25;  // SDA Exclusivo PCA9685 (Pin D25)
constexpr uint8_t PIN_PCA_SCL     = 26;  // SCL Exclusivo PCA9685 (Pin D26)

TwoWire I2C_Servos = TwoWire(1);

// Controladores
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, I2C_Servos);

// Constantes de Servos (Canales 0 a 3)
// Calibración extrema para Tower Pro SG90:
// Rango normal: 500us a 2400us (~180°)
// Rango extendido al máximo físico posible del chip/motor: 450us a 2650us (>180°)
constexpr uint8_t SERVO_CH0 = 0;
constexpr uint8_t SERVO_CH1 = 1;
constexpr uint8_t SERVO_CH2 = 2;
constexpr uint8_t SERVO_CH3 = 3;
constexpr int SERVO_US_MIN = 450;  // 0° absoluto
constexpr int SERVO_US_MAX = 2650; // Máximo absoluto alcanzable sin límite de 180°

// Constantes de 6 LEDs (Canales 4 a 9)
constexpr uint8_t LED_START_CH = 4;
constexpr uint8_t LED_COUNT    = 6;

void setServoAngle(uint8_t ch, int angle) {
  angle = constrain(angle, 0, 180);
  const int pulseUs = map(angle, 0, 180, SERVO_US_MIN, SERVO_US_MAX);
  pwm.writeMicroseconds(ch, pulseUs);
}

void setDualServos(int angleCh0, int angleCh1) {
  setServoAngle(SERVO_CH0, angleCh0);
  setServoAngle(SERVO_CH1, angleCh1);
}

void resetAllServos() {
  setServoAngle(SERVO_CH0, 0);  // Servo 0 (CH0) siempre en 0° por defecto
  setServoAngle(SERVO_CH1, 90); // Servo 1 (CH1) posición neutral (90°)
  setServoAngle(SERVO_CH2, 90); // Servo 2 (CH2) posición neutral (90°)
  setServoAngle(SERVO_CH3, 90); // Servo 3 (CH3) posición neutral (90°)
}

void setLedBrightness(uint8_t index, uint16_t brightness) {
  if (index < LED_COUNT) {
    if (brightness >= 4095) {
      pwm.setPWM(LED_START_CH + index, 4096, 0); // Full ON
    } else if (brightness == 0) {
      pwm.setPWM(LED_START_CH + index, 0, 4096); // Full OFF
    } else {
      pwm.setPWM(LED_START_CH + index, 0, brightness);
    }
  }
}

void setAllLeds(uint16_t brightness) {
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    setLedBrightness(i, brightness);
  }
}

void updateLedsByMusic(int freq) {
  if (freq == 0) {
    setAllLeds(0);
    return;
  }
  int activeCount = map(freq, 220, 800, 1, LED_COUNT);
  activeCount = constrain(activeCount, 1, (int)LED_COUNT);
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    setLedBrightness(i, (i < activeCount) ? 4095 : 0);
  }
}

// Frecuencias de notas musicales (Hz)
constexpr int NOTE_B2 = 123;
constexpr int NOTE_C3 = 131;
constexpr int NOTE_D3 = 147;
constexpr int NOTE_E3 = 165;
constexpr int NOTE_F3 = 175;
constexpr int NOTE_G3 = 196;
constexpr int NOTE_A3 = 220;
constexpr int NOTE_B3 = 247;
constexpr int NOTE_C4 = 262;
constexpr int NOTE_D4 = 294;
constexpr int NOTE_E4 = 330;
constexpr int NOTE_F4 = 349;
constexpr int NOTE_FS4= 370;
constexpr int NOTE_G4 = 392;
constexpr int NOTE_GS4= 415;
constexpr int NOTE_A4 = 440;
constexpr int NOTE_AS4= 466;
constexpr int NOTE_B4 = 494;
constexpr int NOTE_C5 = 523;
constexpr int NOTE_CS5= 554;
constexpr int NOTE_D5 = 587;
constexpr int NOTE_DS5= 622;
constexpr int NOTE_E5 = 659;
constexpr int NOTE_F5 = 698;
constexpr int NOTE_FS5= 740;
constexpr int NOTE_G5 = 784;
constexpr int NOTE_GS5= 831;
constexpr int NOTE_A5 = 880;
constexpr int NOTE_B5 = 988;
constexpr int NOTE_C6 = 1047;
constexpr int REST    = 0;


// 1. NOCHE DE PAZ 🎄
const NoteStep songNoche[] = {
  { NOTE_G4, 450, " Noche de Paz,", " Noche de Amor! " },
  { NOTE_A4, 250, nullptr, nullptr },
  { NOTE_G4, 450, nullptr, nullptr },
  { NOTE_E4, 900, nullptr, nullptr },
  { NOTE_G4, 450, nullptr, nullptr },
  { NOTE_A4, 250, nullptr, nullptr },
  { NOTE_G4, 450, nullptr, nullptr },
  { NOTE_E4, 900, nullptr, nullptr },
  { NOTE_D5, 700, "  Todo duerme   ", "  en derredor   " },
  { NOTE_D5, 450, nullptr, nullptr },
  { NOTE_B4, 900, nullptr, nullptr },
  { NOTE_C5, 700, "Entre los astros", "esparcen su luz " },
  { NOTE_C5, 450, nullptr, nullptr },
  { NOTE_G4, 900, nullptr, nullptr },
  { NOTE_A4, 450, "Brilla estrella ", " de bendicion!  " },
  { NOTE_A4, 300, nullptr, nullptr },
  { NOTE_C5, 300, nullptr, nullptr },
  { NOTE_B4, 300, nullptr, nullptr },
  { NOTE_A4, 300, nullptr, nullptr },
  { NOTE_G4, 450, nullptr, nullptr },
  { NOTE_A4, 250, nullptr, nullptr },
  { NOTE_G4, 450, nullptr, nullptr },
  { NOTE_E4, 900, nullptr, nullptr },
  { NOTE_D5, 700, "   Nace ya el   ", "  Salvador! *   " },
  { NOTE_D5, 300, nullptr, nullptr },
  { NOTE_F5, 300, nullptr, nullptr },
  { NOTE_D5, 300, nullptr, nullptr },
  { NOTE_B4, 400, nullptr, nullptr },
  { NOTE_C5, 800, nullptr, nullptr },
  { NOTE_E5, 800, nullptr, nullptr },
  { NOTE_C5, 500, "Cristo Nacio! * ", "  Aleluya! *    " },
  { NOTE_G4, 300, nullptr, nullptr },
  { NOTE_E4, 400, nullptr, nullptr },
  { NOTE_G4, 400, nullptr, nullptr },
  { NOTE_F4, 300, nullptr, nullptr },
  { NOTE_D4, 400, nullptr, nullptr },
  { NOTE_C4, 1100, nullptr, nullptr }
};

// 2. SUPER MARIO BROS 🍄
const NoteStep songMario[] = {
  { NOTE_E5, 120, "SUPER MARIO BROS", "  WORLD 1-1 *   " },
  { NOTE_E5, 120, nullptr, nullptr },
  { REST,    120, nullptr, nullptr },
  { NOTE_E5, 120, nullptr, nullptr },
  { REST,    120, nullptr, nullptr },
  { NOTE_C5, 120, nullptr, nullptr },
  { NOTE_E5, 150, nullptr, nullptr },
  { NOTE_G5, 250, "   MARIO *01    ", " COINS x05 [?]  " },
  { REST,    150, nullptr, nullptr },
  { NOTE_G4, 250, nullptr, nullptr },
  { REST,    180, nullptr, nullptr },
  { NOTE_C5, 200, " Let's-a Go! *  ", "  1-UP MUSHROOM " },
  { REST,     80, nullptr, nullptr },
  { NOTE_G4, 200, nullptr, nullptr },
  { REST,     80, nullptr, nullptr },
  { NOTE_E4, 200, nullptr, nullptr },
  { REST,     80, nullptr, nullptr },
  { NOTE_A4, 180, nullptr, nullptr },
  { NOTE_B4, 180, nullptr, nullptr },
  { NOTE_AS4,150, nullptr, nullptr },
  { NOTE_A4, 180, nullptr, nullptr },
  { NOTE_G4, 160, "   SUPER STAR   ", " * * INVINCIBLE " },
  { NOTE_E5, 160, nullptr, nullptr },
  { NOTE_G5, 160, nullptr, nullptr },
  { NOTE_A5, 220, nullptr, nullptr },
  { NOTE_F5, 140, nullptr, nullptr },
  { NOTE_G5, 140, nullptr, nullptr },
  { REST,     80, nullptr, nullptr },
  { NOTE_E5, 180, nullptr, nullptr },
  { NOTE_C5, 140, nullptr, nullptr },
  { NOTE_D5, 140, nullptr, nullptr },
  { NOTE_B4, 220, " LEVEL CLEAR! * ", "  STAGE PASSED  " }
};

// 3. STAR WARS: MARCHA IMPERIAL ⚔️
const NoteStep songStarWars[] = {
  { NOTE_A4, 450, "  STAR WARS *   ", "IMPERIAL MARCH ⚔" },
  { NOTE_A4, 450, nullptr, nullptr },
  { NOTE_A4, 450, nullptr, nullptr },
  { NOTE_F4, 320, nullptr, nullptr },
  { NOTE_C5, 150, nullptr, nullptr },
  { NOTE_A4, 450, "LORD VADER RULES", " THE EMPIRE ⚔️  " },
  { NOTE_F4, 320, nullptr, nullptr },
  { NOTE_C5, 150, nullptr, nullptr },
  { NOTE_A4, 800, nullptr, nullptr },
  { REST,    150, nullptr, nullptr },
  { NOTE_E5, 450, "JOIN THE DARK   ", "  SIDE OF FORCE " },
  { NOTE_E5, 450, nullptr, nullptr },
  { NOTE_E5, 450, nullptr, nullptr },
  { NOTE_F5, 320, nullptr, nullptr },
  { NOTE_C5, 150, nullptr, nullptr },
  { NOTE_GS4,450, " DEATH STAR ⚡  ", " FIRE LASER! ⚔️ " },
  { NOTE_F4, 320, nullptr, nullptr },
  { NOTE_C5, 150, nullptr, nullptr },
  { NOTE_A4, 850, nullptr, nullptr }
};

// 4. PIRATAS DEL CARIBE 🏴‍☠️
const NoteStep songPirates[] = {
  { NOTE_D4, 150, "PIRATES OF THE  ", " CARIBBEAN! 🏴‍☠️ " },
  { NOTE_D4, 150, nullptr, nullptr },
  { NOTE_D4, 150, nullptr, nullptr },
  { NOTE_D4, 150, nullptr, nullptr },
  { NOTE_D4, 150, nullptr, nullptr },
  { NOTE_D4, 150, nullptr, nullptr },
  { NOTE_D4, 150, nullptr, nullptr },
  { NOTE_D4, 150, nullptr, nullptr },
  { NOTE_A4, 160, "JACK SPARROW ⚔️ ", " BLACK PEARL ⚓  " },
  { NOTE_C5, 160, nullptr, nullptr },
  { NOTE_D5, 260, nullptr, nullptr },
  { NOTE_D5, 260, nullptr, nullptr },
  { NOTE_D5, 160, nullptr, nullptr },
  { NOTE_E5, 160, nullptr, nullptr },
  { NOTE_F5, 260, " HE'S A PIRATE! ", " FULL SAILS! 🌊 " },
  { NOTE_F5, 260, nullptr, nullptr },
  { NOTE_F5, 160, nullptr, nullptr },
  { NOTE_G5, 160, nullptr, nullptr },
  { NOTE_E5, 260, nullptr, nullptr },
  { NOTE_E5, 260, nullptr, nullptr },
  { NOTE_D5, 160, nullptr, nullptr },
  { NOTE_C5, 160, nullptr, nullptr },
  { NOTE_D5, 500, " TREASURE GOLD! ", "  YO HO HO! 🏴‍☠️  " }
};

// 5. TETRIS (KOROBEINIKI) 🕹️
const NoteStep songTetris[] = {
  { NOTE_E5, 280, " TETRIS THEME 🕹️", "   LINE CLEAR!  " },
  { NOTE_B4, 160, nullptr, nullptr },
  { NOTE_C5, 160, nullptr, nullptr },
  { NOTE_D5, 280, nullptr, nullptr },
  { NOTE_C5, 160, nullptr, nullptr },
  { NOTE_B4, 160, nullptr, nullptr },
  { NOTE_A4, 280, "SCORE: 999999 🕹️", " LEVEL 10 SPEED " },
  { NOTE_A4, 160, nullptr, nullptr },
  { NOTE_C5, 160, nullptr, nullptr },
  { NOTE_E5, 280, nullptr, nullptr },
  { NOTE_D5, 160, nullptr, nullptr },
  { NOTE_C5, 160, nullptr, nullptr },
  { NOTE_B4, 380, "T-SPIN DOUBLE!  ", "  BLOCKS DOWN!  " },
  { NOTE_C5, 160, nullptr, nullptr },
  { NOTE_D5, 280, nullptr, nullptr },
  { NOTE_E5, 280, nullptr, nullptr },
  { NOTE_C5, 280, nullptr, nullptr },
  { NOTE_A4, 280, nullptr, nullptr },
  { NOTE_A4, 400, " TETRIS WINNER! ", " TOP RECORD! 🏆 " }
};

// Estructura de Debounce para los 5 botones
struct ButtonWatcher {
  uint8_t pin;
  int lastSteadyState;
  int lastFlickerState;
  unsigned long lastDebounceMs;
};

ButtonWatcher buttons[5] = {
  { PIN_BTN_NOCHE,   HIGH, HIGH, 0 },
  { PIN_BTN_MARIO,   HIGH, HIGH, 0 },
  { PIN_BTN_SW,      HIGH, HIGH, 0 },
  { PIN_BTN_PIRATES, HIGH, HIGH, 0 },
  { PIN_BTN_TETRIS,  HIGH, HIGH, 0 }
};

int activeSong = -1; // -1: Ninguna seleccionada, 0..4: Cancion elegida
bool isPlaying = false;
bool isPaused  = false;
size_t currentStep = 0; // Posicion actual dentro de la cancion para poder reanudar

// Modo Demostración de Servomotor 0 con Botón 1:
// - Por defecto siempre está en su ángulo 0°.
// - Al pulsar el Botón 1: empieza a girar INMEDIATAMENTE y de forma lenta hasta su último ángulo (límite máximo).
// - Al llegar al final: se queda parado 3 segundos en el límite.
// - Pasados los 3 segundos: regresa rápido a su ángulo 0° y se queda ahí parado.
enum Servo0Stage {
  S0_IDLE,
  S0_SWEEPING_UP,
  S0_HOLD_3S_AT_MAX
};

Servo0Stage servo0State = S0_IDLE;
int servo0CurrentAngle = 0;
unsigned long servo0TimerMs = 0;
unsigned long servo0LastStepMs = 0;

// Control directo por Ticks PCA9685 para forzar todo el ángulo mecánico posible:
// 90 ticks  (~440us)  -> Extremo cero absoluto
// 560 ticks (~2730us) -> Extremo máximo absoluto físico (sobrepasa 180°)
constexpr int SERVO0_TICK_MIN = 90;
constexpr int SERVO0_TICK_MAX = 560;
constexpr unsigned long SERVO0_SWEEP_INTERVAL_MS = 25; // Avance lento continuo

void startServo0Routine() {
  if (servo0State != S0_IDLE) return;

  noTone(PIN_BUZZER_D4);
  digitalWrite(PIN_BUZZER_D4, LOW);
  digitalWrite(PIN_LED_ONBOARD, LOW);
  isPlaying = false;
  isPaused  = false;
  activeSong = -1;
  setAllLeds(0);

  // Asegura el 0° absoluto e inicia el giro hacia el límite
  pwm.setPWM(SERVO_CH0, 0, SERVO0_TICK_MIN);
  servo0CurrentAngle = 0;
  servo0State = S0_SWEEPING_UP;
  servo0LastStepMs = millis();
  showLcdMessage(" FORZANDO MOTOR ", "0 -> Maximo...");
  Serial.println(F("🔘 Boton 1: Forzando recorrido al maximo posible..."));
}

void updateServo0Routine() {
  if (servo0State == S0_IDLE) return;

  switch (servo0State) {
    case S0_SWEEPING_UP:
      if (millis() - servo0LastStepMs >= SERVO0_SWEEP_INTERVAL_MS) {
        servo0LastStepMs = millis();
        if (servo0CurrentAngle < 180) {
          servo0CurrentAngle++;
          const int tick = map(servo0CurrentAngle, 0, 180, SERVO0_TICK_MIN, SERVO0_TICK_MAX);
          pwm.setPWM(SERVO_CH0, 0, tick);
        } else {
          // Llegó al máximo ángulo: entrar en pausa de 3 segundos
          // Al apagar el pulso (setPWM 0,0) el motor se queda quieto en su posición sin recalentarse ni colgarse
          pwm.setPWM(SERVO_CH0, 0, 0); 
          servo0State = S0_HOLD_3S_AT_MAX;
          servo0TimerMs = millis();
          showLcdMessage(" MAXIMO ALCANZADO", "Pausa 3 segundos");
          Serial.println(F("⏸️ Llego al limite maximo. Desconectando pulso para evitar bloqueo por 3s..."));
        }
      }
      break;

    case S0_HOLD_3S_AT_MAX:
      if (millis() - servo0TimerMs >= 3000) {
        // Pasaron los 3 segundos: REGRESA DE GOLPE A SU ANGULO 0°
        pwm.setPWM(SERVO_CH0, 0, SERVO0_TICK_MIN);
        servo0CurrentAngle = 0;
        servo0State = S0_IDLE;
        showLcdMessage(" REPOSO: CERO   ", "Listo para pulsar");
        Serial.println(F("⚡ Pasaron los 3s: Regreso inmediato a 0° de reposo y queda listo."));
      }
      break;

    default:
      servo0State = S0_IDLE;
      break;
  }
}

void showLcdMessage(const char* l1, const char* l2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
}

// Pausa la cancion actual (apaga sonido, luces y deja servos fijos)
void pausePlayback() {
  noTone(PIN_BUZZER_D4);
  digitalWrite(PIN_BUZZER_D4, LOW);
  digitalWrite(PIN_LED_ONBOARD, LOW);
  resetAllServos();
  setAllLeds(0);
  isPlaying = false;
  isPaused  = true;
  showLcdMessage("  == PAUSA ==   ", "Pulsa para seguir");
  Serial.println(F("⏸️ Cancion en PAUSA. Pulsa el boton para continuar."));
}

// Detiene y resetea totalmente la reproduccion
void stopPlayback() {
  noTone(PIN_BUZZER_D4);
  digitalWrite(PIN_BUZZER_D4, LOW);
  digitalWrite(PIN_LED_ONBOARD, LOW);
  resetAllServos();
  setAllLeds(0);
  isPlaying = false;
  isPaused  = false;
  activeSong = -1;
  currentStep = 0;
  showLcdMessage("  JUKEBOX ESP32 ", "Elige Boton 1..5");
  Serial.println(F("⏹️ Reproduccion FINALIZADA."));
}

// Chequea si cualquiera de los 5 botones fue presionado.
// Retorna 0..4 según el botón pulsado, o -1 si ninguno.
int checkAnyButtonPressed() {
  for (int b = 0; b < 5; b++) {
    int current = digitalRead(buttons[b].pin);
    if (current != buttons[b].lastFlickerState) {
      buttons[b].lastDebounceMs = millis();
      buttons[b].lastFlickerState = current;
    }
    if ((millis() - buttons[b].lastDebounceMs) > 40) {
      if (buttons[b].lastSteadyState == HIGH && current == LOW) {
        buttons[b].lastSteadyState = current;
        return b; // Botón presionado confirmado
      }
      buttons[b].lastSteadyState = current;
    }
  }
  return -1;
}

// Maneja la acción al pulsar un botón:
// - Botón 0 (Pin D18): Rutina especial Servomotor 0 (Canal 0) (0° -> espera 3s -> barre a 180° -> inmediato 0° y queda fijo).
// - Botones 1..4 (Mario, Star Wars, Piratas, Tetris): Reproduce/pausa/reanuda música y Servos 1, 2 y 3 funcionan normalmente.
void handleButtonAction(int btn) {
  if (btn < 0 || btn > 4) return;

  // Si se pulsa el Botón 1 (btn == 0): Control exclusivo de Servomotor 0 (el primerito)
  if (btn == 0) {
    startServo0Routine();
    return;
  }

  // Si se pulsa cualquier otra canción (Botones 2 al 5): Cancelar rutina de servo 0 y dejar servo 0 en 0°
  servo0State = S0_IDLE;
  setServoAngle(SERVO_CH0, 0);

  if (activeSong == btn) {
    if (isPlaying) {
      pausePlayback();
    } else if (isPaused) {
      isPaused = false;
      isPlaying = true;
      Serial.print(F("▶️ REANUDANDO cancion: "));
      Serial.println(btn + 1);
    } else {
      currentStep = 0;
      isPaused = false;
      isPlaying = true;
    }
  } else {
    // Canción distinta seleccionada: reiniciar paso y reproducir
    activeSong = btn;
    currentStep = 0;
    isPaused = false;
    isPlaying = true;
    Serial.print(F("🎵 Seleccionada nueva cancion: "));
    Serial.println(btn + 1);
  }
}

// Toca una nota con verificación continua de interrupción por botones
bool playNoteInterruptible(int freq, int durationMs) {
  if (freq != REST) {
    digitalWrite(PIN_LED_ONBOARD, HIGH);
    tone(PIN_BUZZER_D4, freq, durationMs);
  } else {
    digitalWrite(PIN_LED_ONBOARD, LOW);
    noTone(PIN_BUZZER_D4);
  }

  const unsigned long start = millis();
  while (millis() - start < (unsigned long)durationMs) {
    int pressedBtn = checkAnyButtonPressed();
    if (pressedBtn != -1) {
      handleButtonAction(pressedBtn);
      return false; // Interrumpido por botón
    }
    delay(4);
  }

  noTone(PIN_BUZZER_D4);
  digitalWrite(PIN_LED_ONBOARD, LOW);
  return true;
}

// Reproduce la lista de notas seleccionada desde currentStep
void executeSong(const NoteStep* steps, size_t totalSteps, const char* title) {
  Serial.print(F("▶️ Ejecutando: "));
  Serial.println(title);

  for (size_t i = currentStep; i < totalSteps; i++) {
    if (!isPlaying) {
      currentStep = i; // Guardar punto exacto si se pausó
      return;
    }

    currentStep = i;

    if (steps[i].l1 != nullptr) {
      showLcdMessage(steps[i].l1, steps[i].l2);
    }

    // Coreografía Servos:
    // Servo 0 (CH0): Fijo en 0° (no baila con la música)
    setServoAngle(SERVO_CH0, 0);

    // Servo 1 (CH1): Sigue tal cual como antes bailando con el tono de las notas
    int a1 = 90;
    if (steps[i].freq != REST) {
      a1 = map(steps[i].freq, NOTE_A3, NOTE_C6, 20, 160);
      a1 = constrain(a1, 15, 165);
    }
    setServoAngle(SERVO_CH1, a1);

    // 💡 6 LEDs: Vúmetro dinámico
    updateLedsByMusic(steps[i].freq);

    if (!playNoteInterruptible(steps[i].freq, steps[i].durationMs)) {
      return; // Pausado o cambiado de canción
    }

    setServoAngle(SERVO_CH1, 180 - a1);
    setAllLeds(250); // Leve respiración

    // Pausa corta entre notas
    const int pauseMs = steps[i].durationMs * 0.25;
    const unsigned long pStart = millis();
    while (millis() - pStart < (unsigned long)pauseMs) {
      int pBtn = checkAnyButtonPressed();
      if (pBtn != -1) {
        handleButtonAction(pBtn);
        return;
      }
      delay(4);
    }
  }

  // Final con Servos 2 y 3 + Estroboscópico de LEDs
  if (isPlaying) {
    showLcdMessage("  Fin Cancion!  ", "Show Servos&LEDs");
    setServoAngle(SERVO_CH0, 0);
    setServoAngle(SERVO_CH1, 90);

    for (int r = 0; r < 4; r++) {
      if (checkAnyButtonPressed() != -1) {
        stopPlayback();
        return;
      }
      tone(PIN_BUZZER_D4, NOTE_C5, 90);
      setServoAngle(SERVO_CH2, 20);
      setServoAngle(SERVO_CH3, 160);
      setLedBrightness(0, 4095); setLedBrightness(2, 4095); setLedBrightness(4, 4095);
      setLedBrightness(1, 0);    setLedBrightness(3, 0);    setLedBrightness(5, 0);
      delay(120);
      noTone(PIN_BUZZER_D4);

      tone(PIN_BUZZER_D4, NOTE_E5, 90);
      setServoAngle(SERVO_CH2, 160);
      setServoAngle(SERVO_CH3, 20);
      setLedBrightness(0, 0);    setLedBrightness(2, 0);    setLedBrightness(4, 0);
      setLedBrightness(1, 4095); setLedBrightness(3, 4095); setLedBrightness(5, 4095);
      delay(120);
      noTone(PIN_BUZZER_D4);

      tone(PIN_BUZZER_D4, NOTE_G5, 110);
      setServoAngle(SERVO_CH2, 40);
      setServoAngle(SERVO_CH3, 40);
      setAllLeds(4095);
      delay(120);
      setServoAngle(SERVO_CH2, 140);
      setServoAngle(SERVO_CH3, 140);
      setAllLeds(0);
      delay(120);
      noTone(PIN_BUZZER_D4);
    }

    resetAllServos();
    setAllLeds(0);
    isPlaying = false;
    activeSong = -1;
    showLcdMessage("  JUKEBOX ESP32 ", "Elige Boton 1..5");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUZZER_D4, OUTPUT);
  pinMode(PIN_LED_ONBOARD, OUTPUT);
  digitalWrite(PIN_BUZZER_D4, LOW);
  digitalWrite(PIN_LED_ONBOARD, LOW);

  // Configuración de los 5 botones con PULLUP interno
  for (int b = 0; b < 5; b++) {
    pinMode(buttons[b].pin, INPUT_PULLUP);
  }

  // BUS 1: LCD I2C en D21 / D22
  Wire.begin(PIN_LCD_SDA, PIN_LCD_SCL, 100000);
  lcd.init();
  lcd.backlight();
  lcd.display();
  showLcdMessage("  JUKEBOX ESP32 ", "Iniciando...");

  // BUS 2: PCA9685 en D25 / D26
  I2C_Servos.begin(PIN_PCA_SDA, PIN_PCA_SCL, 100000);
  pwm.begin();
  pwm.setPWMFreq(50);

  // Test de LEDs
  for (uint8_t l = 0; l < LED_COUNT; l++) {
    setLedBrightness(l, 4095);
    delay(50);
    setLedBrightness(l, 0);
  }
  setAllLeds(0);

  // Test de Servos (Servo 0 siempre en 0°, los otros hacen test)
  setServoAngle(SERVO_CH0, 0);
  setServoAngle(SERVO_CH1, 135);
  setServoAngle(SERVO_CH2, 45);
  setServoAngle(SERVO_CH3, 135);
  delay(250);
  resetAllServos();

  showLcdMessage("  JUKEBOX ESP32 ", "Elige Boton 1..5");

  Serial.println(F("\n=================================================="));
  Serial.println(F("🎵 JUKEBOX 4 CANCIONES + SERVO 0 DEMO + 6 LEDS 🔘"));
  Serial.println(F("=================================================="));
  Serial.println(F("1) Pin D18 -> Motor 0 (CH0) (0° -> Espera 3s -> 180° -> 0° fijo) 🦾"));
  Serial.println(F("2) Pin D13 -> Super Mario Bros 🍄"));
  Serial.println(F("3) Pin D12 -> Star Wars (Imperial March) ⚔️"));
  Serial.println(F("4) Pin D14 -> Piratas del Caribe 🏴‍☠️"));
  Serial.println(F("5) Pin D27 -> Tetris Theme 🕹️"));
}

void loop() {
  if (!isPlaying) {
    updateServo0Routine(); // Rutina no bloqueante del Servo 0 (0° -> 3s -> 180° -> 0° fijo)
    int b = checkAnyButtonPressed();
    if (b != -1) {
      handleButtonAction(b);
    }
    delay(2);
    return;
  }

  // Reproducir la canción elegida (Botón 1 ahora es Servo 1, 1..4 son canciones)
  switch (activeSong) {
    case 1:
      executeSong(songMario, sizeof(songMario)/sizeof(songMario[0]), "Super Mario Bros");
      break;
    case 2:
      executeSong(songStarWars, sizeof(songStarWars)/sizeof(songStarWars[0]), "Star Wars");
      break;
    case 3:
      executeSong(songPirates, sizeof(songPirates)/sizeof(songPirates[0]), "Piratas del Caribe");
      break;
    case 4:
      executeSong(songTetris, sizeof(songTetris)/sizeof(songTetris[0]), "Tetris Theme");
      break;
    default:
      isPlaying = false;
      break;
  }
}
