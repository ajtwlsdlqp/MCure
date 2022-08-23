#include "myDef.h"
#include "myFuncDef.h"

#include <Wire.h>

int Key = 0xFF;
unsigned long pre_key_readtime = millis();
bool is_key_change = false;

bool is_motor_long_key = false;

signed int real_temp = 20, hope_temp = 30;
unsigned long pre_temp_readtime = millis();

signed int real_psi = 0, hope_psi = 0;
unsigned long pre_psi_readtime = millis();
unsigned long pre_valve_close_time = millis();
bool is_pump_working = false;
bool is_pump_emergency = false;

unsigned long pre_eeprom_time = millis();
bool is_update_infor;

unsigned long pre_encodercheck_time = millis();
unsigned long pre_motor_stop_time = millis();
bool is_encoder_working = false;
signed int pulses = 0;
unsigned int pulses_stop_pos = 0;
unsigned char motor_stop_time = 1;
unsigned char motor_mode_num = 0;

unsigned char ch = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);   // for Debug
  Serial1.begin(9600);    // for ble
  Wire.begin();
  
  // motor
  pinMode(MOTOR_PORT_F, OUTPUT);
  digitalWrite(MOTOR_PORT_F, LOW);

  pinMode(MOTOR_PORT_R, OUTPUT);
  digitalWrite(MOTOR_PORT_R, LOW);
  
  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);
  attachInterrupt(0, A_CHANGE, CHANGE);

  // temp control
  pinMode(PELTIER_PORT, OUTPUT);
  digitalWrite(PELTIER_PORT, LOW);

  pinMode(PELTIER_FAN, OUTPUT);
  digitalWrite(PELTIER_FAN, LOW);

  // psi
  pinMode(SOLENOID_PORT, OUTPUT);
  digitalWrite(SOLENOID_PORT, LOW);

  pinMode(PELTIER_FAN, OUTPUT);
  digitalWrite(PELTIER_FAN, LOW);

  // touch
  for( ch = 0; ch < 4; ch++)  // set sensitivity
  {
    Wire.beginTransmission(0x48);
    Wire.write(0x39+ch);
    Wire.write(0x38);
    Wire.endTransmission();

    delay(250);
  }

  // enable touch ch8 7 6 5 // 4 3 2 1
  Wire.beginTransmission(0x48);
  Wire.write(0x01);
  Wire.write(0x0F);
  Wire.endTransmission();

  pinMode(BUZZER_PWM, OUTPUT);
  noTone(BUZZER_PWM);

  pinMode(BUZZER_POWER, OUTPUT);
  digitalWrite(BUZZER_POWER, LOW);

  pinMode(LED_EMERGENCY, OUTPUT);
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_MOTOR, OUTPUT);
  pinMode(LED_BLUETOOTH, OUTPUT);
  pinMode(LED_BAT_ICO, OUTPUT);
  pinMode(LED_BAT_STATE1, OUTPUT);
  pinMode(LED_BAT_STATE2, OUTPUT);
  pinMode(LED_BAT_STATE3, OUTPUT);
  pinMode(LED_MOTOR_ICO, OUTPUT);
  pinMode(LED_MOTOR_STATE1, OUTPUT);
  pinMode(LED_MOTOR_STATE2, OUTPUT);
  pinMode(LED_MOTOR_STATE3, OUTPUT);
  pinMode(LED_MOTOR_STATE4, OUTPUT);
  // pinMode(LED_TEMP_DANGER, OUTPUT);
  // pinMode(LED_TEMP_NORMAL, OUTPUT);
  
  pulses = 0;
  motor_mode_num = 0;
  pre_encodercheck_time = millis();
}

void loop() 
{
  // put your main code here, to run repeatedly:

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


void Key_Read(void)
{
  unsigned char temp;
  static unsigned char pre_key = 0xFF;
  
  Wire.beginTransmission(0x48);
  Wire.write(0x2A);
  Wire.endTransmission();

  Wire.beginTransmission(0x48+1);
  temp = Wire.read();
  Wire.endTransmission();
  
  if(pre_key == 0xFF)
  {
    if(temp == 0){  Key = 0xFF;   }
    else if(temp & 0x01){ Key = EMERGENCY_STOP;   }
    else if(temp & 0x02){ Key = POWER_KEY;   }
    else if(temp & 0x04){ Key = MOTOR_WORK;   }
    else if(temp & 0x08){ Key = BLUETOOTH;   }
    else{ Key = 0xFF;   }
    
    pre_key = Key;// pre_key
  }
  else
  {
    if(temp == 0){  Key = 0xFF;   }
    else if(temp == 0x01){  Key = EMERGENCY_STOP;   }
    else if(temp == 0x02){  Key = POWER_KEY;   }
    else if(temp == 0x04){  Key = MOTOR_WORK;   }
    else if(temp == 0x08){  Key = BLUETOOTH;   }
    else
    { 
      Key = pre_key;
    }   
    
    pre_key = Key;// pre_key
  }
}

void Key_Scan(void)
{
  static unsigned char AutoKeyCount = 0;
  static unsigned char f_PressedKey = 0;
  static unsigned char PrevKey = 0xFF;
  
  if( millis() - pre_key_readtime < 100) return;
  pre_key_readtime = millis();
      
  Key_Read();             // update Key value
    
  if(Key!=0xFF && PrevKey==Key) 
  {
    if(f_PressedKey == 0) 
    {   
      is_key_change = 1;
      f_PressedKey = 1;
      AutoKeyCount = 15;
    }
    else 
    {         // Hold - Pressed Key
      if(--AutoKeyCount==0) 
      {

      }
    }
  }
  else
  {
    f_PressedKey = 0;
    is_motor_long_key = false;  
  }
  
  PrevKey = Key;
  
  Key_Proc();
}

void Key_Proc(void)
{
  static unsigned long pre_motor_up_push = millis();
  static unsigned long pre_motor_dn_push = millis();
  static bool is_motor_up_push = false;
  static bool is_motor_dn_push = false;
  
  if(is_key_change == false) return;
  is_key_change = false;

  switch(Key)
  {
    case EMERGENCY_STOP :

    break;

    case POWER_KEY :

    break;

    case MOTOR_WORK :

    break;

    case BLUETOOTH :

    break;

    default : break;
  }
  
}

void updateTemperatrue (void)
{
  if( millis() - pre_temp_readtime < 200) return;
  pre_temp_readtime = millis();
  
  // update need atleast 200ms
  real_temp = analogRead(READ_TEMP);

  if( real_temp >= 30 ) 
  {
    digitalWrite(PELTIER_FAN, HIGH);
    digitalWrite(PELTIER_PORT, HIGH);
  }
  else
  {
    digitalWrite(PELTIER_FAN, LOW);
    digitalWrite(PELTIER_PORT, LOW);
  }
}

void updatePSI (void)
{
  double temp_mmhg;
  if( millis() - pre_psi_readtime < 100) return;
  pre_psi_readtime = millis();

  real_psi = analogRead(READ_PSI);

  // Sensor Read Range 0.5 ~ 4.5
  if( real_psi < 102)
  {
    temp_mmhg = 0;  // un clear data
  }
  else
  {
    temp_mmhg = ((double)real_psi * 5) / 1024;
    temp_mmhg = temp_mmhg - 0.5;
    temp_mmhg = temp_mmhg * 3.75;
    temp_mmhg = temp_mmhg * 51.71;
  }

  if( is_pump_working == true)
  {
    if(real_psi < hope_psi) digitalWrite(AIRPUMP_PORT, HIGH);  // active pump
    else if( real_psi > hope_psi) digitalWrite(AIRPUMP_PORT, LOW);  // de-active pump
  }

  if( is_pump_emergency == true)
  {
    if( millis() - pre_valve_close_time > 2 * 1000)
    {
        is_pump_emergency = false;
        digitalWrite(SOLENOID_PORT, LOW);  // block solenoide
    }
  }
}

void updateMotor(void)
{
  static unsigned long pre_hold_time = millis();
  if( is_encoder_working == false) return;

  switch(motor_mode_num)
  {
    default : break;

    case 7 :
      if( pulses >= pulses_stop_pos)  // default 16,000
      {
        digitalWrite(MOTOR_PORT_F, LOW);
        digitalWrite(MOTOR_PORT_R, LOW);
        motor_mode_num += 1;
        
        is_encoder_working = false;
        pre_hold_time = millis();
      }
      else if( millis() - pre_encodercheck_time > 50)  // default 16,000
      {
        digitalWrite(MOTOR_PORT_F, LOW);
        digitalWrite(MOTOR_PORT_R, LOW);
        motor_mode_num = 14;

        is_encoder_working = false;
        pre_hold_time = millis();
      }
      break;

    case 15:
      digitalWrite(MOTOR_PORT_F, LOW);
      digitalWrite(MOTOR_PORT_R, HIGH);
      motor_mode_num += 1;

      pre_encodercheck_time = millis();
      break;

    case 16:
    if( millis() - pre_encodercheck_time > 50)  // default 16,000
      {
        digitalWrite(MOTOR_PORT_F, LOW);
        digitalWrite(MOTOR_PORT_R, LOW);
        motor_mode_num = 0;
        pulses = 0;
        pre_hold_time = millis();

        is_encoder_working = false;
      }
  }
}

void updateMotorStopPos(void) // only check when mcu start @at once
{
  if( motor_mode_num != 0xFF) return;
    
  if( millis() - pre_encodercheck_time > 50) // if encoder update is not working
  {
    motor_mode_num = 0;
    digitalWrite(MOTOR_PORT_F, LOW);
    digitalWrite(MOTOR_PORT_R, LOW);

    pulses = 0;
    is_encoder_working = false;
  }
}

void updateChargeEnable(void)
{
  static bool is_update_charge = false;
  
}
