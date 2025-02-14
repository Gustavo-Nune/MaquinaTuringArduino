#include <LiquidCrystal.h>
#include <Keypad.h>

// Configuração do LCD
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Configuração Fixa do Teclado Matricial
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  { '0', '1', ' ', 'a' },
  { '*', '#', ' ', 'b' },
  { '&', '$', ' ', 'c' },
  { ' ', ' ', ' ', 'd' }
};
byte rowPins[ROWS] = {14, 15, 16, 17};
byte colPins[COLS] = {6, 7, 8, 9};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Estrutura para regras de transição
struct TransitionRule {
  char currentState;
  char readSymbol;
  char nextState;
  char writeSymbol;
  char direction; // 'E' para esquerda, 'D' para direita, 'S' para parar
};

// Variáveis globais
TransitionRule rules[20]; // Array para armazenar até 20 regras
int ruleCount = 0;        // Contador de regras
char initialState = '\0'; // Estado inicial (inicializado com valor nulo)
char finalState = '\0';   // Estado final (inicializado com valor nulo)
String tape = "";         // Fita
int headPosition = 0;     // Posição da cabeça
char currentState = '\0'; // Estado atual (inicializado com valor nulo)

// Função para exibir a fita e o estado atual no LCD
void displayTape() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tape: ");

  for (int i = 0; i < tape.length(); i++) {
    if (i == headPosition) {
      lcd.print("[");
      lcd.print(tape[i]);
      lcd.print("]");
    } else {
      lcd.print(tape[i]);
    }
  }

  lcd.setCursor(0, 1);
  lcd.print("State: ");
  if (currentState != '\0') { // Verifica se o estado atual é válido
    lcd.print(currentState);
  } else {
    lcd.print("N/A"); // Exibe "N/A" se o estado for inválido
  }
}

// Função para processar a fita com base nas regras de transição
void processTape() {
  if (initialState == '\0' || finalState == '\0') {
    lcd.clear();
    lcd.print("Estados nao config!");
    return;
  }

  currentState = initialState;
  headPosition = 0;

  while (true) {
    displayTape();
    delay(500);

    char currentSymbol = tape[headPosition];
    bool ruleFound = false;

    for (int i = 0; i < ruleCount; i++) {
      if (rules[i].currentState == currentState && rules[i].readSymbol == currentSymbol) {
        currentState = rules[i].nextState;
        tape[headPosition] = rules[i].writeSymbol;

        // Movimentação da cabeça
        if (rules[i].direction == 'E') {
          headPosition = (headPosition > 0) ? headPosition - 1 : 0;
        } else if (rules[i].direction == 'D') {
          headPosition = (headPosition < tape.length() - 1) ? headPosition + 1 : tape.length() - 1;
        }

        // Verifica se atingiu o estado final (independente da direção)
        if (currentState == finalState) {
          displayTape();
          lcd.setCursor(0, 1);
          lcd.print("ACEITA!");
          return;
        }

        ruleFound = true;
        break;
      }
    }

    if (!ruleFound) {
      displayTape();
      lcd.setCursor(0, 1);
      lcd.print("REJEITADA!");
      return;
    }
  }
}

// Função para adicionar uma nova regra de transição
void addRule(char currentState, char readSymbol, char nextState, char writeSymbol, char direction) {
  if (ruleCount < 20) {
    rules[ruleCount] = {currentState, readSymbol, nextState, writeSymbol, direction};
    ruleCount++;
  }
}

// Função para processar todas as regras recebidas pelo Serial
void processRules(String content) {
  lcd.clear();
  lcd.print("Processando regras...");

  // Reinicializa as variáveis
  ruleCount = 0;
  initialState = '\0';
  finalState = '\0';

  // Divide o conteúdo em partes separadas por ';'
  int start = 0;
  int end = content.indexOf(';');
  while (end != -1 && ruleCount < 20) {
    String line = content.substring(start, end);
    line.trim(); // Remove espaços em branco e caracteres especiais

    // Ignora linhas vazias
    if (line.length() == 0) {
      start = end + 1;
      end = content.indexOf(';', start);
      continue;
    }

    // Processa o estado inicial
    if (line.startsWith("initialState:")) {
      initialState = line.charAt(line.indexOf(':') + 1); // Pega o caractere após ':'
      lcd.clear();
      lcd.print("Initial: ");
      lcd.print(initialState);
    }
    // Processa o estado final
    else if (line.startsWith("finalState:")) {
      finalState = line.charAt(line.indexOf(':') + 1); // Pega o caractere após ':'
      lcd.clear();
      lcd.print("Final: ");
      lcd.print(finalState);
    }
    // Processa as regras de transição
    else if (line.indexOf(',') != -1) {
      // Formato da regra: q0,0,q1,1,R
      char currentState = line.charAt(0);
      char readSymbol = line.charAt(2);
      char nextState = line.charAt(4);
      char writeSymbol = line.charAt(6);
      char direction = line.charAt(8);
      addRule(currentState, readSymbol, nextState, writeSymbol, direction);
    }

    start = end + 1;
    end = content.indexOf(';', start);
  }

  lcd.clear();
  if (initialState != '\0' && finalState != '\0') {
    lcd.print("Regras carregadas!");
  } else {
    lcd.print("Erro nas regras!");
  }
}

void setup() {
  // Inicializa o LCD
  lcd.begin(16, 2);
  lcd.print("Aguardando regras...");

  // Inicializa o Serial
  Serial.begin(115200);
  while (!Serial) {
    ; // Aguarda a conexão Serial
  }
}

void loop() {
  // Verifica se há dados disponíveis no Serial
  if (Serial.available()) {
    String content = Serial.readStringUntil('\n'); // Lê toda a entrada até a quebra de linha
    content.trim(); // Remove espaços em branco e caracteres especiais
    processRules(content); // Processa todas as regras
  }

  // Verifica as entradas do teclado matricial
  char key = keypad.getKey();
  if (key) {
    if (key == '#') { // Tecla '#' para processar a fita
      processTape();
    } else if (key == '*') { // Tecla '*' para limpar a fita
      tape = "";
      headPosition = 0;
      currentState = initialState;
      displayTape();
    } else {
      tape += key;
      headPosition = tape.length() - 1;
      displayTape();
    }
  }
}