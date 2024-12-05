#include <EEPROM.h>     // Vamos ler e escrever os UIDs das Tags na EEPROM
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>
#include <SPI.h>
#include <Wire.h>

// Create instances
MFRC522 mfrc522(53, 9); // MFRC522 mfrc522(SS_PIN, RST_PIN)
LiquidCrystal_I2C lcd(0x27, 20, 4);
Servo myServo;  // criar objeto servo para controlar um servo

// Declare `message` como uma variável global
String message;
char nome[11];   // Buffer para o nome (10 caracteres + terminador null)
#define TAMANHO_NOME 10  // Define o tamanho máximo de cada nome

// Definir pinos para LEDs, servo, buzzer e botão de limpeza
constexpr uint8_t BuzzerPin = 41;
constexpr uint8_t wipeB = 40;     // Pino do botão para o modo de limpeza
constexpr uint8_t greenLed = 39;
constexpr uint8_t blueLed = 38;
constexpr uint8_t redLed = 37;
constexpr uint8_t ServoPin = 36;

boolean match = false;       // Inicializar correspondência de cartão como falso
boolean programMode = false;  // Inicializar modo de programação como falso
boolean replaceMaster = false;

uint8_t successRead;    // Variable integer to keep if we have Successful Read from Reader

byte storedName[TAMANHO_NOME + 1];  // Armazena o nome (TAMANHO_NOME bytes + terminador nulo)
char storedPass[4]; // Variable to get password from EEPROM
byte storedCard[4];   // Stores an ID read from EEPROM

byte readCard[4];   // Stores scanned ID read from RFID Module
byte masterCard[4];   // Stores master card's ID read from EEPROM



char password[4];   // Variable to store users password
char masterPass[4]; // Variable to store master password

boolean RFIDMode = true; // boolean to change modes
boolean NormalMode = true; // boolean to change modes

char key_pressed = 0; // Variable to store incoming keys
uint8_t i = 0;  // Variable used for counter

// defining how many rows and columns our keypad have
const byte rows = 4;
const byte columns = 4;

// Keypad pin map
char hexaKeys[rows][columns] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Initializing pins for keypad
byte row_pins[rows] = {45, 44, 43, 42}; // Conecte às linhas do teclado
byte column_pins[columns] = {46, 47, 48, 49}; // Conecte às colunas do teclado

unsigned long previousMillisHora = 0;
const long intervalHora = 1000; // Intervalo para atualizar a hora

unsigned long previousMillisBuzzer = 0;
const long intervalBuzzer = 1000; // Intervalo para o buzzer

unsigned long previousMillisPassword = 0;
const long intervalPassword = 2000; // Intervalo para a senha

// Create instance for keypad
Keypad newKey = Keypad( makeKeymap(hexaKeys), row_pins, column_pins, rows, columns);


///////////////////////////////////////// Setup ///////////////////////////////////
void setup() {

  //Arduino Pin Configuration
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(wipeB, INPUT_PULLUP);   // Enable pin's pull up resistor

  // Make sure leds are off
  digitalWrite(redLed, LOW);
  digitalWrite(greenLed, LOW);
  digitalWrite(blueLed, LOW);

  //Protocol Configuration
  Serial.begin(115200); // Inicializa a comunicação serial para o Monitor Serial
  Serial1.begin(9600); // Inicializa a Serial1 a 9600 baud
  lcd.begin();  // initialize the LCD
  lcd.backlight();
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware
  myServo.attach(ServoPin);   // attaches the servo on pin 8 to the servo object
  myServo.write(10);   // Initial Position

  // Se você definir o ganho da antena para o máximo, isso aumentará a distância de leitura
  // mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  ShowReaderDetails();  // Show details of PCD - MFRC522 Card Reader details

  //Wipe Code - If the Button (wipeB) Pressed while setup run (powered on) it wipes EEPROM
  if (digitalRead(wipeB) == LOW) {  // when button pressed pin should get low, button connected to ground
    digitalWrite(redLed, HIGH); // Red Led stays on to inform user we are going to wipe

    lcd.setCursor(0, 0);
    lcd.print("PF| Controlo ACESSOS");
    lcd.setCursor(0, 1);
    lcd.print("     Yuri SOUSA");
    lcd.setCursor(0, 3);
    lcd.print(" RESET ATIVO! ");
    digitalWrite(BuzzerPin, HIGH);
    delay(1000);
    digitalWrite(BuzzerPin, LOW);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PF| Controlo ACESSOS");
    lcd.setCursor(0, 1);
    lcd.print("     Yuri SOUSA");
    lcd.setCursor(0, 2);
    lcd.print("...apagar");
    lcd.setCursor(0, 3);
    lcd.print("  todos registos...");
    delay(2000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PF| Controlo ACESSOS");
    lcd.setCursor(0, 1);
    lcd.print("     Yuri SOUSA");
    lcd.setCursor(0, 3);
    lcd.print("10 seg. confirmar ");
    delay(2000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PF| Controlo ACESSOS");
    lcd.setCursor(0, 1);
    lcd.print("     Yuri SOUSA");
    lcd.setCursor(0, 2);
    lcd.print("tira rset p/cancelar");
    lcd.setCursor(0, 3);
    lcd.print("a contar: ");

    bool buttonState = monitorWipeButton(10000); // chamada da funçao contagem 10segundos

    if (buttonState == true && digitalRead(wipeB) == LOW) {    // Se o botão ainda estiver pressionado, limpe a EEPROM
      lcd.clear();
      lcd.print("apagar EEPROM...");
      for (uint16_t x = 0; x < EEPROM.length(); x = x + 1) {    //Fim do loop do endereço EEPROM
        if (EEPROM.read(x) == 0) {              //If EEPROM address 0
          // do nothing, already clear, go to the next address in order to save time and reduce writes to EEPROM
        }
        else {
          EEPROM.write(x, 0); // se não escrever 0 para limpar, leva 3,3mS
        }
      }
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PF| Controlo ACESSOS");
      lcd.setCursor(0, 1);
      lcd.print("     Yuri SOUSA");
      lcd.setCursor(0, 3);
      lcd.print("Removido c/ sucesso");
      delay(2000);

      // visualize a successful wipe
      cycleLeds();
    }
    else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PF| Controlo ACESSOS");
      lcd.setCursor(0, 1);
      lcd.print("     Yuri SOUSA");
      lcd.setCursor(0, 3);
      lcd.print("op. cancelada..."); // Show some feedback that the wipe button did not pressed for 10 seconds
      digitalWrite(redLed, LOW);
      delay(3000);

    }
  }

  // Verifica se o cartão master está definido, se não, deixa o usuário escolher um cartão master
  // Isso também é útil apenas para redefinir o Master Card
  // Você pode manter outros registros EEPROM, basta escrever diferente de 143 no endereço EEPROM 1
  // O endereço EEPROM 1 deve conter o número mágico que é '143'
  if (EEPROM.read(1) != 143) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PF| Controlo ACESSOS");
    lcd.setCursor(0, 1);
    lcd.print("     Yuri SOUSA");
    lcd.setCursor(0, 3);
    lcd.print("Sem Master Card ");
    delay(2000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PF| Controlo ACESSOS");
    lcd.setCursor(0, 1);
    lcd.print("     Yuri SOUSA ");
    lcd.setCursor(0, 3);
    lcd.print("Passar Tag Master ");


    do {
      successRead = getID();  // define sucessoRead como 1 quando somos lidos pelo leitor, caso contrário, 0
      // Visualizar Cartão Mestre precisa ser definido
      digitalWrite(blueLed, HIGH);
      digitalWrite(BuzzerPin, HIGH);
      delay(200);
      digitalWrite(BuzzerPin, LOW);
      digitalWrite(blueLed, LOW);
      delay(200);
    }
    while (!successRead);  // O programa não avançará enquanto não obtiver uma leitura bem-sucedida
    for (uint8_t j = 0; j < 4; j++) {  // Loop 4 vezes
      EEPROM.write(2 + j, readCard[j]);  // Escreve o UID do Tag lido na EEPROM, começando do endereço 3
    }
    EEPROM.write(1, 143);  // Escreve na EEPROM que definimos o Cartão Mestre.
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PF| Controlo ACESSOS");
    lcd.setCursor(0, 1);
    lcd.print("     Yuri SOUSA");
    lcd.setCursor(0, 3);
    lcd.print("Master Definido!");
    delay(2000);
    storePassword(6);  // Armazena a senha para o tag mestre. 6 é a posição na EEPROM
  }

  // Lê o UID do Cartão Mestre e a senha mestre da EEPROM
  for (uint8_t i = 0; i < 4; i++) {
    masterCard[i] = EEPROM.read(2 + i);  // Escreve no masterCard
    masterPass[i] = EEPROM.read(6 + i);  // Escreve no masterPass
  }

  ShowOnLCD();  // Imprime no LCD
  cycleLeds();  // Tudo pronto, vamos dar algum feedback ao usuário alternando os LEDs
}


///////////////////////////////////////// Main Loop ///////////////////////////////////
void loop() {
  unsigned long currentMillis = millis();
  // System will first look for mode. if RFID mode is true then it will get the tags otherwise it will get keys
  int n = 0;
  int tentativa = 0;

  if (RFIDMode == true) {
    do {
      successRead = getID();  // define successRead como 1 quando lemos do leitor, caso contrário, 0

      if (programMode) {
        cycleLeds();  // Modo de Programação: alterna entre Vermelho, Verde e Azul à espera de ler um novo cartão

      } else {
        normalModeOn();  // Modo Normal: o LED de Alimentação azul está ligado, todos os outros estão desligados
        atualizarHora();
      }
    } while (!successRead);  // o programa não avançará enquanto não obtiver uma leitura bem-sucedida

    if (programMode) {
      if (isMaster(readCard)) {  // Quando em modo de programação, verifica primeiro se o cartão mestre foi lido novamente para sair do modo de programação
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("PF| Controlo ACESSOS");
        lcd.setCursor(0, 1);
        lcd.print("       Yuri SOUSA");
        lcd.setCursor(0, 3);
        lcd.print("Sair do MENU MASTER ");
        digitalWrite(BuzzerPin, HIGH);
        previousMillisBuzzer = currentMillis;
        while (millis() - previousMillisBuzzer < intervalBuzzer) {
          // Espera 1 segundo sem bloquear o programa
        }
        digitalWrite(BuzzerPin, LOW);
        ShowOnLCD();
        programMode = false;
        return;
      } else {
        if (findID(readCard)) { // Se o cartão digitalizado for conhecido, exclua-o
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("  PF| MENU MASTER ");
          lcd.setCursor(0, 2);
          lcd.print(" Tag Existente");
          deleteID(readCard);


          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("  PF| MENU MASTER ");
          lcd.setCursor(0, 1);
          lcd.print("Passa Tag ADD/REM");
          lcd.setCursor(0, 2);
          lcd.print("Passar Master P/Sair");
        } else { // Se o cartão digitalizado não for conhecido, adicione-o

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("  PF| MENU MASTER ");
          lcd.setCursor(0, 2);
          lcd.print("Nova tag adicionando...");
          lcd.setCursor(0, 3);
          lcd.print("Adicionando Tag...");
          // writeID(readCard);
          escreverIDComNome(readCard);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("  PF| MENU MASTER ");
          lcd.setCursor(0, 1);
          lcd.print("Passa Tag ADD/REM");
          lcd.setCursor(0, 3);
          lcd.print("Passar Master P/Sair");

        }
      }
    } else {
      if (isMaster(readCard)) { // Se o ID do cartão digitalizado corresponder ao ID do Master Card - entre no modo Master
        programMode = true;
        matchpass();
        if (programMode == true) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("  PF| MENU MASTER ");
          uint8_t count = EEPROM.read(0); // Leia o primeiro byte da EEPROM que armazena o número de IDs na EEPROM
          lcd.setCursor(0, 2);
          lcd.print(count);
          lcd.print(" Tag Registos");
          digitalWrite(BuzzerPin, HIGH);
          previousMillisBuzzer = currentMillis;
          delay(2000);
          digitalWrite(BuzzerPin, LOW);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("  PF| MENU MASTER ");
          lcd.setCursor(0, 1);
          lcd.print("     Yuri Sousa");
          lcd.setCursor(0, 2);
          lcd.print("Passa Tag ADD/REM");
          lcd.setCursor(0, 3);
          lcd.print("Passar Master P/Sair");
        }
      } else {
        if (findID(readCard)) { // Caso contrário, veja se o cartão do usuario
          granted();
          RFIDMode = false; // Tornar o modo RFID falso
          ShowOnLCD();
        } else { // Caso contrário, mostre que o acesso foi negado
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("PF| Controlo ACESSOS");
          lcd.setCursor(0, 1);
          lcd.print("     Yuri Sousa");
          lcd.setCursor(0, 3);
          lcd.print("   Acesso negado");
          denied();
          ShowOnLCD();
        }
      }
    }
  } else if (RFIDMode == false) { // Se o modo RFID for falso, obtenha as chaves
    while (n < 1) { // Aguarde até obtermos 4 teclas
      key_pressed = newKey.getKey(); // Armazenar nova chave

      if (key_pressed) {
        password[i++] = key_pressed; // Armazenando em variável de senha
        lcd.print("*");
      }
      if (i == 4) { // Se 4 chaves forem concluídas
        /*previousMillisPassword = currentMillis;
          while (millis() - previousMillisPassword < intervalPassword / 10) {
          // Espera 200ms sem bloquear o programa
          }*/
        delay(200);

        if (!(strncmp(password, storedPass, 4))) { // Se a senha for correspondida
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("PF| Controlo Acessos");
          lcd.setCursor(0, 1);
          lcd.print("     Yuri Sousa");
          lcd.setCursor(0, 2);
          lcd.print("    Passe Aceite");
          previousMillisPassword = currentMillis;
          while (millis() - previousMillisPassword < intervalPassword * 2) {
            // Espera 2 segundos sem bloquear o programa
          }

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("PF| Controlo Acessos");
          lcd.setCursor(0, 1);
          lcd.print("     Yuri Sousa");
          lcd.setCursor(0, 3);
          lcd.print("    Porta Aberta");

          while (millis() - previousMillisPassword < intervalPassword * 2) {
            // Espera 2 segundos sem bloquear o programa
          }

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("PF| Controlo Acessos");
          lcd.setCursor(0, 1);
          lcd.print("     Yuri Sousa");
          lcd.setCursor(0, 2);
          lcd.print("BEM VINDO: ");
          lcd.print((char*)storedName);

          granted();
          RFIDMode = true; // Tornar o modo RFID verdadeiro
          ShowOnLCD();
          i = 0;
          tentativa = 0; // Reinicia as tentativas após sucesso
          n = 4; // Sai do loop após sucesso
        } else { // Se a senha não corresponder
          tentativa++; // Incrementa o contador de tentativas
          if (tentativa >= 3) { // Verifica se o número de tentativas excedeu o limite
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("PF| Controlo ACESSOS");
            lcd.setCursor(0, 1);
            lcd.print("     Yuri Sousa");
            lcd.setCursor(0, 3);
            lcd.print("  Acesso Bloqueado");
            digitalWrite(BuzzerPin, HIGH);
            previousMillisBuzzer = currentMillis;
            while (millis() - previousMillisBuzzer < intervalBuzzer) {
              // Espera 1 segundo sem bloquear o programa
            }
            digitalWrite(BuzzerPin, LOW);
            RFIDMode = true;
            ShowOnLCD();
            i = 0;
            n = 4; // Sai do loop após 3 tentativas falhadas
          } else { // Se a senha estiver incorreta
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("PF| Controlo Acessos");
            lcd.setCursor(0, 1);
            lcd.print("     Yuri Sousa");
            lcd.setCursor(0, 3);
            lcd.print("Password Errada :");
            previousMillisPassword = currentMillis;
            while (millis() - previousMillisPassword < intervalPassword * 2) {
              // Espera 2 segundos sem bloquear o programa
            }

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("PF| Controlo Acessos");
            lcd.setCursor(0, 1);
            lcd.print("     Yuri Sousa ");
            lcd.setCursor(0, 2);
            lcd.print(" Tente Novamente...");
            previousMillisPassword = currentMillis;
            while (millis() - previousMillisPassword < intervalPassword * 2) {
              // Espera 2 segundos sem bloquear o programa
            }
            denied();
            RFIDMode = false;   // Make RFID mode true
            ShowOnLCD();
            i = 0;
          }
        }
      }
    }
  }
}


void atualizarHora() {
    if (Serial1.available()) {
        message = Serial1.readStringUntil('\n');
        message.trim(); // Remove espaços em branco e quebras de linha no início e no fim

        // Debug: Mostra a mensagem recebida no Monitor Serial
        Serial.println("Hora recebida: " + message);

        // Extrai apenas os 8 primeiros caracteres da mensagem
        String horaFormatada = message.substring(0, 11);

        // Exibe a hora formatada no display
        lcd.setCursor(0, 2);
        lcd.print("    ");
        lcd.print(horaFormatada);

        // Debug: Mostra a hora formatada no Monitor Serial
        Serial.println("Hora formatada: " + horaFormatada);
    }
}
/////////////////////////////////////////  Access Granted    ///////////////////////////////////
void granted() {
  digitalWrite(blueLed, LOW);   // Turn off blue LED
  digitalWrite(redLed, LOW);  // Turn off red LED
  digitalWrite(greenLed, HIGH);   // Turn on green LED
  if (RFIDMode == false) {
    myServo.write(180);
    delay(3000);
    myServo.write(10);
  }
  delay(1000);
  digitalWrite(blueLed, HIGH);
  digitalWrite(greenLed, LOW);
  mfrc522.PCD_Init();  // Reinicializa o leitor RFID

}

///////////////////////////////////////// Access Denied  ///////////////////////////////////
void denied() {
  digitalWrite(greenLed, LOW);  // Make sure green LED is off
  digitalWrite(blueLed, LOW);   // Make sure blue LED is off
  digitalWrite(redLed, HIGH);   // Turn on red LED
  digitalWrite(BuzzerPin, HIGH);
  delay(1000);
  digitalWrite(BuzzerPin, LOW);
  digitalWrite(blueLed, HIGH);
  digitalWrite(redLed, LOW);
}


///////////////////////////////////////// Get Tag's UID ///////////////////////////////////
uint8_t getID() {
  // Getting ready for Reading Tags
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //Se uma nova etiqueta for colocada no leitor RFID, continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a Tag placed get Serial and continue
    return 0;
  }
  // There are Mifare Tags which have 4 byte or 7 byte UID care if you use 7 byte Tag
  // I think we should assume every Tag as they have 4 byte UID
  // Until we support 7 byte Tags
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
  }
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

/////////////////////// Check if RFID Reader is correctly initialized or not /////////////////////
void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);

  // Quando 0x00 ou 0xFF é retornado, a comunicação provavelmente falhou
  if ((v == 0x00) || (v == 0xFF)) {
    lcd.setCursor(0, 0);
    lcd.print("PF| Controlo Acessos");
    lcd.setCursor(0, 1);
    lcd.print("     Yuri Sousa");
    lcd.setCursor(0, 3);
    lcd.print("Falha de comunicacao");
    digitalWrite(BuzzerPin, HIGH);
    delay(5000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PF| Controlo Acessos");
    lcd.setCursor(0, 1);
    lcd.print("     Yuri Sousa");
    lcd.setCursor(0, 2);
    lcd.print("     Verifique ");
    lcd.setCursor(0, 3);
    lcd.print("    As Conexoes");
    digitalWrite(BuzzerPin, HIGH);
    delay(2000);
    // Visualize que o sistema está parado
    digitalWrite(greenLed, LOW);  // Make sure green LED is off
    digitalWrite(blueLed, LOW);   // Make sure blue LED is off
    digitalWrite(redLed, HIGH);   // Turn on red LED
    digitalWrite(BuzzerPin, LOW);
    while (true); // do not go further
  }
}

///////////////////////////////////////// Cycle Leds (Program Mode) ///////////////////////////////////
void cycleLeds() {
  digitalWrite(redLed, LOW);  // Make sure red LED is off
  digitalWrite(greenLed, HIGH);   // Make sure green LED is on
  digitalWrite(blueLed, LOW);   // Make sure blue LED is off
  delay(200);
  digitalWrite(redLed, LOW);  // Make sure red LED is off
  digitalWrite(greenLed, LOW);  // Make sure green LED is off
  digitalWrite(blueLed, HIGH);  // Make sure blue LED is on
  delay(200);
  digitalWrite(redLed, HIGH);   // Make sure red LED is on
  digitalWrite(greenLed, LOW);  // Make sure green LED is off
  digitalWrite(blueLed, LOW);   // Make sure blue LED is off
  delay(200);
  digitalWrite(redLed, LOW);   // Make sure red LED is on
  digitalWrite(greenLed, LOW);  // Make sure green LED is off
  digitalWrite(blueLed, HIGH);   // Make sure blue LED is off
}

//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn () {
  digitalWrite(blueLed, HIGH);  // Blue LED ON and ready to read card
  digitalWrite(redLed, LOW);  // Make sure Red LED is off
  digitalWrite(greenLed, LOW);  // Make sure Green LED is off
}

//////////////////////////////////////// Read an ID, Name, and PIN from EEPROM //////////////////////////////
void readID(uint8_t number) {
  uint8_t start = (number * (4 + TAMANHO_NOME + 4)) + 10; // Calcula a posição inicial para a leitura
  for (uint8_t i = 0; i < 4; i++) {
    storedCard[i] = EEPROM.read(start + i);  // Lê o ID (4 bytes)
  }

  // Lê o nome
  for (uint8_t j = 0; j < TAMANHO_NOME; j++) {
    storedName[j] = EEPROM.read(start + 4 + j);  // Lê o nome (até TAMANHO_NOME caracteres)
  }
  storedName[TAMANHO_NOME] = '\0';  // Adiciona o terminador nulo para garantir que seja uma string

  for (uint8_t i = 0; i < 4; i++) {
    storedPass[i] = EEPROM.read(start + 4 + TAMANHO_NOME + i);  // Lê o PIN (4 bytes)
  }
}

void capturarNome() {
  // Limpa a variável nome antes de capturar o novo nome
  memset(nome, 0, sizeof(nome));

  Serial.println("Digite o nome associado ao cartão:");

  // Aguarda que o usuário digite o nome
  uint8_t i = 0;
  unsigned long timeout = millis() + 60000; // Timeout de 10 segundos para a captura do nome

  while (i < TAMANHO_NOME) {
    if (Serial.available()) {
      char c = Serial.read();  // Lê o caractere

      // Se pressionar Enter ou uma tecla de nova linha, interrompe
      if (c == '\n' || c == '\r') {
        break;
      }

      // Armazena o caractere no nome
      nome[i] = c;
      i++;
    }

    // Se passar o tempo limite, sai
    if (millis() > timeout) {
      Serial.println("Tempo limite excedido.");
      break;
    }

    delay(10); // Delay para evitar que a leitura ocorra rapidamente demais
  }

  // Adiciona o terminador nulo para garantir que o nome seja uma string válida
  nome[i] = '\0';

  // Exibe o ID e o nome no monitor serial
  Serial.print("Tag ");
  Serial.print(i + 1);
  Serial.print(": ID: ");
  for (uint8_t j = 0; j < 4; j++) {
    Serial.print(storedCard[j], HEX); // Exibe o ID em formato hexadecimal
    Serial.print(" ");
  }
  Serial.print("Nome: ");
  Serial.println((char*)storedName); // Exibe o nome
}


///////////////////////////////////////// Add ID E NOme EEPROM   ///////////////////////////////////
void escreverIDComNome(byte a[]) {
  Serial.println("Iniciando gravação do ID com nome...");

  if (!findID(a)) {  // Verifica se o ID do cartão já está armazenado
    uint8_t num = EEPROM.read(0);     // Lê o número atual de IDs armazenados
    uint8_t start = (num * (4 + TAMANHO_NOME + 4)) + 10; // Calcula a posição de início do próximo slot (ID + Nome + PIN)
    num++;  // Incrementa o contador
    EEPROM.write(0, num);  // Armazena a contagem atualizada
    Serial.print("Número de tags (num): ");
    Serial.println(num);
    Serial.print("Posição inicial (start): ");
    Serial.println(start);

    // Armazena o ID no EEPROM
    for (uint8_t j = 0; j < 4; j++) {
      EEPROM.write(start + j, a[j]);
      Serial.print("Byte ID ");
      Serial.print(j);
      Serial.print(": ");
      Serial.println(a[j], HEX); // Debug: mostra o ID sendo escrito
    }

    // Chama a função para capturar o nome via serial
    capturarNome();

    // Armazena o nome no EEPROM (até o TAMANHO_NOME caracteres)
    uint8_t nome_len = strlen(nome); // Calcula o comprimento do nome uma vez
    for (uint8_t j = 0; j < TAMANHO_NOME; j++) {
      if (j < nome_len) {
        EEPROM.write(start + 4 + j, nome[j]);
        Serial.print("Gravando caractere ");
        Serial.print(j);
        Serial.print(": ");
        Serial.println(nome[j]);
      } else {
        EEPROM.write(start + 4 + j, 0); // Preenche o restante com 0
      }
    }

    // Verifica os dados gravados (para depuração)
    Serial.println("Verificando dados gravados:");
    for (uint8_t i = 0; i < TAMANHO_NOME; i++) {
      Serial.print("EEPROM ");
      Serial.print(start + 4 + i);
      Serial.print(": ");
      Serial.println(EEPROM.read(start + 4 + i), DEC); // Exibe o conteúdo gravado
    }

    // Ajusta a posição para o PIN e senha
    uint8_t pinStart = start + 4 + TAMANHO_NOME; // Calcula a posição para o PIN
    // Armazena o PIN na EEPROM
    Serial.println("Armazenando PIN...");
    storePassword(pinStart); // Chama função de armazenamento do PIN
    BlinkLEDS(greenLed);

    // Mostra mensagem de sucesso no LCD
    lcd.setCursor(0, 1);
    lcd.print("   Tag Adicionada");
    delay(1000);
    listarTags();
  } else {
    // Mostra mensagem de falha no LCD
    Serial.println("Falha: ID já existente ou EEPROM com problema.");
    BlinkLEDS(redLed);
    lcd.setCursor(0, 0);
    lcd.print("Falhou!");
    lcd.setCursor(0, 1);
    lcd.print("ID errado ou EEPROM");
    lcd.setCursor(0, 2);
    lcd.print("Com defeito");
    delay(2000);
  }
}

void listarTags() {
  uint8_t numTags = EEPROM.read(0); // Lê o número de tags armazenadas (supondo que está no endereço 0)

  Serial.print("Número de tags gravadas: ");
  Serial.println(numTags);

  for (uint8_t i = 0; i < numTags; i++) {
    uint8_t start = (i * (4 + TAMANHO_NOME + 4)) + 10; // Calcula a posição inicial para a leitura

    // Lê o ID
    byte storedCard[4];
    for (uint8_t j = 0; j < 4; j++) {
      storedCard[j] = EEPROM.read(start + j);  // Lê os 4 bytes do ID
    }

    // Lê o nome
    byte storedName[TAMANHO_NOME + 1]; // Para armazenar o nome + terminador nulo
    for (uint8_t j = 0; j < TAMANHO_NOME; j++) {
      storedName[j] = EEPROM.read(start + 4 + j);  // Lê o nome armazenado
    }
    storedName[TAMANHO_NOME] = '\0';  // Adiciona o terminador nulo para garantir que seja uma string válida

    // Exibe o ID e o nome no monitor serial
    Serial.print("Tag ");
    Serial.print(i + 1);
    Serial.print(": ID: ");
    for (uint8_t j = 0; j < 4; j++) {
      Serial.print(storedCard[j], HEX); // Exibe o ID em formato hexadecimal
      Serial.print(" ");
    }
    Serial.print("Nome: ");
    Serial.println((char*)storedName); // Exibe o nome
  }
}


///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Antes de deletar da EEPROM, verifique se temos este cartão!
    BlinkLEDS(redLed);      // If not
    lcd.setCursor(0, 0);
    lcd.print("Falhou!");
    lcd.setCursor(0, 1);
    lcd.print("ID errado ou EEPROM");
    lcd.setCursor(0, 2);
    lcd.print("Com defeito");
    delay(2000);
  }
  else {
    uint8_t num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t slot;       // Figure out the slot number of the card
    uint8_t start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    uint8_t looping;    // The number of times the loop repeats
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 8) + 10;
    looping = ((num - slot)) * 4;
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( uint8_t k = 0; k < 4; k++ ) {         // Shifting loop
      EEPROM.write( start + j + k, 0);
    }
    BlinkLEDS(blueLed);
    lcd.setCursor(0, 1);
    lcd.print("Tag Removida");
    delay(2000);
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != 0 )      // Make sure there is something in the array first
    match = true;       // Assume they match at first
  for ( uint8_t k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] )     // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if ( match ) {      // Check to see if if match is still true
    return true;      // Return true
  }
  else  {
    return false;       // Return false
  }
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
uint8_t findIDSLOT( byte find[] ) {
  uint8_t count = EEPROM.read(0);       // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i;         // The slot number of the card
      break;          // Stop looking we found it
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
boolean findID(byte find[]) {
  uint8_t count = EEPROM.read(0);     // Lê o número de IDs armazenados no primeiro byte do EEPROM

  for (uint8_t i = 0; i < count; i++) {    // Loop para percorrer todos os IDs armazenados
    readID(i);  // Lê um ID do EEPROM, os dados são armazenados em storedCard[4]

    // Depuração: Imprimir os IDs lidos para verificação
    Serial.print("ID lido: ");
    for (int j = 0; j < 4; j++) {
      Serial.print(storedCard[j], HEX);  // Exibe o conteúdo do ID armazenado
      Serial.print(" ");
    }
    Serial.println();  // Pula para a linha seguinte para facilitar a leitura no monitor serial


    if (checkTwo(find, storedCard)) {  // Compara o ID encontrado com o ID que estamos procurando
      return true;  // Se encontrar, retorna verdadeiro
    }
  }

  return false;  // Se não encontrar o ID, retorna falso
}



///////////////////////////////////////// Blink LED's For Indication   ///////////////////////////////////
void BlinkLEDS(int led) {
  digitalWrite(blueLed, LOW);   // Make sure blue LED is off
  digitalWrite(redLed, LOW);  // Make sure red LED is off
  digitalWrite(greenLed, LOW);  // Make sure green LED is off
  digitalWrite(BuzzerPin, HIGH);
  delay(200);
  digitalWrite(led, HIGH);  // Make sure blue LED is on
  digitalWrite(BuzzerPin, LOW);
  delay(200);
  digitalWrite(led, LOW);   // Make sure blue LED is off
  digitalWrite(BuzzerPin, HIGH);
  delay(200);
  digitalWrite(led, HIGH);  // Make sure blue LED is on
  digitalWrite(BuzzerPin, LOW);
  delay(200);
  digitalWrite(led, LOW);   // Make sure blue LED is off
  digitalWrite(BuzzerPin, HIGH);
  delay(200);
  digitalWrite(led, HIGH);  // Make sure blue LED is on
  digitalWrite(BuzzerPin, LOW);
  delay(200);
}

////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) ) {
    return true;
  }
  else
    return false;
}

/////////////////// Função de contagem 10segundos pelo rset pressionado   /////////////////////
bool monitorWipeButton(uint32_t interval) {
  unsigned long currentMillis = millis(); // grab current time

  while (millis() - currentMillis < interval)  {
    int timeSpent = (millis() - currentMillis) / 1000;
    Serial.println(timeSpent);
    lcd.setCursor(10, 3);
    lcd.print(timeSpent);

    // check on every half a second
    if (((uint32_t)millis() % 10) == 0) {
      if (digitalRead(wipeB) != LOW) {
        return false;
      }
    }
  }
  return true;
}

////////////////////// Informacao no LCD - standby ///////////////////////////////////
void ShowOnLCD() {
  if (RFIDMode == false) {

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PF| Controlo ACESSOS");
    lcd.setCursor(0, 1);
    lcd.print("     Yuri SOUSA");
    lcd.setCursor(0, 2);
    lcd.print("  Inserir Password:");
    lcd.setCursor(0, 3);
  } else if (RFIDMode == true) {

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PF| Controlo ACESSOS");
    lcd.setCursor(0, 1);
    lcd.print("      Yuri SOUSA ");
     lcd.setCursor(0, 2);
    lcd.print("                    ");
    lcd.setCursor(0, 3);
    lcd.print("    Passar a Tag");
  }
}

//////////////////////  ESCREVER PASSWORD EEPROM   ///////////////////////////////////
void storePassword(int j) {
  int k = j + 4;  // Aqui, o PIN começa após o nome, garantindo que o nome e o PIN não se sobrescrevam
  BlinkLEDS(blueLed);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PF| Controlo ACESSOS");
  lcd.setCursor(0, 1);
  lcd.print("  PF| MENU MASTER ");
  lcd.setCursor(0, 2);
  lcd.print("   Nova Password:");
  lcd.setCursor(0, 3);
  while (j < k ) {  // Armazena o PIN na EEPROM
    char key = newKey.getKey();
    if (key) {
      lcd.print("*");
      EEPROM.write(j, key);  // Armazena cada caractere do PIN na posição correta
      j++;
    }
  }
}


////////////////////// Correspondência de Senhas  ///////////////////////////////////
void matchpass() {
  RFIDMode = false;
  ShowOnLCD();
  int n = 0;
  int tentativas = 0;  // Contador de tentativas
  i = 0;  // Índice de entrada da senha

  while (n < 1) {  // Aguarda até obtermos 4 teclas
    key_pressed = newKey.getKey();  // Lê nova tecla

    if (key_pressed) {
      password[i++] = key_pressed;  // Armazena a tecla na senha
      lcd.print("*");  // Exibe asterisco para cada tecla

      if (i == 4) {  // Verifica se 4 teclas foram digitadas
        delay(200);

        if (strncmp(password, masterPass, 4) == 0) {  // Se a senha for correta
          RFIDMode = true;
          programMode = true;
          i = 0;
          tentativas = 0;  // Reseta as tentativas após sucesso
          n = 4;  // Sai do loop após sucesso
          ShowOnLCD();
        } else {  // Se a senha estiver incorreta
          tentativas++;  // Incrementa o contador de tentativas
          i = 0;  // Reseta o índice de entrada de senha

          if (tentativas >= 3) {  // Verifica se excedeu o limite de tentativas
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("PF| Controlo ACESSOS");
            lcd.setCursor(0, 1);
            lcd.print("     Yuri Sousa");
            lcd.setCursor(0, 3);
            lcd.print("  Acesso Bloqueado");

            digitalWrite(BuzzerPin, HIGH);
            delay(1000);
            digitalWrite(BuzzerPin, LOW);
            programMode = false;
            RFIDMode = true;
            ShowOnLCD();
            n = 4;  // Sai do loop após 3 tentativas falhas
          } else {  // Tentativa incorreta, mas abaixo do limite
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("PF| Controlo ACESSOS");
            lcd.setCursor(0, 1);
            lcd.print("     Yuri Sousa");
            lcd.setCursor(0, 3);
            lcd.print("    Senha Errada");

            digitalWrite(BuzzerPin, HIGH);
            delay(1000);
            digitalWrite(BuzzerPin, LOW);
            programMode = false;
            RFIDMode = false;
            ShowOnLCD();
          }
        }
      }
    }
  }
}
