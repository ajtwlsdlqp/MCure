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
unsigned long pre_motor_stop_time = millis();
bool is_encoder_working = false;
signed int pulses = 0;
unsigned int pulses_stop_pos = 16000;
unsigned char motor_stop_time = 1;
unsigned char motor_mode_num = 0xFF;


void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);

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
  dsp_2.print("now pos :");
  dsp_2.setCursor(60,10);
  dsp_2.print(pulses);
  
  dsp_2.setCursor(5,20);
  dsp_2.print("target : ");
  dsp_2.setCursor(60,20);
  dsp_2.print(pulses_stop_pos);
  
  dsp_2.setCursor(5,30);
  dsp_2.print("time  : ");
  dsp_2.setCursor(50, 30);
  dsp_2.print(motor_stop_time);
  dsp_2.setCursor(65, 30);
  dsp_2.print("S");
  
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

  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);
  attachInterrupt(0, A_CHANGE, CHANGE);

  // temp control
  digitalWrite(PTR_PORT_A, LOW);
  digitalWrite(PTR_PORT_B, LOW);
  digitalWrite(PTR_PORT_FAN, LOW);
  // Define PSI
  digitalWrite(PUMP_PORT, LOW);
  digitalWrite(SOLENOID, LOW);
  // Define ENCODER Motor
  digitalWrite(MOTOR_PORT_A, LOW);
  digitalWrite(MOTOR_PORT_B, LOW);
  
  pulses = 0;
  motor_mode_num = 0xFF;
  digitalWrite(MOTOR_PORT_A, LOW);
  digitalWrite(MOTOR_PORT_B, HIGH);    
  pre_encodercheck_time = millis();
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
  
  updateMotor();
  updateMotorStopPos();
  
  updateEEPROM();

  dsp_2.fillRect(60-2,10,80,10, GREEN);
  dsp_2.setCursor(60,10);
  dsp_2.print(pulses);
}

void A_CHANGE() 
{                                     //Interrupt function to read the x2 pulses of the encoder.
  if ( digitalRead(ENCODER_B) == 0 ) 
  {
    if ( digitalRead(ENCODER_A) == 0 )
    {
      pulses++; // Moving forward
    }
    else
    {
      pulses--; // Moving reverse
    }
  } 
  else 
  {
    if ( digitalRead(ENCODER_A) == 0 )
    {
      pulses--; // Moving reverse
    }
    else
    {
      pulses++; // Moving forward
    }
  }
  pre_encodercheck_time = millis();
}


void Key_Read_A(void)
{
  int data1 = analogRead(A1);
  
  if( data1 > 937 ){ KeyA = 0xFF; }
  else if(data1 > 819){ KeyA = TEMP_UP; }
  else if(data1 > 694){ KeyA = TEMP_DN; }
  else if(data1 > 558){ KeyA = MOTOR_MANUAL; }
  else if(data1 > 388){ KeyA = MOTOR_TIME; }
  else if(data1 > 146) { KeyA = MOTOR_SAVE; }
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
        if( KeyA == TEMP_UP || KeyA == TEMP_DN ) 
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
  
  if(is_key_A_change == false) return;
  is_key_A_change = false;
  switch(KeyA)
  {
    case TEMP_UP  : //Serial.println("TEMP_UP");
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
    
    case TEMP_DN  : //Serial.println("TEMP_Dn");
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
    
    case MOTOR_MANUAL : //Serial.println("MOTOR_MANUAL"); // manual control motor stop pos
      if(motor_mode_num >= 7) break;  // if motor working 1 cycle mode not enterence
      if(motor_mode_num == 0xFF) break; // if motor goto start pos break;
      is_encoder_working = false;
      if(motor_mode_num  == 1)
      {
        motor_mode_num = 2;
        dsp_2.fillRect(5-2,0,100,10, GREEN);
        dsp_2.setCursor(5,0);
        dsp_2.print("manual move");

        digitalWrite(MOTOR_PORT_A, HIGH);
        digitalWrite(MOTOR_PORT_B, LOW);
       }
       else if( motor_mode_num == 2)
       {
        motor_mode_num = 3;
        dsp_2.fillRect(5-2,0,100,10, GREEN);
        dsp_2.setCursor(5,0);
        dsp_2.print("manual stop");

        digitalWrite(MOTOR_PORT_A, LOW);
        digitalWrite(MOTOR_PORT_B, LOW);
       }
       else if( motor_mode_num == 3)
       {
        motor_mode_num = 4;
        dsp_2.fillRect(5-2,0,100,10, GREEN);
        dsp_2.setCursor(5,0);
        dsp_2.print("manual move");

        digitalWrite(MOTOR_PORT_A, LOW);
        digitalWrite(MOTOR_PORT_B, HIGH);
       }
       else
       {
        motor_mode_num = 1;
        dsp_2.fillRect(5-2,0,100,10, GREEN);
        dsp_2.setCursor(5,0);
        dsp_2.print("manual stop");

        digitalWrite(MOTOR_PORT_A, LOW);
        digitalWrite(MOTOR_PORT_B, LOW);
       }

        dsp_2.fillRect(60-2,20,50,10, GREEN);
        dsp_2.setCursor(60,20);
        dsp_2.print(pulses);
       
    break;
    
    case MOTOR_SAVE: //Serial.println("MOTOR_SAVE"); // set motor stop pos
      if(motor_mode_num  >= 7) break;
      if(motor_mode_num == 0xFF) break; // if motor goto start pos break;
      
      is_encoder_working = false;
      if(motor_mode_num  != 5)
      {
        motor_mode_num  = 5;
        dsp_2.fillRect(5-2,0,100,10, GREEN);
        dsp_2.setCursor(5,0);
        dsp_2.print("stop pos save");

        pulses_stop_pos = pulses;
        is_update_infor = true;
        pre_eeprom_time = millis();

        motor_mode_num = 0xFF;
        digitalWrite(MOTOR_PORT_A, LOW);
        digitalWrite(MOTOR_PORT_B, HIGH);    
        pre_encodercheck_time = millis();
      }
    break;
    
    case MOTOR_TIME: //Serial.println("MOTOR_WORK");  // setting motor stop time
      if(motor_mode_num >= 7) break;
      if(motor_mode_num == 0xFF) break; // if motor goto start pos break;
      
      is_encoder_working = false;
      motor_mode_num = 6;
      dsp_2.fillRect(5-2,0,100,10, GREEN);
      dsp_2.setCursor(5,0);
      dsp_2.print("select time");

      if( motor_stop_time > 10) motor_stop_time  = 1;
      else motor_stop_time += 1;

      dsp_2.fillRect(50-2,30,16,10, GREEN);
      dsp_2.setCursor(50, 30);
      dsp_2.print(motor_stop_time);

      is_update_infor = true;
      pre_eeprom_time = millis();
    break;

    default : break;
  }
}


void Key_Read_B(void)
{
  int data2 = analogRead(A2);

  if( data2 > 937 ){ KeyB = 0xFF; }
  else if(data2 > 819){ KeyB = PSI_DN; }
  else if(data2 > 694){ KeyB = PSI_UP; }
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
  static unsigned char update_num = 0;
  
  if(is_key_B_change == false) return;
  is_key_B_change = false;

  switch(KeyB)
  {
    case MOTOR_WORK  : //Serial.println("MOTOR_WORK");  // motor work 1 cycle
      if(motor_mode_num != 7)
      {
        motor_mode_num = 7;
        dsp_2.fillRect(5-2,0,100,10, GREEN);
        dsp_2.setCursor(5,0);
        dsp_2.print("work 1 cycle");

        dsp_2.fillRect(60-2,20,100,10, GREEN);
        dsp_2.setCursor(60,20);
        dsp_2.print(pulses_stop_pos);

        pulses = 0;
        
        digitalWrite(MOTOR_PORT_A, HIGH);
        digitalWrite(MOTOR_PORT_B, LOW);
        pre_encodercheck_time = millis();
        is_encoder_working = true;
       }
    break;
    
    case PSI_UP  : //Serial.println("PSI_UP");
      hope_psi += 1;
      if( update_num != 1)
      {
        update_num = 1;
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
    
    case PSI_DN : //Serial.println("PSI_DN");
      hope_psi -= 1;
      if( update_num != 2)
      {
        update_num = 2;
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
    
    case PSI_STOP: //Serial.println("PSI_STOP");
      if( update_num != 3)
      {
        update_num = 3;
        dsp_3.fillRect(5-2,0,80,10, BLUE);
        dsp_3.setCursor(5,0);
        dsp_3.print("! stop !");
      }
      is_pump_working = false;
      is_pump_emergency = true;
      digitalWrite(PUMP_PORT, LOW);
    break;
    
    case PSI_WORK: //Serial.println("PSI_WORK");
      if( update_num != 4)
      {
        update_num = 4;
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
    if( real_psi < 100)
    {
      digitalWrite(SOLENOID, LOW);
      is_pump_emergency = false;
    }
  }
}

void updateMotor(void)
{
  static unsigned long pre_hold_time = millis();
  if( is_encoder_working == false) return;

  switch(motor_mode_num)
  {
    default :
      motor_mode_num = 0; // set as default
      is_encoder_working = false;
      break;

    case 7 :
      if( pulses >= pulses_stop_pos)  // default 16,000
      {
        digitalWrite(MOTOR_PORT_A, LOW);
        digitalWrite(MOTOR_PORT_B, LOW);
        motor_mode_num = 8;
        pre_hold_time = millis();
      }
      break;

    case 8 :
      if( millis() - pre_hold_time > motor_stop_time * 1000)
      {
        motor_mode_num = 9;
        digitalWrite(MOTOR_PORT_A, LOW);
        digitalWrite(MOTOR_PORT_B, HIGH);
      }
      break;
      
    case 9 :
      if( millis() - pre_encodercheck_time > 50) // if encoder update is not working
      {
        is_encoder_working = false;
        digitalWrite(MOTOR_PORT_A, LOW);
        digitalWrite(MOTOR_PORT_B, LOW);
        motor_mode_num = 0;
      }
      break;
  }

  dsp_2.fillRect(60,10,50,10, GREEN);
  dsp_2.setCursor(60,10);
  dsp_2.print(pulses);
}

void updateMotorStopPos(void)
{
  if( motor_mode_num != 0xFF) return;
    
  if( millis() - pre_encodercheck_time > 50) // if encoder update is not working
  {
    motor_mode_num = 0;
    digitalWrite(MOTOR_PORT_A, LOW);
    digitalWrite(MOTOR_PORT_B, LOW);

    pulses = 0;
    is_encoder_working = false;
  }
}

void readEEPROM (void)
{
  int addr = 0x00;
  signed int data[6];
    
  for (int i = 0; i<6; i++)
  {
    data[i] = EEPROM.read(addr);
    addr += sizeof(signed int);
    delay(1);
  }
  pulses_stop_pos = EEPROM.read(addr);
  pulses_stop_pos = pulses_stop_pos << 8;
  addr += sizeof(signed int);
  unsigned char temp = EEPROM.read(addr);
  pulses_stop_pos = pulses_stop_pos | temp;

  real_temp = data[0];
  hope_temp = data[1];
  // pulses_stop_pos = data[2];
  motor_stop_time = data[3];
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
  data[2] = pulses_stop_pos;
  data[3] = motor_stop_time;
  data[4] = real_psi;
  data[5] = hope_psi;
  
  for (int i = 0; i<6; i++)
  {
    EEPROM.write(addr, data[i]);
    addr += sizeof(signed int);
    delay(1);
  }
  EEPROM.write(addr, (pulses_stop_pos >> 8) );
  addr += sizeof(signed int);
  EEPROM.write(addr, (pulses_stop_pos & 0x00FF) );
}
