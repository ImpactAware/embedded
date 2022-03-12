#include <Arduino.h>
#include <painlessMesh.h>

#define MESSAGE_DELAY 1

#define SSID "BombSensors"
#define PASS "1amp0ilr0peb0mb5"

void broadcastReport();
void waitingBlink();
void readyBlink();
void incrementCounter();
void recievedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void droppedConnectionCallback(uint32_t nodeId);
void nodeTimeAdjustedCallback(int32_t offset);

painlessMesh sensorMesh;

Scheduler userScheduler;

int triggers = 0;

void incrementCounter()
{
    triggers += 1;
}

Task reportTask(TASK_SECOND * MESSAGE_DELAY, TASK_FOREVER, &broadcastReport);

void broadcastReport()
{
    if (triggers > 0)
    {
        String msg = "VIBR ";
        msg += triggers;

        sensorMesh.sendBroadcast(msg, true);
    }

    triggers = 0;
}

Task waitingBlinkTask(TASK_SECOND, TASK_FOREVER, &waitingBlink, &userScheduler, true, NULL, &readyBlink);

// Waiting for connection to network
void waitingBlink() {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

// Connection established
void readyBlink() {
    for (char i = 0; i < 3; i++)
    {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
}

void receivedCallback(uint32_t from, String &msg)
{
    if (msg.startsWith("VIBR")) {
        Serial.printf("%s %u\n", msg.c_str(), from);
    }
}

void newConnectionCallback(uint32_t nodeId)
{
    Serial.printf("CONN %u\n", nodeId);

    // Stop LED blinking
    if (waitingBlinkTask.isEnabled())
        waitingBlinkTask.disable();
    
}

void droppedConnectionCallback(uint32_t nodeId)
{
    Serial.printf("DCON %u\n", nodeId);

    // Start LED blinking again if the last node disconnects
    if (sensorMesh.getNodeList().size() == 0)
        waitingBlinkTask.enable();
    
}

// void nodeTimeAdjustedCallback(int32_t offset)
// {
//     Serial.printf("Adjusted time %u. Offset = %d\n", sensorMesh.getNodeTime(), offset);
// }

void setup()
{
    Serial.begin(115200);
    pinMode(SENSOR_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    // Increment counter upon sensor trigger
    attachInterrupt(SENSOR_PIN, incrementCounter, RISING);

    // Create mesh network for boards
    sensorMesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages
    sensorMesh.init(SSID, PASS, &userScheduler, 5555);

    sensorMesh.onReceive(&receivedCallback);
    sensorMesh.onNewConnection(&newConnectionCallback);
    sensorMesh.onDroppedConnection(&droppedConnectionCallback);
    // sensorMesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

    // Add reporting task
    userScheduler.addTask(reportTask);
    reportTask.enable();
}

void loop()
{
    sensorMesh.update();
}