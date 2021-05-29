extern unsigned long co2_value;
extern unsigned long co2_status;
extern unsigned long co2_ABCperiod;
/*
0: no status
1: couldn't start calibration
2: calibration started, awaiting calibration acknowledgement
3: acknowledgement received
*/
extern byte calibrationStatus;

void co2_setup();
void co2_requestValue();
void co2_requestStatus();
void co2_requestValueAndStatus();
void co2_requestABCperiod();

bool co2_clearBackgroundCalibrationAck();
bool co2_startBackgroundCalibration();
bool co2_checkBackgroundCalibrationAck();
