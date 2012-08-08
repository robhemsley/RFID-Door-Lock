/*
Access Controller - MIT RFID Door Lock

Sketch provides remote admin of users via serial interface. Type h in Serial Monitor
to view all available cmds. New users can also be added/removed by completing the following steps:

1) Take existing users card and hold against reader for more than 7 seconds
2) Bring the new user card to the reader so that both cards are present
3) If an account already existed they shall be removed from the acess list otherwise a new entry is created.

Created: July 3, 2012 by Rob Hemsley (http://www.robhemsley.co.uk)

Build up on Aaron Weiss "RFID Eval 13.56MHz Shield example sketch v10" 
*/

//Library Imports
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <Servo.h> 

//START USER EDIT
const int STATIC_ID_LOAD[] = {};          //List of Card IDs to automatically be loaded to memory (Supports previous version)
const int SERVO_PIN = 14;                 //Pin number Servo Logic is attached to
const int SOFT_RX = 7;                    //RFID SoftwareSerial RX pin
const int SOFT_TX = 8;                    //RFID SoftwareSerial TX pin
const int ADMIN_TIMEOUT = 7000;           //Number of milli seconds an admin card must be present before entering admin mode
//END USER EDIT

int app_state = 0;
int lock_state = 0;
int ids[15] = {};
unsigned long current_ids_time[4] = {};
int current_ids[4] = {-1, -1, -1, -1};
SoftwareSerial rfid(SOFT_RX, SOFT_TX);
Servo lock_servo; 

//STRINGS
const String ABOUT = "\n\nMIT RFID Door\n-------------\no - Opens the lock\nc - Closes the lock\ns - Prints lock state\nu - Prints user IDS\na <id> - Adds specified ID\nr <id> - Removes specified ID\nf - Factory reset (Clears all EEPROM)\nv - View current IDS";
const String NO_USERS = "No Current Users - Entering Admin State";
const String FACTORY_RESET = "\nFactory Reset\n----------\nALL IDS DELETED!";
const String STORED_IDS = "\nStored IDs\n----------";
const String UNLOCKING_DOOR = "\nUnlocking Door\n----------\nDone";
const String LOCKING_DOOR = "\nLocking Door\n----------\nDone";
const String LOCK_STATE_UNLOCK = "\nLock State\n----------\nUnlocked";
const String LOCK_STATE_LOCKED = "\nLock State\n----------\nLocked";
const String ADDING_USR = "\nAdding User\n----------";
const String USR_EXISTS = "User Already Added";
const String REMOVING_USR = "\nRemoving User\n----------";
const String USR_NOT_FOUND = "No User Found";
const String CURRENT_IDS = "\nCurrent IDS\n-----------\n";

//INIT
void setup(){
  /*  setup - Function
   *    Setup method for script which creates and initilises the serial comm etc.
   */
  Serial.begin(9600);
  Serial.println(ABOUT);
    
  rfid.begin(19200);
  delay(10);
  halt();
  
  lock_state = EEPROM.read(0);
  
  //Add static IDs if needed
  for(int i=0; i<sizeof(STATIC_ID_LOAD)/sizeof(int); i++){
    if (check_id(STATIC_ID_LOAD[i]) == false){
       EEPROM_action(3, STATIC_ID_LOAD[i]);
    }
  }
  
  int count = 0;
  while(count < 255){
    int tmp = EEPROMReadInt(count);
    if(tmp == -1){
      break; 
     } 
     count += 1;
  }

  if (count == 0){
    EEPROM_action(0);
    Serial.println(NO_USERS);
    app_state = 1;
  }
  
  change_lock(1);
}

void loop(){
  /*  loop - Function
   *    Main execution loop for code
   */  
  get_all_ids();
  process();
  check_serial();
}

void check_serial(){
  /*  check_serial - Function
   *    Checks and executes any data being sent over the serial port
   */
  int id = 0;
  String readString = "";
  
  while(Serial.available()){
    int tmp = Serial.read();
    
    switch (tmp){
      case 118:
        get_all_ids();
        Serial.print(CURRENT_IDS);
        print_ids();
        break;
      case 102:
        Serial.println(FACTORY_RESET);
        EEPROM_action(0);
        break;
      case 117:
        Serial.println(STORED_IDS);
        EEPROM_action(1);
        break;
      case 111:
        Serial.println(UNLOCKING_DOOR);
        change_lock(0);
        break;  
      case 99:
        Serial.println(LOCKING_DOOR);
        change_lock(1);
        break;
      case 115:
        if(lock_state == 0){
          Serial.println(LOCK_STATE_UNLOCK);
        }else{
          Serial.println(LOCK_STATE_LOCKED);
        }
        break;
      case 97:
        Serial.println(ADDING_USR);
        while(Serial.available()){
          char c = Serial.read();  //gets one byte from serial buffer
          if(c != ' '){
            readString += c; //makes the string readString
          }
        }
        id = readString.toInt();
        if(check_id(id) == false){
          EEPROM_action(3, id);
        }else{
          Serial.println(USR_EXISTS);
        }
        break;
      case 114:
        Serial.println(REMOVING_USR);
        while(Serial.available()){
          char c = Serial.read();  //gets one byte from serial buffer
          readString += c; //makes the string readString
        }
        id = readString.toInt();
        if(check_id(id)){
          EEPROM_action(2, id);
        }else{
          Serial.println(USR_NOT_FOUND);
        }
        break;
      case 104:
        Serial.println(ABOUT);
        break;
    }     
  }
}


void halt(){
  /*  halt - Function
   *    Stops the RFID reader sheild from scanning
   */
  rfid.write((byte)255);
  rfid.write((byte)0);
  rfid.write((byte)1);
  rfid.write((byte)147);
  rfid.write((byte)148);
}

int get_id(){
  /*  get_id - Function
   *    Returns the ID of a currently available RFID card on the readers
   *
   *  @return The calculated ID of the currend RFID card
   *  @rtype int
   */  
  int id[8];
  int output = 0;
  scan();
  while(rfid.available()){
    if(rfid.read() == 255){
      rfid.read();
      if(rfid.read() == 6){
        for(int i=0; i<8; i++){
          id[i]= rfid.read();
        }
        output = id[3] * id[4] * id[5] * id[6];
      }
    }
  }
  
  return output;
}

void get_all_ids(){
  /*  get_all_ids - Function
   *    Reads the current IDS into the global variable ids
   */  
  for(int i =0; i< (sizeof(ids)/sizeof(int)); i++){
    ids[i] = 0;
  }
  
  for(int i =0; i< (sizeof(ids)/sizeof(int))+1; i++){
    int tmp = get_id();
    if(tmp == ids[0]){
      break;
    }else{
      ids[i] = tmp;
    }
  }
}

void print_ids(){
  /*  check_id - Function
   *    Prints the currently available card IDS
   */  
  for(int i=0; i<sizeof(ids)/sizeof(int); i++){
    if(ids[i] != 0){
      Serial.println(ids[i]);
    }
  }
}

void process(){
  /*  process - Function
   *    Function checks to see if an ID card has been added or removed from the reader
   */  
  for(int i=0; i<sizeof(ids)/sizeof(int); i++){
    if(ids[i] != 0){
      boolean found = false;
      for(int j=0; j<sizeof(current_ids)/sizeof(int); j++){
        if (current_ids[j] == ids[i]){
          found = true;
          break;
        }
      }
      if(found == false){
        for(int k=0; k<sizeof(current_ids)/sizeof(int); k++){
          if (current_ids[k] == -1){
            current_ids[k] = ids[i];
            current_ids_time[k] = millis(); 
            
            if(check_id(current_ids[i])){
              /*change_lock(!lock_state);
              if(lock_state == 0){
                Serial.println(LOCK_STATE_UNLOCK);
              }else{
                Serial.println(LOCK_STATE_LOCKED);
              }*/
              change_lock(0);
              Serial.println(LOCK_STATE_UNLOCK);
              delay(2000);
              change_lock(1);
              Serial.println(LOCK_STATE_LOCKED);
            }       
            if(app_state == 1){
              app_state = 2;
              if(check_id(ids[i]) == false){
                Serial.println(ADDING_USR);
                EEPROM_action(3, ids[i]);
              }else{
                Serial.println(REMOVING_USR);
                EEPROM_action(2, ids[i]);
              }
            }
            break;
          }
        }
      }
    }
  }
   
  for(int i=0; i<sizeof(current_ids)/sizeof(int); i++){
    if(current_ids[i] != -1){
      boolean found = false;
      for(int j=0; j<sizeof(ids)/sizeof(int); j++){
        if (ids[j] == current_ids[i]){
          found = true;
          break;
        }
      }
      if(found == false){
        for(int k=0; k<sizeof(current_ids)/sizeof(int); k++){
          if (current_ids[k] == current_ids[i]){
            current_ids[k] = -1;
            if (app_state > 0){
              Serial.println("Exiting Admin State");
              app_state = 0;
            }
            break;
          }
        }
      }
    }
  }

  for(int i=0; i<sizeof(current_ids)/sizeof(int); i++){
   if(current_ids[i] != -1){
     if(check_id(current_ids[i])){
       if((millis() - current_ids_time[i]) > ADMIN_TIMEOUT & app_state == 0){
         Serial.println("Entering Admin State");
         app_state = 1;
       }
     }
   } 
  }
} 


boolean check_id(int id){
  /*  check_id - Function
   *    Checks if the passed ID can be found within the EEPROM memory
   *
   *  @param id: The id of the card being checked
   *  @type id: int
   */    
  int count = 0;
  while(count < 255){
    int tmp = EEPROMReadInt(count);
    if(tmp == -1){
      break; 
     } 
     if(tmp == id){
       return true;
     }
     count += 1;
  }
  return false;
}

void scan(){
  /*  scan - Function
   *    Calls a scan command on the RFID reader sheild
   */
  rfid.write((byte)255);
  rfid.write((byte)0);
  rfid.write((byte)1);
  rfid.write((byte)130);
  rfid.write((byte)131); 
  delay(50);
}

void EEPROM_action(int action){
  /*  EEPROM_action - Function
   *    Method adjusts the internal EEPROM based on the called instruction.
   *
   *  @param action: The id of the action being called
   *  @type action: int
   */  
  EEPROM_action(action, 0);
}

void EEPROM_action(int action, int val){
  /*  EEPROM_action - Function
   *    Method adjusts the internal EEPROM based on the called instruction.
   *
   *  @param action: The id of the action being called
   *  @type action: int
   *
   *  @param val: The val for which the action is being taken
   *  @type val: int
   */  
  int count = 0;
  int tmp = 0;
  while(count < 255){
    tmp = EEPROMReadInt(count);
    if(tmp == -1 && action != 3){
      break; 
    }
    switch (action){
      case 0:
        EEPROMWriteInt(count, -1);
        break;
      case 1:
        if(tmp != 0){
          Serial.print("ID: ");
          Serial.println(tmp);
        }
        break;
      case 2:
        if(tmp == val){
          EEPROMWriteInt(count, 0);
          Serial.print("Removed ID: ");
          Serial.println(val);
         }
         break;
       case 3:
         if(tmp == 0 || tmp == -1){
           EEPROMWriteInt(count, val);
           count = 256;
           Serial.print("Added ID: ");
           Serial.println(val);
         }
         break;
    }
    count += 1;
  } 
}


void change_lock(int state){
  /*  change_lock - Function
   *    Changes the current lock to the specified state (0 - Open, 1 - Locked)
   *
   *  @param state: The new state the lock should be changed to
   *  @type state: int
   */  
  if(lock_state != state){
    if(state == 0){
      lock_servo.attach(SERVO_PIN);
      lock_servo.write(170); 
      delay(1000); 
      lock_servo.detach(); 
      lock_state = 0;
      EEPROM.write(0, 0);
    }else{
      lock_servo.attach(SERVO_PIN);
      lock_servo.write(70); 
      delay(1000); 
      lock_servo.detach();
      lock_state = 1;
      EEPROM.write(0, 1);
    }
  }
}

void EEPROMWriteInt(int p_address, int p_value){
  /*  EEPROMWriteInt - Function
   *    Writes the passed two byte int value to EEPROM memory at the specified location
   *
   *  @param p_address: The position in memory the vlaue is to be stored
   *  @type p_address: int
   *
   *  @param p_value: A two byte int to be stored within memory
   *  @type p_value: int   
   */
  p_address = 2*(p_address+1);
  byte lowByte = ((p_value >> 0) & 0xFF);
  byte highByte = ((p_value >> 8) & 0xFF);

  EEPROM.write(p_address, lowByte);
  EEPROM.write(p_address + 1, highByte);	
}

unsigned int EEPROMReadInt(int p_address){
  /*  EEPROMReadInt - Function
   *    Returns a two bytes int found at the passed memory location
   *
   *  @param p_address: The position in memory the stored value is located
   *  @type p_address: int
   *
   *  @return The stored int value lcoated at the specified location
   *  @rtype int
   */
  p_address = 2*(p_address+1);
  byte lowByte = EEPROM.read(p_address);
  byte highByte = EEPROM.read(p_address + 1);

  return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
}
