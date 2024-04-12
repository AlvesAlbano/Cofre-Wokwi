#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define I2C_ADDR    0x27
#define LCD_COLUMNS 16
#define LCD_LINES   2

const int travaPin = 12;
const int btnApg = 17, btnIn = 16;
const int ledVERD = 18, ledVERM = 19;
const int ledA = 15;
const int buzzerPin = 2;

volatile unsigned int interruptCounter = 0;
int totalInterruptCounter;
hw_timer_t *timer = NULL;

// char notes[] = "gabygabyxzCDxzCDabywabywzCDEzCDEbywFCDEqywFGDEqi        azbC";
// int length = sizeof(notes);
// int beats[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 3, 16,};
// int tempo = 75;

String senhaCorreta = "12"; // Senha predefinida
String senhaOculta;
// const String senhaEmergencia = "ABCD*#";
int tentativas = 0;

bool trancado;

String senha = "";

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

byte colPins[COLS] = { 27, 13, 23, 4 };
byte rowPins[ROWS] = { 33, 25, 26, 14 };

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);
Servo trava;

//BROKER MQTT============================================================================================================================


const char* mqtt_server = "broker.hivemq.com"; //servidor mqtt
WiFiClient espClient;             //criação do objeto espClient do tipo WiFiClient
PubSubClient client(espClient);   //abstrai
unsigned long lastMsg = 0;        //unsigned long = inteiro de 32 bits sem sinal
#define MSG_BUFFER_SIZE  (50)     //abstrai
char msg[MSG_BUFFER_SIZE];        //abstrai

void IRAM_ATTR onTimer() {
  interruptCounter++;
}

void conectarBroker() {         
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  if (!client.connected()) {
    Serial.print("Conectando ao servidor MQTT...");
    String clientId = "";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      digitalWrite(ledA,HIGH);
      Serial.println("Conectado");              
      client.subscribe("cofre/acesso");  	      //inscrição no tópico para receber mensagens
      client.subscribe("cofre/senha");  	      //inscrição no tópico para receber mensagens
    }
  }
}

void reconectarBroker() {
  if (!client.connected()) {
    conectarBroker();
  }
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int lenght) {
  // if ((char)payload[0] == 'a') {
  //   digitalWrite(ledA, LOW);
   
  // }
  // if ((char)payload[0] == 'A') {
  //   digitalWrite(ledA, HIGH);
    
  // }

  String mensagem = "";
  
  for (int i = 0; i < lenght; i++) {
    mensagem += (char)payload[i];
  }

  if (mensagem == "ON") {
    Serial.println("cofre desbloqueado");
    digitalWrite(ledVERD,HIGH);
    digitalWrite(ledVERM,LOW);
    tentativas = 0;
    trancado = false;
    senha = "";
    lcd.clear();
    lcd.setCursor(1, 1);
    lcd.print("DESBLOQUEADO");
    delay(2000);
    lcd.clear();
    digitalWrite(ledVERD,LOW);
  } else if (mensagem == "OFF") {
    Serial.println("cofre bloqueado");
    digitalWrite(ledVERM,HIGH);
    trancado = true;
    senha = "";
    lcd.clear();
    lcd.setCursor(3, 1);
    lcd.print("BLOQUEADO");
  } else {
    
    senha = mensagem;

    Serial.println("Nova senha definida: " + senha);
  }
}
//==========================================================================================================================================

void setup() {
  Serial.begin(115200);
//BROKER MQTT============================================================================================================================
  Serial.print("Conectando-se ao Wi-Fi");
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Conectado!");
  conectarBroker();
  pinMode(ledA, OUTPUT);
//==========================================================================================================================================
  pinMode(btnIn, INPUT_PULLUP);
  pinMode(btnApg, INPUT_PULLUP);
  pinMode(ledVERM, OUTPUT);
  pinMode(ledVERD, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  trava.attach(travaPin);
  trava.write(180); 
  lcd.init();
  lcd.backlight();
  lcd.blink();

  timer = timerBegin(0, 40, true);//true -> count up
  timerAttachInterrupt(timer, &onTimer, true);//true -> edge interrupt
  timerAlarmWrite(timer, 1000000, true);//true -> automatic reload
  timerAlarmEnable(timer);//play
}

void loop() {
  reconectarBroker();
  digitar();
  apagar();
  inserir();
}

void digitar(){
  char key = keypad.getKey();

  if (key != NO_KEY && !(trancado)) {
    trava.detach(); // Desliga o sinal enviado para o servo, "travando-o"
    tone(buzzerPin, 800, 25);
    senha += key;
    senhaOculta += '*';
    
    lcd.setCursor(1, 0);
    lcd.print(senhaOculta);
  }
}

void apagar(){
  if (digitalRead(btnApg) == LOW) {
    if (senha.length() > 0) {
      tone(buzzerPin, 200, 50);
      senha.remove(senha.length() - 1);
      senhaOculta.remove(senhaOculta.length()-1);
      lcd.setCursor(1, 0);
      lcd.clear(); // Limpa o campo de senha
      lcd.setCursor(1, 0);
      lcd.print(senhaOculta);
      delay(200); // Pequeno atraso para evitar múltiplas leituras do botão
    }
  }
}

void inserir(){
  if (digitalRead(btnIn) == LOW && !(trancado)) {
    if (senha == senhaCorreta && tentativas < 3) {
      liberar();
    } else if(tentativas == 3) {
      bloqueado();
    } else {
      travar();
    } 
  }
}

void liberar(){
  trava.attach(travaPin); // Liga o sinal enviado para o servo, "liberando-o"
  tentativas = 0;
  digitalWrite(ledVERD, HIGH);
  trava.write(90);
  somAcesso();
  lcd.setCursor(0, 1);
  lcd.print("Acesso permitido");
  client.publish("cofre/historico","Acesso Concedido");
  reset();
}

void travar(){
  digitalWrite(ledVERM, HIGH);
  tentativas++;
  if(tentativas == 3){
    lcd.setCursor(1, 0);
    lcd.clear(); // Limpa o campo de senha
    senha = ""; // Limpa a senha digitada
    trancado = true;
    bloqueado();
  } else {
    trava.detach(); // Desliga o sinal enviado para o servo, "travando-o"
    somTrava();
    lcd.setCursor(0, 1);
    lcd.print("Senha incorreta");
    client.publish("cofre/historico","Acesso Negado");
    reset();
  }
}

void reset(){
  delay(2000); // Exibe a mensagem por 2 segundos
  digitalWrite(ledVERD, LOW);
  digitalWrite(ledVERM, LOW);
  trava.write(180);
  lcd.setCursor(0, 1);
  lcd.clear(); // Limpa a linha
  senha = ""; // Limpa a senha digitada
  senhaOculta = "";
}

void bloqueado(){
  trancado = true;
  somTravaTotal();
  lcd.setCursor(3, 1);
  lcd.print("BLOQUEADO");
  delay(1000);
  // digitar();
  // if(senha == senhaEmergencia){
  //   tentativas = 0;
  //   trancado = false;
  //   reset();
  // }
  // else{
  //   lcd.setCursor(1, 0);
  //   lcd.clear(); // Limpa o campo de senha
  //   senha = ""; // Limpa a senha digitada
  // }
}

// void playNote(char note, int duration) {
//   char names[] = { 'c', 'd', 'e', 'f', 'g', 'x', 'a', 'z', 'b', 'C', 'y', 'D', 'w', 'E', 'F', 'q', 'G', 'i' };
//   int tones[] = { 1898, 1690, 1500, 1420, 1265, 1194, 1126, 1063, 1001, 947, 893, 843, 795, 749, 710, 668, 630, 594 };
//   for (int i = 0; i < 18; i++) {
//     if (names[i] == note) {
//       playTone(tones[i], duration);
//     }
//   }
// }

// void playTone(int tone, int duration) {
//   for (long i = 0; i < duration * 1000L; i += tone * 2) {
//     digitalWrite(buzzerPin, HIGH);
//     delayMicroseconds(tone);
//     digitalWrite(buzzerPin, LOW);
//     delayMicroseconds(tone);
//   }
// }

void somAcesso(){
  // for (int i = 0; i < length; i++) {
  //   if (notes[i] == ' ') {
  //     delay(beats[i] * tempo);
  //   } else {
  //     playNote(notes[i], beats[i] * tempo);
  //   }
  //   delay(tempo / 2);
  // }
  tone(buzzerPin, 1000, 1000);
}

void somTrava(){
  tone(buzzerPin, 262, 1000);
}

void somTravaTotal(){
  tone(buzzerPin, 400, 1000);
}

//O Painel de Controle deve apresentar um histórico de todas as tentativas de entradas, deve
//permitir que o usuário bloqueie ou libere a trava, e acione o buzzer.
