#include <ctime>

void timeHelper_setup();
void timeHelper_afterWiFiConnected();
void timeHelper_update();
bool timeHelper_realTimeAvailable();
void getLocalTimeStr(char* localTimeStr);
std::time_t getEpochTime(const std::string& strDate, const char* format);