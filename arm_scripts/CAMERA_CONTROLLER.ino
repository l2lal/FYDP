/* Minimum_Source*/
#include <String.h>

#define DXL_BUS_SERIAL1 1  //Dynamixel on Serial1(USART1)  <-OpenCM9.04
#define DXL_BUS_SERIAL2 2  //Dynamixel on Serial2(USART2)  <-LN101,BT210
#define DXL_BUS_SERIAL3 3  //Dynamixel on Serial3(USART3)  <-OpenCM 485EXP

#define ID_NUM 3
#define POS_L 37
#define POS_H 38
#define TORQUE_ENABLE 24
#define GOAL_POSITION 30

#define ID_M1 1
#define ID_M2 2 
#define ID_M3 3 
#define ID_M4 4

#define ID_1_MAX 750
#define ID_1_MIN 240

#define ID_2_MAX 822
#define ID_2_MIN 512

#define ID_3_MAX 1010
#define ID_3_MIN 512

#define ID_4_MAX 1023
#define ID_4_MIN 0

#define HOME_M1 740
#define HOME_M2 525
#define HOME_M3 800
#define HOME_M4 800

#define DEBUG 0

const int MSG_SIZE = 50; 
const int startByte_ind = 0; 
const int modeByte_ind = 2; 
const int dataByte_ind = 4;
const int checksumByte_ind = 6; 

Dynamixel Dxl(DXL_BUS_SERIAL1);

void setup(){
  // Initialize the dynamixel bus:
  // Dynamixel 2.0 Baudrate -> 0: 9600, 1: 57600, 2: 115200, 3: 1Mbps
  Dxl.begin(3);
  Dxl.goalSpeed(ID_M4, 100);  //Dynamixel ID 1 Speed Control 100 setting
  
  Dxl.jointMode(ID_M4);

  SerialUSB.begin();
  pinMode(BOARD_LED_PIN, OUTPUT);
}

void loop() {
   static char temp[MSG_SIZE];
//  toggleLED();  //toggle digital value based on current value.
//  delay(100);   //Wait for 0.1 second
  
  while (!SerialUSB.available()){}
  
  int j = 0; 
  while (SerialUSB.available()) {
        temp[j++] = SerialUSB.read();
        // show the byte on serial monitor
  }
  
  temp[j] = '\0';
  
  char start_byte = temp[0];
  int good_start = 1; 
  if(start_byte != 's') 
  {
    SerialUSB.println("faulty start"); 
    good_start = 0;
  } 
  
  if(good_start)
  {
    char mode = temp[2];
    
    if(mode == '0')
    {
      goHome(); 
    }
    
  
    else if(mode == '2') // record mode
    {
      int record = recordMotorPositions(); 
      //if(record)
      //SerialUSB.println("recording successful"); 
    }
    
    else if(mode == '1') // playback mode
    {
      int validate = 0;
      int num_commas = 0; 
      for(int k = 0; k < strlen(temp); k++)
      {
        validate += temp[k];
      }
      
      int pos[3]; 
      char* command = strtok(temp, ","); //split command by commas, this is start
      SerialUSB.println(command); 
      command = strtok(0, ","); //split again, which is mode
      SerialUSB.println(command);
      command = strtok(0, ","); // this is the first motor position (ID 4)
      SerialUSB.println(command);
      int i = 0;
      
      while (command != 0)
      {        
          pos[i] = atoi(command);
          i++; 
          if(DEBUG)
          {
            SerialUSB.println(pos[i]);
          }
          // Do something with servoId and position
  
          // Find the next command in input string
          command = strtok(0, ",");
      }
      int playback = playBackPositions(pos); 
      //if(playback)
      //SerialUSB.println("playback successful"); 
    }
    
    else
    {
      SerialUSB.println("faulty mode"); 
    }
  }
  
  //SerialUSB.print(temp);

  /*Structure of buffer:
  start character (1) | mode (1) | checksum (1) | 
  */ 
}

int recordMotorPositions()
{
  int pos1 = 0; 
  char finalWord[35] = {};  
  
  //NEED TO TURN THIS INTO COMPLIANCE MODE, RIGHT NOW ITS JUST NO TORQUE MODE (DEAD BOT)
  Dxl.writeWord(ID_M4, TORQUE_ENABLE, 0);


  pos1 = Dxl.readWord(ID_M4, POS_L);
  
  if(DEBUG)
  {
    pos1 = 255; 
  }
  //build the string to send
  finalWord[0] = 's'; 
  finalWord[1] = ',';
  finalWord[2] = '\0';  
  itoa(pos1, finalWord+strlen(finalWord), 10);
  finalWord[strlen(finalWord)] = ','; 
 
  int csum = 0; 
  for(int i = 0; i < strlen(finalWord); i++)
  {
    csum += finalWord[i];
  }
  
  csum = csum % 256;
  
  itoa(csum, finalWord+strlen(finalWord), 10);
  
  SerialUSB.println(finalWord);
  
  //NEED TO ADD A CHECKSUM
  
  return 1; 
}

int playBackPositions(int pos[1])
{
  if(DEBUG)
  {
    SerialUSB.println(pos[0]);
  }
  
  Dxl.writeWord(ID_M4, TORQUE_ENABLE, 1);

  
  if(pos[0] > ID_4_MAX) pos[0] = ID_4_MAX; 
  else if(pos[0] < ID_4_MIN) pos[0] = ID_4_MIN;
  
  Dxl.writeWord(ID_M4, GOAL_POSITION, pos[0]);

  
  if(DEBUG)
  {
    SerialUSB.println(pos[0]);
  }
  
  return 1;
}

void goHome()
{
  Dxl.writeWord(ID_M4, TORQUE_ENABLE, 1);

  
  Dxl.writeWord(ID_M4, GOAL_POSITION, HOME_M4);

  
}

