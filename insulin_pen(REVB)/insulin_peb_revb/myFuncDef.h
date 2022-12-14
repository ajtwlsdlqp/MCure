void Key_Read(void);
void Key_Scan(void);
void Key_Proc(void);

void updateTemperatrue (void);
void updatePSI (void);
void updateMotor(void);
void updateLED (void);
void updateBLE (void);

void A_CHANGE();  //Interrupt function to read the x2 pulses of the encoder.

void updateEEPROM (void);
void readEEPROM (void);

void Melody_Proc(void);

void ledOffAll(void);
