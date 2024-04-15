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

//BROKER MQTT============================================================================================================================

const int ledA = 15;
const char* mqtt_server = "broker.hivemq.com"; //servidor mqtt
WiFiClient espClient;             //criação do objeto espClient do tipo WiFiClient
PubSubClient client(espClient);   //abstrai
unsigned long lastMsg = 0;        //unsigned long = inteiro de 32 bits sem sinal
#define MSG_BUFFER_SIZE  (50)     //abstrai
char msg[MSG_BUFFER_SIZE];        //abstrai

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
  if ((char)payload[0] == 'a') {
    digitalWrite(ledA, LOW);
   
  }
  if ((char)payload[0] == 'A') {
    digitalWrite(ledA, HIGH);
    
  }

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
    
    String novaSenha = "";

    // Encontrar a posição do número
    int posicaoInicio = mensagem.indexOf(':') + 1;
    int posicaoFim = mensagem.lastIndexOf('}');
    // int posicaoFim = mensagem.length();

    // Extrair o número como uma string
    novaSenha = mensagem.substring(posicaoInicio, posicaoFim);

    // Converter a string para um número inteiro (opcional)
    senhaCorreta = novaSenha;

    // Agora, 'numero' contém o número extraído
    Serial.println("nova senha definida: " + senhaCorreta);
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
  pinMode(ledA, OUTPUT);
  conectarBroker();
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
  lcd.setCursor(0, 0);
  lcd.print("Digite a senha:");
  lcd.setCursor(1, 1);
  lcd.blink();
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
      lcd.clear(); // Limpa o campo de senha
      lcd.print("Digite a senha:");
      lcd.setCursor(1, 1);
      lcd.setCursor(1, 1);
      lcd.print(senhaOculta);
      delay(200); // Pequeno atraso para evitar múltiplas leituras do botão
    }
  }
}

void inserir(){
  if (digitalRead(btnIn) == LOW && !(trancado)) {
    if (senha == senhaCorreta && tentativas<3) {
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
  trava.attach(travaPin); // Liga o sinal enviado para o servo, "liberando-o"
  trava.write(180);
  lcd.setCursor(0, 1);
  lcd.clear(); // Limpa a linha
  senha = ""; // Limpa a senha digitada
  senhaOculta = "";
  lcd.setCursor(0, 0);
  lcd.print("Digite a senha:");
  lcd.setCursor(1, 1);
}

void bloqueado(){
  digitalWrite(ledVERM, HIGH);
  lcd.setCursor(0, 0);
  lcd.clear(); // Limpa a linha
  trava.detach(); // Desliga o sinal enviado para o servo, "travando-o"
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
