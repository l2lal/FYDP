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
#define MOVING 46
#define MOVING_SPEED_REG 32
#define COMP_MARGIN_CW 26
#define COMP_MARGIN_CWW 27
#define TORQ_LIMIT 34
#define MAX_TORQ 14

#define CW_COMP_SLOPE 28
#define CCW_COMP_SLOPE 29

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

#define HOME_M1 740
#define HOME_M2 525
#define HOME_M3 800

#define MOVING_SPEED 50

#define EXPECTED_COMMAS 5

#define DEBUG 0
#define MSG_SIZE 50 

#define DROOP_OFFSET 12
#define REVOLVE_OFFSET 0

const int startByte_ind = 0; 
const int modeByte_ind = 2; 
const int dataByte_ind = 4;
const int checksumByte_ind = 6; 

Dynamixel Dxl(DXL_BUS_SERIAL1);

void setup(){
  // Initialize the dynamixel bus:
  // Dynamixel 2.0 Baudrate -> 0: 9600, 1: 57600, 2: 115200, 3: 1Mbps
  Dxl.begin(3);
  //Dxl.goalSpeed(ID_M1, 100);  //Dynamixel ID 1 Speed Control 100 setting
  //Dxl.goalSpeed(ID_M2, 100);  //Dynamixel ID 1 Speed Control 100 setting
  //Dxl.goalSpeed(ID_M3, 100);  //Dynamixel ID 1 Speed Control 100 setting

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
  
  //temp[j] = '\0';
  
  char start_byte = temp[0];
  int good_start = 1; 
  if(start_byte != 's') 
  {
    //SerialUSB.println("faulty start"); 
    good_start = 0;
  } 
  
  if(good_start)
  {
    char mode = temp[2];
    
    if(mode == '0')
    {
      goHome(); 
    }
    
    else if(mode == '6')
    {
      /*int torq_lim = 0;
      int max_torq = 0; 
      Dxl.writeByte(ID_M2, TORQ_LIMIT, 1023);
      Dxl.writeByte(ID_M2, MAX_TORQ, 1023);
      char Torq[10] = {}; 
      Torq[0] = 's'; 
      Torq[1] = ',';
      torq_lim = Dxl.readWord(ID_M2, TORQ_LIMIT);
      max_torq = Dxl.readWord(ID_M2, MAX_TORQ);
      //Torq[2] = '\0';
      //itoa(torq_lim, Torq+strlen(Torq), 10);
      //Torq[strlen(Torq)] = ',';
      //itoa(max_torq, Torq+strlen(Torq), 10);
      SerialUSB.println(torq_lim);
      SerialUSB.println(max_torq);*/
       
    }
    
    else if(mode == '3') // STATE: Check if finished moving
    {
      char PoseResponse[5] = {};
      PoseResponse[0] = 's'; 
      PoseResponse[1] = ',';
       
      int mov1 = Dxl.readByte(ID_M1, MOVING);
      int mov2 = Dxl.readByte(ID_M2, MOVING);
      int mov3 = Dxl.readByte(ID_M3, MOVING);
  
      
      /*SerialUSB.println(mov1);
      SerialUSB.println(mov2);
      SerialUSB.println(mov3);*/
      
      if(mov1 || mov2 || mov3)
      {
        PoseResponse[2] = '0';
      }
      
      else
      {
        PoseResponse[2] = '1';
      }
      
      PoseResponse[3] = ',';
      PoseResponse[5] = '\0';
      int state3_chksum = generate_checksum(PoseResponse);
      itoa(state3_chksum, PoseResponse+strlen(PoseResponse), 10);
      SerialUSB.println(PoseResponse); 
    }
  
    else if(mode == '2') // record mode
    {
      int record = recordMotorPositions(); 
      //if(record)
      //SerialUSB.println("recording successful"); 
    }
    
    else if(mode == '1') // playback mode
    {
      
      if(validate_checksum(temp, EXPECTED_COMMAS))
      {
        int pos[3]; 
        char* command = strtok(temp, ","); //split command by commas, this is start
        //SerialUSB.println(command); 
        command = strtok(0, ","); //split again, which is mode
        //SerialUSB.println(command);
        command = strtok(0, ","); // this is the first motor position (ID 1)
        //SerialUSB.println(command);
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
      }
      //if(playback)
      //SerialUSB.println("playback successful"); 
    }
    
    else
    {
      //SerialUSB.println("faulty mode"); 
    }
  } 
}

int recordMotorPositions()
{
  int pos1, pos2, pos3 = 0; 
  char finalWord[35] = {};  
  
  //NEED TO TURN THIS INTO COMPLIANCE MODE, RIGHT NOW ITS JUST NO TORQUE MODE (DEAD BOT)
  Dxl.writeWord(ID_M1, TORQUE_ENABLE, 0);
  Dxl.writeWord(ID_M2, TORQUE_ENABLE, 0);
  Dxl.writeWord(ID_M3, TORQUE_ENABLE, 0);

  do
  {
    pos1 = Dxl.readWord(ID_M1, POS_L);
  } while(pos1 >= 1024);
  //itoa(pos, pos_1, 10); 
  
  do
  {
    pos2 = Dxl.readWord(ID_M2, POS_L);
  } while(pos2 >= 1024);  //itoa(pos, pos_2, 10);
  
  do
  {
    pos3 = Dxl.readWord(ID_M3, POS_L);
  } while(pos3 >= 1024);  //itoa(pos, pos_3, 10);

  
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

int playBackPositions(int pos[3])
{
  if(DEBUG)
  {
    SerialUSB.println(pos[0]);
    SerialUSB.println(pos[1]);
    SerialUSB.println(pos[2]);
  }
  
  //setCompliance(); // make strict error margins in position
  Dxl.writeWord(ID_M1, TORQUE_ENABLE, 1);
  Dxl.writeWord(ID_M2, TORQUE_ENABLE, 1);
  Dxl.writeWord(ID_M3, TORQUE_ENABLE, 1);
  
  if(pos[0] > ID_1_MAX) pos[0] = ID_1_MAX; 
  else if(pos[0] < ID_1_MIN) pos[0] = ID_1_MIN;
  
  if(pos[1] > ID_2_MAX) pos[1] = ID_2_MAX; 
  else if(pos[1] < ID_2_MIN) pos[1] = ID_2_MIN;
  
  if(pos[2] > ID_3_MAX) pos[2] = ID_3_MAX; 
  else if(pos[2] < ID_3_MIN) pos[2] = ID_3_MIN;
  
  Dxl.writeWord(ID_M1, MOVING_SPEED_REG, MOVING_SPEED);
  Dxl.writeWord(ID_M2, MOVING_SPEED_REG, MOVING_SPEED);
  Dxl.writeWord(ID_M3, MOVING_SPEED_REG, MOVING_SPEED);

  Dxl.writeWord(ID_M1, GOAL_POSITION, pos[0] - REVOLVE_OFFSET);
  Dxl.writeWord(ID_M2, GOAL_POSITION, pos[1] - DROOP_OFFSET);
  Dxl.writeWord(ID_M3, GOAL_POSITION, pos[2]);
  
  if(DEBUG)
  {
    SerialUSB.println(pos[0]);
    SerialUSB.println(pos[1]);
    SerialUSB.println(pos[2]);
  }
  
  return 1;
}

void goHome()
{
  //setCompliance(); 
  
  Dxl.writeWord(ID_M1, TORQUE_ENABLE, 1);
  Dxl.writeWord(ID_M2, TORQUE_ENABLE, 1);
  Dxl.writeWord(ID_M3, TORQUE_ENABLE, 1);
  
  Dxl.writeWord(ID_M1, MOVING_SPEED_REG, MOVING_SPEED);
  Dxl.writeWord(ID_M2, MOVING_SPEED_REG, MOVING_SPEED);
  Dxl.writeWord(ID_M3, MOVING_SPEED_REG, MOVING_SPEED);
  
  Dxl.writeWord(ID_M1, GOAL_POSITION, HOME_M1 - REVOLVE_OFFSET);
  Dxl.writeWord(ID_M2, GOAL_POSITION, HOME_M2 - DROOP_OFFSET);
  Dxl.writeWord(ID_M3, GOAL_POSITION, HOME_M3);
  
}

int validate_checksum(char temp[MSG_SIZE], int commas_before)
{
  // validate checksum
      static char chksum_char[10]; 
      int validate = 0;
      int num_commas = 0; // waiting for s,1,CMD_1,CMD_2,CMD_3,CHECKSUM -> want 5 commas before checksum
      int old_K = 0; 
      
      for(int k = 0; k < strlen(temp); k++)
      {
        if(num_commas < commas_before)
        {
          validate += temp[k];

          if(temp[k] == ',')
          {
            num_commas++; 
          }

        }
        
        else if(num_commas == EXPECTED_COMMAS) 
        {
          old_K = k; 
          break;
        }
      }

      int start_ind = 0;

      for(int new_ind = old_K; new_ind < strlen(temp); new_ind++)
      {
        chksum_char[start_ind++] = temp[new_ind];
      } 
      
      int chksum_int = atoi(chksum_char);
      chksum_int = chksum_int % 256;
      validate = validate % 256; 
      
      if(chksum_int != validate)
      {
        return 0;
      }
      
      else
      {
        return 1; 
      }
      
}

int generate_checksum(char* temp)
{
  int chksum_int = 0; 
  for(int k = 0; k < strlen(temp); k++)
  {
        {
          chksum_int += temp[k];
        }   
  }
  
  chksum_int = chksum_int % 256;
  
  return chksum_int; 
  
}

void setCompliance()
{
   Dxl.writeByte(ID_M1, CW_COMP_SLOPE, 32);
   Dxl.writeByte(ID_M1, CCW_COMP_SLOPE, 32);
   
   Dxl.writeByte(ID_M2, CW_COMP_SLOPE, 32);
   Dxl.writeByte(ID_M2, CCW_COMP_SLOPE, 32);
   
   Dxl.writeByte(ID_M3, CW_COMP_SLOPE, 32);
   Dxl.writeByte(ID_M3, CCW_COMP_SLOPE, 32);
}

