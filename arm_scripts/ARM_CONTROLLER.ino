/* Minimum_Source*/
#include <String.h>

#define DXL_BUS_SERIAL1 1  //Dynamixel on Serial1(USART1)  <-OpenCM9.04
#define DXL_BUS_SERIAL2 2  //Dynamixel on Serial2(USART2)  <-LN101,BT210
#define DXL_BUS_SERIAL3 3  //Dynamixel on Serial3(USART3)  <-OpenCM 485EXP

#define ID_NUM 3
#define POS_L 36
#define POS_H 37
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
  Dxl.jointMode(ID_M1);
  Dxl.jointMode(ID_M2);
  Dxl.jointMode(ID_M3);
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
  
    if(mode == '2') // record mode
    {
      int record = recordMotorPositions(); 
      if(record)
      SerialUSB.println("recording successful"); 
    }
    
    else if(mode == '1') // playback mode
    {
      int pos[3]; 
      char* command = strtok(temp, ","); //split command by commas, this is start
      SerialUSB.println(command); 
      command = strtok(0, ","); //split again, which is mode
      SerialUSB.println(command);
      command = strtok(0, ","); // this is the first motor position (ID 1)
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
      if(playback)
      SerialUSB.println("playback successful"); 
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
  int pos1, pos2, pos3 = 0; 
  char finalWord[30]; 
  
  //NEED TO TURN THIS INTO COMPLIANCE MODE, RIGHT NOW ITS JUST NO TORQUE MODE (DEAD BOT)
  Dxl.writeWord(ID_M1, TORQUE_ENABLE, 0);
  Dxl.writeWord(ID_M2, TORQUE_ENABLE, 0);
  Dxl.writeWord(ID_M3, TORQUE_ENABLE, 0);

  pos1 = Dxl.readWord(ID_M1, POS_L);
  //itoa(pos, pos_1, 10); 
  
  pos2 = Dxl.readWord(ID_M2, POS_L);
  //itoa(pos, pos_2, 10);
  
  pos3 = Dxl.readWord(ID_M3, POS_L);
  //itoa(pos, pos_3, 10);
  
  if(DEBUG)
  {
    pos1 = 255;
    pos2 = 357;
    pos3 = 755; 
  }
  //build the string to send
  finalWord[0] = 's'; 
  finalWord[1] = ',';
  finalWord[2] = '\0';  
  itoa(pos1, finalWord+strlen(finalWord), 10);
  finalWord[strlen(finalWord)] = ','; 
  itoa(pos2, finalWord+strlen(finalWord), 10);
  finalWord[strlen(finalWord)] = ','; 
  itoa(pos3, finalWord+strlen(finalWord), 10); 
  finalWord[strlen(finalWord)] = ','; 
  itoa(199623, finalWord+strlen(finalWord), 10);
  SerialUSB.println(finalWord); 
  //NEED TO ADD A CHECKSUM
  
  return 1; 
}

int playBackPositions(int pos[3])
{
  if(DEBUG)
  {
    SerialUSB.println(pos[0]);
    SerialUSB.println(pos[1]);
    SerialUSB.println(pos[2]);
  }
  
  Dxl.writeWord(ID_M1, TORQUE_ENABLE, 1);
  Dxl.writeWord(ID_M2, TORQUE_ENABLE, 1);
  Dxl.writeWord(ID_M3, TORQUE_ENABLE, 1);
  
  if(pos[0] > ID_1_MAX) pos[0] = ID_1_MAX; 
  else if(pos[0] < ID_1_MIN) pos[0] = ID_1_MIN;
  
  if(pos[1] > ID_2_MAX) pos[1] = ID_2_MAX; 
  else if(pos[1] < ID_2_MIN) pos[1] = ID_2_MIN;
  
  if(pos[2] > ID_3_MAX) pos[2] = ID_3_MAX; 
  else if(pos[2] < ID_3_MIN) pos[2] = ID_3_MIN;
  
  Dxl.writeWord(ID_M1, GOAL_POSITION, pos[0]);
  Dxl.writeWord(ID_M2, GOAL_POSITION, pos[1]);
  Dxl.writeWord(ID_M3, GOAL_POSITION, pos[2]);
  
  if(DEBUG)
  {
    SerialUSB.println(pos[0]);
    SerialUSB.println(pos[1]);
    SerialUSB.println(pos[2]);
  }
  
  return 1;
}
