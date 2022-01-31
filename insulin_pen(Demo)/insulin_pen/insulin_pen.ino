#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1331.h"
#include "max6675.h"

#include "myDef.h"
#include "myFuncDef.h"

Adafruit_SSD1331 dsp_1(&SPI, CS1, DC1, RST1);
Adafruit_SSD1331 dsp_2(&SPI, CS2, DC2, RST2);
Adafruit_SSD1331 dsp_3(&SPI, CS3, DC3, RST3);

int KeyA = 0xFF;
int KeyB = 0xFF;
unsigned long pre_key_A_readtime = millis();
unsigned long pre_key_B_readtime = millis();
bool is_key_A_change = false;
bool is_key_B_change = false;

signed int real_temp, hope_temp;
MAX6675 thermocouple(TEMP_CLK, TEMP_CS, TEMP_DO);
unsigned long pre_temp_readtime = millis();

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("M-Cure Demo Board Start");
  Serial.println("Use Command cli (N)");

  Serial.println("OLED Test Start");
  delay(50);

  dsp_1.begin();
  delay(50);

  dsp_1.fillScreen(RED);
  dsp_1.setCursor(5,0);
  dsp_1.print("temp display");
  
  dsp_1.setCursor(5,10);
  dsp_1.print("target : ");
  dsp_1.setCursor(60,10);
  dsp_1.print(hope_temp);
  
  dsp_1.setCursor(5,20);
  dsp_1.print("read temp : ");
  dsp_1.setCursor(80,20);
  dsp_1.print(real_temp);

  dsp_1.setCursor(5,30);
  dsp_1.print("FAN state : ");
  dsp_1.setCursor(80,30);
  dsp_1.print("OF");

  delay(50);
  
  dsp_2.begin();
  delay(50);
  dsp_2.fillScreen(GREEN);
  dsp_2.setCursor(0,0);
  dsp_2.setTextColor(WHITE);
  dsp_2.setTextSize(1);
  dsp_2.print("Disp_2 Work");
  delay(50);
  
  dsp_3.begin();
  delay(50);
  dsp_3.fillScreen(BLUE);
  dsp_3.setTextColor(WHITE);
  dsp_3.setTextSize(1);
  dsp_3.print("Disp_3 Work");
  delay(50);
  
  Serial.println("OLED Test end");
  delay(50);

}

void loop() 
{
  // put your main code here, to run repeatedly:
  Key_Scan_A();
  Key_Proc_A();

  Key_Scan_B();
  Key_Proc_B();

  updateTemperatrue();
}


void Key_Read_A(void)
{
  int data1 = analogRead(A1);
  
  if( data1 > 937 ){ KeyA = 0xFF; }
  else if(data1 > 819){ KeyA = TEMP_UP; }
  else if(data1 > 694){ KeyA = TEMP_DN; }
  else if(data1 > 558){ KeyA = MOTOR_CW; }
  else if(data1 > 388){ KeyA = MOTOR_SEL; }
  else if(data1 > 146) { KeyA = MOTOR_CCW; }
  else { KeyA = 0xFF; }
}

void Key_Scan_A(void)   //10ms
{
  static unsigned char AutoKeyCountA = 0;
  static unsigned char f_PressedKeyA = 0;
  static unsigned char PrevKeyA = 0xFF;

  if( millis() - pre_key_A_readtime < 100) return;
  pre_key_A_readtime = millis();

  Key_Read_A();             // update Key value
    
  if(KeyA!=0xFF && PrevKeyA==KeyA) 
  {
    if(f_PressedKeyA == 0) 
    {   // First Detect - Pressed Key
      is_key_A_change = 1;
      f_PressedKeyA = 1;
        AutoKeyCountA = 15;    // about 0.5s
      }
    else 
    {         // Hold - Pressed Key
      if(--AutoKeyCountA==0) 
      {
        if( KeyA == TEMP_UP || KeyA == TEMP_DN || KeyA == MOTOR_CW || KeyA == MOTOR_CCW  ) 
        {
          is_key_A_change = 1;
          AutoKeyCountA = 3; // about 0.15s
        }
      }
    }
  }
  else  
  {
      f_PressedKeyA = 0;
  }
  PrevKeyA = KeyA;
}

void Key_Proc_A(void)
{ 
  static bool is_update_up = false;
  static bool is_update_dn = false;
  if(is_key_A_change == false) return;
  is_key_A_change = false;
  switch(KeyA)
  {
    case TEMP_UP  : Serial.println("TEMP_UP");
      hope_temp += 1;
      is_update_dn = false;
      if( is_update_up == false)
      {
        is_update_up = true;
        dsp_1.fillRect(5-2,0,80,10, RED);
        dsp_1.setCursor(5,0);
        dsp_1.print("temp up !");
      }

      dsp_1.fillRect(60-2,10,20,10, RED);
      dsp_1.setCursor(5,10);
      dsp_1.print("target : ");
      dsp_1.setCursor(60,10);
      dsp_1.print(hope_temp);
  
    break;
    
    case TEMP_DN  : Serial.println("TEMP_Dn");
      hope_temp -= 1;
      is_update_up = false;
      if(is_update_dn == false)
      {
        is_update_dn = true;
        dsp_1.fillRect(5-2,0,80,10, RED);
        dsp_1.setCursor(5,0);
        dsp_1.print("temp down !");
       }
      
      dsp_1.fillRect(60-2,10,20,10, RED);
      dsp_1.setCursor(5,10);
      dsp_1.print("target : ");
      dsp_1.setCursor(60,10);
      dsp_1.print(hope_temp);
  
    break;
    
    case MOTOR_CW : Serial.println("MOTOR_CW");
      dsp_2.fillScreen(GREEN);
      dsp_2.setCursor(0,0);
      dsp_2.setTextColor(WHITE);
      dsp_2.setTextSize(1);
      dsp_2.print("motor cw");
    break;
    
    case MOTOR_CCW: Serial.println("MOTOR_CCW");
      dsp_2.fillScreen(GREEN);
      dsp_2.setCursor(0,0);
      dsp_2.setTextColor(WHITE);
      dsp_2.setTextSize(1);
      dsp_2.print("motor ccw");
    break;
    
    case MOTOR_SEL: Serial.println("MOTOR_SEL");
      dsp_2.fillScreen(GREEN);
      dsp_2.setCursor(0,0);
      dsp_2.setTextColor(WHITE);
      dsp_2.setTextSize(1);
      dsp_2.print("motor sel");
    break;

    default : break;
  }
}


void Key_Read_B(void)
{
  int data2 = analogRead(A2);

  if( data2 > 937 ){ KeyB = 0xFF; }
  else if(data2 > 819){ KeyB = PSI_UP; }
  else if(data2 > 694){ KeyB = PSI_DN; }
  else if(data2 > 558){ KeyB = PSI_SEL; }
  else if(data2 > 388){ KeyB = PSI_WORK; }
  else if(data2 > 146) { KeyB = MOTOR_WORK; }
  else { KeyB = 0xFF; }
}

void Key_Scan_B(void)   //10ms
{
  static unsigned char AutoKeyCountB = 0;
  static unsigned char f_PressedKeyB = 0;
  static unsigned char PrevKeyB = 0xFF;
  
  if( millis() - pre_key_B_readtime < 100) return;
  pre_key_B_readtime = millis();

  Key_Read_B();             // update Key value
    
  if(KeyB!=0xFF && PrevKeyB==KeyB) 
  {
    if(f_PressedKeyB == 0) 
    {   // First Detect - Pressed Key
      is_key_B_change = 1;
      f_PressedKeyB = 1;
        AutoKeyCountB = 15;    // about 0.5s
      }
    else 
    {         // Hold - Pressed Key
      if(--AutoKeyCountB==0) 
      {
        if( KeyB == PSI_UP || KeyB == PSI_DN  ) 
        {
          is_key_B_change = 1;
          AutoKeyCountB = 3; // about 0.15s
        }
      }
    }
  }
  else  
  {
      f_PressedKeyB = 0;
  }
  PrevKeyB = KeyB;
}

void Key_Proc_B(void)
{ 
  if(is_key_B_change == false) return;
  is_key_B_change = false;

  switch(KeyB)
  {
    case MOTOR_WORK  : Serial.println("MOTOR_WORK");
      dsp_2.fillScreen(GREEN);
      dsp_2.setCursor(0,0);
      dsp_2.setTextColor(WHITE);
      dsp_2.setTextSize(1);
      dsp_2.print("motor work");
    break;
    
    case PSI_UP  : Serial.println("PSI_UP");
      dsp_3.fillScreen(BLUE);
      dsp_3.setCursor(0,0);
      dsp_3.setTextColor(WHITE);
      dsp_3.setTextSize(1);
      dsp_3.print("psi up");
    break;
    
    case PSI_DN : Serial.println("PSI_DN");
      dsp_3.fillScreen(BLUE);
      dsp_3.setCursor(0,0);
      dsp_3.setTextColor(WHITE);
      dsp_3.setTextSize(1);
      dsp_3.print("psi dn");
    break;
    
    case PSI_SEL: Serial.println("PSI_SEL");
      dsp_3.fillScreen(BLUE);
      dsp_3.setCursor(0,0);
      dsp_3.setTextColor(WHITE);
      dsp_3.setTextSize(1);
      dsp_3.print("psi sel");
    break;
    
    case PSI_WORK: Serial.println("PSI_WORK");
      dsp_3.fillScreen(BLUE);
      dsp_3.setCursor(0,0);
      dsp_3.setTextColor(WHITE);
      dsp_3.setTextSize(1);
      dsp_3.print("psi work");
    break;
    
    default : break;
  }
}

void updateTemperatrue (void)
{
  if( millis() - pre_temp_readtime < 200) return;
  pre_temp_readtime = millis();
  
  // update need atleast 200ms
  real_temp = thermocouple.readCelsius();

  dsp_1.fillRect(78,18,14,14, RED);
  dsp_1.setCursor(80,20);
  dsp_1.print(real_temp);

  dsp_1.fillRect(78,28,14,14, RED);
  dsp_1.setCursor(80,30);
  if( real_temp <= hope_temp+2 && real_temp >= hope_temp-2)
  {
    digitalWrite(PTR_PORT_A, LOW);
    digitalWrite(PTR_PORT_B, LOW);
    digitalWrite(PTR_PORT_FAN, LOW);
    dsp_1.print("OF");
  }
  else if( real_temp > hope_temp) // Need Cooling
  {
    digitalWrite(PTR_PORT_A, LOW);
    digitalWrite(PTR_PORT_B, HIGH);
    digitalWrite(PTR_PORT_FAN, HIGH);
    dsp_1.print("ON");
  }
  else if( real_temp < hope_temp)
  {
    digitalWrite(PTR_PORT_A, HIGH);
    digitalWrite(PTR_PORT_B, LOW);
    digitalWrite(PTR_PORT_FAN, HIGH);
    dsp_1.print("ON");
  }

}
