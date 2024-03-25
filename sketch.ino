#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

#define I2C_ADDR    0x27
#define LCD_COLUMNS 16
#define LCD_LINES   2

Servo trava; 
const int travaPin = 12;

const int btnApg = 17, btnIn = 16;
const int ledG = 18, ledR = 19;

const int buzzerPin = 2;
char notes[] = "gabygabyxzCDxzCDabywabywzCDEzCDEbywFCDEqywFGDEqi        azbC";
int length = sizeof(notes);
int beats[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 3, 16,};
int tempo = 75;

const String senhaCorreta = "123456"; // Senha predefinida

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);

String senha = "";

const uint8_t ROWS = 4;
const uint8_t COLS = 4;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

uint8_t colPins[COLS] = { 27, 13, 23, 4 };
uint8_t rowPins[ROWS] = { 33, 25, 26, 14 };

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(115200);
  pinMode(btnIn, INPUT_PULLUP);
  pinMode(btnApg, INPUT_PULLUP);
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  trava.attach(travaPin);
  lcd.init();
  lcd.backlight();
}

void loop() {
  digitar();
  apagar();
  inserir();
}

void digitar(){
  char key = keypad.getKey();

  if (key != NO_KEY) {
    trava.detach(); // Desliga o sinal enviado para o servo, "travando-o"
    tone(buzzerPin, 800, 25);
    senha += key;

    lcd.setCursor(1, 0);
    lcd.print(senha);
  }
}

void apagar(){
  if (digitalRead(btnApg) == LOW) {
    if (senha.length() > 0) {
      tone(buzzerPin, 200, 50);
      senha.remove(senha.length() - 1);
      lcd.setCursor(1, 0);
      lcd.print("                "); // Limpa o campo de senha
      lcd.setCursor(1, 0);
      lcd.print(senha);
      delay(200); // Pequeno atraso para evitar múltiplas leituras do botão
    }
  }
}

void inserir(){
  if (digitalRead(btnIn) == LOW) {
    if (senha == senhaCorreta) {
      liberar();
    } else {
      travar();
    }
    reset();
  }
}

void liberar(){
  trava.attach(travaPin); // Liga o sinal enviado para o servo, "liberando-o"
  digitalWrite(ledG, HIGH);
  trava.write(180);
  somAcesso();
  lcd.setCursor(0, 1);
  lcd.print("Acesso permitido");
}

void travar(){
  trava.detach(); // Desliga o sinal enviado para o servo, "travando-o"
  digitalWrite(ledR, HIGH);
  somTrava();
  lcd.setCursor(0, 1);
  lcd.print("Senha incorreta");
}

void reset(){
  delay(2000); // Exibe a mensagem por 2 segundos
  digitalWrite(ledG, LOW);
  digitalWrite(ledR, LOW);
  trava.write(90);
  lcd.setCursor(0, 1);
  lcd.print("                "); // Limpa a linha
  senha = ""; // Limpa a senha digitada
  lcd.setCursor(1, 0);
  lcd.print("                "); // Limpa o campo de senha
}

void playNote(char note, int duration) {
  char names[] = { 'c', 'd', 'e', 'f', 'g', 'x', 'a', 'z', 'b', 'C', 'y', 'D', 'w', 'E', 'F', 'q', 'G', 'i' };
  int tones[] = { 1898, 1690, 1500, 1420, 1265, 1194, 1126, 1063, 1001, 947, 893, 843, 795, 749, 710, 668, 630, 594 };
  for (int i = 0; i < 18; i++) {
    if (names[i] == note) {
      playTone(tones[i], duration);
    }
  }
}

void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(buzzerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(buzzerPin, LOW);
    delayMicroseconds(tone);
  }
}

void somAcesso(){
  for (int i = 0; i < length; i++) {
    if (notes[i] == ' ') {
      delay(beats[i] * tempo);
    } else {
      playNote(notes[i], beats[i] * tempo);
    }
    delay(tempo / 2);
  }
}

void somTrava(){
  tone(buzzerPin, 262, 999);
}

//O Painel de Controle deve apresentar um histórico de todas as tentativas de entradas, deve
//permitir que o usuário bloqueie ou libere a trava, e acione o buzzer.
