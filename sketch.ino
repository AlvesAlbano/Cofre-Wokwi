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
const int ledVERD = 18, ledVERM = 19, ledA = 15;
const int buzzerPin = 2;

String senhaCorreta = "12"; // Senha predefinida
String senhaOculta;
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

const char* mqtt_server = "broker.hivemq.com"; //servidor mqtt
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

volatile unsigned int interruptCounter = 0;
int totalInterruptCounter;
hw_timer_t *timer = NULL;

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
      // digitalWrite(ledA,HIGH);
      Serial.println("Conectado");              
      client.subscribe("cofre/acesso");
      client.subscribe("cofre/senha");
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
    lcd.clear();
    lcd.setCursor(2, 1);
    lcd.print("DESBLOQUEADO");
    somAcesso();
    delay(1000);
    reset();
  } else if (mensagem == "OFF") {
    bloqueado();
  } else {
    senhaCorreta = mensagem;
    Serial.println("Nova senha definida: " + senhaCorreta);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.print("Conectando-se ao Wi-Fi");
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Conectado!");
  pinMode(ledA, OUTPUT);
  conectarBroker();
  pinMode(btnIn, INPUT_PULLUP);
  pinMode(btnApg, INPUT_PULLUP);
  pinMode(ledVERM, OUTPUT);
  pinMode(ledVERD, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  trava.attach(travaPin);
  trava.write(180); 
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Digite a senha:");
  lcd.setCursor(1, 1);
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
  ledInterrupcao();
}

void digitar(){
  char key = keypad.getKey();

  if (key != NO_KEY && !(trancado)) {
    trava.detach();
    tone(buzzerPin, 800, 25);
    senha += key;
    senhaOculta += '*';
    
    lcd.setCursor(1, 1);
    lcd.print(senhaOculta);
  }
}

void apagar(){
  if (digitalRead(btnApg) == LOW) {
    if (senha.length() > 0) {
      tone(buzzerPin, 200, 50);
      senha.remove(senha.length() - 1);
      senhaOculta.remove(senhaOculta.length()-1);
      lcd.setCursor(1, 1);
      lcd.clear();
      lcd.print("Digite a senha:");
      lcd.setCursor(1, 1);
      lcd.setCursor(1, 1);
      lcd.print(senhaOculta);
      delay(200);
    }
  }
}

void inserir(){
  if (digitalRead(btnIn) == LOW && !(trancado)) {
    if (senha == senhaCorreta && tentativas < 3) {
      liberar();
    } 
    else if(tentativas == 3) {
      bloqueado();
    }
    else{
      travar();
    } 
  }
}

void liberar(){
  trava.attach(travaPin);
  tentativas = 0;
  digitalWrite(ledVERD, HIGH);
  trava.write(0);
  somAcesso();
  lcd.setCursor(0, 1);
  lcd.print("Acesso Concedido");
  client.publish("cofre/historico","Acesso Concedido");
  reset();
}

void travar(){
  digitalWrite(ledVERM, HIGH);
  tentativas++;
  if(tentativas == 3){
    lcd.setCursor(1, 0);
    lcd.clear();
    senha = "";
    trancado = true;
    bloqueado();
  } else {
    trava.detach();
    somTrava();
    lcd.setCursor(0, 1);
    lcd.print("Acesso Negado");
    client.publish("cofre/historico","Acesso Negado");
    reset();
  }
}

void reset(){
  delay(2000);
  digitalWrite(ledVERD, LOW);
  digitalWrite(ledVERM, LOW);
  trava.attach(travaPin);
  trava.write(180);
  lcd.setCursor(0, 1);
  lcd.clear();
  senha = "";
  senhaOculta = "";
  lcd.setCursor(0, 0);
  lcd.print("Digite a senha:");
  lcd.setCursor(1, 1);
}

void bloqueado(){
  digitalWrite(ledVERM, HIGH);
  lcd.setCursor(0, 0);
  lcd.clear();
  trava.detach();
  trancado = true;
  somTravaTotal();
  lcd.setCursor(3, 1);
  lcd.print("BLOQUEADO");
  delay(1000);
}

void somAcesso(){
  tone(buzzerPin, 1000, 1000);
}

void somTrava(){
  tone(buzzerPin, 262, 1000);
}

void somTravaTotal(){
  tone(buzzerPin, 400, 1000);
}

void ledInterrupcao(){
  
  int estado = interruptCounter % 2;
  
  if(WiFi.status() == WL_CONNECTED){
    if (estado == 1) {
      digitalWrite(ledA,HIGH);
    } else {
      digitalWrite(ledA,LOW);
    }
  }
}
