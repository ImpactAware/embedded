/* Dependencies */
#include <Arduino.h>
#include <painlessMesh.h>

/* Definitions */
#define MESSAGE_DELAY 1
#define SSID "BombSensors"
#define PASS "1amp0ilr0peb0mb5"
#define PORT 5555

/* Function Prototypes */
void broadcastReport();
void waitingBlink();
void readyBlink();
void incrementCounter();
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void droppedConnectionCallback(uint32_t nodeId);
void nodeTimeAdjustedCallback(int32_t offset);

/* Global Variables */
int triggers = 0;

/* Constructors */
painlessMesh sensorMesh;
Scheduler userScheduler;
Task reportTask(TASK_SECOND * MESSAGE_DELAY, TASK_FOREVER, &broadcastReport);
Task waitingBlinkTask(TASK_SECOND, TASK_FOREVER, &waitingBlink, &userScheduler, true, NULL, &readyBlink);

void setup()
{
    Serial.begin(115200);
    pinMode(SENSOR_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    // Increment counter upon sensor trigger
    attachInterrupt(SENSOR_PIN, incrementCounter, RISING);

    // Create mesh network for boards
    // Set debug before init() so that you can see startup messages
    sensorMesh.setDebugMsgTypes( ERROR | STARTUP );
    sensorMesh.init(SSID, PASS, &userScheduler, PORT);

    sensorMesh.onReceive(&receivedCallback);
    sensorMesh.onNewConnection(&newConnectionCallback);
    sensorMesh.onDroppedConnection(&droppedConnectionCallback);

    // Add reporting task
    userScheduler.addTask(reportTask);
    reportTask.enable();
}

void loop()
{
    sensorMesh.update();
}

/* Interrupt Handler */
void incrementCounter()
{
    triggers += 1;
}

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