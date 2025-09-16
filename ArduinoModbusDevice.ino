#include <ArduinoRS485.h> // ArduinoModbus зависит от библиотеки ArduinoRS485
#include <ArduinoModbus.h>
#include <math.h>

const int SlaveID = 7;  // Работает только с адресами до 9 включительно
const int BaudRate = 9600;
const int Config = SERIAL_8N1;

struct ModbusRegistersConfig {
  int startAddress;
  int count;
};

ModbusRegistersConfig coilsConfig = { startAddress: 0, count: 10 };                // RW
ModbusRegistersConfig discreteInputsConfig = { startAddress: 0, count: 10 };       // Read only
ModbusRegistersConfig holdingRegistersConfig = { startAddress: 0, count: 10 };     // RW
// При указании количества входных регистров нужно учитывать, 
// что начиная с адреса 0x0A копируются значения регистров хранения.
ModbusRegistersConfig inputRegistersConfig = { startAddress: 0, count: 20 };       // Read only

const int numberOfSinPoints = 20;
int displayedSinPointIndex = 0;

float sinPoints[numberOfSinPoints];
const float sinAmplitude = 2.5;
const float sinPhaseShift = 0;

const int ledPin = 13;
uint8_t ledState = LOW;

int packetReceived;

uint16_t counter = 0;


void setup() {
  pinMode(ledPin, OUTPUT);
  
  ConfigModbusServer();

  digitalWrite(ledPin, ledState);

  GenerateSin();

  ChangeReadOnlyValues();
}

void loop() {  
   packetReceived = ModbusRTUServer.poll();
   
   if(packetReceived > 0) {
    PacketReceivedNotification();
    ChangeReadOnlyValues();
   }
}

// Основная функция для смены состояния дискретных входов и регистров ввода.
void ChangeReadOnlyValues() {
  randomSeed(analogRead(A0)); // Устанавливаем начальное значение генератора для более рандомного рандома

  // Discrete Inputs
  uint8_t discreteInputs[discreteInputsConfig.count];
  
  for (int i = 0; i < discreteInputsConfig.count; i++) {
    discreteInputs[i] = (uint8_t)random(0, 2);
  }

  ModbusRTUServer.writeDiscreteInputs(
    discreteInputsConfig.startAddress, 
    discreteInputs, 
    discreteInputsConfig.count
    );

  // Input Registers
  uint16_t inputRegisters[inputRegistersConfig.count];

  counter = counter + 1;

  inputRegisters[0] = counter;
  inputRegisters[1] = 65535 - counter;
  GenerateFloatToArray(inputRegisters, 2, 10);
  GenerateFloatToArray(inputRegisters, 4, 100);

  GenerateFloatToArray(inputRegisters, 6, sinPoints[displayedSinPointIndex]);

  displayedSinPointIndex = displayedSinPointIndex + 1;

  if (displayedSinPointIndex == numberOfSinPoints) {
    displayedSinPointIndex = 0;
  }
  
  inputRegisters[8] = (uint16_t)random(0, 65536);
  inputRegisters[9] = (uint16_t)random(0, 65536);

  for (int i = 0; i < holdingRegistersConfig.count; i++) {
    inputRegisters[10 + i] = ModbusRTUServer.holdingRegisterRead(i);
  }

  ModbusRTUServer.writeInputRegisters(
    inputRegistersConfig.startAddress, 
    inputRegisters, 
    inputRegistersConfig.count
    );
}

void GenerateFloatToArray(uint16_t* array, int startIndex, int maxIntegerPart) {
  union {
    float number;
    uint16_t words[2];
  } converter;

  int integerPart = random(0, maxIntegerPart);
  int fractionalPart = random(0, 100);
  float value = integerPart + fractionalPart / 100.0;

  converter.number = value;
  array[startIndex] = converter.words[0];
  array[startIndex + 1] = converter.words[1];
}

void GenerateFloatToArray(uint16_t* array, int startIndex, float value) {
  union {
    float number;
    uint16_t words[2];
  } converter;

  converter.number = value;
  array[startIndex] = converter.words[0];
  array[startIndex + 1] = converter.words[1];
}

void PacketReceivedNotification() {
  ledState = ledState == LOW ? HIGH : LOW;
  digitalWrite(ledPin, ledState);
}

void GenerateSin() {
  for (int i = 0; i < numberOfSinPoints; i++) {
    float angle = 2.0 * PI * i / numberOfSinPoints + sinPhaseShift; // угол от 0 до 2π
    sinPoints[i] = sinAmplitude * sin(angle);
  }
}

/*
*
* Конфигурация Modbus устройства.
*
*/

void ConfigModbusServer() {
  Serial.begin(BaudRate);

  if(!Serial) {
    digitalWrite(ledPin, HIGH); // Ошибка
  }

  if (!ModbusRTUServer.begin(SlaveID, BaudRate, Config)) {
    digitalWrite(ledPin, HIGH); // Ошибка
    while (1);
  }

  ModbusRTUServer.configureCoils(
    coilsConfig.startAddress, 
    coilsConfig.count
    ); 

  ModbusRTUServer.configureDiscreteInputs(
    discreteInputsConfig.startAddress, 
    discreteInputsConfig.count
    );    

  ModbusRTUServer.configureHoldingRegisters(
    holdingRegistersConfig.startAddress, 
    holdingRegistersConfig.count
    ); 

  ModbusRTUServer.configureInputRegisters(
    inputRegistersConfig.startAddress, 
    inputRegistersConfig.count
    ); 
}
