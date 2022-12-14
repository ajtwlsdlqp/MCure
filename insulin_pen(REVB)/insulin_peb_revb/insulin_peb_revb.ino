
#define MOKUP_MODE    1
#define BTN_MODE      1

#include "myDef.h"
#include "myFuncDef.h"
#include "EEPROM.h"
//#include "Melody.h"

#include "LowPower.h"

#include <Wire.h>
#include <I2C.h>
#include <SoftwareSerial.h>

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
unsigned char f_ble_state = 0;


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

unsigned long pre_led_update_time = millis();
unsigned long next_sleep_ent_time = millis();

unsigned char mokup_motor_run_state;
unsigned char mokup_motor_stop_state;
// for debug
unsigned char ch;

unsigned char off_led_show_state = true;
unsigned long interval_ble_tx = millis();


SoftwareSerial ble(18, 19); // RX, TX

void setup() {
  // put your setup code here, to run once:
  Serial.end();
  Serial1.end();

  pinMode(BLE_STATE, INPUT);

  // after connected keep high -> atmode
  // after connected keep low  -> bypass mode // default
  pinMode(BLE_MODE, OUTPUT);
  digitalWrite(BLE_MODE, LOW);

  // rising edge -> uart off
  // falling edge-> uart on
  pinMode(BLE_UART, OUTPUT);
  digitalWrite(BLE_UART, LOW);

  ble.begin(9600);
  
  #if !BTN_MODE
  I2c.begin();
  I2c.timeOut(2000);
  I2c.pullup(true);
  #endif
  
  // motor
  pinMode(MOTOR_PORT_F, OUTPUT);
  digitalWrite(MOTOR_PORT_F, LOW);

  pinMode(MOTOR_PORT_R, OUTPUT);
  digitalWrite(MOTOR_PORT_R, LOW);

  pinMode(MOTOR_SLEEP, OUTPUT);
  digitalWrite(MOTOR_SLEEP, LOW);

  pinMode(MOTOR_FALT, INPUT_PULLUP);
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

  pinMode(LED_BLE, OUTPUT);
  pinMode(LED_BAT_ICO, OUTPUT);
  pinMode(LED_BAT_STATE1, OUTPUT);
  pinMode(LED_BAT_STATE2, OUTPUT);
  pinMode(LED_BAT_STATE3, OUTPUT);
  pinMode(LED_MOTOR_ICO, OUTPUT);
  pinMode(LED_MOTOR_STATE1, OUTPUT);
  pinMode(LED_MOTOR_STATE2, OUTPUT);
  pinMode(LED_MOTOR_STATE3, OUTPUT);
  pinMode(LED_MOTOR_STATE4, OUTPUT);
  pinMode(LED_TEMP_DANGER, OUTPUT);
  pinMode(LED_TEMP_NORMAL, OUTPUT);

  #if BTN_MODE
  pinMode(LED_PWR, INPUT_PULLUP);
  pinMode(LED_MTF, INPUT_PULLUP);
  pinMode(LED_MTR, INPUT_PULLUP);
  #else
  pinMode(LED_PWR, OUTPUT);
  pinMode(LED_MTF, OUTPUT);
  pinMode(LED_MTR, OUTPUT);
  
  digitalWrite(LED_PWR, HIGH);
  digitalWrite(LED_MTF, HIGH);
  digitalWrite(LED_MTR, HIGH);
  #endif
  
  digitalWrite(LED_BLE, HIGH);
  digitalWrite(LED_BAT_ICO, HIGH);
  digitalWrite(LED_BAT_STATE1, HIGH);
  digitalWrite(LED_BAT_STATE2, HIGH);
  digitalWrite(LED_BAT_STATE3, HIGH);
  digitalWrite(LED_MOTOR_ICO, HIGH);
  digitalWrite(LED_MOTOR_STATE1, HIGH);
  digitalWrite(LED_MOTOR_STATE2, HIGH);
  digitalWrite(LED_MOTOR_STATE3, HIGH);
  digitalWrite(LED_MOTOR_STATE4, HIGH);
  digitalWrite(LED_TEMP_DANGER, HIGH);
  digitalWrite(LED_TEMP_NORMAL, HIGH);

  pulses = 0;
  pre_encodercheck_time = millis();

  active_step = STEP_USER_OFF;
  is_target_psi_set = false;

  readEEPROM();
  working_mode = MODE_MANUAL;
  
  
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

  updateBLE();


  if ( analogRead(READ_USB_CON) > 1000)
  {
    next_sleep_ent_time = millis();
  }
  else
  {
    if( is_touch_enable == 1)
    {
      if( f_power_state == 0 && ( millis() - next_sleep_ent_time > 2000) )
      {
        LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);       
        next_sleep_ent_time = millis();
      }
    }
  }
}

void A_CHANGE()
{ //Interrupt function to read the x2 pulses of the encoder.
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

#if BTN_MODE
void Key_Read(void)
{
  static unsigned char pre_key = 0xFF;
  
  if (pre_key == 0xFF)
  {
    if( digitalRead(LED_PWR) == LOW){ Key = POWER; }
    else if( digitalRead(LED_MTF) == LOW){ Key = MOTOR_F; }
    else if( digitalRead(LED_MTR) == LOW){ Key = MOTOR_R; }
    else 
    {
      Key = 0xFF;
      
      if ( f_power_state == 1 && mokup_motor_run_state != 0)
      {
        mokup_motor_run_state = 0;
        digitalWrite(MOTOR_PORT_F, LOW);
        digitalWrite(MOTOR_PORT_R, LOW);
      }
    }
    pre_key = Key;// pre_key
  }
  else
  {
    if( digitalRead(LED_PWR) == LOW){ Key = POWER; }
    else if( digitalRead(LED_MTF) == LOW){ Key = MOTOR_F; }
    else if( digitalRead(LED_MTR) == LOW){ Key = MOTOR_R; }
    else { Key = 0xFF; }

    pre_key = Key;// pre_key
  }
}
#else
void Key_Read(void)
{
  unsigned char temp = 0;
  static unsigned char pre_key = 0xFF;
  unsigned char rx_buff[2] = {0, 0};

  temp = I2c.read(0x24, 0x2A, 2, rx_buff);

  if (pre_key == 0xFF)
  {
    if (rx_buff[0] == 0)
    {
      Key = 0xFF;

      if ( f_power_state == 1 && mokup_motor_run_state != 0)
      {
        mokup_motor_run_state = 0;
        digitalWrite(MOTOR_PORT_F, LOW);
        digitalWrite(MOTOR_PORT_R, LOW);
      }
    }
    else if (rx_buff[0] & 0x01) {
      Key = BLE;
    }
    else if (rx_buff[0] & 0x02) {
      Key = MOTOR_R;
    }
    else if (rx_buff[0] & 0x04) {
      Key = MOTOR_F;
    }
    else if (rx_buff[0] & 0x08) {
      Key = POWER;
    }
    else {
      Key = 0xFF;
    }

    pre_key = Key;// pre_key
  }
  else
  {
    if (rx_buff[0] == 0) {
      Key = 0xFF;
    }
    else if (rx_buff[0] == 0x01) {
      Key = BLE;
    }
    else if (rx_buff[0] == 0x02) {
      Key = MOTOR_R;
    }
    else if (rx_buff[0] == 0x04) {
      Key = MOTOR_F;
    }
    else if (rx_buff[0] == 0x08) {
      Key = POWER;
    }
    else
    {
      Key = pre_key;
    }

    pre_key = Key;// pre_key
  }
}
#endif


void Key_Scan(void)
{
  static unsigned char AutoKeyCount = 0;
  static unsigned char f_PressedKey = 0;
  static unsigned char PrevKey = 0xFF;
  static unsigned char is_notify = 0;

  if( millis() - pre_key_readtime < 20) return;
  pre_key_readtime = millis();

  Key_Read();             // update Key value

  if (Key != 0xFF && PrevKey == Key)
  {
    if (f_PressedKey == 0)
    {
      if( Key == POWER)
      {
        AutoKeyCount = 0xFF;
        is_notify = 3;
      }
      else
      {
        AutoKeyCount = 1;
        is_key_change = 1;
      }

      f_PressedKey = 1;
    }
    else
    { // Hold - Pressed Key
        if( Key == MOTOR_R || Key == MOTOR_F  )
        {
          if (--AutoKeyCount == 0)
          {
            is_key_change = 1;
            AutoKeyCount = 1; // about 0.15s
          }
        }
        else if(Key == POWER)
        {
          --AutoKeyCount;
          if (AutoKeyCount < 0xFC && is_notify == 3) // 20ms * 10 = 200ms
          {
            is_notify = 2;
            if(active_step == STEP_USER_INPUT || active_step == STEP_MAKE_PSI)
            {
              Sound_Update = 2; Sound_Num = 3;
              pre_buzzer_tic = millis();
            }
          }
          else if (AutoKeyCount < 0xCD && is_notify == 2)                // 20ms * 50 = 1S;
          {
            is_notify = 1;
            if( f_power_state == 0)
            {
              Sound_Update = 2; Sound_Num = 5;
              pre_buzzer_tic = millis();
            }
            else
            {
              Sound_Update = 2; Sound_Num = 1;
              pre_buzzer_tic = millis();
            }
          }
        }
    }
  }
  else
  {
    f_PressedKey = 0;

    if( is_notify == 2 && (active_step == STEP_USER_INPUT || active_step == STEP_MAKE_PSI) )
    {
      Key = POWER_PUMP;
      is_key_change = 1;
    }
    else if( is_notify == 1)
    {
      Key = POWER;
      is_key_change = 1;
    }
    
    is_notify = 0;
  }

  PrevKey = Key;
  Key_Proc();
}

void Key_Proc(void)
{
  if (is_key_change == false) return;
  is_key_change = false;

  switch (Key)
  {
    case POWER_PUMP :
      if ( f_power_state == 0) break;

      if( active_step == STEP_USER_INPUT)
      {
        active_step = STEP_MAKE_PSI;
      }
      else if( active_step == STEP_MAKE_PSI)
      {
        active_step = STEP_USER_INPUT;
        digitalWrite(AIRPUMP_PORT, LOW);  // de-active pump
      }
      break;
      
    case POWER :
      if ( f_power_state == 0)
      {
        f_power_state = 1;
        digitalWrite(MOTOR_SLEEP, HIGH); // wake up motor drive
        active_step = STEP_USER_INPUT;
        is_target_psi_set = false;

        working_mode = MODE_MANUAL;
      }
      else
      {
        if ( active_step == STEP_USER_INPUT) // user off function
        {
          f_power_state = 0;
          active_step = STEP_USER_OFF;
          
          next_sleep_ent_time = millis();

          digitalWrite(MOTOR_PORT_F, LOW);
          digitalWrite(MOTOR_PORT_R, LOW);

          digitalWrite(SOLENOID_PORT, LOW);
          digitalWrite(AIRPUMP_PORT, LOW);

          digitalWrite(MOTOR_SLEEP, LOW); // when power off -> enable sleep motor
        }
        else if (active_step != STEP_EMERGENCY_STOP)                               // emergency off function
        {
          digitalWrite(AIRPUMP_PORT, LOW);  // de-active pump
          digitalWrite(SOLENOID_PORT, HIGH);  // block solenoide
          pre_valve_close_time = millis();

          active_step = STEP_EMERGENCY_STOP;
        }
      }
      break;

    case MOTOR_F :
      if ( f_power_state == 0) break;

      if ( mokup_motor_run_state != 1)
      {
        mokup_motor_run_state = 1;
        pre_encodercheck_time = millis();
        pre_user_motor_worktime = millis();
        digitalWrite(MOTOR_PORT_F, HIGH);
        digitalWrite(MOTOR_PORT_R, LOW);
      }
      else if ( millis() - pre_encodercheck_time > 100) // if encoder update is not working
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

    case MOTOR_R :
      if ( f_power_state == 0) break;

      if ( mokup_motor_run_state != 2)
      {
        mokup_motor_run_state = 2;
        pre_encodercheck_time = millis();

        digitalWrite(MOTOR_PORT_F, LOW);
        digitalWrite(MOTOR_PORT_R, HIGH);
        pre_user_motor_worktime = millis();
      }
      else if ( millis() - pre_encodercheck_time > 100) // if encoder update is not working
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

    case BLE :
      if ( f_power_state == 0) break;
      if ( f_ble_state == 0) // turn on ble
      {
        f_ble_state = 1;
        digitalWrite(BLE_UART, LOW);
      }
      else // turn off ble
      {
        f_ble_state = 0;
        digitalWrite(BLE_UART, HIGH);
      }

      Sound_Update = 2; Sound_Num = 3;
      pre_buzzer_tic = millis();
      break;

    default : break;
  }
}

/*
   temperature control function
   must operating every time
   10K NTC -> 25'c -> 10K
   29'c    -> 8.38690 KOhm ->
   27'c    -> 9.14743 KOhm

   base    -> 3KOhm
*/
void updateTemperatrue (void)
{
  unsigned int real_temp = 1024;

  if ( millis() - pre_temp_readtime < 1000) return;
  pre_temp_readtime = millis();

  real_temp = analogRead(READ_TEMP);

  if ( real_temp <= 754 ) // 29'c turn on
  {
    f_peltier_state = true;
    digitalWrite(PELTIER_FAN, HIGH);
    digitalWrite(PELTIER_PORT, HIGH);
  }
  //else if ( real_temp >= 771) // 27'c turn off
  else if( real_temp >= 763) // 28'c
  {
    f_peltier_state = false;
    digitalWrite(PELTIER_FAN, LOW);
    digitalWrite(PELTIER_PORT, LOW);
  }
}

/*
   activate auto mode
   activate manual mode via press button
   target mmHg 110
   ADC -> 219

   temp_mmhg = ((double)real_psi * 5) / 1024;
   temp_mmhg = temp_mmhg - 0.5;
   temp_mmhg = temp_mmhg * 3.75;
   temp_mmhg = temp_mmhg * 51.71;

   // Ref 3v3 adc340 target 115.72 mmhg
   // ref 3v3 adc310 target 96.956 mmhg
*/
void updatePSI (void)
{
  if ( f_power_state == 0) return;

  if ( millis() - pre_psi_readtime < 50) return;
  pre_psi_readtime = millis();

  switch ( active_step)
  {
    default : break;

    case STEP_USER_INPUT :
      is_target_psi_set = false;
      break;

    case STEP_MAKE_PSI :
    case STEP_MOTOR_MOVE :
    case STEP_MOTOR_WAITE :
    case STEP_MOTOR_HOLD :
      // if( analogRead(READ_PSI) < 220) -> MOKUP REVA hard to get 110mmhg
      //if ( analogRead(READ_PSI) < 198) // 90mmhg -> ref 5V
      
      // if ( analogRead(READ_PSI) < 310) // 90mmhg -> ref 3v3
      if ( analogRead(READ_PSI) < 620) // 90mmhg -> ref 3v3
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
      if ( millis() - pre_valve_close_time > 2 * 1000)
      {
        digitalWrite(SOLENOID_PORT, LOW);  // block solenoide
        digitalWrite(AIRPUMP_PORT, LOW);
        digitalWrite(MOTOR_SLEEP, LOW); // when power off -> enable sleep motor

        f_power_state = 0;
        next_sleep_ent_time = millis();

        digitalWrite(MOTOR_PORT_F, LOW);
        digitalWrite(MOTOR_PORT_R, LOW);

        digitalWrite(SOLENOID_PORT, LOW);
        digitalWrite(AIRPUMP_PORT, LOW);

        Sound_Update = 2; Sound_Num = 1;
        pre_buzzer_tic = millis();
      }
      break;

    case STEP_EMERGENCY_STOP :
      if ( millis() - pre_valve_close_time > 2 * 1000)
      {
        digitalWrite(SOLENOID_PORT, LOW);  // block solenoide
        digitalWrite(AIRPUMP_PORT, LOW);

        f_power_state = 0;
        active_step = STEP_USER_OFF;
        
        next_sleep_ent_time = millis();
        
        digitalWrite(MOTOR_PORT_F, LOW);
        digitalWrite(MOTOR_PORT_R, LOW);

        digitalWrite(SOLENOID_PORT, LOW);
        digitalWrite(AIRPUMP_PORT, LOW);

        digitalWrite(MOTOR_SLEEP, LOW); // when power off -> enable sleep motor
      }
      break;
  }
}

void updateMotor(void)
{
  if ( f_power_state == 0) return;

  switch ( active_step)
  {
    default : break;

    case STEP_USER_INPUT :
      break;

    case STEP_MAKE_PSI :
      if ( is_target_psi_set == true)
      {
        Sound_Update = 2; Sound_Num = 3;
        pre_buzzer_tic = millis();

        pre_motor_stop_time = millis();
        active_step = STEP_MOTOR_MOVE;
      }
      break;

    case STEP_MOTOR_MOVE :
      if ( millis() - pre_motor_stop_time > 200)
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
      if ( millis() - pre_motor_stop_time > 200 &&
           millis() - pre_encodercheck_time > 80) // if encoder update is not working
      {
        digitalWrite(MOTOR_PORT_F, LOW);
        digitalWrite(MOTOR_PORT_R, LOW);
        pulses = 0;

        pre_motor_stop_time = millis();
        active_step = STEP_MOTOR_HOLD;
      }
      break;

    case STEP_MOTOR_HOLD :
      if ( millis() - pre_motor_stop_time > 5 * 1000)
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
  static uint8_t led_timing = 0;
//   if( micros() - pre_led_update_time < 400) return;
//   pre_led_update_time = micros();

  if ( led_timing >= 4) led_timing = 0;
//  if ( f_power_state == 0) ledOffAll();
  ledOffAll();

  switch ( led_timing)
  {
    case 0 :  // case for allways turn on ico
      if ( f_power_state == 0) break;

      #if !BTN_MODE
      digitalWrite(LED_PWR, LOW);
      digitalWrite(LED_MTF, LOW);
      digitalWrite(LED_MTR, LOW);
      #endif
      
      /*
        Connected -> Keep High
        disconnected -> Keep Low
      */
      if ( digitalRead(BLE_STATE) == HIGH)
      {
        digitalWrite(LED_BLE, LOW);
      }
      else
      {
        if ( flash_statae == 0) digitalWrite(LED_BLE, LOW);
        else digitalWrite(LED_BLE, HIGH);
      }
      break;

    case 1 :
      if ( f_power_state == 0 && (analogRead(READ_USB_CON) < 1000) )break;
      
      if ( analogRead(READ_USB_CON) > 1000)
      {
        if( analogRead(READ_FULL_CHARGE) > 1000) 
        {
          digitalWrite(LED_BAT_ICO, LOW);
        }
        else
        {
          if ( flash_statae == 0) digitalWrite(LED_BAT_ICO, LOW);
          else digitalWrite(LED_BAT_ICO, HIGH);
        }

      }
      else
      {
        digitalWrite(LED_BAT_ICO, LOW);
      }
      
      // ref 3V3
      if ( analogRead(READ_BAT) > 635)
      {
        digitalWrite(LED_BAT_STATE1, LOW);
        digitalWrite(LED_BAT_STATE2, LOW);
        digitalWrite(LED_BAT_STATE3, LOW);
      }
      else if ( analogRead(READ_BAT) > 589)
      {
        digitalWrite(LED_BAT_STATE1, LOW);
        digitalWrite(LED_BAT_STATE2, LOW);
        digitalWrite(LED_BAT_STATE3, HIGH);
      }
      else if ( analogRead(READ_BAT) > 558)
      {
        digitalWrite(LED_BAT_STATE1, LOW);
        digitalWrite(LED_BAT_STATE2, HIGH);
        digitalWrite(LED_BAT_STATE3, HIGH);
      }
      else
      {
        if ( flash_statae == 0) digitalWrite(LED_BAT_STATE1, LOW);
        else digitalWrite(LED_BAT_STATE1, HIGH);
        digitalWrite(LED_BAT_STATE2, HIGH);
        digitalWrite(LED_BAT_STATE3, HIGH);
      }
      break;

    case 2 :
      if ( f_power_state == 0) break;

      digitalWrite(LED_MOTOR_ICO, LOW);
      switch ( active_step)
      {
        default : break;

        case STEP_USER_INPUT :
          digitalWrite(LED_MOTOR_STATE1, HIGH);
          digitalWrite(LED_MOTOR_STATE2, HIGH);
          digitalWrite(LED_MOTOR_STATE3, HIGH);
          digitalWrite(LED_MOTOR_STATE4, HIGH);
          break;

        case STEP_MAKE_PSI :
          if ( flash_statae == 0) digitalWrite(LED_MOTOR_STATE1, LOW);
          else digitalWrite(LED_MOTOR_STATE1, HIGH);
          digitalWrite(LED_MOTOR_STATE2, HIGH);
          digitalWrite(LED_MOTOR_STATE3, HIGH);
          digitalWrite(LED_MOTOR_STATE4, HIGH);
          break;

        case STEP_MOTOR_MOVE :
        case STEP_MOTOR_WAITE :
          digitalWrite(LED_MOTOR_STATE1, LOW);
          if ( flash_statae == 0) digitalWrite(LED_MOTOR_STATE2, LOW);
          else digitalWrite(LED_MOTOR_STATE2, HIGH);
          digitalWrite(LED_MOTOR_STATE3, HIGH);
          digitalWrite(LED_MOTOR_STATE4, HIGH);
          break;

        case STEP_MOTOR_HOLD :
          digitalWrite(LED_MOTOR_STATE1, LOW);
          digitalWrite(LED_MOTOR_STATE2, LOW);
          if ( flash_statae == 0) digitalWrite(LED_MOTOR_STATE3, LOW);
          else digitalWrite(LED_MOTOR_STATE3, HIGH);
          digitalWrite(LED_MOTOR_STATE4, HIGH);
          break;

        case STEP_BREAK_PSI :
        case STEP_WORKING_END :
          digitalWrite(LED_MOTOR_STATE1, LOW);
          digitalWrite(LED_MOTOR_STATE2, LOW);
          digitalWrite(LED_MOTOR_STATE3, LOW);
          if ( flash_statae == 0) digitalWrite(LED_MOTOR_STATE4, LOW);
          else digitalWrite(LED_MOTOR_STATE4, HIGH);
          break;
      }
      break;

    case 3 :
      if( off_led_show_state == 0 && f_power_state == 0) break;
      
      if ( f_peltier_state == true)
      {
        digitalWrite(LED_TEMP_DANGER, LOW);
        digitalWrite(LED_TEMP_NORMAL, HIGH);
      }
      else
      {
        digitalWrite(LED_TEMP_DANGER, HIGH);
        digitalWrite(LED_TEMP_NORMAL, LOW);
      }
      break;
  }
  // 50ms -> 400ms
  if ( millis() - pre_led_flash_time > 300) // func for flash
  {
    pre_led_flash_time = millis();

    if ( flash_statae == 0) flash_statae = 1;
    else flash_statae = 0;
  }

  led_timing += 1;
}

void ledOffAll(void)
{
  #if !BTN_MODE
  digitalWrite(LED_PWR, HIGH);
  digitalWrite(LED_MTR, HIGH);
  digitalWrite(LED_MTF, HIGH);
  #endif
  
  digitalWrite(LED_BLE, HIGH);
  digitalWrite(LED_BAT_ICO, HIGH);
  digitalWrite(LED_BAT_STATE1, HIGH);
  digitalWrite(LED_BAT_STATE2, HIGH);
  digitalWrite(LED_BAT_STATE3, HIGH);
  digitalWrite(LED_MOTOR_ICO, HIGH);
  digitalWrite(LED_MOTOR_STATE1, HIGH);
  digitalWrite(LED_MOTOR_STATE2, HIGH);
  digitalWrite(LED_MOTOR_STATE3, HIGH);
  digitalWrite(LED_MOTOR_STATE4, HIGH);

  digitalWrite(LED_TEMP_DANGER, HIGH);
  digitalWrite(LED_TEMP_NORMAL, HIGH);
}

void updateBLE (void)
{
  if ( digitalRead(BLE_STATE) == LOW) return;
  if ( millis() - interval_ble_tx < 100) return;
  interval_ble_tx = millis();

  //i 0000[step] b [2 1 0]00XXX t 000XX l 0000X
  // 69 20 30 30 30 30 32 20 62 20 31 30 30 39 39 20 74 20 30 30 30 33 32 20 6c 20 30 30 30 30 31 a
  unsigned char insuData[32] = 
    { 0x69, 0x20, 0x30, 0x30, 0x30, 0x30, 0x30, 0x20,   // i[]00002[] 
      0x62, 0x20, 0x30, 0x30, 0x30, 0x30, 0x30, 0x20,   // b[]x0xxx[]
      0x74, 0x20, 0x30, 0x30, 0x30, 0x30, 0x30, 0x20,   // t[]000xx[]
      0x6C, 0x20, 0x30, 0x30, 0x30, 0x30, 0x30, 0x0A};  // l[]0000x\n

  insuData[6] = (0x30+((unsigned char)active_step));// step 

  if( analogRead(READ_FULL_CHARGE) > 1000) insuData[10] = 0x32;
  else if ( analogRead(READ_USB_CON) > 1000) insuData[10] = 0x31;
  else insuData[10] = 0x30;

  unsigned char bat_lim = map( constrain(analogRead(READ_BAT), 512, 651), 512, 651, 0, 100);
  insuData[12] = 0x30+(bat_lim/100);
  insuData[13] = 0x30+((bat_lim%100)/10);
  insuData[14] = 0x30+(bat_lim%10);

  unsigned char tmp_def = (unsigned char)(abs(analogRead(READ_TEMP)-753) / 8.5);// |(X-753)|/8.5 -> def 
  unsigned char tmp_lim = 29;
  
  if(  analogRead(READ_TEMP) <= 753) tmp_lim += tmp_def;
  else tmp_lim -= tmp_def;
  
  insuData[21] = 0x30 + (tmp_lim / 10);
  insuData[22] = 0x30 + (tmp_lim % 10);

  if( off_led_show_state == 1) insuData[31] = 0x31;
  else insuData[31] = 0x30;
  
  ble.write(insuData, 32);  

  if( ble.available() > 0)
  {
    unsigned char read_buf[2];
    ble.readBytes(read_buf, 2);

    if(read_buf[0] == 0x74)
    {
      if( read_buf[1] == 0x74) off_led_show_state = 1;
      else if( read_buf[1] == 0x66) off_led_show_state = 0;

      is_update_infor = true;
      ble.flush();
    }
  }
}

void readEEPROM (void)
{
  int addr = 0x00;
  signed int data[10];

  for (int i = 0; i < 10; i++)
  {
    data[i] = EEPROM.read(addr);
    addr += sizeof(signed int);
    delay(1);
  }

  off_led_show_state = data[2];
  is_update_infor = true;

  // TODO : update Ble Data
  ble.write("AT");
  ble.write(0x0D);
  delay(20);

  ble.write("AT+MANUF=insuAID");
  ble.write(0x0D);
  delay(20);

  ble.write("AT+ADVDATA=insuAID");
  ble.write(0x0D);
  delay(20);

  digitalWrite(BLE_UART, LOW);
  
}

void updateEEPROM (void)
{
  if ( is_update_infor == false) return;
  is_update_infor = false;

  for (int i = 0 ; i < 10; i++)
  {
    EEPROM.write(i, 0);
    delay(1);
  }

  int addr = 0x00;
  unsigned int data[10];
  data[0] = 0x55;
  data[1] = 0x55;
  data[3] = off_led_show_state;

  for (int i = 0; i < 3; i++)
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
  if ( millis() - pre_buzzer_tic < 70) return;
  pre_buzzer_tic = millis();

  if (Sound_Update == 2)
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
      case 3: tone(BUZZER_PWM, 3300); break;
      case 5: tone(BUZZER_PWM, 2500); break;
      case 7: tone(BUZZER_PWM, 2000); break;
      case 9: tone(BUZZER_PWM, 1600); break;
      case 11: tone(BUZZER_PWM, 1200); break;
      case 13: digitalWrite(BUZZER_POWER, LOW); break;
      case 14:
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
        digitalWrite(BUZZER_POWER, HIGH);
        break;
      case 3: tone(BUZZER_PWM, 1600); break; //PWM ON At1.666KHz
      case 5: tone(BUZZER_PWM, 2500); break; //PWM Change 2.5KHz
      case 7: digitalWrite(BUZZER_POWER, LOW); break;
      case 8:
        Sound_Update = 0; Val = 0;
        noTone(BUZZER_PWM);
        digitalWrite(BUZZER_PWM, LOW);
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
      case 3: tone(BUZZER_PWM, 2500); break; //PWM ON At 2.5KHz
      case 5: tone(BUZZER_PWM, 1600); break; //PWM Change 1.666KHz
      case 7: digitalWrite(BUZZER_POWER, LOW); break;
      case 8:
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
      case 3: tone(BUZZER_PWM, 1200); break;
      case 5: tone(BUZZER_PWM, 1600); break;
      case 7: tone(BUZZER_PWM, 2000); break;
      case 9: tone(BUZZER_PWM, 2500); break;
      case 11: tone(BUZZER_PWM, 3300); break;
      case 13: digitalWrite(BUZZER_POWER, LOW); break;
      case 14:
        Sound_Update = 0; Val = 0;
        noTone(BUZZER_PWM);
        digitalWrite(BUZZER_PWM, LOW);
        // working_mode = MODE_AUTO;
        break;
    }
  }

  #if !BTN_MODE
  if ( is_touch_enable == 0)
  {
    is_touch_enable = 1;
    enableTouch();
  }
  #endif

}

void enableTouch (void)
{
  //set sensitive
  I2c.write(0x24, 0x39, 0x05);
  I2c.write(0x24, 0x3A, 0x05);
  I2c.write(0x24, 0x3B, 0x05);
  I2c.write(0x24, 0x3C, 0x05);
  I2c.write(0x24, 0x3D, 0x05);
  I2c.write(0x24, 0x3E, 0x05);
  I2c.write(0x24, 0x3F, 0x05);
  I2c.write(0x24, 0x40, 0x05);

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
