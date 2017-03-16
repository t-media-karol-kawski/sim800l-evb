/* This piece of code receives phonecalls in order to turn off LED attached on PIN 13 on Arduino UNO board,
   Turns ON or OFF PIN 13 via SMS if previously registered on SIM Card,
   and adds a new contact on SIM Card via SMS command and establishing a HTTP connection
   to www.castillolk.com.ve/WhiteList.txt using SIM800L-EVB Module.

  If phone number is not on contact (contacts) the system rejects anything from them.

  if on contact turns off LED.

  //////////////////////////QUITAR
  Check out how to add a contact on the SIM Card as mentioned in the Spanish version of manual
  "InstalaciÃ¯Â¿Â½n de software Control de Encendido y Apagado por SMS .Prueba de Concepto del SIM800L-EVB en Arduino UNO_Finalizado"
  on page 2 from https://drive.google.com/folderview?id=0BxtBNyHdFnkkUnl0RGt0RTFOZzQ
  This only processes national numbers (ones that don't include the +  prefix).

  For example (AT+CPBW=1,"04161587896",129,"name") where 129 means national numbers.
  It is IMPERATIVE to save the number with a name or alias and not to leave it empty.
  If by mistake someone tries something like (AT+CPBW= 1,"+584161587896",129,"name") or (AT+CPBW= 1,"04161587896",145,"name")
  it won't work at all since it has a "+" sign in the first case and the 145 integer in the second.
  //////////////////////////QUITAR



  We will use the GPRS library from SEEEDUINO. It has a lot of functions for the SIM800L board that will make the development process easier and faster.
  You can get it from https://github.com/Seeed-Studio/Seeeduino_GPRS.git

  In case you don't know or remember how to implement a library on Arduino IDE,
  go on https://www.arduino.cc/en/hacking/libraries. Everything is explanied there very detailed.

*/


/****************************************************************Start of the code********************************************************/



//--------------------------------Start-Libraries Section----------------------------------------------------//

#include <gprs.h>
#include <SoftwareSerial.h>


//SIM800 TX is connected to Arduino D8
#define SIM800_TX_PIN 8

//SIM800 RX is connected to Arduino D7
#define SIM800_RX_PIN 7

//Create software serial object to communicate with SIM800
SoftwareSerial serialSIM800(SIM800_TX_PIN, SIM800_RX_PIN);


//--------------------------------End-Libraries Section----------------------------------------------------//


//--------------------------------Start-Definition Section--------------------------------------------------//

#define TIMEOUT    15000
#define LED_PIN    13
#define TIMEOUTINTERNET 30000

//--------------------------------End-Definition Section--------------------------------------------------//


//--------------------------------Start-Object Creation--------------------------------------------------//

GPRS gprs;

//--------------------------------End-Object Creation--------------------------------------------------//


//--------------------------------Start-Variable Declaration-------------------------------------------//

//Boolean to be set to false if number is on contact
bool ledStatus = HIGH;

////Variable to hold last line of serial output from SIM800
char currentLine[500] = "";
int currentLineIndex = 0;

//Boolean to be set to true if number is found on contact
bool isIncontact = false;

// Variable to hold contact from call
char contact[13]; // changes this if needed (a contact name with more than 13 characters)

// Variable to hold number from call
char phonenumber[13];

// Variable to send SMS
char *number = NULL;

////Boolean to be set to true if call notificaion was found and next line is NOT empty
//bool nextLineIsCall = false;
bool nextValidLineIsCall = false;

////Boolean to be set to true if message notificaion was found and next
////line of serial output is the actual SMS message content
bool nextLineIsMessage = false;

//Boolean to be set to true if number is found on phonebook
bool isInPhonebook = false;

// String which holds the last line read from Serial activitiy
String lastLine = "";

// Integer indexes
int firstComma = -1;
int secondComma = -1;
int thirdComma = -1;
int forthComma = -1;
int fifthComma = -1;
int firstQuote = -1;
int secondQuote = -1;
int len = -1;
int j = -1;
int i = -1;
int f = -1;
int r = -1;

// Contact Counters
int OldCounter = 0;
int NewCounter = 0;

// Temporal variable when using LoadWhiteList() ClearWhiteList()
String tmp = "";


int out = false;



/////////////////////////////////////Libelium///////////////////////////////////////////////////////////////////////

int8_t answer;
char data[512];
int data_size;

char aux_str[100];
char aux;
int x = 0;

/////////////////////////////////////Libelium///////////////////////////////////////////////////////////////////////


//--------------------------------End-Variable Declaration-------------------------------------------//



//------------------------------End-Libraries Section--------------------------------------------------------//


//--------------------------------Start-Setup Section-----------------------------------------------//

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  while (!Serial);

  // initialize digital pin 13 as an output.
  pinMode(LED_PIN, OUTPUT);

  //Write current status to LED pin
  digitalWrite(LED_PIN, ledStatus);

  Serial.println("Starting SIM800 SMS-Call Command Processor");
  gprs.preInit();
  delay(1000);

  while (0 != gprs.init())
  {
    delay(1000);
    Serial.print("init error\r\n");
  }

  //Check Call Availability
  if (0 != gprs.sendCmdAndWaitForResp("AT+CCALR?\r\n", "1", TIMEOUT))
  {
    ERROR("ERROR:CCALR");
    return;
  }


  //Set call notification
  if (0 != gprs.sendCmdAndWaitForResp("AT+CLIP=1\r\n", "OK", TIMEOUT))
  {
    ERROR("ERROR:CLIP");
    return;
  }

  //Set SMS mode to ASCII
  if (0 != gprs.sendCmdAndWaitForResp("AT+CMGF=1\r\n", "OK", TIMEOUT))
  {
    ERROR("ERROR:CMGF");
    return;
  }

  //Start listening to New SMS Message Indications
  if (0 != gprs.sendCmdAndWaitForResp("AT+CNMI=1,2,0,0,0\r\n", "OK", TIMEOUT))
  {
    ERROR("ERROR:CNMI");
    return;
  }

  /////////////////////////////////////Libelium///////////////////////////////////////////////////////////////////////

//  Serial.println("Starting Libelium Commands...");
//
//  // ////char pin[]="*******"; Mine has not got any PIN
//
//  // /////char apn[]="*******";
//  char apn[] = "internet.movistar.ve";
//  /////char user_name[]="*******";
//  char user_name[] = "";
//  ////char password[]="*******";
//  char password[] = "";
//
//  char url[ ] = "test.libelium.com/test-get-post.php?a=1&b=2";
//
//  ////while (sendATcommand2("AT+CREG?", "+CREG: 0,1", "+CREG: 0,5", 2000) == 0);
//
//  ////while (sendATcommand("AT+CREG?", "OK", 2000) == 0);
//
//  // sets APN , user name and password
//  sendATcommand("AT+SAPBR=3,1,\"Contype\", \"GPRS\"", "OK", 2000);
//
//  snprintf(aux_str, sizeof(aux_str), "AT+SAPBR=3,1,\"APN\",\"%s\"", apn);
//
//  sendATcommand(aux_str, "OK", 2000);
//
//  //snprintf(aux_str, sizeof(aux_str), "AT+SAPBR=3,1,\"USER\",\"%s\"", user_name);
//  //sendATcommand(aux_str, "OK", 2000);
//
//  //snprintf(aux_str, sizeof(aux_str), "AT+SAPBR=3,1,\"PWD\",\"%s\"", password);
//  //sendATcommand(aux_str, "OK", 2000);
//
//  while (sendATcommand("AT+SAPBR=1,1", "OK", 20000) == 0)
//  {
//    delay(5000);
//  }

  /////////////////////////////////////Libelium///////////////////////////////////////////////////////////////////////

  Serial.println("Init success");

}

//--------------------------------End-Setup Section----------------------------------------------//


//--------------------------------Start loop Section----------------------------------------------//


void loop()
{

  ConnectToInternet();
}


//--------------------------------End-loop Section----------------------------------------------//


void ConnectToInternet()
{
  Serial.println("****************************************************");

  // //AT+HTTPPARA="URL","www.castillolk.com.ve/WhiteList.txt"  "AT+HTTPPARA=\"URL\",\""
  
  //"AT+HTTPPARA=\"URL\",\"www.castillolk.com.ve/WhiteList.txt" + "\r\n"   "AT+SAPBR=3,1,\"APN\",\"internet.movistar.ve\"\r\n")

  //Serial.println("AT+HTTPPARA=\"URL\",\"www.castillolk.com.ve/WhiteList.txt\"\r\n");   
  
  
  //AT+SAPBR=3,1,"Contype", "GPRS"
  int numb1 = 3;
  int numb2 = 1;
  String n1;
  String n2;
  n1 = numb1;
  n2 = numb2;
  tmp = "AT+SAPBR=" + n1 + "," + n2 + "," +  "\"Contype\"" + "," + " " + "\"GPRS\"" + "\r\n";
  //tmp = 'AT+SAPBR=3,1,"Contype", "GPRS"';
  //Serial.println(tmp); //"AT+SAPBR=3,1,\"ConType\",\"GPRS\"\r\n"
  
  Serial.println("AT+SAPBR=3,1,\"ConType\",\"GPRS\"\r\n"); 
  if (0 != gprs.sendCmdAndWaitForResp("AT+SAPBR=3,1,\"ConType\",\"GPRS\"\r\n", "OK", TIMEOUT))
  {
    ERROR("ERROR:SAPBR-1");
    return;
  }
  Serial.println("1-Passed!");
  ///////////////////////////////////////////////////////////////
  
  //tmp = 'AT+SAPBR=3,1,"APN","int.movilnet.com.ve"';
  // Set APN Command. In our case is MOVILNET /////TELEFONICA MOVISTAR VENEZUELA
  //AT+SAPBR=3,1,"APN","int.movilnet.com.ve"  "internet.movistar.ve"  "\"internet.movistar.ve\""
  //tmp = "AT+SAPBR=" + n1 + "," + n2 + "," + "\"APN\"" + "," + "\"internet.movistar.ve\"" + "\r\n";
  
  //Serial.println(tmp); "AT+SAPBR=3,1,\"APN\",\"www\"\r\n"
  Serial.println("AT+SAPBR=3,1,\"APN\",\"internet.movistar.ve\"\r\n");
  if (0 != gprs.sendCmdAndWaitForResp("AT+SAPBR=3,1,\"APN\",\"internet.movistar.ve\"\r\n", "OK", TIMEOUT))
  {
    ERROR("ERROR:SAPBR-2");
    return;
  }
  Serial.println("2-Passed!");

  ///////////////////////////////////////////////////////////////

  //Open a GPRS context command. It takes a time longer than any other command (30000 seconds)
    //AT+SAPBR=1,1
    numb1 = 1;
    numb2 = 1;
    n1 = numb1;
    n2 = numb2;
    tmp = "AT+SAPBR=" + n1 + "," + n2 + "\r\n";
   //Serial.println(tmp.c_str()); "AT+SAPBR=1,1\r\n"

    Serial.println("AT+SAPBR=1,1\r\n");
    if (0 != gprs.sendCmdAndWaitForResp("AT+SAPBR=1,1\r\n", "OK", 50000)) //("AT+CMGF=1\r\n", "OK", TIMEOUT))
    {
     ERROR("ERROR:SAPBR-3");
     return;
    }
   Serial.println("3-Passed!");

///////////////////////////////////////////////////////////////

   //"AT+SAPBR=2,1\r\n"
   Serial.println("AT+SAPBR=2,1\r\n");
   if (0 != gprs.sendCmdAndWaitForResp("AT+SAPBR=2,1\r\n", "OK", TIMEOUT)) //("AT+CMGF=1\r\n", "OK", TIMEOUT))
    {
     ERROR("ERROR:SAPBR-4");
     return;
    }
   Serial.println("4-Passed!");
   
///////////////////////////////////////////////////////////////////////////////////////

  //"AT+HTTPINIT\r\n"
  Serial.println("AT+HTTPINIT\r\n");
   if (0 != gprs.sendCmdAndWaitForResp("AT+HTTPINIT\r\n", "OK", TIMEOUT)) //("AT+CMGF=1\r\n", "OK", TIMEOUT))
    {
     ERROR("ERROR:SAPBR-5");
     return;
    }
   Serial.println("5-Passed!");   //AT+HTTPPARA="URL","www.castillolk.com.ve/WhiteList.txt"  "AT+HTTPPARA=\"URL\",\""

//////////////////////////////////////////////////////////////////////////////////////////////

  //"AT+HTTPPARA=\"URL\",\"www.castillolk.com.ve/WhiteList.txt\"\r\n

  Serial.println("AT+HTTPPARA=\"URL\",\"www.castillolk.com.ve/WhiteList.txt\"\r\n");
   if (0 != gprs.sendCmdAndWaitForResp("AT+HTTPPARA=\"URL\",\"www.castillolk.com.ve/WhiteList.txt\"\r\n", "OK", TIMEOUT)) //("AT+CMGF=1\r\n", "OK", TIMEOUT))
    {
     ERROR("ERROR:SAPBR-6");
     return;
    }
  Serial.println("6-Passed!"); 

///////////////////////////////////////////////////////////////////////////////////////

  //AT+HTTPACTION=0
  Serial.println("AT+HTTPACTION=0\r\n");
   if (0 != gprs.sendCmdAndWaitForResp("AT+HTTPACTION=0\r\n", "OK", TIMEOUT)) //("AT+CMGF=1\r\n", "OK", TIMEOUT))
    {
     ERROR("ERROR:SAPBR-7");
     return;
    }
  Serial.println("7-Passed!"); 

///////////////////////////////////////////////////////////////////////////////////////

// Turn OFF




  //AT+HTTPREAD
  Serial.println("AT+HTTPREAD\r\n");
   if (0 != gprs.sendCmdAndWaitForResp("AT+HTTPREAD\r\n", "OK", TIMEOUT)) //("AT+CMGF=1\r\n", "OK", TIMEOUT))
    {
     ERROR("ERROR:SAPBR-8");
     return;
    }
  Serial.println("8-Passed!"); 

  // Turn OFF



///////////////////////////////////////////////////////////////////////////////////////

 //AT+HTTPTERM
 Serial.println("AT+HTTPTERM\r\n");
   if (0 != gprs.sendCmdAndWaitForResp("AT+HTTPTERM\r\n", "OK", 50000)) //("AT+CMGF=1\r\n", "OK", TIMEOUT))
    {
     ERROR("ERROR:SAPBR-8");
     return;
    }
 Serial.println("8-Passed!"); 


///////////////////////////////////////////////////////////////////////////////////////

 //AT+SAPBR=0,1
  Serial.println("AT+SAPBR=0,1\r\n");
   if (0 != gprs.sendCmdAndWaitForResp("AT+SAPBR=0,1\r\n", "OK", 50000)) //("AT+CMGF=1\r\n", "OK", TIMEOUT))
    {
     ERROR("ERROR:SAPBR-9");
     return;
    }
 Serial.println("9-Passed!"); 
 
///////////////////////////////////////////////////////////////////////////////////////
  
}

