// Define key
#define POWER             0x01
#define MOTOR_F           0x02
#define MOTOR_R           0x04
#define BLE               0x08

// Define LED
#define LED_PWR           36
#define LED_MTF           37
#define LED_MTR           40
#define LED_BLE           41

#define LED_BAT_ICO       34
#define LED_BAT_STATE1    35
#define LED_BAT_STATE2    33
#define LED_BAT_STATE3    32

#define LED_MOTOR_ICO     14
#define LED_MOTOR_STATE1  31
#define LED_MOTOR_STATE2  30
#define LED_MOTOR_STATE3  15
#define LED_MOTOR_STATE4  39

#define LED_TEMP_DANGER   29
#define LED_TEMP_NORMAL   28

// Define temp control
#define PELTIER_PORT      24
#define PELTIER_FAN       26

// Define PSI
#define AIRPUMP_PORT      25
#define SOLENOID_PORT     23

// Define ENCODER Motor
#define MOTOR_PORT_F  8
#define MOTOR_PORT_R  7
#define ENCODER_A     3
#define ENCODER_B     2
#define MOTOR_SLEEP   9
#define MOTOR_FALT    6

// Define Buzzer
#define BUZZER_PWM    46
#define BUZZER_POWER  45

// Define Touch senst
#define TOUCH_DIO     20
#define TOUCH_CLK     21

// Define Ble
#define BLE_TX        18
#define BLE_RX        19
#define BLE_STATE     44  // connection or not
#define BLE_MODE      43  // at-command or bypass
#define BLE_UART      42  // UART On / Off

// Define Anoalg
#define READ_BAT      A0
#define READ_PSI      A2

#define READ_USB_CON     A4
#define READ_NOW_CHARGE  A5
#define READ_FULL_CHARGE A6

#define READ_TEMP     A14

typedef enum
{
  MODE_AUTO = 1,    // delete user mode Auto mode is default
  MODE_MANUAL,
}working_mode_list;

typedef enum
{
  STEP_USER_INPUT = 0,  // power on
  
  STEP_MAKE_PSI,        // step 1
  
  STEP_MOTOR_MOVE,      // 
  STEP_MOTOR_WAITE,     // step 2
  
  STEP_MOTOR_HOLD,      // step 3
  
  STEP_BREAK_PSI,       // step 4
  
  STEP_WORKING_END,      // power off

  STEP_WAIT_INPUT,

  // Emergency

  STEP_EMERGENCY_STOP
  
}active_step_list;
