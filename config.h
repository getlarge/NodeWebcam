/**********************************
 * Sketch configuration
 */

#define SKETCH_NAME "NodeWebcam"
#define SKETCH_VERSION "1.0"

/**********************************
 * Global configuration
 */
 
 
#define MY_DEBUG
#define MY_BAUD_RATE 115200

#define OTA_BUTTON_PIN D3
#define STATE_LED D4

char devicePass[30]="motdepasse", deviceId[20], devicePrefix[10] = "Camera";
char mqtt_client[60], mqtt_user[20], mqtt_password[30], mqtt_server[40], mqtt_port[6], http_server[40], http_port[6]; 
char mqtt_topic_out[70], mqtt_topic_in[70], out[10]= "-out", in[10]= "-in/#";
char post_prefix[10] = "/cam/", post_destination[70]; 
int mqttPort, httpPort;
const char* mqttServer;
const char* mqttClient;
const char* mqttUser;
const char* mqttPassword;
const char* mqttTopicOut;
const char* mqttTopicIn;
const char *postDestination;
const char *httpServer;
const char* otaUrl = "https://app.getlarge.eu/firmware/NodeWebcam.ino.bin";
const char* currentVersion = "4712";
const char* httpsFingerprint = "1D AE 00 E7 68 70 87 09 A6 1D 27 76 F5 85 C0 F3 AB F2 60 9F"; 

bool resetConfig = false, wifiResetConfig = false; // set to true to reset FS and/or Wifimanager, don't forget to set this to false after
unsigned long configTimeout = 180, lastMqttReconnectAttempt = 0, lastWifiReconnectAttempt = 0;
unsigned long lastUpdate=0, lastRequest=0, lastPic = 0;
bool shouldSaveConfig = false, executeOnce = false, switchOnCam = false, otaSignalReceived = false, manualConfig = false;
int configCount = 0, wifiFailCount = 0, mqttFailCount = 0, configMode = 0, _otaSignal = 0;

// set GPIO16 as the slave select :
const int CS = 16;
// if the video is chopped or distored, try using a lower value for the buffer
// lower values will have fewer frames per second while streaming
static const size_t bufferSize = 2048; // 4096; //2048; //1024;
static uint8_t buffer[bufferSize] = {0xFF}; // new? 

static const int fileSpaceOffset = 700000;
const String otaFile = "ota.txt";
const String fName = "res.txt";
const String f2Name = "fpm.txt";
int fileTotalKB = 0;
int fileUsedKB = 0; 
int fileCount = 0;
String errMsg = "";
int resolution = 4;
int minDelayBetweenframes, interval1 = 5000; 
int fpm = 0;

