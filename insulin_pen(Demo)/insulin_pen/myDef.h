// Define OLED
#define MOSI   51
#define CLK    52

#define DC1    36
#define CS1    37
#define RST1   35

#define DC2    32
#define CS2    33
#define RST2   31

#define DC3    28
#define CS3    29
#define RST3   27

#define  BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

#define TEMP_UP     0x01
#define TEMP_DN     0x02

#if PEN_VERSION
#define MOTOR_MANUAL 0x04
#define MOTOR_SAVE   0x08
#define MOTOR_TIME   0x10
#define MOTOR_WORK   0x0C
#else
#define MOTOR_UP   0x04
#define MOTOR_DN   0x08
#define MOTOR_BACK   0x10
#define MOTOR_WORK   0x0C
#endif

#define PSI_UP      0x20
#define PSI_DN      0x40
#define PSI_STOP     0x80
#define PSI_WORK    0x60

// Define Key Read
#define KeySet1 A1
#define KetSet2 A2

// Define MAX6675 Temp sensor
#define TEMP_DO   22
#define TEMP_CS   23
#define TEMP_CLK  24

// Define temp control I/O
#define PTR_PORT_A    48
#define PTR_PORT_B    47
#define PTR_PORT_FAN  46

// Define PSI
#define PUMP_PORT     41
#define SOLENOID      40

// Define ENCODER Motor
#define MOTOR_PORT_A  43
#define MOTOR_PORT_B  44
#define ENCODER_A     3
#define ENCODER_B     2

// Define OLED
#define CHARGE_ENABLE 13
#define CHARGE_ON     8
// Define OLED
// Define OLED
