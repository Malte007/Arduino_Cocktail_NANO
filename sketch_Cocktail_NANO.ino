#include <EEPROM.h>

enum arduinoStates {
  ARDUINO_INIT_STATE,
  ARDUINO_READY_STATE, // pump is ready to do action
  ARDUINO_COOK_STATE,
  ARDUINO_SERVICE_STATE,
  ARDUINO_ERROR_STATE// pump is in error state
};

enum arduinoStates arduinoState;

enum pumpStates {
  INIT_STATE,
  READY_STATE, // pump is ready to do action
  RUN_STATE, // pump is in action
  FORCE_ON_STATE,
  STOP_STATE,
  ERROR_STATE// pump is in error state
};

enum enumCMD : byte {
  FORCE_CMD = 0x2C,
  CANCEL_COOK_CMD = 0x37,
  COOK_CMD = 0x42,
  GET_RECEPTS_CMD = 0x65,
  GET_ZUTATEN_CMD = 0x66,
  RES_ARDUINO_CMD = 0xFF,
  ERR_CMD = 0xFE,
  RES_ERR_CMD = 0x71,
  NR_OF_PU_CMD = 0x72
};

enum enumMessage : byte
{
  KEINE_MSG = 0x00,
  ON_MSG = 0x01,
  OFF_MSG = 0x02,
  OK_MSG = 0x10,
  NOK_MSG = 0x20,
  TIMEOUT_MSG = 0x30,
  UNKNOWN_MSG = 0x40,
  FAILD_MSG = 0x44
};

typedef struct {
  byte pinPump;
  enum pumpStates statePump;
  long startTimePump;
  byte fillInMlPump;
  double pumpMlPerMin;
  bool resetErrorStatePump;
}Pumpe;
Pumpe pumpen[4];

bool resetErrorStateArduino;

const int MAX_PUMP_AT_SAME =4;
const int constPACKAGESIZE =20;
const int constHEADER =0;
const int constPAYLOAD_START = 1;
const byte pinReadyLED = 2;
const byte pinCookLED = 3;
const byte pinErrorLED = 4;

const int HIGH_INVERT = LOW;  //Invertierter Wert für das Schalten von Relais Shield
const int LOW_INVERT = HIGH;   //Invertierter Wert für das Schalten von Relais Shield
const int comTimeOutInMS = 5000;  //TimeOut Zeit für die Kommunikation per Serial
long comTimeOutCounterInMS;       //TimeOut-Zähler der die Zeit bis zum TimeOut zählt

byte cookRunCount = 0; // Zählt die Anzahl der über die Fill-Menge laufenden Pumpen. Wird benutzt für eine begrenzung der gleichzeitigen Pumpen.
byte cookCount = 0; // Zählt die Anzahl an Pumpen die für das Rezept NOCH laufen müssen. Wird genutzt um im Arduino-StateM die LED zu beeinflussen.

void setup()
{  
  pumpen[0].pinPump = 17;//Analog out
  pumpen[0].statePump = INIT_STATE;
  pumpen[0].startTimePump = 0;
  pumpen[0].fillInMlPump = 0;
  pumpen[0].resetErrorStatePump = false;
  pumpen[0].pumpMlPerMin = 500;
  
  pumpen[1].pinPump = 16;//Analog out
  pumpen[1].statePump = INIT_STATE;
  pumpen[1].startTimePump = 0;
  pumpen[1].fillInMlPump = 0;
  pumpen[1].resetErrorStatePump = false;
  pumpen[1].pumpMlPerMin = 400;
  
  pumpen[2].pinPump = 15;//Analog out
  pumpen[2].statePump = INIT_STATE;
  pumpen[2].startTimePump = 0;
  pumpen[2].fillInMlPump = 0;
  pumpen[2].resetErrorStatePump = false;
  pumpen[2].pumpMlPerMin = 440;
  
  pumpen[3].pinPump = 14;//Analog out
  pumpen[3].statePump = INIT_STATE;
  pumpen[3].startTimePump = 0;
  pumpen[3].fillInMlPump = 0;
  pumpen[3].resetErrorStatePump = false;
  pumpen[3].pumpMlPerMin = 440;

  
  pinMode(pinReadyLED, OUTPUT);
  pinMode(pinCookLED, OUTPUT);
  pinMode(pinErrorLED, OUTPUT);
  
  cookRunCount= 0;
  cookCount = 0;
  resetErrorStateArduino = false;
  arduinoState = ARDUINO_READY_STATE;

  Serial.begin(9600);
}

void loop()
{

  arduinoStateMachine();
  
} 

void pumpenTask()
{
  for (int i = 0 ; i < (sizeof pumpen / sizeof pumpen[0]) ; i++) {    
    StateMachine(&pumpen[i]);    
  }
}

void comTask()
{
  if (Serial && Serial.available() > 0)
    {
      readData();
    }
}

void sendOK(byte cmd)
{
  byte buff[constPACKAGESIZE];
  buff[constHEADER]=cmd;
  buff[constPAYLOAD_START]= OK_MSG;
  for (int i = 0 ; i < (sizeof buff / sizeof (byte)) ; i++) {    
    Serial.write(buff[i])  ;  
  }
}

void sendNOK(byte cmd)
{
  byte buff[constPACKAGESIZE];
  buff[constHEADER]= cmd;
  buff[constPAYLOAD_START]= NOK_MSG;
  for (int i = 0 ; i < (sizeof buff / sizeof (byte)) ; i++) {    
    Serial.write(buff[i])  ;  
  }
}

void readData()
{  

  if(Serial.available() <constPACKAGESIZE && Serial.available() >0)
  {
     if(comTimeOutCounterInMS == 0)
     {
      comTimeOutCounterInMS = millis();
     }
     else
     {
      if(millis() - comTimeOutCounterInMS > comTimeOutInMS)
      {
        Serial.print("TimeOut");
        while(Serial.available() > 0) 
        {
         char t = Serial.read();
        }
        comTimeOutCounterInMS = 0;
      }
     }
     
  }
  else
  {
    comTimeOutCounterInMS = 0;
    byte incomingByte[constPACKAGESIZE];
    Serial.readBytes(incomingByte,constPACKAGESIZE);
    
    switch(incomingByte[constHEADER])
    {
      case RES_ARDUINO_CMD://Reset error state
      arduinoState = ARDUINO_INIT_STATE;
      break;
    case RES_ERR_CMD://Reset error state
    if(arduinoState == ARDUINO_ERROR_STATE)
    {
      resetErrorStateArduino = true;
    }
     break;
     case ERR_CMD://Go to error state
      arduinoState = ARDUINO_ERROR_STATE;
     break;
     case FORCE_CMD://Force SERVICE [2C] [Pumpe 1-4] [01ON/02OFF] ..[0]
          if((sizeof pumpen / sizeof pumpen[0])>=  incomingByte[constPAYLOAD_START] && incomingByte[constPAYLOAD_START]>0)
          { 
            switch(incomingByte[constPAYLOAD_START +1])
            {
              case ON_MSG:
              if(arduinoState == ARDUINO_READY_STATE)
              {
                pumpen[incomingByte[constPAYLOAD_START]-1].statePump = FORCE_ON_STATE;
                arduinoState = ARDUINO_SERVICE_STATE;
                //sendOK(FORCE_CMD);
              }
              else
              {
                //sendNOK(FORCE_CMD);
              }
              break;
              case OFF_MSG:
              if(arduinoState == ARDUINO_SERVICE_STATE && pumpen[incomingByte[1]-1].statePump == FORCE_ON_STATE)
              {
                pumpen[incomingByte[1]-1].statePump = STOP_STATE;
                arduinoState = ARDUINO_READY_STATE;
                //sendOK(FORCE_CMD);
              }
              else
              {
                //sendNOK(FORCE_CMD);
              }
              break;
              default:break;//sendNOK(FORCE_CMD);break;
            }

          }
          else
          {
                //sendNOK(incomingByte[FORCE_CMD]);                
          }
      
     break;
     case CANCEL_COOK_CMD://Cancel cook [22] ... [0]
      if(arduinoState == ARDUINO_COOK_STATE)
      {
          for (int i = 0 ; i < (sizeof pumpen / sizeof pumpen[0]) ; i++) {    
          pumpen[i].statePump = STOP_STATE;    
          }
                sendOK(incomingByte[constHEADER]);    
      }
      else
      {
          sendNOK(incomingByte[constHEADER]);
      }
     break;
     case COOK_CMD://Cook rezept[66] [RezeptNr] ... [0]
     if(arduinoState == ARDUINO_READY_STATE)
     {
        if(cookTelegramm(incomingByte))
        {    
           sendOK(incomingByte[constHEADER]);
        }
        else
        {
          sendNOK(incomingByte[constHEADER]);
        }
     }
     else
     {
          sendNOK(incomingByte[constHEADER]);      
     }
     break;
     
      case NR_OF_PU_CMD:
        sendNrOFPumps();
      break;
     default:break;
    }
  } 
  
  
}

void sendNrOFPumps()
{  
  byte buff[20] = {NR_OF_PU_CMD,(sizeof pumpen / sizeof pumpen[0]),0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  for (int it = 0 ; it < sizeof buff ; it++)
  {    
    Serial.write(buff[it]);  
  }
}

void arduinoStateMachine()
{
  switch(arduinoState)
  {
    case ARDUINO_INIT_STATE:
    digitalWrite(pinCookLED, LOW);
    digitalWrite(pinErrorLED, LOW);
    digitalWrite(pinReadyLED, LOW);
    setup();
    break;
    case ARDUINO_READY_STATE:
    digitalWrite(pinCookLED, LOW);
    digitalWrite(pinErrorLED, LOW);
    digitalWrite(pinReadyLED, HIGH);
    if(cookCount !=0)
    {
      arduinoState = ARDUINO_ERROR_STATE;
      break;
    }

    pumpenTask();

    comTask();
    
    
    break;
    case ARDUINO_COOK_STATE:
    digitalWrite(pinReadyLED, LOW);
    digitalWrite(pinErrorLED, LOW);
    digitalWrite(pinCookLED, HIGH);

    comTask();

    pumpenTask();

    if(cookCount ==0)
    {
      arduinoState = ARDUINO_READY_STATE;
    }
        
    break;
    
    case ARDUINO_SERVICE_STATE:    
    digitalWrite(pinReadyLED, HIGH);
    digitalWrite(pinErrorLED, LOW);
    digitalWrite(pinCookLED, HIGH);

    comTask();

    pumpenTask();

    break;
    case ARDUINO_ERROR_STATE:
    digitalWrite(pinReadyLED, LOW);
    digitalWrite(pinErrorLED, HIGH);
    digitalWrite(pinCookLED, LOW);
    for (int i = 0 ; i < (sizeof pumpen / sizeof pumpen[0]) ; i++)
    {
      if(pumpen[i].statePump != ERROR_STATE && pumpen[i].statePump != READY_STATE && pumpen[i].statePump != INIT_STATE)
      {
        pumpen[i].statePump = STOP_STATE;
        StateMachine(&pumpen[i]);
      }
    }
    comTask();
    
    if(resetErrorStateArduino == true)
    {
      arduinoState = ARDUINO_INIT_STATE;
    }
    
    break;
  }
  
}

void StateMachine(Pumpe *pumpe)
{
  switch(pumpe->statePump)
  {
    case INIT_STATE:
    pinMode(pumpe->pinPump, OUTPUT);
    digitalWrite(pumpe->pinPump, LOW_INVERT);
    pumpe->startTimePump = 0;
    pumpe->fillInMlPump = 0;
    pumpe->statePump = READY_STATE;
    break;
    case READY_STATE:
    
    if((pumpe->fillInMlPump > 0) && (cookRunCount < MAX_PUMP_AT_SAME))//Prüfe ob Füllmenge vorhanden ist und ob schon die max. Anz an Pumpen läuft
    {
      if(digitalRead(pumpe->pinPump) == LOW_INVERT)
      delay(100);
        digitalWrite(pumpe->pinPump, HIGH_INVERT);
        pumpe->startTimePump = millis();
      pumpe->statePump = RUN_STATE;
      cookRunCount++;
    }
    break;
    case RUN_STATE:  
    
    //if(arduinoState != ARDUINO_COOK_STATE)
    //{
     //arduinoState = ARDUINO_ERROR_STATE ;
     //break;
    //}      
    if((pumpe->fillInMlPump / pumpe->pumpMlPerMin) < ((double)(millis() - pumpe->startTimePump)/60000 ))//Prüfe ob Füllmenge erreicht.
    {          
      if(cookCount >0)
      {
        cookCount--;
      } 
        digitalWrite(pumpe->pinPump, LOW_INVERT);        
        pumpe->statePump = INIT_STATE;

      if(cookRunCount >0)
      {
        cookRunCount--;
      }   
    }
    break;
    case FORCE_ON_STATE:
      if(digitalRead(pumpe->pinPump) == LOW_INVERT)
        digitalWrite(pumpe->pinPump, HIGH_INVERT);
    break;
    case STOP_STATE:
    if(digitalRead(pumpe->pinPump) == HIGH_INVERT)
    {
        if(cookRunCount >0)
        {
          cookRunCount--;
        }   
      
      if(cookCount > 0 && arduinoState == ARDUINO_COOK_STATE)
      {
        cookCount--;                 
      }
      digitalWrite(pumpe->pinPump, LOW_INVERT);
    } 
        pumpe->statePump = INIT_STATE;
    break;
    case ERROR_STATE:
    if(digitalRead(pumpe->pinPump) == HIGH_INVERT)
    {
      digitalWrite(pumpe->pinPump, LOW_INVERT);
    }
    if(pumpe->resetErrorStatePump)
    {
      pumpe->statePump = INIT_STATE;
    }
    break;
  }
}

bool cookTelegramm(byte telegramm[])
{  
  bool success = false;
  int numberPumpen = (sizeof pumpen / sizeof pumpen[0]); 
  
  for(int index = 1; numberPumpen >0; index +=2)
  {
    if(telegramm[index] != KEINE_MSG)
    {
      cookCount ++;
      pumpen[telegramm[index] -1].fillInMlPump = telegramm[index+1];
      arduinoState = ARDUINO_COOK_STATE;
      success = true;
    } 
    numberPumpen --;
  }
  return success;
}
