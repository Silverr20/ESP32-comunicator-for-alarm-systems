#include <WiFi.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Wifi network station credentials
struct WifiCredential {
    const char* ssid;
    const char* password;
};

// Define global variables
String updateChatId;

WifiCredential credentials[] = {
    {"WIFI1", "password1"},
    {"wifi2", "password2"},
    // Add more networks if needed
};

// Telegram BOT Token (Get from Botfather)
#define BOT_TOKEN "3333333333:AAEEFVC............................" //insert your bot token
#define CHAT_ID "-1111111111" //insert your chat id


#define ALARM_PIN 2
const int ARM_DISARM_PIN = 23;
#define STATUS_PIN 4
const int LED_PIN = 2;

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;
unsigned long wifi_lost_time = 0;

WiFiClient client;

enum UpdateState {
    WAITING_FOR_COMMAND,
    WAITING_FOR_UPDATE_LINK
};

UpdateState updateState = WAITING_FOR_COMMAND;
String updateURL;

void connectToWiFi() {
    for (size_t i = 0; i < sizeof(credentials) / sizeof(credentials[0]); i++) {
        Serial.print("Connecting to ");
        Serial.println(credentials[i].ssid);

        WiFi.begin(credentials[i].ssid, credentials[i].password);
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 10) {
            delay(1000);
            Serial.println(WiFi.status());
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi connected");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            break; 
        } else {
            Serial.println("\nConnection failed");
        }
    }
}

String alarmStatus = "";
String actionStatus = ""; 

void handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
        if (!bot.messages[i].chat_title.isEmpty() && (bot.messages[i].chat_title != bot.messages[i].from_name)) {
            if (bot.messages[i].text == "Info") {
                String responseMessage = "You can use the following commands: 'Disarm' for disarming the alarm system; "
                                        "'Arming' for arming the alarm system; 'Status' to check if the system is armed or disarmed; "
                                        "The device will send 'Device connected' each time is started and conected to internet via wifi. If the system does not answer to the commands, it is probably offline. ";
                bot.sendMessage(bot.messages[i].chat_id, responseMessage);
            } else if (bot.messages[i].text == "Version") {
                bot.sendMessage(bot.messages[i].chat_id, "Version 2.5");
            } else if (bot.messages[i].text == "Arming") {
                digitalWrite(ARM_DISARM_PIN, HIGH);
                digitalWrite(LED_PIN, HIGH);
                 delay(5000);
                digitalWrite(ARM_DISARM_PIN, LOW);
                digitalWrite(LED_PIN, LOW);
                actionStatus = "Arming...";
                bot.sendMessage(bot.messages[i].chat_id,  actionStatus);
            } else if (bot.messages[i].text == "Disarm") {
                digitalWrite(ARM_DISARM_PIN, HIGH);
                digitalWrite(LED_PIN, HIGH);
                 delay(5000);
                 digitalWrite(LED_PIN, LOW);
                digitalWrite(ARM_DISARM_PIN, LOW);
                actionStatus = "Disarming...";
                bot.sendMessage(bot.messages[i].chat_id,  actionStatus);
            } else if (bot.messages[i].text == "Status") {
                // Check the status pin
                if (digitalRead(STATUS_PIN) == HIGH) {
                    // If status pin is high, send "Sistem Armat" message
                    bot.sendMessage(bot.messages[i].chat_id, "System Armed");
                } else {
                    // If status pin is low, send "Sistem Dezarmat" message
                    bot.sendMessage(bot.messages[i].chat_id, "System Disarmed");
                }
            } 
        }
    }
}

void sendConnectionMessage() {
    Serial.println("Sending connection message...");
    bot.sendMessage(CHAT_ID, "Device connected!");
    Serial.println("Connection message sent.");
}


void sendAlarmMessage() {
    Serial.println("Sending alarm message...");
    for (int i = 0; i < 3; i++) {
        bot.sendMessage(CHAT_ID, "Intruder alert!");
        delay(1000);
    }
    Serial.println("Alarm message sent.");
}

void sendStatusMessage() {
    bot.sendMessage(CHAT_ID, "Sistem " + alarmStatus);
    
}



void sendWifiRestoredMessage(unsigned long wifiLostTime) {
    // Make an HTTP GET request to the World Time API
    HTTPClient http;
    http.begin("https://worldtimeapi.org/api/ip");
    int httpResponseCode = http.GET();

    if (httpResponseCode == HTTP_CODE_OK) {
        // If the request was successful, parse the response
        String payload = http.getString();

        // Parse the JSON response
        DynamicJsonDocument jsonDoc(1024);
        DeserializationError error = deserializeJson(jsonDoc, payload);

        if (error) {
            Serial.println("Error parsing JSON");
            return;
        }

        // Extract the "datetime" field and parse it to get the time
        const char* dateTime = jsonDoc["datetime"]; // Get the datetime string
        String timeStr = String(dateTime).substring(11, 19); // Extract the time part

        // Send the "WiFi connection restored" message along with the retrieved time
        String message = "WiFi connection restored at " + timeStr;
        bot.sendMessage("-4159354497", message);
    } else {
        Serial.println("Error retrieving time from World Time API");
    }
}


void sendWifiLostMessage(unsigned long wifiLostTime) {
    // Make an HTTP GET request to the World Time API
    HTTPClient http;
    http.begin("https://worldtimeapi.org/api/ip");
    int httpResponseCode = http.GET();

    if (httpResponseCode == HTTP_CODE_OK) {
        // If the request was successful, parse the response
        String payload = http.getString();

        // Parse the JSON response
        DynamicJsonDocument jsonDoc(1024);
        DeserializationError error = deserializeJson(jsonDoc, payload);

        if (error) {
            Serial.println("Error parsing JSON");
            return;
        }

        // Extract the "datetime" field and parse it to get the time
        const char* dateTime = jsonDoc["datetime"]; // Get the datetime string
        String timeStr = String(dateTime).substring(11, 19); // Extract the time part

        // Send the "WiFi connection restored" message along with the retrieved time
        String message = "WiFi connection lost at " + timeStr;
        bot.sendMessage("-4159354497", message);
    } else {
        Serial.println("Error retrieving time from World Time API");
    }
}

void logWifiLostTime(unsigned long wifiLostTime) {
    // Get the current time
    time_t now = time(nullptr);
    struct tm* timeinfo;
    timeinfo = localtime(&now);
    char timeBuffer[10];
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", timeinfo);
    
    // Log the time when WiFi connection was lost
    Serial.print("WiFi connection lost at ");
    Serial.println(timeBuffer);
}

void sendWifiLostMessageAndLogTime(unsigned long wifiLostTime) {
    // Log the time when WiFi connection was lost
    logWifiLostTime(wifiLostTime);
    
    // Send the message when WiFi connection is reestablished
    sendWifiLostMessage(wifiLostTime);
}






void setup() {
    Serial.begin(115200);
    Serial.println();

    pinMode(ALARM_PIN, INPUT);
    pinMode(ARM_DISARM_PIN, OUTPUT);
    pinMode(STATUS_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    //digitalWrite(ARM_DISARM_PIN, LOW);

    Serial.print("Connecting to Wifi SSID ");
    connectToWiFi();
    secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); 
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.print("\nWiFi connected. IP address: ");
    Serial.println(WiFi.localIP());

    sendConnectionMessage();

    Serial.print("Retrieving time: ");
    configTime(0, 0, "pool.ntp.org");
    time_t now = time(nullptr);
    while (now < 24 * 3600) {
        Serial.print(".");
        delay(100);
        now = time(nullptr);
    }
    Serial.println(now);

    bot_lasttime = millis();
}

void loop() {
  


    // Check if the alarm pin is HIGH
    if (digitalRead(ALARM_PIN) == HIGH) {
        // Send the alarm message
        sendAlarmMessage();
        delay(2000); // Delay for 2 seconds to avoid continuous messages (adjust as needed)
    }

    // Check WiFi status
    if (WiFi.status() != WL_CONNECTED) {
        // WiFi connection is lost
        if (wifi_lost_time == 0) {
            // Log the time when WiFi connection was lost
            wifi_lost_time = millis();
            Serial.println("WiFi connection lost.");
        }
        // Attempt to reconnect to WiFi every 5 minutes
        if (millis() - wifi_lost_time >= 300000) {
            Serial.println("Attempting to reconnect to WiFi...");
            connectToWiFi();
            if (WiFi.status() == WL_CONNECTED) {
                // WiFi connection reestablished, send message
                sendConnectionMessage();
                // Reset the WiFi lost time
                wifi_lost_time = 0;
            }
        }
    } else {
        // WiFi connection is established
        if (wifi_lost_time != 0) {
            // WiFi connection was lost previously, send message with time
            sendWifiRestoredMessage(wifi_lost_time);
            //sendWifiLostMessageAndLogTime(wifi_lost_time);

            // Reset the WiFi lost time
            wifi_lost_time = 0;
        }
    }

    


    // Check for new messages
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
        Serial.println("got response");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    // Delay for a short time to avoid excessive checking
    delay(1000);

    // Uncomment the line below if you want to check for new messages more frequently
    // delay(100);
}

