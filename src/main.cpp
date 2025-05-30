#include <Arduino.h>
#include <IO7F32.h>  // io7 전용 라이브러리

char* ssid_pfix = (char*)"IOTMotion";

const int trigPin = 15;
const int echoPin = 2;

float prevDistance = 0;
float threshold = 10.0;  // 감지 기준: 거리 변화가 10cm 이상이면 감지
unsigned long lastMillis = 0;
unsigned long sendInterval = 1000;  // 최소 전송 주기(ms)

// 초음파 거리 측정 함수
float measureDistance();

void setup() {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    initDevice();  // IO7용 초기화
    WiFi.mode(WIFI_STA);
    WiFi.begin((const char*)cfg["ssid"], (const char*)cfg["w_pw"]);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    set_iot_server();  // IO7 플랫폼 MQTT 서버 설정
    iot_connect();     // MQTT 연결

    // 이전 거리 초기화
    prevDistance = measureDistance();
}

void loop() {
    if (!client.connected()) {
        iot_connect();
    }
    client.loop();

    float distance = measureDistance();

    if (abs(distance - prevDistance) > threshold && millis() - lastMillis > sendInterval) {
        StaticJsonDocument<256> doc;
        JsonObject d = doc.createNestedObject("d");

        d["motion"] = "detected";
        d["distance"] = String(distance, 1);  // 소수점 1자리

        serializeJson(doc, msgBuffer);
        client.publish(evtTopic, msgBuffer);

        Serial.print("Motion detected! Distance: ");
        Serial.print(distance, 1);
        Serial.println(" cm");

        lastMillis = millis();
        prevDistance = distance;
    }

    delay(100);  // 측정 간격
}

// 초음파 거리 측정 함수
float measureDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, 30000);  // 타임아웃 30ms
    return duration * 0.017;  // cm
}
