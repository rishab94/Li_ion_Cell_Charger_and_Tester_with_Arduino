/*  SD card datalogger

 The circuit:
 * analog sensors on analog ins 0, 1, and 2
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 51
 ** MISO - pin 50
 ** CLK - pin 52
 ** CS - pin 53 (for MKRZero SD: SDCARD_SS_PIN)

 created  24 Nov 2010
 modified 9 Apr 2012
 by Tom Igoe

 This example code is in the public domain.
 */
 
 /* Li-ion cell test system - Rishab Anand and Amarkumar K.
    IIT Bombay, Summer 2018.
 */

#include <SPI.h>
#include <SD.h>
#include <Timer1.h>
#include <Timer3.h>
#include <Timer4.h>
//#include <Timer5.h>

// ************ Cell Parameters ************* //
const float V_max = 4.2;             // Upper cell voltage limit
const float V_min = 3.0;               // Lower cell voltage limit
const float Q = 2000;                // Approximate cell capacity in mAh

//***** Charging Process Parameters *********//
const float C_rate = 0.03;           // Charging current in CC mode
const float I_cut_off = 0.03;        // Cut-off current for CV mode in C
const int chg_dir  = 1;              // Charge:0; Discharge:1

float Vt;                      // Variable to hold sensed terminal voltage (final value in V)
float I;                       // Variable to hold sensed current (final value in A)
float Iref;                    // Reference current for current controller (A)

const int chipSelect = 53;     // Required for Mega 2560
String dataString = "";        // Initialize data string to store log 
int duty=80; 

int flag;                      // flag to be used for data logging. Log the data only when flag is set
int volt_flag = 0;             // flag to be used to check voltage limits to switch to CV mode from CC mode
int cur_flag_cnt = 0;          // Counts number of instances the current is registered below the cut-off limit in CV mode
int cur_flag = 0;              // flag to be used to check current in CV mode to stop charging

float T_pi=0.04;               // Timestep
float er,ei=0;                 // initialize PI controller parameters
int sat_flag = 0;              // Saturation flag for anti-windup
unsigned long tim;             // variable to store time in millisecond

int chg_mode = 0;              // CC:0; CV:1
int chg_hlth = 1;              // Charger Healthy:1; Fault:0

void setup() 
{
  pinMode(53,OUTPUT);   //Chip Select
  pinMode(10,OUTPUT);
  pinMode(9,INPUT);
  pinMode(8,OUTPUT);    //Relay Coils
  pinMode(32,OUTPUT);   //PWM Output
  pinMode(33,OUTPUT);
  pinMode(A0,INPUT);    //Current Input
  pinMode(A1,INPUT);    //Voltage Input
  pinMode(13,OUTPUT);
  
  pinMode(11,OUTPUT);   //Green LED - Cycle Complete - Charger Healthy
  pinMode(12,OUTPUT);   //Red RED - Fault
  
  chg_mode = 0;         //Start process in CC mode
  digitalWrite(11,0);
  chg_hlth = 1;
  digitalWrite(12,0);
  
  digitalWrite(53, 0); //Chip Select for memory card
  
  volt_flag = 0;
  cur_flag_cnt = 0;
  cur_flag = 0;
  
  //********* Set relay positions according to charging mode *********//
  if(chg_dir == 0)      //Charging
  {
    digitalWrite(8,0);  //De-energize relay coils
  }
  else if(chg_dir == 1) //Discharging
  {
    digitalWrite(8,1);  //Energize relay coils
  }
  //*******************************************************************//
  
  startTimer1(0.5*1000000);       // data logging interval in microsecond(us)
  startTimer3(T_pi*1000000);      // sampling interval for PI controller in us
  startTimer4(500);               // reference current generator using PWM. Period of the carrier wave in us 
  //startTimer5(3000);
 
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) 
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.print("Initializing SD card...\n");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) 
  {
    Serial.println("Card failed, or not present. \n");
    return;
  }
  Serial.println("Card initialized. \n");  
}

//Function to check whether terminal voltage values are corrupted
void chk_hlth(float Vt)
{
  if((Vt < (V_min - 0.2))||(Vt > (V_max + 0.2)))
  {
    chg_hlth = 0;
    digitalWrite(12,1);
  }
}

//******* Timer 1 ISR *******//
// Sets flag for data logging
// Samples processor time, current and voltage
ISR(timer1Event) 
{
  resetTimer1();
  flag=1;                               // set the flag to enable data logging in main loop
  tim = micros();
  if(chg_dir == 0)                      //Calibration inputs - ADC reference fluctuations
  {
    //Vt=analogRead(A1)*0.00192 +2.495;
    Vt = analogRead(A1)*0.001944 +2.495;  
    I =analogRead(A0)*0.0051;       
    //chk_hlth(Vt);                      //Verify terminal voltage is within limits
  }
  else if(chg_dir == 1)
  {
    Vt=analogRead(A1)*0.00195 +2.495; 
    //I =analogRead(A0)*0.0052;
    I = analogRead(A0)*0.0051;
    //chk_hlth(Vt);                      //Verify terminal voltage is within limits
  }
}

//******* Timer 3 ISR *******//
// Controls switching from CC to CV mode 
// Controls terminal voltage in CV mode by reducing current reference
ISR(timer3Event) //PI controller
{
  resetTimer3();
  if(chg_dir == 0)                      //Calibration inputs - ADC reference fluctuations
  {
    //Vt=analogRead(A1)*0.00195 +2.495;
    Vt = analogRead(A1)*0.001944 +2.495;
    //chk_hlth(Vt);  
  }
  else if(chg_dir == 1)
  {
    Vt=analogRead(A1)*0.001195 +2.495;
    //chk_hlth(Vt);  
  }
  if (chg_mode == 0)  //CC mode
  {
    Iref = C_rate*Q/1000;
    if(((chg_dir == 0)&&(Vt > V_max))||((chg_dir == 1)&&(Vt < V_min)))
    {
      /*
      if(volt_flag == 0)
      {
        volt_flag = 1; //Wait for next reading
      }
      else if(volt_flag == 1)
      {
        volt_flag = 0;
        ei = 0;        //Reset accumulator for integrator
        chg_mode = 1;  //Switch mode to CV
      } 
      */
      volt_flag = volt_flag+1;
      if(volt_flag == 500)
      {
        //volt_flag = 0;
        ei = 0;
        //chg_mode = 1;
      }
    }
  }
  else if (chg_mode == 1)  //CV mode
  {
    if(I <= (Q*I_cut_off/1000))
    {
      cur_flag_cnt+=1;
      if(cur_flag_cnt == 50)
      {
        cur_flag_cnt = 0;
        cur_flag = 1;      //Stop Charging/Discharging
        digitalWrite(11,1);  //Turn ON Green LED
      }   
    }
    if  (chg_dir == 0)     //Charge
    {
      er = V_max - Vt;     // error; increasing current ref during charging increases cell terminal voltage
    }
    else if (chg_dir == 1) //Discharge
    {
      er = Vt - V_min;     //error; Increasing current ref during discharge decreases cell terminal voltage
    }
    
    if(sat_flag == 0)        //Anti-windup - Stop accumulation if output saturated
    {
      ei = ei + er*T_pi;     // accumulated error for integral action
    }
    Iref = 0.2*er + 30*ei;   // reference generation

  }
  //Iref = 1;
  duty = 50*Iref;          // convert reference current into appropriate duty ratio
  //Serial.println(Iref);
  //Serial.println(Vt);
}

//******* Timer 4 ISR *******//
// Generates PWM pulses from given duty
// Checks for saturation and implements anti-windup
ISR(timer4Event) // reference generation
{
  resetTimer4();
  //Anti-windup implemented
  if(cur_flag == 0)
  {
    if (duty > 100) // upper saturation for the reference
    {
      duty = 100;
      sat_flag = 1;      //Flag to stop integral error accumulation
    }
    else if (duty <1)    // lower saturation for the reference
    {  
      duty = 1;
      sat_flag = 1;      //Flag to stop integral error accumulation
    }
    else 
    {
      sat_flag = 0;      //Ref within linear range
    }
    digitalWrite(32,1);
    delayMicroseconds(2*duty);
    digitalWrite(32,0);
  }
  else if(cur_flag == 1)
  {
    digitalWrite(32, 0);  //Stop charging/discharging completely
  }
}

void loop() 
{
  if (flag == 1)
  {
  //if (digitalRead(9) !=0) //Check if card is to be written or not. To avoid write operation while card is being removed.
    {
      dataString = String(tim);
      dataString += String(",");
      dataString += String(chg_dir);
      //dataString += String((Vt>V_min));
      dataString += String(",");
      dataString += String(chg_mode);
      dataString += String(",");
      dataString += String(I,3);
      dataString += String(",");
      dataString += String(Vt,3);
      dataString += String(",");
      dataString += String(cur_flag);
      //dataString += String(volt_flag);
      Serial.println(dataString);
      File dataFile = SD.open("2C2209.txt", FILE_WRITE); //open file on the sd card to write. File name should not exceed 8 characters, e.g. datalog11 is invalid.
  
      if (dataFile) // check if the datafile is present. If yes, write to it.
        {
          dataFile.println(dataString);
          dataFile.close();
        }
    }
  }
    flag =0; //reset the flag when data is logged    
}
