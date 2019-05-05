#include <LiquidCrystal.h>
#include <Fsm.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <Keypad.h>

using namespace std;


struct user //8 bytes
{
  unsigned tag; //2 bytes
  unsigned password; //2 bytes
  char mobile[11]; //4 bytes
};

//GLOBALS 
const int BUFFER_SIZE = 14; // RFID DATA FRAME FORMAT: 1byte head (value: 2), 10byte data (2byte version + 8byte tag), 2byte checksum, 1byte tail (value: 3)
const int DATA_SIZE = 10; // 10byte data (2byte version + 8byte tag)
const int DATA_VERSION_SIZE = 2; // 2byte version (actual meaning of these two bytes may vary)
const int DATA_TAG_SIZE = 8; // 8byte tag
const int CHECKSUM_SIZE = 2; // 2byte checksum
//unsigned tag1 =7776767 ;
unsigned tag1 =9285385;
int eeStartAddress = 0;
int eeAddress1 = 1;
int eeAddress2 = 9;
unsigned tag2 = 7776441;
int ok=-1;

int countRFID = 0;
int countPassword = 0;
uint8_t countUsers = 0;

user currentUser;




//RFID
SoftwareSerial ssrfid = SoftwareSerial(6,7); 
uint8_t buffer[BUFFER_SIZE]; // used to store an incoming data frame 
int buffer_index = 0;


int in_pin=A0;

//Events

static const int CorrectRFID = 1;
static const int WrongRFID = 2;
static const int CardScanned = 3;
static const int TryAgain = 4;
static const int FalsePassword=5 ;
static const int CorrectPassword=6;
static const int LongTimeIdleMenu=7;
static const int Key1Pressed=8;
static const int Key2Pressed=9;
static const int Key3Pressed=10;
static const int Key4Pressed=11;
static const int FinishRemove=12;
static const int FinishModify=13;
static const int FinishAdd=14;
static const int DoorClosed=15;
static const int NOTIFY1 = 16;
static const int CardScannedForAddUser = 17;
static const int CardScannedForModifiedUser = 18;

static const int AddedUser=19;
static const int modifiedUser = 20;

static const int AdminRemoveUser=21;
static const int AdminDeleteUser=22;
static const int FinishDelete = 23;

bool addUserCardScanned = false;
bool deleteUserCardScanned = false;
bool modifyUserScanned = false;

//Constants
const int MAX_OPEN_TIME = 10000; //10 seconds
const int MAX_IDLE_TIME = 50000; //50 seconds


////////////////////////////////     Keypad initialization   ////////////////////////
const byte numRows=4;
const byte numCols=3;

char keymap[numRows][numCols]={{'1','2','3'},
                               {'4','5','6'},
                               {'7','8','9'},
                               {'*','0','#'}};

byte rowPins[numRows]={34,32,30,28};
byte colPins[numCols]={26,24,22};

char key;

Keypad mykeypad= Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols);

///////////////////////////////////////////   end keypad init   /////////////////




////////////////////////////////     LCD initialization   ////////////////////////
// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
//  LCD RS pin to digital pin 12
 //LCD Enable pin to digital pin 11
 // LCD D4 pin to digital pin 5
  //LCD D5 pin to digital pin 4
 // LCD D6 pin to digital pin 3
 // LCD D7 pin to digital pin 2
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 8, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

///////////////////////////////////////////   end LCD init   /////////////////





//to read the input key from the keypad
//int readKey()
//{
//  char key=mykeypad.getKey();
//
//  switch(key){
//    case '1':
//      {
//        Serial.println("key 1");
//        return 1;
//        break;
//      }
//      case '2':
//      {
//        Serial.println("key 2");
//        return 2;
//        break;  
//      }
//      case '3':
//      {
//        Serial.println("key 3");
//        return 3;
//        break;  
//      }
//      case '4':
//      {
//        Serial.println("key 4");
//        return 4;
//        break;  
//      }
//      case '5':
//      {
//        Serial.println("key 5");
//        return 5;
//        break;  
//      }
//      case '6':
//      {
//        Serial.println("key 6");
//        return 6;
//        break;  
//      }
//      case '7':
//      {
//        Serial.println("key 7");
//        break;  
//        return 7;
//      }
//      case '8':
//      {
//        Serial.println("key 8");
//        return 8;
//        break;  
//      }
//      case '9':
//      {
//        Serial.println("key 9");
//        return 9;
//        break;  
//      }
//      case '0':
//      {
//        Serial.println("key 0");
//        return 0;
//        break;  
//      }
//      case '*':
//      {
//        Serial.println("key *");
//        return 10;
//        break;  
//      }
//      case '#':
//      {
//        Serial.println("key #");
//        return 11;
//        break;  
//      }
//      default:
//      {return 13;
//      break;
//      }
//
//}

int readKey()
{
  int key = mykeypad.getKey();
  return key;
}


void savePassword()
{
  int default_pass = 12345;
  //Currently 2 users
   EEPROM.put(eeStartAddress,2);
  //Save data for the first user
   EEPROM.put(eeAddress1, tag1);
   EEPROM.put(eeAddress1+2, default_pass);
   EEPROM.put(eeAddress2, tag2);
   EEPROM.put(eeAddress2+2, default_pass);
  /////Saving 
   Serial.print("Saved Successfully\n");
} 

void updateInfo(int check,int pass,long mobile)
{
  Serial.println("update info\n");

  bool found= false;
  int i=0;
  int address = 1;

  int saved_tag;

  //unsigned tag = extract_tag();
  
  while(i<countUsers && !found)
  {
    EEPROM.get(address,saved_tag);
    if(saved_tag == currentUser.tag)
    {
    found = true;
    if(check == 1)
      EEPROM.get(address+2, pass);
    else if(check == 2)
    {
      EEPROM.get(address+2, pass);
      EEPROM.get(address+4, mobile);
    }
    else
      EEPROM.get(address+4, mobile);
      break;
    }
    i++;
    address += 8;

    
    }
  }

void saveUser(unsigned tag,int pass,long phone)
{
  
  
   countUsers++;
   EEPROM.put(eeStartAddress,countUsers);
   int address = eeAddress1 + 8*(countUsers-1);
  //Save data for the first user
   EEPROM.put(address, tag);
   EEPROM.put(address+2, pass);
   EEPROM.put(address+2, phone);

  /////Saving 
   Serial.print("Saved Successfully\n");
} 


unsigned readPassword(int x)
{
  Serial.println("Read from EEPROM: ");
  ///////
  if(x==1)
  {
    unsigned retrivePass1;
    EEPROM.get(eeAddress1, retrivePass1);
    return retrivePass1;
  }
  else 
  {
    unsigned retrivePass2;
    EEPROM.get(eeAddress2, retrivePass2);
    return retrivePass2;
    }
 

}


State WaitForCard(0,&scanCard,0);
State CardChecking(0,&checkRFIDcard,0);
//State InvalidCard(0,&displayInvalid,0);
State CorrectCard(0,&promptForPassword,0);
State Notify(0,&notify,0);
State MenuDisplay(0,&displayMenu,0);
State OpenLock(0,&openLock,0);
State AddingUser(0,&addUser,0);
State ModifyinigUser(0,&modifyUser,0);
State RemovingUser(0,&removeUser,0);
State AdduserInformation(0,&addUserInfo,0);
State ModifyUserInfo(0,&modifyUserInfo,0);

State DeleteUser(0,&deleteAllUser,0);

//FSM
 Fsm SystemFSM(&WaitForCard);


void scanCard()
{
  Serial.println("Scanning Card\n");

    if (Serial1.available() > 0){
    bool call_extract_tag = false;
    
    int ssvalue = Serial1.read(); // read 
    Serial.print("ssvalue=");
    Serial.println(ssvalue);
    if (ssvalue == -1) { // no data was read
      return;
    }

    if (ssvalue == 2) { // RDM630/RDM6300 found a tag => tag incoming 
      buffer_index = 0;
    } else if (ssvalue == 3) { // tag has been fully transmitted       
      call_extract_tag = true; // extract tag at the end of the function call
    }

    if (buffer_index >= BUFFER_SIZE) { // checking for a buffer overflow (It's very unlikely that an buffer overflow comes up!)
      Serial.println("Error: Buffer overflow detected!");
      return;
    }
    
    buffer[buffer_index++] = ssvalue; // everything is alright => copy current value to buffer
   
    if (call_extract_tag == true){ 
      if(buffer_index == BUFFER_SIZE) {
      if(addUserCardScanned==true)
      {
        delay(1000);
//        while(Serial1.available())
        Serial1.flush();
        Serial.println("Scanned  new RFID");
        
        SystemFSM.trigger(CardScannedForAddUser);
      }
      else if(modifyUserScanned==true)
      {
        delay(1000);
//        while(Serial1.available())
        Serial1.flush();
        Serial.println("Scanned  modified User RFID");
        
        SystemFSM.trigger(CardScannedForModifiedUser);
      }
      else if (deleteUserCardScanned==true)
      {
        delay(1000);
//        while(Serial1.available())
        Serial1.flush();
        Serial.println("Scanned  modified User RFID");
        SystemFSM.trigger(CardScannedForModifiedUser);
      }
      else
      {
        delay(1000);
//  while(Serial1.available())
        Serial1.flush();
        SystemFSM.trigger(CardScanned);
      }
      }
      else { // something is wrong... start again looking for preamble (value: 2)
        buffer_index = 0;
        return;
      }

    }
   
  
      }
      

 //SystemFSM.trigger(CardScanned);
    }    

//void checkRFIDcard()
//{
//  Serial.println("Checking RFID card\n");
//
//    unsigned tag = extract_tag();
//    unsigned pass1=readPassword(1);
//    unsigned pass2=readPassword(2);
//    if(tag==pass1 || tag ==pass2)
//    {
//      ok=1;
//      SystemFSM.trigger(CorrectRFID);
//      Serial.println("Password matched");
//    
//    }
//  else {
//      ok=0;
//      Serial.println("Password Unmatched");
//      countRFID++;
//      if(countRFID>=3)
//      {
//        countRFID = 0;
//        SystemFSM.trigger(NOTIFY1);
//      }
//      else
//      {
//        SystemFSM.trigger(TryAgain);
//      }
//      
//    }
//  
//}

void checkRFIDcard()
{
  Serial.println("Checking RFID card\n");

  bool found= false;
  int i=0;
  int address = 1;

  int saved_tag;
  int pass;
  long mobile;

  unsigned tag = extract_tag();
  
  while(i<countUsers && !found)
  {
    EEPROM.get(address,saved_tag);
    if(saved_tag == tag)
    {
    found = true;
    EEPROM.get(address+2, pass);
    EEPROM.get(address+4, mobile);
    currentUser.tag = tag;
    currentUser.password = pass;
    longToCharacterArray(mobile,currentUser.mobile);
    Serial.print("After conversion");
    Serial.println(currentUser.mobile);
    Serial.println(currentUser.password);
    Serial.println(currentUser.tag);
    SystemFSM.trigger(CorrectRFID);
    }
    i++;

    address += 8;
  }

  if(!found)
  {
      Serial.println("Password Unmatched");
      countRFID++;
      if(countRFID>=3)
      {
        countRFID = 0;
        SystemFSM.trigger(NOTIFY1);
      }
      else
      {
        //Display invalid to the user
        lcd.setCursor(0,0);
        lcd.print("Invalid RFID!");
        lcd.setCursor(3,1);
        lcd.print("Try again :(");
        SystemFSM.trigger(TryAgain);
      }
  }

 //SystemFSM.trigger(CorrectRFID); 
}


void addUserInfo()
{
  addUserCardScanned = false;
  unsigned tag = extract_tag();
  
  lcd.setCursor(0,0);
  lcd.print("Enter password");
  
  int integer_pass = enterPassword();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Enter phone");
  long phone_number = enterPhoneNumber();

  saveUser(tag,integer_pass,phone_number);


  SystemFSM.trigger(FinishAdd);
  
  
}

void modifyUserInfo()
{
  v:
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("1-pass  2-both");
  lcd.setCursor(0,1);
  lcd.print("3-mobile no");

  char* x="";
  readFromKeyPad(x,1);

  lcd.clear();
  
  if(x == '1')
  {
    lcd.setCursor(0,0);
    lcd.print("enter new pass");

    char* y = "";
    int i = readFromKeyPad(y,5);

    int  password = (int)converttoInt(y,i);
    updateInfo(x-'0',password,0);
  }
  else if(x == '2')
  {
    lcd.setCursor(0,0);
    lcd.print("enter no & pass");

    char* y = "";
    int i = readFromKeyPad(y,5);

    int  password = (int)converttoInt(y,i);

    char* y1 = "";
    i = readFromKeyPad(y1,11);

    long  mobileno = converttoInt(y1,11);
    updateInfo(x-'0',password,mobileno);
  }
  else if(x == '3')
  {
    lcd.setCursor(0,0);
    lcd.print("enter no & pass");

    char* y1 = "";
    int i = readFromKeyPad(y1,11);

    long  mobileno = converttoInt(y1,11);
    updateInfo(x-'0',0,mobileno);
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("invalid key");
    lcd.setCursor(0,1);
    lcd.print("enter key again");
    goto v;
  }
  
  
  modifyUserScanned = false;
  SystemFSM.trigger(FinishModify);
  
}





//void displayInvalid()
//{
//  Serial.println("Display Invalid Card\n");
//}



int enterPhoneNumber()
{

  char mobileNumber[11];
  int i = readFromKeyPad(mobileNumber,11);
  long  phoneNumber = converttoInt(mobileNumber,11);
  return phoneNumber;
  
}





int readFromKeyPad(char * arr,int arr_size)
{

  int i=0;
  int prod = 1; 
  int pass = 0;
  
  while(i<arr_size)
  {
    
    int x = readKey();
    char c = char(x);
    
    if(x==42) //delete *
    {
      if(i>0) i--;
      arr[i] = '*';
      
      
      lcd.setCursor(i,1);
      lcd.print(' ');
    }
    else if(x==35) //enter #
    {
      break;
    }
    else if(x!=0)
    {
      
      lcd.setCursor(i,1);
      lcd.print(c);
      arr[i]=c;
      i++;
      
      Serial.println(arr);
    }
  }




return i;
  
}



int enterPassword()
{

  lcd.setCursor(0,0);
  lcd.print("Enter password");
  int i=0;
  int prod = 1; 
  int pass = 0;
  char pass_arr[5]="*****";
  Serial.println("Prompting for password\n");
//  while(i<5)
//  {
//    
//    int x = readKey();
//    char c = char(x);
//    
//    if(x==42) //delete *
//    {
//      if(i>0) i--;
//      pass_arr[i] = '*';
//      
//      
//      lcd.setCursor(i,1);
//      lcd.print(' ');
//    }
//    else if(x==35) //enter #
//    {
//      break;
//    }
//    else if(x!=0)
//    {
//      
//      lcd.setCursor(i,1);
//      lcd.print(c);
//      pass_arr[i]=c;
//      i++;
//      
//      Serial.println(pass_arr);
//    }
//  }

   i = readFromKeyPad(pass_arr,5);

  int integer_pass = (int)converttoInt(pass_arr,i);
  Serial.print("integer pass=");
  Serial.println(integer_pass);

  return integer_pass;

  
}



void promptForPassword()
{
//  lcd.setCursor(0,0);
//  lcd.print("Enter password");
//  int i=0;
//  int prod = 1; 
//  int pass = 0;
//  char pass_arr[5]="*****";
//  while(i<5)
//  {
//    Serial.println("Prompting for password\n");
//    int x = readKey();
//    char c = char(x);
//    
//    if(x==42) //delete *
//    {
//      if(i>0) i--;
//      pass_arr[i] = '*';
//      
//      
//      lcd.setCursor(i,1);
//      lcd.print(' ');
//    }
//    else if(x==35) //enter #
//    {
//      break;
//    }
//    else if(x!=0)
//    {
//      
//      lcd.setCursor(i,1);
//      lcd.print(c);
//      pass_arr[i]=c;
//      i++;
//      
//      Serial.println(pass_arr);
//    }
//  }

//  int integer_pass = convertPasswordtoInt(pass_arr,i);
   int integer_pass = enterPassword();
  
  Serial.print("current pass");
  Serial.println(currentUser.password);
  if(integer_pass == currentUser.password)
  {
    SystemFSM.trigger(CorrectPassword);
    
  }
  else
  {
    countPassword++;
    if(countPassword>=3)
    {
      countPassword = 0;
      SystemFSM.trigger(FalsePassword);
    }
    
  }
  
 
}
void notify()
{
  Serial.println("Notify user\n");
  Serial2.println("Breaking into safe");
}
void displayMenu()
{
  char* options[4]={"Open door","Add user","Modify user","Remove user"};
  Serial.println("Displaying menu");
  lcd.clear();
  int x= readKey();
  int current=0;

  lcd.print(options[0]);
  while(true)
  {
    x= readKey();
  //let * be next and # be prev
   if(x==42) //next *
    {
      lcd.clear();
      current = (current+1)%4;
      lcd.setCursor(1,0);
      lcd.print(options[current]);
      
      
    }
    else if(x==35) //enter prev
    {
      if(current>0)
      current = (current-1);
      else
      {
        current = 3;
      }
      
      Serial.print(current);
      lcd.clear();
      lcd.setCursor(1,0);
      lcd.print(options[current]);
      
    }
    else
    {
     
      if(x=='1')
      {
        SystemFSM.trigger(Key1Pressed);
        break;
      }
      else if(x=='2')
      {
          Serial.println("Pressed 2 Adding user");

        SystemFSM.trigger(Key2Pressed);
        Serial.println("Triggered state key2pressed");
        break;
        
        
      }
      else if(x=='3')
      {
        SystemFSM.trigger(Key3Pressed);
        break;
      }
      else if(x=='4')
      {
        SystemFSM.trigger(Key4Pressed);
        break;
      }
      
    }
  }
   
}
void openLock()
{
  Serial.println("Openning lock\n");
  for(int i=0; i<1000; i++)
  {
    if(analogRead(in_pin) != 0)
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("door closed");
      SystemFSM.trigger(DoorClosed);
    }
    delay(1);
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("door open");
  SystemFSM.trigger(MAX_OPEN_TIME);
}
void addUser(){

  Serial.println("in function adding user");
  addUserCardScanned = true;
  SystemFSM.trigger(AddedUser);
  Serial.println("Adding user\n");
  addUserCardScanned = false;
}
void modifyUser()
{
  
  Serial.println("Modifying user\n");
  modifyUserScanned = true;
  SystemFSM.trigger(modifiedUser);
  modifyUserScanned = false;
  

  
}
void removeUser()
{
  Serial.println("Removing user\n");

  int address = 1;

  int saved_tag;

  //unsigned tag = extract_tag();
  EEPROM.get(address,saved_tag);
  if(saved_tag != currentUser.tag)
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("not allowed");
    lcd.setCursor(0,1);
    lcd.print("to delete user");
    SystemFSM.trigger(FinishRemove);
  }
  else
  {
    deleteUserCardScanned=true;
    SystemFSM.trigger(AdminRemoveUser);
    deleteUserCardScanned=false;
    
  }
  //SystemFSM.trigger(FinishRemove);
  
}
void deleteAllUser()
{
  deleteUserCardScanned = false;
  
  unsigned tag = extract_tag();

  bool found= false;
  int i=1;
  int address = 9;

  int saved_tag;
  
  while(i<=countUsers && !found)
  {
    EEPROM.get(address,saved_tag);
    if(saved_tag == tag)
    {
      found = true;
      if(i==countUsers)
      {
        countUsers --;
        EEPROM.put(0,countUsers);
        break;
        //or trigger here
      }
      else
      {
        int last_address = (1 + countUsers*8) - 8;
        int last_tag,last_pass;
        long last_mobile;
        EEPROM.get(last_address,last_tag);
        EEPROM.get(last_address+2,last_pass);
        EEPROM.get(last_address+4,last_mobile);

        EEPROM.put(address,last_tag);
        EEPROM.put(address+2,last_pass);
        EEPROM.put(address+4,last_mobile);

        break;
      }
      
    }
    i++;
    address += 8;
   }
   
   lcd.clear();
   lcd.setCursor(0,0);
   if(!found)
   {
      lcd.print("user not found");
   }
   else
      lcd.print("finish delete");
      
   SystemFSM.trigger(FinishDelete);
}



void addSystemTransitions()
{
  
  SystemFSM.add_transition(&WaitForCard,&CardChecking,CardScanned,NULL); //uncomment this
  SystemFSM.add_transition(&WaitForCard,&AdduserInformation,CardScannedForAddUser,NULL);
   SystemFSM.add_transition(&AdduserInformation,&MenuDisplay,FinishAdd,NULL);
  SystemFSM.add_transition(&AddingUser,&WaitForCard,AddedUser,NULL);
  
  //SystemFSM.add_transition(&CardChecking,&InvalidCard,WrongRFID,NULL);
  SystemFSM.add_transition(&CardChecking,&CorrectCard,CorrectRFID,NULL);
  
  SystemFSM.add_transition(&CardChecking,&WaitForCard,TryAgain,NULL); //changed 
  SystemFSM.add_transition(&CardChecking,&Notify,NOTIFY1,NULL); //changed
  SystemFSM.add_transition(&CorrectCard,&Notify,FalsePassword,NULL);
  SystemFSM.add_transition(&CorrectCard,&MenuDisplay,CorrectPassword,NULL);
  SystemFSM.add_transition(&MenuDisplay,&OpenLock,Key1Pressed,NULL);
  SystemFSM.add_transition(&MenuDisplay,&AddingUser,Key2Pressed,NULL);
  //SystemFSM.add_transition(&MenuDisplay,&ModifyinigUser,Key3Pressed,NULL);
  SystemFSM.add_transition(&MenuDisplay,&ModifyUserInfo,Key3Pressed,NULL);
  SystemFSM.add_transition(&MenuDisplay,&RemovingUser,Key4Pressed,NULL);

  //SystemFSM.add_transition(&ModifyingUser,&WaitForCard,modifiedUser,NULL);
  //SystemFSM.add_transition(&WaitForCard,&ModifyUserInfo,CardScannedForModifiedUser,NULL);
  
  //SystemFSM.add_transition(&OpenLock,&MenuDisplay,FinishModify,NULL);
  SystemFSM.add_transition(&RemovingUser,&MenuDisplay,FinishRemove,NULL);
  SystemFSM.add_transition(&ModifyUserInfo,&MenuDisplay,FinishModify,NULL);

  SystemFSM.add_transition(&RemovingUser,&WaitForCard,AdminRemoveUser,NULL);
   SystemFSM.add_transition(&WaitForCard,&DeleteUser,AdminDeleteUser,NULL);
  //SystemFSM.add_transition(&CardChecking,&DeleteUser,AdminDeleteUser,NULL);
  SystemFSM.add_transition(&DeleteUser,&MenuDisplay,FinishDelete,NULL);
  

  
  SystemFSM.add_transition(&OpenLock,&WaitForCard,DoorClosed,NULL);

  //Timed transitions
  SystemFSM.add_timed_transition(&OpenLock,&Notify,MAX_OPEN_TIME,NULL);
  SystemFSM.add_timed_transition(&MenuDisplay,&WaitForCard,MAX_IDLE_TIME,NULL);


}


void setup() {


   Serial.begin(9600); 
   Serial1.begin(9600);
   Serial2.begin(9600);
   //ssrfid.begin(9600);
   savePassword();
   addSystemTransitions();
   readUserCount();
  lcd.begin(16, 2);
  lcd.clear();

  pinMode(in_pin,INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  

   SystemFSM.run_machine();


}


void readUserCount()
{

  EEPROM.get(eeStartAddress, countUsers);

  
}

long converttoInt(char * pass,int n)
{

  Serial.print("array=");
  Serial.println(pass);
  
  long prod=1;
  long sum = 0; 
  for(int i=0;i<n;i++)
  {
   sum += (pass[n-i-1]-'0')*prod;
   Serial.println(sum);
   prod*=10;
    
  }
  Serial.println(sum);
  return sum;
}

unsigned extract_tag() {
    uint8_t msg_head = buffer[0];
    uint8_t *msg_data = buffer + 1; // 10 byte => data contains 2byte version + 8byte tag
    uint8_t *msg_data_version = msg_data;
    uint8_t *msg_data_tag = msg_data + 2;
    uint8_t *msg_checksum = buffer + 11; // 2 byte
    uint8_t msg_tail = buffer[13];

    // print message that was sent from RDM630/RDM6300
    Serial.println("--------");

    Serial.print("Message-Head: ");
    Serial.println(msg_head);

    Serial.println("Message-Data (HEX): ");
    for (int i = 0; i < DATA_VERSION_SIZE; ++i) {
      Serial.print(char(msg_data_version[i]));
    }
    Serial.println(" (version)");
    for (int i = 0; i < DATA_TAG_SIZE; ++i) {
      Serial.print(char(msg_data_tag[i]));
    }
    Serial.println(" (tag)");

    Serial.print("Message-Checksum (HEX): ");
    for (int i = 0; i < CHECKSUM_SIZE; ++i) {
      Serial.print(char(msg_checksum[i]));
    }
    Serial.println("");

    Serial.print("Message-Tail: ");
    Serial.println(msg_tail);

    Serial.println("--");

    long tag = hexstr_to_value(msg_data_tag, DATA_TAG_SIZE);
    Serial.print("Extracted Tag: ");
    Serial.println(tag);

    long checksum = 0;
    for (int i = 0; i < DATA_SIZE; i+= CHECKSUM_SIZE) {
      long val = hexstr_to_value(msg_data + i, CHECKSUM_SIZE);
      checksum ^= val;
    }
    Serial.print("Extracted Checksum (HEX): ");
    Serial.print(checksum, HEX);
    if (checksum == hexstr_to_value(msg_checksum, CHECKSUM_SIZE)) { // compare calculated checksum to retrieved checksum
      Serial.print(" (OK)"); // calculated checksum corresponds to transmitted checksum!
    } else {
      Serial.print(" (NOT OK)"); // checksums do not match
    }

    Serial.println("");
    Serial.println("--------");

    return tag;
}

long hexstr_to_value(char *str, unsigned int length) { // converts a hexadecimal value (encoded as ASCII string) to a numeric value
  char* copy = malloc((sizeof(char) * length) + 1); 
  memcpy(copy, str, sizeof(char) * length);
  copy[length] = '\0'; 
  // the variable "copy" is a copy of the parameter "str". "copy" has an additional '\0' element to make sure that "str" is null-terminated.
  long value = strtol(copy, NULL, 16);  // strtol converts a null-terminated string to a long value
  free(copy); // clean up 
  return value;
}

void longToCharacterArray(long num,char * str)
{
  char number[11];
  int cnt=11;
  int digit = 0;
  for(int i=0;i<cnt-1;i++)
  {
    digit = num%10;
    num/=10;
    number[cnt-i-1] = char(30+digit);
    
  }
  number[0]='0';
   Serial.print("mobile number: ");
   Serial.print(number);

   strcpy(str,number);
   //<ake sure string has the correct value

   Serial.print("mobile number");
   Serial.println(str);
  
}
//}
