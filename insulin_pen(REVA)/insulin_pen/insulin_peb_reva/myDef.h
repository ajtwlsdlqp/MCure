// Define key
#define EMERGENCY_STOP    0x01
#define POWER_KEY         0x02
#define MOTOR_WORK        0x04
#define BLUETOOTH         0x08

// Define LED
#define LED_EMERGENCY     28
#define LED_POWER         14
#define LED_MOTOR         30
#define LED_BLUETOOTH     31

#define LED_BAT_ICO       15
#define LED_BAT_STATE1    39
#define LED_BAT_STATE2    25
#define LED_BAT_STATE3    29

#define LED_MOTOR_ICO     6
#define LED_MOTOR_STATE1  23
#define LED_MOTOR_STATE2  22
#define LED_MOTOR_STATE3  16
#define LED_MOTOR_STATE4  17

#define LED_TEMP_DANGER   1
#define LED_TEMP_NORMAL   0

// Define temp control
#define PELTIER_PORT      35
#define PELTIER_FAN       41

// Define PSI
#define AIRPUMP_PORT      37
#define SOLENOID_PORT     38

// Define ENCODER Motor
#define MOTOR_PORT_F  32
#define MOTOR_PORT_R  33
#define ENCODER_A     3
#define ENCODER_B     2

// Define Buzzer
#define BUZZER_PWM    9
#define BUZZER_POWER  24

// Define Touch senst
#define TOUCH_DIO     20
#define TOUCH_CLK     21

// Define Ble
#define BLE_TX        18
#define BLE_RX        19

// Define Anoalg
#define READ_TEMP     A1
#define READ_PSI      A2
#define READ_MOTOR    A3
#define READ_CHARGE   A4
