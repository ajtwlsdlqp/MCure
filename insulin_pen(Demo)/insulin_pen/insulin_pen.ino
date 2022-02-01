#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1331.h"
#include "max6675.h"
#include "EEPROM.h"

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

signed int real_temp = 20, hope_temp = 0;
MAX6675 thermocouple(TEMP_CLK, TEMP_CS, TEMP_DO);
unsigned long pre_temp_readtime = millis();

signed int real_psi = 0, hope_psi = 0;
unsigned long pre_psi_readtime = millis();
bool is_pump_working = false;
bool is_pump_emergency = false;

unsigned long pre_eeprom_time = millis();
bool is_update_infor;

unsigned long pre_encodercheck_time = millis();
bool is_encoder_working = false;
signed int cw_pos = 0, ccw_pos = 0;
signed int pulses = 0;
bool is_select_opsit = false;

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("M-Cure Demo Board Start");
  Serial.println("Use Command cli (N)");

  Serial.println("OLED Test Start");
  delay(5);

  readEEPROM();

  dsp_1.begin();
  delay(5);

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

  delay(5);
  
  dsp_2.begin();
  delay(5);
  dsp_2.fillScreen(GREEN);
  dsp_2.setCursor(5,0);
  dsp_2.print("motor display");

  dsp_2.setCursor(5,10);
  dsp_2.print(" cw pos :");
  dsp_2.setCursor(60,10);
  dsp_2.print(cw_pos);
  
  dsp_2.setCursor(5,20);
  dsp_2.print("ccw pos : ");
  dsp_2.setCursor(60,20);
  dsp_2.print(ccw_pos);
  
  dsp_2.setCursor(5,30);
  dsp_2.print("state : ");
  dsp_2.setCursor(50, 30);
  dsp_2.print("up");
  
  delay(5);
  
  dsp_3.begin();
  delay(5);
  dsp_3.fillScreen(BLUE);
  dsp_3.setCursor(5,0);
  dsp_3.print("PSI display");
    
  dsp_3.setCursor(5,10);
  dsp_3.print("target : ");
  dsp_3.setCursor(60,10);
  dsp_3.print(hope_psi);
  
  dsp_3.setCursor(5,20);
  dsp_3.print("read psi : ");
  dsp_3.setCursor(80,20);
  dsp_3.print(real_psi);

  dsp_3.fillRect(0,28,10,10, BLUE);
  delay(5);
  
  Serial.println("OLED Test end");
  delay(5);

  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);
  attachInterrupt(0, A_CHANGE, CHANGE);
}

void loop() 
{
  // put your main code here, to run repeatedly:
  Key_Scan_A();
  Key_Proc_A();

  Key_Scan_B();
  Key_Proc_B();

  updateTemperatrue();
  updatePSI();
  updateMotorStopPos();
  
  updateEEPROM();
}

void A_CHANGE() 
{                                     //Interrupt function to read the x2 pulses of the encoder.
  if ( digitalRead(ENCODER_B) == 0 ) 
  {
    if ( digitalRead(ENCODER_A) == 0 )
    {
      pulses--; // Moving forward
    }
    else
    {
      pulses++; // Moving reverse
    }
  } 
  else 
  {
    if ( digitalRead(ENCODER_A) == 0 )
    {
      pulses++; // Moving reverse
    }
    else
    {
      pulses--; // Moving forward
    }
  }
  pre_encodercheck_time = millis();
  is_encoder_working = true;
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

  if( millis() - pre_key_A_readtime < 50) return;
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
  static unsigned char update_title1 = 0x00;
  static unsigned char update_title2 = 0x00;
  
  if(is_key_A_change == false) return;
  is_key_A_change = false;
  switch(KeyA)
  {
    case TEMP_UP  : Serial.println("TEMP_UP");
      hope_temp += 1;
      
      if( update_title1 != 1)
      {
        update_title1 = 1;
        dsp_1.fillRect(5-2,0,80,10, RED);
        dsp_1.setCursor(5,0);
        dsp_1.print("temp up !");
      }

      dsp_1.fillRect(60-2,10,20,10, RED);
      dsp_1.setCursor(5,10);
      dsp_1.print("target : ");
      dsp_1.setCursor(60,10);
      dsp_1.print(hope_temp);

      is_update_infor = true;
      pre_eeprom_time = millis();
    break;
    
    case TEMP_DN  : Serial.println("TEMP_Dn");
      hope_temp -= 1;

      if(update_title1 != 2)
      {
        update_title1 = 2;
        dsp_1.fillRect(5-2,0,80,10, RED);
        dsp_1.setCursor(5,0);
        dsp_1.print("temp down !");
       }
      
      dsp_1.fillRect(60-2,10,20,10, RED);
      dsp_1.setCursor(5,10);
      dsp_1.print("target : ");
      dsp_1.setCursor(60,10);
      dsp_1.print(hope_temp);
      
      is_update_infor = true;
      pre_eeprom_time = millis();
    break;
    // signed int cw_pos = 0; ccw_pos = 0;
    case MOTOR_CW : Serial.println("MOTOR_CW");
      if( is_select_opsit == false) cw_pos += 10;
      else cw_pos -= 10;

      if(update_title2 != 1)
      {
        update_title2 = 1;
        dsp_2.fillRect(5-2,0,100,10, GREEN);
        dsp_2.setCursor(5,0);
        dsp_2.print("cw pos control");

        digitalWrite(MOTOR_PORT_A, HIGH);
        digitalWrite(MOTOR_PORT_B, LOW);
       }
      
      dsp_2.fillRect(60-2,10,80,10, GREEN);
      dsp_2.setCursor(60,10);
      dsp_2.print(cw_pos);
    break;
    
    case MOTOR_CCW: Serial.println("MOTOR_CCW");
      if( is_select_opsit == false) ccw_pos += 10;
      else ccw_pos -= 10;
      
      if(update_title2 != 2)
      {
        update_title2 = 2;
        dsp_2.fillRect(5-2,0,100,10, GREEN);
        dsp_2.setCursor(5,0);
        dsp_2.print("ccw pos control");

        
        digitalWrite(MOTOR_PORT_A, LOW);
        digitalWrite(MOTOR_PORT_B, HIGH);
       }
      
      dsp_2.fillRect(60-2,20,80,10, GREEN);
      dsp_2.setCursor(60,20);
      dsp_2.print(ccw_pos);
    break;
    
    case MOTOR_SEL: Serial.println("MOTOR_SEL");
      is_select_opsit = !is_select_opsit;
      if(update_title2 != 3)
      {
        update_title2 = 3;
        dsp_2.fillRect(5-2,0,100,10, GREEN);
        dsp_2.setCursor(5,0);
        dsp_2.print("select control");

        digitalWrite(MOTOR_PORT_A, LOW);
        digitalWrite(MOTOR_PORT_B, LOW);
       }
    
      dsp_2.setCursor(5,30);
      dsp_2.print("state : ");
      dsp_2.setCursor(5,30);
      
      dsp_2.setCursor(50, 30);
      dsp_2.fillRect(50, 30, 40, 10, GREEN);
      if( is_select_opsit == false) dsp_2.print("up");
      else dsp_2.print("down");
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
  else if(data2 > 558){ KeyB = PSI_STOP; }
  else if(data2 > 388){ KeyB = PSI_WORK; }
  else if(data2 > 146) { KeyB = MOTOR_WORK; }
  else { KeyB = 0xFF; }
}

void Key_Scan_B(void)   //10ms
{
  static unsigned char AutoKeyCountB = 0;
  static unsigned char f_PressedKeyB = 0;
  static unsigned char PrevKeyB = 0xFF;
  
  if( millis() - pre_key_B_readtime < 50) return;
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
  static bool is_update_up = false;
  static bool is_update_dn = false;
  static bool is_update_stop = false;
  static bool is_update_work = false;
  
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
      hope_psi += 1;
      is_update_dn = false;
      is_update_stop = false;
      is_update_work = false;
      if( is_update_up == false)
      {
        is_update_up = true;
        dsp_3.fillRect(5-2,0,80,10, BLUE);
        dsp_3.setCursor(5,0);
        dsp_3.print("psi up !");
      }

      dsp_3.fillRect(60-2,10,30,10, BLUE);
      dsp_3.setCursor(5,10);
      dsp_3.print("target : ");
      dsp_3.setCursor(60,10);
      dsp_3.print(hope_psi);
      
      is_update_infor = true;
      pre_eeprom_time = millis();
    break;
    
    case PSI_DN : Serial.println("PSI_DN");
      hope_psi -= 1;
      is_update_up = false;
      is_update_stop = false;
      is_update_work = false;
      if( is_update_up == false)
      {
        is_update_up = true;
        dsp_3.fillRect(5-2,0,80,10, BLUE);
        dsp_3.setCursor(5,0);
        dsp_3.print("psi down !");
      }

      dsp_3.fillRect(60-2,10,30,10, BLUE);
      dsp_3.setCursor(5,10);
      dsp_3.print("target : ");
      dsp_3.setCursor(60,10);
      dsp_3.print(hope_psi);
      
      is_update_infor = true;
      pre_eeprom_time = millis();
    break;
    
    case PSI_STOP: Serial.println("PSI_STOP");
      is_update_up = false;
      is_update_up = false;
      is_update_work = false;
      if( is_update_stop == false)
      {
        is_update_stop = true;
        dsp_3.fillRect(5-2,0,80,10, BLUE);
        dsp_3.setCursor(5,0);
        dsp_3.print("! stop !");
      }
      is_pump_working = false;
      is_pump_emergency = true;
      digitalWrite(PUMP_PORT, LOW);
    break;
    
    case PSI_WORK: Serial.println("PSI_WORK");
      is_update_up = false;
      is_update_up = false;
      is_update_stop = false;
      if( is_update_work == false)
      {
        is_update_work = true;
        dsp_3.fillRect(5-2,0,80,10, BLUE);
        dsp_3.setCursor(5,0);
        dsp_3.print("puum on !");
      }
      is_pump_working = true;
      is_pump_emergency = false;
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
  if( real_temp <= hope_temp+1 && real_temp >= hope_temp-1)
  {
    digitalWrite(PTR_PORT_A, LOW);
    digitalWrite(PTR_PORT_B, LOW);
    digitalWrite(PTR_PORT_FAN, LOW);
    dsp_1.print("OF");
  }
  else if( real_temp > hope_temp+2) // Need Cooling
  {
    digitalWrite(PTR_PORT_A, LOW);
    digitalWrite(PTR_PORT_B, HIGH);
    digitalWrite(PTR_PORT_FAN, HIGH);
    dsp_1.print("ON");
  }
  else if( real_temp < hope_temp-2)
  {
    digitalWrite(PTR_PORT_A, HIGH);
    digitalWrite(PTR_PORT_B, LOW);
    digitalWrite(PTR_PORT_FAN, HIGH);
    dsp_1.print("ON");
  }

}

void updatePSI (void)
{
  if( millis() - pre_psi_readtime < 100) return;
  pre_psi_readtime = millis();

  real_psi = analogRead(A8);
  dsp_3.fillRect(68,18,40,14, BLUE);
  dsp_3.setCursor(70,20);
  dsp_3.print(real_psi);

  if( is_pump_working == true)
  {
    if(real_psi < hope_psi) digitalWrite(PUMP_PORT, HIGH);  // active pump
    else if( real_psi >= hope_psi) digitalWrite(PUMP_PORT, LOW);  // de-active pump
  }

  if( is_pump_emergency == true)
  {
    digitalWrite(SOLENOID, HIGH);
    if( real_psi < 110)
    {
      digitalWrite(SOLENOID, LOW);
      is_pump_emergency = false;
    }
  }
}

void updateMotorStopPos(void)
{
  if( is_encoder_working == false) return;

  if( millis() - pre_encodercheck_time > 50) // if encoder update is not working
  {
    is_encoder_working = false;
    digitalWrite(MOTOR_PORT_A, LOW);
    digitalWrite(MOTOR_PORT_B, LOW);
  }
}

void readEEPROM (void)
{
  int addr = 0x00;
  signed int data[6];
    
  for (int i = 0; i<6; i++)
  {
    data[i] = EEPROM.read(addr);
    Serial.print(addr);
    Serial.print(" : ");
    Serial.println(data[i]);
    addr += sizeof(signed int);
    delay(1);
  }
  real_temp = data[0];
  hope_temp = data[1];
  cw_pos = data[2];
  ccw_pos = (signed int)data[3];
  real_psi = data[4];
  hope_psi = data[5];
  
}

void updateEEPROM (void)
{
  if( is_update_infor == false) return;
  if( millis() - pre_eeprom_time < 1000) return;

  is_update_infor = false;

  for (int i = 0 ; i < 50; i++) 
  {
    EEPROM.write(i, 0);
    delay(1);
  }

  int addr = 0x00;
  signed int data[6];
  data[0] = real_temp;
  data[1] = hope_temp;
  data[2] = cw_pos;
  data[3] = ccw_pos;
  data[4] = real_psi;
  data[5] = hope_psi;
  
  for (int i = 0; i<6; i++)
  {
    EEPROM.write(addr, data[i]);
    addr += sizeof(signed int);
    delay(1);
  }
}
