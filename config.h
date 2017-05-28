#define BAUD_RATE 115200

#define OTA_BUTTON_PIN D3

bool resetConfig = false; // set to true to reset FS and Wifimanager, don't forget to set this to false after
bool wifiResetConfig= false;

char devicePass[30]="motdepasse", deviceId[20], devicePrefix[10] = "camera";
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
const char* otaUrl = "https://dr.courget.com/firmware/NodeWebcam.ino.bin";

WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
WiFiManagerParameter custom_mqtt_client("client", "mqtt client", mqtt_client, 60);
WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 20);
WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 30);
WiFiManagerParameter custom_http_server("httpServer", "http server", http_server, 40);
WiFiManagerParameter custom_http_port("httpPort", "http port", http_port, 6);

unsigned long lastMqttReconnectAttempt = 0, lastWifiReconnectAttempt = 0, lastNtpReconnectAttempt = 0;
unsigned long lastUpdate=0, lastRequest=0, lastPic = 0;
bool shouldSaveConfig = false, executeOnce = false, switchOnCam = false;
int wifiCount = 0, mqttCount = 0;
// set GPIO16 as the slave select :
const int CS = 16;
// if the video is chopped or distored, try using a lower value for the buffer
// lower values will have fewer frames per second while streaming
static const size_t bufferSize = 2048; // 4096; //2048; //1024;
static uint8_t buffer[bufferSize] = {0xFF}; // new? 

static const int fileSpaceOffset = 700000;
const String fName = "res.txt";
const String f2Name = "fpm.txt";
int fileTotalKB = 0;
int fileUsedKB = 0; 
int fileCount = 0;
String errMsg = "";
int resolution = 4;
int minDelayBetweenframes, interval1 = 5000; 
int fpm = 0;


