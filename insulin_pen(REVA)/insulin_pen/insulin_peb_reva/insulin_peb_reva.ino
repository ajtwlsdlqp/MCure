#define DEBUG_MODE    0
#define MOKUP_MODE    1

#include "myDef.h"
#include "myFuncDef.h"
#include "EEPROM.h"
//#include "Melody.h"

#include <Wire.h>
#include <I2C.h>

int Key = 0xFF;

bool is_key_change = false;
bool is_update_infor;
bool is_target_psi_set;
bool f_peltier_state;

unsigned char f_power_state = 0;
unsigned char active_step;
unsigned char working_mode;
unsigned char flash_statae;
unsigned char Sound_Num;
unsigned char Sound_Update;

signed int pulses = 0;

unsigned long pre_key_readtime = millis();
unsigned long pre_temp_readtime = millis();
unsigned long pre_psi_readtime = millis();
unsigned long pre_valve_close_time = millis();
unsigned long pre_eeprom_time = millis();
unsigned long pre_encodercheck_time = millis();
unsigned long pre_motor_stop_time = millis();
unsigned long pre_led_flash_time = millis();
unsigned long pre_buzzer_tic = millis();
unsigned long pre_user_motor_worktime = millis();

unsigned long pre_touch_stabil_time = millis();
unsigned char is_touch_enable;

unsigned char mokup_motor_run_state;
unsigned char mokup_motor_stop_state;
// for debug
unsigned char ch;

void setup() {
  // put your setup code here, to run once:
#if DEBUG_MODE
  Serial.begin(115200);   // for Debug
#else
  Serial.end();
#endif
  Serial1.begin(115200);    // for ble

  I2c.begin();
  I2c.timeOut(2000);
  I2c.pullup(true);

#if DEBUG_MODE
  Serial.println("Debug Start");
#endif
  // motor
  pinMode(MOTOR_PORT_F, OUTPUT);
  digitalWrite(MOTOR_PORT_F, LOW);

  pinMode(MOTOR_PORT_R, OUTPUT);
  digitalWrite(MOTOR_PORT_R, LOW);

  pinMode(MOTOR_SLEEP, OUTPUT);
  digitalWrite(MOTOR_SLEEP, LOW);
  
  pinMode(ENCODER_A, INPUT_PULLUP);           
  pinMode(ENCODER_B, INPUT_PULLUP);
  attachInterrupt(0, A_CHANGE, CHANGE);

  // temp control
  pinMode(PELTIER_PORT, OUTPUT);
  digitalWrite(PELTIER_PORT, LOW);

  pinMode(PELTIER_FAN, OUTPUT);
  digitalWrite(PELTIER_FAN, LOW);

  // psi
  pinMode(SOLENOID_PORT, OUTPUT);
  digitalWrite(SOLENOID_PORT, LOW);

  pinMode(AIRPUMP_PORT, OUTPUT);
  digitalWrite(AIRPUMP_PORT, LOW);


  pinMode(BUZZER_PWM, OUTPUT);
  noTone(BUZZER_PWM);
  digitalWrite(BUZZER_PWM, LOW);
  
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

  digitalWrite(LED_EMERGENCY, HIGH);
  digitalWrite(LED_POWER, HIGH);
  digitalWrite(LED_MOTOR, HIGH);
  digitalWrite(LED_BLUETOOTH, HIGH);
  digitalWrite(LED_BAT_ICO, HIGH);
  digitalWrite(LED_BAT_STATE1, HIGH);
  digitalWrite(LED_BAT_STATE2, HIGH);
  digitalWrite(LED_BAT_STATE3, HIGH);
  digitalWrite(LED_MOTOR_ICO, HIGH);
  digitalWrite(LED_MOTOR_STATE1, HIGH);
  digitalWrite(LED_MOTOR_STATE2, HIGH);
  digitalWrite(LED_MOTOR_STATE3, HIGH);
  digitalWrite(LED_MOTOR_STATE4, HIGH);

#if !DEBUG_MODE
  pinMode(LED_TEMP_DANGER, OUTPUT);
  pinMode(LED_TEMP_NORMAL, OUTPUT);
  digitalWrite(LED_TEMP_DANGER, HIGH);
  digitalWrite(LED_TEMP_NORMAL, HIGH);
#endif
  
  pulses = 0;
  pre_encodercheck_time = millis();



#if DEBUG_MODE
  Serial.println("setup end");
#endif

  active_step = STEP_USER_INPUT;
  is_target_psi_set = false;

  readEEPROM();

#if MOKUP_MODE  // fit to mode manual
  working_mode = MODE_MANUAL;
#endif

  Sound_Update = 2;
  Sound_Num = 3;
  is_touch_enable = 0;

}

void loop() 
{
  // put your main code here, to run repeatedly:
  // active allways

  Key_Scan();
  updateTemperatrue();    
  updateLED();
  
  // active only power on
  updatePSI();
  updateMotor();
  Melody_Proc();
  
  // active when it need
  updateEEPROM();
}

void A_CHANGE() 
{                                     //Interrupt function to read the x2 pulses of the encoder.
  if ( digitalRead(ENCODER_B) == 0 ) 
  {
    if ( digitalRead(ENCODER_A) == 0 ) pulses++; // Moving forward
    else pulses--; // Moving reverse
  } 
  else 
  {
    if ( digitalRead(ENCODER_A) == 0 ) pulses--; // Moving reverse
    else pulses++; // Moving forward
  }
  pre_encodercheck_time = millis();
}


void Key_Read(void)
{
  unsigned char temp = 0;
  static unsigned char pre_key = 0xFF;
  unsigned char rx_buff[2] = {0, 0};

  temp = I2c.read(0x24, 0x2A, 2, rx_buff);
  
#if (DEBUG_MODE && 0)
  Serial.print("DATA");
  Serial.print("[0] :");
  Serial.print(rx_buff[0]);
  Serial.print(" [1] :");
  Serial.println(rx_buff[1]);
#endif

  if(pre_key == 0xFF)
  {
    if(rx_buff[0] == 0)
    {  
      Key = 0xFF;

      if( f_power_state == 1 && mokup_motor_run_state != 0)
      {
        mokup_motor_run_state = 0;
        digitalWrite(MOTOR_PORT_F, LOW);
        digitalWrite(MOTOR_PORT_R, LOW);
      }
    }
    else if(rx_buff[0] & 0x01){ Key = MOTOR_WORK;   }
    else if(rx_buff[0] & 0x02){ Key = POWER_KEY;   }
    else if(rx_buff[0] & 0x04){ Key = MANUAL_KEY;   }
    else if(rx_buff[0] & 0x08){ Key = BLUETOOTH;   }
    else{ Key = 0xFF;   }
    
    pre_key = Key;// pre_key
  }
  else
  {
    if(rx_buff[0] == 0){  Key = 0xFF;   }
    else if(rx_buff[0] == 0x01){  Key = MOTOR_WORK;   }
    else if(rx_buff[0] == 0x02){  Key = POWER_KEY;   }
    else if(rx_buff[0] == 0x04){  Key = MANUAL_KEY;   }
    else if(rx_buff[0] == 0x08){  Key = BLUETOOTH;   }
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
  
  if( millis() - pre_key_readtime < 50) return;
  pre_key_readtime = millis();
      
  Key_Read();             // update Key value
    
  if(Key!=0xFF && PrevKey==Key) 
  {
    if(f_PressedKey == 0) 
    {   
      is_key_change = 1;
      f_PressedKey = 1;
      AutoKeyCount = 5;
    }
    else 
    {         // Hold - Pressed Key
      if(--AutoKeyCount==0) 
      {
#if MOKUP_MODE
        if( Key == MANUAL_KEY || Key == BLUETOOTH  ) 
        {
          is_key_change = 1;
          AutoKeyCount = 1; // about 0.15s
        }
#endif
      }
    }
  }
  else
  {
    f_PressedKey = 0;
  }
  
  PrevKey = Key;
  
  Key_Proc();
}

void Key_Proc(void)
{
  if(is_key_change == false) return;
  is_key_change = false;

  switch(Key)
  {
    case MOTOR_WORK :
      if( f_power_state == 0) break;
      if( working_mode == MODE_AUTO) break;
      active_step = STEP_MAKE_PSI;
      Sound_Update = 2; Sound_Num = 3;
      pre_buzzer_tic = millis();
    break;

    case POWER_KEY :
      if( f_power_state == 0)
      {
        f_power_state = 1;
        digitalWrite(MOTOR_SLEEP, HIGH); // wake up motor drive
        active_step = STEP_USER_INPUT;
        is_target_psi_set = false;

        Sound_Update = 2; Sound_Num = 5;
        pre_buzzer_tic = millis();
      }
      else
      {
        if( active_step == STEP_USER_INPUT) // user off function
        {
          f_power_state = 0;

          digitalWrite(MOTOR_PORT_F, LOW);
          digitalWrite(MOTOR_PORT_R, LOW);
        
          digitalWrite(SOLENOID_PORT, LOW);
          digitalWrite(AIRPUMP_PORT, LOW);

          digitalWrite(MOTOR_SLEEP, LOW); // when power off -> enable sleep motor
        
          Sound_Update = 2; Sound_Num = 1;
          pre_buzzer_tic = millis();
        }
        else if(active_step != STEP_EMERGENCY_STOP)                                // emergency off function
        {
          digitalWrite(AIRPUMP_PORT, LOW);  // de-active pump
          digitalWrite(SOLENOID_PORT, HIGH);  // block solenoide
          pre_valve_close_time = millis();

          Sound_Update = 2; Sound_Num = 1;
          pre_buzzer_tic = millis();
          active_step = STEP_EMERGENCY_STOP;
        }
      }
    break;

    case MANUAL_KEY :
      if( f_power_state == 0) break;

      if( mokup_motor_run_state != 1)
      {
        mokup_motor_run_state = 1;
        pre_encodercheck_time = millis();
        pre_user_motor_worktime = millis();
        digitalWrite(MOTOR_PORT_F, HIGH);
        digitalWrite(MOTOR_PORT_R, LOW);
      }
      else if( millis() - pre_encodercheck_time > 100) // if encoder update is not working
      {
        digitalWrite(MOTOR_PORT_F, LOW);
        digitalWrite(MOTOR_PORT_R, LOW);
        pulses = 0;
      }
      else
      {
        pre_user_motor_worktime = millis();
      }

    break;

    case BLUETOOTH :
      if( f_power_state == 0) break;
      if( mokup_motor_run_state != 2)
      {
        mokup_motor_run_state = 2;
        pre_encodercheck_time = millis();

        digitalWrite(MOTOR_PORT_F, LOW);
        digitalWrite(MOTOR_PORT_R, HIGH);
        pre_user_motor_worktime = millis();
      }
      else if( millis() - pre_encodercheck_time > 100) // if encoder update is not working
      {
        digitalWrite(MOTOR_PORT_F, LOW);
        digitalWrite(MOTOR_PORT_R, LOW);
        pulses = 0;
      }
      else
      {
        pre_user_motor_worktime = millis();
      }
    break;
    
    default : break;
  }
}

/*
 * temperature control function 
 * must operating every time
 * 10K NTC -> 25'c -> 10K
 * 29'c    -> 8.38690 KOhm ->
 * 27'c    -> 9.14743 KOhm
 * 
 * base    -> 3KOhm
 */
void updateTemperatrue (void)
{
  unsigned int real_temp = 1024;
  
  if( millis() - pre_temp_readtime < 200) return;
  pre_temp_readtime = millis();
  
  real_temp = analogRead(READ_TEMP);

  if( real_temp <= 754 ) // 29'c turn on 
  {
    f_peltier_state = true;
    digitalWrite(PELTIER_FAN, HIGH);
    digitalWrite(PELTIER_PORT, HIGH);
  }
  else if( real_temp >= 771) // 27'c turn off
  {
    f_peltier_state = false;
    digitalWrite(PELTIER_FAN, LOW);
    digitalWrite(PELTIER_PORT, LOW);
  }
}

/*
 * activate auto mode
 * activate manual mode via press button
 * target mmHg 110
 * ADC -> 219
 * 
 * temp_mmhg = ((double)real_psi * 5) / 1024;
   temp_mmhg = temp_mmhg - 0.5;
   temp_mmhg = temp_mmhg * 3.75;
   temp_mmhg = temp_mmhg * 51.71;
 */
void updatePSI (void)
{
  if( f_power_state == 0) return;
  
  if( millis() - pre_psi_readtime < 50) return;
  pre_psi_readtime = millis();

  switch( active_step)
  {
    default : break;
    
    case STEP_USER_INPUT : 
      if( working_mode == MODE_AUTO)
      {
        working_mode = STEP_MAKE_PSI;
        Sound_Update = 2; Sound_Num = 3;
        pre_buzzer_tic = millis();
      }
      is_target_psi_set = false;
      break;

    case STEP_MAKE_PSI :
    case STEP_MOTOR_MOVE :
    case STEP_MOTOR_WAITE :
    case STEP_MOTOR_HOLD :
      // if( analogRead(READ_PSI) < 220) -> MOKUP REVA hard to get 110mmhg 
      if( analogRead(READ_PSI) < 198) // 90mmhg
      {
        digitalWrite(AIRPUMP_PORT, HIGH);  // active pump
      }
      else
      {
        digitalWrite(AIRPUMP_PORT, LOW);  // de-active pump
        is_target_psi_set = true;         // set only once
      }
      break;
      
    case STEP_BREAK_PSI :
      digitalWrite(AIRPUMP_PORT, LOW);  // de-active pump
      digitalWrite(SOLENOID_PORT, HIGH);  // block solenoide
      pre_valve_close_time = millis();
      active_step = STEP_WORKING_END;
      break;

    case STEP_WORKING_END :
      if( millis() - pre_valve_close_time > 2 * 1000)
      {
        digitalWrite(SOLENOID_PORT, LOW);  // block solenoide
        digitalWrite(AIRPUMP_PORT, LOW);
        digitalWrite(MOTOR_SLEEP, LOW); // when power off -> enable sleep motor

        f_power_state = 0;

        Sound_Update = 2; Sound_Num = 1;
        pre_buzzer_tic = millis();
      }
      break;

    case STEP_EMERGENCY_STOP :
      if( millis() - pre_valve_close_time > 2 * 1000)
      {
        digitalWrite(SOLENOID_PORT, LOW);  // block solenoide
        digitalWrite(AIRPUMP_PORT, LOW);
        active_step = STEP_USER_INPUT;
      }
      break;
  }
}

void updateMotor(void)
{
  if( f_power_state == 0) return;
  
  switch( active_step)
  {
    default : break;

    case STEP_USER_INPUT : 
      break;
      
    case STEP_MAKE_PSI :
      digitalWrite( MOTOR_PORT_F, LOW);
      digitalWrite( MOTOR_PORT_R, LOW);

      if( is_target_psi_set == true)
      {
        Sound_Update = 2; Sound_Num = 3;
        pre_buzzer_tic = millis();
        
        pre_motor_stop_time = millis();
        active_step = STEP_MOTOR_MOVE;
      }
      break;
    
    case STEP_MOTOR_MOVE :
      if( millis() - pre_motor_stop_time > 300)
      {
        digitalWrite( MOTOR_PORT_F, HIGH);
        active_step = STEP_MOTOR_WAITE;
        pre_encodercheck_time = millis();
        pre_motor_stop_time = millis();

        Sound_Update = 2; Sound_Num = 3;
        pre_buzzer_tic = millis();
      }
      break;
      
    case STEP_MOTOR_WAITE :
      if( millis() - pre_motor_stop_time > 300 &&
          millis() - pre_encodercheck_time > 100) // if encoder update is not working
      {
        digitalWrite(MOTOR_PORT_F, LOW);
        digitalWrite(MOTOR_PORT_R, LOW);
        pulses = 0;

        pre_motor_stop_time = millis();
        active_step = STEP_MOTOR_HOLD;
      }
      break;
      
    case STEP_MOTOR_HOLD :
      if( millis() - pre_motor_stop_time > 10 * 1000)
      {
        active_step = STEP_BREAK_PSI;

        Sound_Update = 2; Sound_Num = 3;
        pre_buzzer_tic = millis();
      }
      break;
      
    case STEP_BREAK_PSI :
    case STEP_WORKING_END :
      digitalWrite(MOTOR_PORT_F, LOW);
      digitalWrite(MOTOR_PORT_R, LOW);
      break;
  }
}

void updateLED (void)
{
  if( f_power_state == 0)
  {
    digitalWrite(LED_EMERGENCY, HIGH);
    digitalWrite(LED_POWER, HIGH);
    digitalWrite(LED_MOTOR, HIGH);
    digitalWrite(LED_BLUETOOTH, HIGH);
    digitalWrite(LED_BAT_ICO, HIGH);
    digitalWrite(LED_BAT_STATE1, HIGH);
    digitalWrite(LED_BAT_STATE2, HIGH);
    digitalWrite(LED_BAT_STATE3, HIGH);
    digitalWrite(LED_MOTOR_ICO, HIGH);
    digitalWrite(LED_MOTOR_STATE1, HIGH);
    digitalWrite(LED_MOTOR_STATE2, HIGH);
    digitalWrite(LED_MOTOR_STATE3, HIGH);
    digitalWrite(LED_MOTOR_STATE4, HIGH);

    if( f_peltier_state == true)
    {
      #if !DEBUG_MODE
      digitalWrite(LED_TEMP_DANGER, LOW);
      digitalWrite(LED_TEMP_NORMAL, HIGH);
      #endif
    }
    else
    {
      #if !DEBUG_MODE
      digitalWrite(LED_TEMP_DANGER, HIGH);
      digitalWrite(LED_TEMP_NORMAL, LOW);
      #endif
    }
  }
  else
  {
    if( working_mode == MODE_MANUAL) digitalWrite(LED_EMERGENCY, LOW);
    else digitalWrite(LED_EMERGENCY, HIGH);
    
    digitalWrite(LED_POWER, LOW);

    if( working_mode == MODE_MANUAL) digitalWrite(LED_MOTOR, LOW);
    else digitalWrite(LED_MOTOR, HIGH);

    if( millis() - pre_led_flash_time > 300)  // func for flash 
    {
      pre_led_flash_time = millis();
#if MOKUP_MODE
      digitalWrite(LED_BLUETOOTH, LOW);
#else
      if( Serial1.available() > 0) digitalWrite(LED_BLUETOOTH, LOW);
      else
      {
        if( flash_statae == 0) digitalWrite(LED_BLUETOOTH, LOW);
        else digitalWrite(LED_BLUETOOTH, HIGH);
      }
#endif
      
      if( analogRead(READ_CHARGE) > 512) 
      {
        if( flash_statae == 0) digitalWrite(LED_BAT_ICO, LOW);
        else digitalWrite(LED_BAT_ICO, HIGH);
      }
      else
      {
        digitalWrite(LED_BAT_ICO, LOW);
      }

      if( analogRead(READ_BAT) > 748)
      {
        digitalWrite(LED_BAT_STATE1, LOW);
        digitalWrite(LED_BAT_STATE2, LOW);
        digitalWrite(LED_BAT_STATE3, LOW);
      }
      else if( analogRead(READ_BAT) > 572)
      {
        digitalWrite(LED_BAT_STATE1, LOW);
        digitalWrite(LED_BAT_STATE2, LOW);
        digitalWrite(LED_BAT_STATE3, HIGH);
      }
      else if( analogRead(READ_BAT) > 220)
      {
        digitalWrite(LED_BAT_STATE1, LOW);
        digitalWrite(LED_BAT_STATE2, HIGH);
        digitalWrite(LED_BAT_STATE3, HIGH);
      }
      else
      {
        if( flash_statae == 0) digitalWrite(LED_BAT_STATE1, LOW);
        else digitalWrite(LED_BAT_STATE1, HIGH);
        digitalWrite(LED_BAT_STATE2, HIGH);
        digitalWrite(LED_BAT_STATE3, HIGH);
      }

      digitalWrite(LED_MOTOR_ICO, LOW);
      switch( active_step)
      {
        default : break;
        
        case STEP_USER_INPUT : 
          digitalWrite(LED_MOTOR_STATE1, HIGH);
          digitalWrite(LED_MOTOR_STATE2, HIGH);
          digitalWrite(LED_MOTOR_STATE3, HIGH);
          digitalWrite(LED_MOTOR_STATE4, HIGH);
          break;
          
        case STEP_MAKE_PSI :
          if( flash_statae == 0) digitalWrite(LED_MOTOR_STATE1, LOW);
          else digitalWrite(LED_MOTOR_STATE1, HIGH);
          digitalWrite(LED_MOTOR_STATE2, HIGH);
          digitalWrite(LED_MOTOR_STATE3, HIGH);
          digitalWrite(LED_MOTOR_STATE4, HIGH);
          break;
          
        case STEP_MOTOR_MOVE :
        case STEP_MOTOR_WAITE :
          digitalWrite(LED_MOTOR_STATE1, LOW);
          if( flash_statae == 0) digitalWrite(LED_MOTOR_STATE2, LOW);
          else digitalWrite(LED_MOTOR_STATE2, HIGH);
          digitalWrite(LED_MOTOR_STATE3, HIGH);
          digitalWrite(LED_MOTOR_STATE4, HIGH);
          break;
          
        case STEP_MOTOR_HOLD :
          digitalWrite(LED_MOTOR_STATE1, LOW);
          digitalWrite(LED_MOTOR_STATE2, LOW);
          if( flash_statae == 0) digitalWrite(LED_MOTOR_STATE3, LOW);
          else digitalWrite(LED_MOTOR_STATE3, HIGH);
          digitalWrite(LED_MOTOR_STATE4, HIGH);
          break;
          
        case STEP_BREAK_PSI :
        case STEP_WORKING_END :
          digitalWrite(LED_MOTOR_STATE1, LOW);
          digitalWrite(LED_MOTOR_STATE2, LOW);
          digitalWrite(LED_MOTOR_STATE3, LOW);
          if( flash_statae == 0) digitalWrite(LED_MOTOR_STATE4, LOW);
          else digitalWrite(LED_MOTOR_STATE4, HIGH);
          break;
      }

      if( flash_statae == 0) flash_statae = 1;
      else flash_statae = 0;
    }

    if( f_peltier_state == true)
    {
      #if !DEBUG_MODE
      digitalWrite(LED_TEMP_DANGER, LOW);
      digitalWrite(LED_TEMP_NORMAL, HIGH);
      #endif
    }
    else
    {
      #if !DEBUG_MODE
      digitalWrite(LED_TEMP_DANGER, HIGH);
      digitalWrite(LED_TEMP_NORMAL, LOW);
      #endif
    }
  }
}
void readEEPROM (void)
{
  int addr = 0x00;
  signed int data[2];
    
  for (int i = 0; i<2; i++)
  {
    data[i] = EEPROM.read(addr);
    addr += sizeof(signed int);
    delay(1);
  }
  
  if( data[0] == data[1]) working_mode = data[0];
  else working_mode = MODE_AUTO;
}

void updateEEPROM (void)
{
  if( is_update_infor == false) return;
  is_update_infor = false;

  for (int i = 0 ; i < 10; i++) 
  {
    EEPROM.write(i, 0);
    delay(1);
  }

  int addr = 0x00;
  unsigned int data[2];
  data[0] = working_mode;
  data[1] = working_mode;

  for (int i = 0; i<2; i++)
  {
    EEPROM.write(addr, data[i]);
    addr += sizeof(signed int);
    delay(1);
  }
}

void Melody_Proc(void)
{
  static unsigned char Val = 0;
  
  if (Sound_Update == 0) return;
  if( millis() - pre_buzzer_tic < 70) return;
  pre_buzzer_tic = millis();

  if(Sound_Update == 2)
  {
    Val = 0;
    Sound_Update = 1;
  }
  Val += 1;

  if (Sound_Num == 1) //Goodbye
  {
    switch (Val)
    {
      case 1: 
        noTone(BUZZER_PWM);
        digitalWrite(BUZZER_PWM, LOW);
        digitalWrite(BUZZER_POWER, HIGH); break;
      case 4: tone(BUZZER_PWM, 3300); break;
      case 6: tone(BUZZER_PWM, 2500); break;
      case 8: tone(BUZZER_PWM, 2000); break;
      case 10: tone(BUZZER_PWM, 1600); break;
      case 12: tone(BUZZER_PWM, 1200); break;
      case 14: digitalWrite(BUZZER_POWER, LOW); break;
      case 16:
        Sound_Update = 0; Val = 0;
        noTone(BUZZER_PWM);
        digitalWrite(BUZZER_PWM, LOW);
        break;
    }
  }
  else if (Sound_Num == 2) //Beep
  {
    switch (Val)
    {
      case 1: 
        noTone(BUZZER_PWM);
        digitalWrite(BUZZER_PWM, LOW);
        digitalWrite(BUZZER_POWER, HIGH); break;
      case 4: tone(BUZZER_PWM, 2000); break; //PWM ON At 2KHz
      case 6: digitalWrite(BUZZER_POWER, LOW);
      case 8:
        Sound_Update = 0; Val = 0;
        noTone(BUZZER_PWM);
        digitalWrite(BUZZER_PWM, LOW);
        break;
    }
  }
  else if (Sound_Num == 3) //On_Beep
  {
    switch (Val)
    {
      case 1: 
        noTone(BUZZER_PWM);
        digitalWrite(BUZZER_PWM, LOW);
        digitalWrite(BUZZER_POWER, HIGH); break;
      case 4: tone(BUZZER_PWM, 1600); break; //PWM ON At1.666KHz
      case 6: tone(BUZZER_PWM, 2500); break; //PWM Change 2.5KHz
      case 8: digitalWrite(BUZZER_POWER, LOW); break;
      case 10:
        Sound_Update = 0; Val = 0;
        noTone(BUZZER_PWM);
        digitalWrite(BUZZER_PWM, LOW);

        if( is_touch_enable == 0)
        {
          is_touch_enable = 1;
          enableTouch();          
        }
        break;
    }
  }
  else if (Sound_Num == 4) //off_Beep
  {
    switch (Val)
    {
      case 1: 
        noTone(BUZZER_PWM);
        digitalWrite(BUZZER_PWM, LOW);
        digitalWrite(BUZZER_POWER, LOW);
        break;
      case 4: tone(BUZZER_PWM, 2500); break; //PWM ON At 2.5KHz
      case 6: tone(BUZZER_PWM, 1600); break; //PWM Change 1.666KHz
      case 8: digitalWrite(BUZZER_POWER, LOW); break;
      case 10: 
        Sound_Update = 0; Val = 0;
        noTone(BUZZER_PWM);
        digitalWrite(BUZZER_PWM, LOW);
        break;
    }
  }
  else if (Sound_Num == 5) //Beep
  {
    switch (Val)
    {
      case 1: 
        noTone(BUZZER_PWM);
        digitalWrite(BUZZER_PWM, LOW);
        digitalWrite(BUZZER_POWER, HIGH); break;
      case 4: tone(BUZZER_PWM, 1200); break;
      case 6: tone(BUZZER_PWM, 1600); break;
      case 8: tone(BUZZER_PWM, 2000); break;
      case 10: tone(BUZZER_PWM, 2500); break;
      case 12: tone(BUZZER_PWM, 3300); break;
      case 14: digitalWrite(BUZZER_POWER, LOW); break;
      case 16:
        Sound_Update = 0; Val = 0;
        noTone(BUZZER_PWM);
        digitalWrite(BUZZER_PWM, LOW);
        break;
    }
  }

}

void enableTouch (void)
{
#if DEBUG_MODE
    // touch
  //Serial.println("i2c Start");
  //I2c.scan();
#endif

  I2c.write(0x24, 0x39, 0x28);
  I2c.write(0x24, 0x3A, 0x28);
  I2c.write(0x24, 0x3B, 0x28);
  I2c.write(0x24, 0x3C, 0x28);
  I2c.write(0x24, 0x3D, 0x28);
  I2c.write(0x24, 0x3E, 0x28);
  I2c.write(0x24, 0x3F, 0x28);
  I2c.write(0x24, 0x40, 0x28);

  delay(100);

  I2c.write(0x24, 0x01, 0x0F);
}

void disableTouch (void)
{
  I2c.write(0x24, 0x01, 0x00);
}

void resetTouch (void)
{
  /*
  response_off_ctrl / response_ctrl / bf_mode / software_rst
        0 1 0     /     0 1 1     /   0     /       1
  */
  I2c.write(0x24, 0x36, 0x4D);
}
