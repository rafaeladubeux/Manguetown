#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"

LiquidCrystal_I2C lcd(0x27, 16, 2); 

// --- CONFIGURAÇÃO DO DFPLAYER MINI (Atualizado para pinos 50 e 53) ---
#define pinRX 50  // Conecta no TX do DFPlayer através de um resistor de 1k
#define pinTX 53  // Conecta no RX do DFPlayer

SoftwareSerial serialMP3(pinRX, pinTX);
DFRobotDFPlayerMini moduloMP3;

// --- REORGANIZADO PARA: Vermelho, Verde, Amarelo, Azul ---
const int BTN[4] = {4, 8, 10, 12};          // Vermelho (4), Verde (8), Amarelo (10), Azul (12)
const int LED_BOTOES[4] = {5, 7, 9, 11};    // Vermelho (5), Verde (7), Amarelo (9), Azul (11)

// Matriz de LEDs de Vida: [Jogador][LED 1, LED 2, LED 3]
const int LED_VIDAS[4][3] = {
  {22, 24, 26},   // Jogador 0 (Vermelho) - Pinos dos 3 LEDs
  {31, 33, 35},   // Jogador 1 (Verde)    - Pinos dos 3 LEDs
  {28, 30, 32},   // Jogador 2 (Amarelo)  - Pinos dos 3 LEDs
  {40, 37, 38}    // Jogador 3 (Azul)     - Pinos dos 3 LEDs
};

const int BTN_BATALHA = 2; 

// Nomes reordenados para bater com os índices acima
const String cores[4] = {"Vermelho", "Verde", "Amarelo", "Azul"};

int jogadorDaVez = 0;  
int faseJogada = 0;    
int resultadoDado = 0;

int vidas[4] = {3, 3, 3, 3}; // Todos começam com 3 vidas (3 LEDs acesos)
int contadorJogadas = 0;     
int limiteJogadasBatalha = 0;

void atualizarPainelELEDs();
void executarBatalhaAleatoria();
void atualizarTodosOsLEDsDeVida();
void verificarFimDeJogo();
void somBatalha();
void printRetorno(uint8_t type, int value);

void setup() {
  Serial.begin(9600);
  serialMP3.begin(9600); // Inicializa a serial do MP3
  
  Serial.println("--- Jogo Iniciado ---");

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Jogo Iniciado ");

  Serial.println("Inicializando DFPlayer...");
  if (!moduloMP3.begin(serialMP3, true, true)) {
    Serial.println("Falha ao iniciar DFPlayer.");
  } else {
    Serial.println("DFPlayer Online.");
    moduloMP3.volume(30);              // Configura o volume (0 a 30)
    moduloMP3.EQ(DFPLAYER_EQ_NORMAL); // Configura o equalizador
    
    // Som de introdução/abertura (Pasta 1, Arquivo 01)
    moduloMP3.playFolder(1, 1); 
  }

  delay(75000);

  randomSeed(analogRead(0));
  
  for (int i = 0; i < 4; i++) {
    pinMode(BTN[i], INPUT_PULLUP); 
    pinMode(LED_BOTOES[i], OUTPUT);
    digitalWrite(LED_BOTOES[i], LOW);
  }

  // Inicializa todos os pinos de LEDs de vida como SAÍDA
  for (int j = 0; j < 4; j++) {
    for (int v = 0; v < 3; v++) {
      pinMode(LED_VIDAS[j][v], OUTPUT);
    }
  }

  pinMode(BTN_BATALHA, INPUT_PULLUP);
  limiteJogadasBatalha = random(3, 7); 

  atualizarTodosOsLEDsDeVida(); // Liga os 3 LEDs de todo mundo no início
  atualizarPainelELEDs();
}

void loop() {
  int jogadorAtual = jogadorDaVez;

  // Se o jogador da vez não tiver mais vida, passa para o próximo ativo
  if (vidas[jogadorAtual] <= 0) {
    do {
      jogadorDaVez = (jogadorDaVez + 1) % 4;
    } while (vidas[jogadorDaVez] <= 0);
    atualizarPainelELEDs();
    return;
  }

  if (digitalRead(BTN[jogadorAtual]) == LOW) {
    if (faseJogada == 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sorteando...");
      delay(2000); 
      
      resultadoDado = random(1, 7); 
      contadorJogadas++; 
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Resultado: ");
      lcd.setCursor(12, 0);
      lcd.print(resultadoDado);
      lcd.setCursor(0, 1);
      lcd.print("  Passar a vez ");
      
      Serial.println("Jogador " + cores[jogadorAtual] + " tirou " + String(resultadoDado));
      faseJogada = 1; 
    }
    else if (faseJogada == 1) {
      if (contadorJogadas >= limiteJogadasBatalha) {
        executarBatalhaAleatoria();
        contadorJogadas = 0; 
        limiteJogadasBatalha = random(3, 7); 
      }
      
      do {
        jogadorDaVez = (jogadorDaVez + 1) % 4; 
      } while (vidas[jogadorDaVez] <= 0); 
      
      faseJogada = 0; 
      atualizarPainelELEDs();
    }

    while (digitalRead(BTN[jogadorAtual]) == LOW) { delay(10); }
    delay(200); 
  }

  if (serialMP3.available()) {
    printRetorno(moduloMP3.readType(), serialMP3.read());
  }
}

void executarBatalhaAleatoria() {
  somBatalha(); 

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" !! HORA DA !!");
  lcd.setCursor(0, 1);
  lcd.print(" !! BATALHA !!");
  delay(4000);

  int p1 = -1;
  int p2 = -1;

  // Sorteia dois jogadores que ainda tenham vidas
  do { p1 = random(0, 4); } while (vidas[p1] <= 0);
  do { p2 = random(0, 4); } while (vidas[p2] <= 0 || p2 == p1);

  // Desliga temporariamente indicadores de botões para focar na batalha
  for (int j = 0; j < 4; j++) {
    digitalWrite(LED_BOTOES[j], LOW);
  }

  atualizarTodosOsLEDsDeVida();

  // Acende os botões dos envolvidos para avisar o duelo
  digitalWrite(LED_BOTOES[p1], HIGH);
  digitalWrite(LED_BOTOES[p2], HIGH);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(cores[p1].substring(0,3) + " VS " + cores[p2].substring(0,3));
  lcd.setCursor(0, 1);
  lcd.print("Duelo de D20!");
  delay(3000);

  int valorD20 = 1;
  unsigned long tempoGiro = 0;
  int duelistaDaVez = p1;
  bool flagInterrompido = false;
  bool dadoTravado = false;

  lcd.clear();

  // --- LOOP DO D20 ROTATIVO E ALTERNADO ---
  while (true) {

    // D20 fica girando sozinho enquanto não estiver travado
    if (!dadoTravado && millis() - tempoGiro > 80) {
      valorD20 = random(1, 21);
      tempoGiro = millis();
    }

    lcd.setCursor(0, 0);
    lcd.print("Vez do: " + cores[duelistaDaVez] + "      ");

    lcd.setCursor(0, 1);
    lcd.print("Resultado: " + String(valorD20) + "   ");

    // Se qualquer botão colorido for apertado, inicia a fase de reflexo
    if (digitalRead(BTN[0]) == LOW ||
        digitalRead(BTN[1]) == LOW ||
        digitalRead(BTN[2]) == LOW ||
        digitalRead(BTN[3]) == LOW) {

      flagInterrompido = true;
      break;
    }

    // Botão batalha
    if (digitalRead(BTN_BATALHA) == LOW) {

      delay(50);

      while (digitalRead(BTN_BATALHA) == LOW) {
        delay(10);
      }

      // Primeiro clique trava o dado
      if (!dadoTravado) {

        dadoTravado = true;

        lcd.setCursor(0, 1);
        lcd.print("Resultado: " + String(valorD20) + "   ");

      } 
      // Segundo clique passa a vez
      else {

        dadoTravado = false;

        if (duelistaDaVez == p1) {
          duelistaDaVez = p2;
        } else {
          duelistaDaVez = p1;
        }

        valorD20 = random(1, 21);

        lcd.clear();
      }

      delay(250);
    }
  }

  // Delay de transição seguro
  delay(200);

  // --- FASE DE REFLEXO E VELOCIDADE ---
  int perdedor = -1;

  while (true) {
    if (digitalRead(BTN[p1]) == LOW) { perdedor = p2; break; } 
    if (digitalRead(BTN[p2]) == LOW) { perdedor = p1; break; } 
    delay(10);
  }

  // Desliga os indicadores dos botões
  digitalWrite(LED_BOTOES[p1], LOW);
  digitalWrite(LED_BOTOES[p2], LOW);

  // --- EXECUTA A PENALIDADE DE VIDA ---
  vidas[perdedor]--; 
  atualizarTodosOsLEDsDeVida(); 

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(cores[perdedor] + " perdeu!");
  lcd.setCursor(0, 1);
  lcd.print("Vida: -1");
  Serial.println(cores[perdedor] + " perdeu 1 vida. Restam: " + String(vidas[perdedor]));
  
  // Animação de erro/derrota do perdedor
  for(int i = 0; i < 4; i++) {
    digitalWrite(LED_BOTOES[perdedor], HIGH);
    for (int v = 0; v < 3; v++) {
      if (v < vidas[perdedor]) digitalWrite(LED_VIDAS[perdedor][v], HIGH);
    }
    delay(250);
    
    digitalWrite(LED_BOTOES[perdedor], LOW);
    for (int v = 0; v < 3; v++) { 
      digitalWrite(LED_VIDAS[perdedor][v], LOW); 
    }
    delay(250);
  }

  atualizarTodosOsLEDsDeVida();

  while (digitalRead(BTN[perdedor]) == LOW) { delay(10); }
  delay(500); 

  if (vidas[perdedor] <= 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(cores[perdedor]);
    lcd.setCursor(0, 1);
    lcd.print("ELIMINADO!"); 
    delay(3000);
    verificarFimDeJogo();
  }
}

void atualizarTodosOsLEDsDeVida() {
  for (int j = 0; j < 4; j++) {
    for (int v = 0; v < 3; v++) {
      if (v < vidas[j]) {
        digitalWrite(LED_VIDAS[j][v], HIGH); 
      } else {
        digitalWrite(LED_VIDAS[j][v], LOW);  
      }
    }
  }
}

void atualizarPainelELEDs() {
  atualizarTodosOsLEDsDeVida();

  for (int i = 0; i < 4; i++) {
    digitalWrite(LED_BOTOES[i], LOW);
  }
  
  if(vidas[jogadorDaVez] > 0) {
    digitalWrite(LED_BOTOES[jogadorDaVez], HIGH);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Vez: " + cores[jogadorDaVez]);
  lcd.setCursor(0, 1);
  lcd.print("Vidas:" + String(vidas[jogadorDaVez]));
}

void verificarFimDeJogo() {
  int jogadoresVivos = 0;
  int vencedor = -1;

  for (int i = 0; i < 4; i++) {
    if (vidas[i] > 0) {
      jogadoresVivos++;
      vencedor = i;
    }
  }

  if (jogadoresVivos == 1) {
    while(true) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("FIM DE JOGO!!");
      lcd.setCursor(0, 1);
      lcd.print("Vencedor: " + cores[vencedor]);
      
      digitalWrite(LED_BOTOES[vencedor], HIGH);
      delay(500);
      digitalWrite(LED_BOTOES[vencedor], LOW);
      delay(500);
    }
  }
}

void somBatalha() {
  moduloMP3.playFolder(1, 0);
}

void printRetorno(uint8_t type, int value) {
  switch (type) {
    case TimeOut: Serial.print('.'); break;
    case WrongStack: Serial.println(F("Pilha Invalida!")); break;
    case DFPlayerCardInserted: Serial.println(F("Cartao Inserido!")); break;
    case DFPlayerCardRemoved: Serial.println(F("Cartao Removido!")); break;
    case DFPlayerCardOnline: Serial.println(F("Cartao Online!")); break;
    case DFPlayerUSBInserted: Serial.println("USB Inserido!"); break;
    case DFPlayerUSBRemoved: Serial.println("USB Removido!"); break;
    case DFPlayerPlayFinished:
      Serial.print(F("Numero:"));
      Serial.print(value);
      Serial.println(F(" Reproducao Concluida!"));
      break;
    case DFPlayerError:
      Serial.print(F("Erro no modulo MP3:"));
      switch (value) {
        case Busy: Serial.println(F("Cartao nao Encontrado")); break;
        case Sleeping: Serial.println(F("Dormindo")); break;
        case SerialWrongStack: Serial.println(F("Pilha Incorreta Recebida")); break;
        case CheckSumNotMatch: Serial.println(F("Soma de Verificacao nao Confere")); break;
        case FileIndexOut: Serial.println(F("Indice de Arquivo Fora dos Limites")); break;
        case FileMismatch: Serial.println(F("Arquivo nao encontrado")); break;
        case Advertise: Serial.println(F("Em Publicidade")); break;
        default: break;
      }
      break;
    default: break;
  }
}
