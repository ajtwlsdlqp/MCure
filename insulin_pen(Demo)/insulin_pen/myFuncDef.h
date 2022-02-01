void Key_Read_A(void);
void Key_Scan_A(void);
void Key_Proc_A(void);

void Key_Read_B(void);
void Key_Scan_B(void);
void Key_Proc_B(void);

void updateTemperatrue (void);

void updatePSI (void);

void readEEPROM (void);
void updateEEPROM (void);

void updateMotor(void);
void updateMotorStopPos(void);
void A_CHANGE();  //Interrupt function to read the x2 pulses of the encoder.
