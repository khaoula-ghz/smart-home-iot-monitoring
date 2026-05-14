# Smart Home Environmental Monitoring System (Cloud & IoT)

A complete end-to-end IoT and cloud computing project simulating a smart home environmental monitoring system — collecting temperature, humidity, and smoke data through ESP32 sensors, transmitting via MQTT, and routing through a full AWS serverless pipeline for storage, processing, and real-time visualization.

> **Academic project** — University Constantine 2 – Abdelhamid Mehri | Cloud & IoT Mini Project | January 2025

---

## Project Structure

```
smart-home-iot/
│
├── src/
    ├── main.cpp                 # ESP32 Arduino firmware (sensor reading + MQTT publish)
│   └── main.py                  # Flask backend — receives sensor data, publishes to CloudWatch
├── lambda_function.py           # AWS Lambda — processes IoT messages, stores to DynamoDB
├── Dockerfile                   # Container configuration
├── requirements.txt             # Python dependencies
├── platformio.ini               # ESP32 PlatformIO configuration
├── diagram.json                 # Wokwi circuit diagram
├── wokwi.toml                   # Wokwi simulation config
├── report/
│   └── Smart_Home_Environmental_Monitoring_System.pdf   # Full project report
└── README.md
```

---

## Project Overview

The system simulates a smart home IoT network capable of:
- Collecting real-time sensor data (temperature, humidity, smoke) via ESP32
- Transmitting data securely via MQTT over TLS to AWS IoT Core
- Processing data automatically with serverless Lambda functions
- Persisting readings in DynamoDB with a time-series schema
- Publishing custom metrics and triggering alarms via CloudWatch
- Serving a containerized Flask REST API deployed on AWS ECS Fargate

---

## System Architecture — 5 Layers

```
┌─────────────────────────────────────────────────────────────┐
│  EDGE LAYER                                                  │
│  ESP32 (Wokwi) + DHT22 + LDR + LEDs + Buzzer              │
└────────────────────────┬────────────────────────────────────┘
                         │ Wi-Fi
┌────────────────────────▼────────────────────────────────────┐
│  NETWORK LAYER                                               │
│  Cisco Packet Tracer — Router, Switch, Wi-Fi AP, Gateway    │
└────────────────────────┬────────────────────────────────────┘
                         │ MQTT over TLS (port 8883)
┌────────────────────────▼────────────────────────────────────┐
│  INGESTION LAYER                                             │
│  AWS IoT Core — MQTT Broker + X.509 Auth + IoT Rules       │
└──────────┬──────────────────────────────────────────────────┘
           │ Async invoke
┌──────────▼──────────────────────────────────────────────────┐
│  PROCESSING LAYER                                            │
│  AWS Lambda (ProcessIoTData) + AWS ECS Fargate (Flask API)  │
└──────────┬──────────────────────────────────────────────────┘
           │ PutItem / PutMetricData
┌──────────▼──────────────────────────────────────────────────┐
│  STORAGE & VISUALIZATION LAYER                               │
│  DynamoDB (IoTSensorData) + CloudWatch (Metrics/Alarms/Dash)│
└─────────────────────────────────────────────────────────────┘
```

---

## Technologies Used

| Layer | Technology | Purpose |
|---|---|---|
| Network simulation | Cisco Packet Tracer | Smart home network topology |
| Sensor simulation | Wokwi + ESP32 | Environmental sensor simulation |
| Embedded development | PlatformIO + VS Code | ESP32 C++ development environment |
| IoT protocol | MQTT over TLS (port 8883) | Lightweight secure device-to-cloud comms |
| Cloud IoT gateway | AWS IoT Core | Device management + MQTT broker |
| Serverless processing | AWS Lambda (Python) | Event-driven data processing |
| Database | Amazon DynamoDB | NoSQL time-series sensor storage |
| Backend service | Flask (Python) | REST API for CloudWatch metrics |
| Containerization | Docker | Portable microservice runtime |
| Container registry | AWS ECR (AES-256) | Private Docker image registry |
| Orchestration | AWS ECS Fargate | Serverless container deployment |
| Load balancing | AWS Application Load Balancer | HTTP traffic distribution |
| Cloud IDE | AWS Cloud9 | ECR image push environment |
| Monitoring | AWS CloudWatch | Metrics, alarms, dashboards |

---

## ESP32 Sensor Simulation (Wokwi)

### Circuit Components

| Component | Type | GPIO Pin | Purpose |
|---|---|---|---|
| DHT22 | Temp & Humidity Sensor | **GPIO15** (Data) | Measures °C and % humidity |
| Photoresistor (LDR) | Light/Smoke Sensor | **GPIO34** (Analog) | Simulates smoke via light level (< 500 → smoke detected) |
| Red LED | Status Indicator | **GPIO12** | Smoke alarm / warning state |
| Green LED | Status Indicator | **GPIO13** | Normal / no smoke state |
| Buzzer | Audio Alert | **GPIO14** | Audible alarm when smoke detected |
| Blue LED | AC Indicator | **GPIO26** | Activates when temperature > 30°C |
| Orange LED | Heating Indicator | **GPIO25** | Activates when temperature < 10°C |

> **Note on smoke detection:** The Wokwi simulation uses a **photoresistor (LDR)** to simulate the smoke sensor. A low light reading (analog value < 500) triggers the smoke alarm. In a physical deployment, this would be replaced with an MQ-2 gas sensor on the same pin.

### PlatformIO Configuration (`platformio.ini`)

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

lib_deps =
    knolleary/PubSubClient @ ^2.8
    bblanchon/ArduinoJson @ ^6.21.0

monitor_speed = 115200
board_build.partitions = default.csv
```

### MQTT Communication

**Topics:**
```
environment/pub   -- Device publishes sensor readings
environment/sub   -- Device subscribes to control commands
```

**JSON Payload Format:**
```json
{
  "humidity": "65.20",
  "temperature": "27.50",
  "smoke": "No smoke detected."
}
```
 
**Connection:** Mutual TLS on port 8883 with X.509 certificates from AWS IoT Core.  
**Thing name:** `EnvironmentalMonitor`  

### LED Logic Summary
 
```
Smoke detected (LDR < 500)  → Red LED ON + Buzzer ON + Green LED OFF
No smoke                    → Green LED ON + Red LED OFF + Buzzer OFF
Temperature > 30°C          → Blue LED ON  (simulates AC activation)
Temperature < 10°C          → Orange LED ON (simulates heating activation)
10°C ≤ Temperature ≤ 30°C  → Both Blue and Orange LEDs OFF
```
 
---

## AWS Cloud Pipeline

### 1. AWS IoT Core
- IoT Thing: `iotsmarthome`
- Policy: `iotsmarthomePolicy` (least privilege — only `environment/pub` and `environment/sub`)
- Security: X.509 certificate + private key embedded in `secrets.h`

### 2. IoT Rule — `StoreSensorDataToDynamoDB`
```sql
SELECT temperature, humidity, smoke
FROM 'environment/pub'
```
Triggers Lambda on every message received.

### 3. AWS Lambda — `ProcessIoTData`
Python function triggered automatically by the IoT rule:

```python
import boto3, json, time

dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('IoTSensorData')

def lambda_handler(event, context):
    temperature = event['temperature']
    humidity = event['humidity']
    smoke = event['smoke']

    data = {
        'timestamp': str(int(time.time())),
        'Temperature': temperature,
        'Humidity': humidity,
        'Smoke': smoke
    }
    table.put_item(Item=data)
    print(f"Data stored successfully: {data}")
```

**Function flow:** Event parsing → Data validation → Timestamp generation → DynamoDB insert → CloudWatch metrics publish → Return success

### 4. DynamoDB — `IoTSensorData`

| Attribute | Type | Description |
|---|---|---|
| `timestamp` | String (PK) | ISO 8601 time of sensor reading |
| `device_id` | String | Unique device identifier |
| `temperature` | Number | Temperature in °C |
| `humidity` | Number | Relative humidity % |
| `smoke` | String | Smoke detection status |


### 5. Flask Backend (`src/main.py`)

Containerized REST API that receives sensor data and publishes CloudWatch metrics:

```python
from flask import Flask, request, jsonify
import boto3
from datetime import datetime

app = Flask(__name__)
cloudwatch = boto3.client('cloudwatch', region_name='us-east-1')

@app.route('/data', methods=['POST'])
def receive_sensor_data():
    data = request.get_json()
    cloudwatch.put_metric_data(
        Namespace='IoTSensorMetrics',
        MetricData=[
            {'MetricName': 'Temperature', 'Value': data['temperature'],
             'Unit': 'None', 'Timestamp': datetime.utcnow()},
            {'MetricName': 'Humidity', 'Value': data['humidity'],
             'Unit': 'Percent', 'Timestamp': datetime.utcnow()}
        ]
    )
    return jsonify({"status": "success", "message": "Data processed successfully"})
```

### 6. Docker + AWS ECS Fargate

```dockerfile
FROM python:3.12-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install -r requirements.txt
COPY . .
EXPOSE 5000
CMD ["python", "app.py"]
```

**ECS Task:** Fargate, 0.25 vCPU, 0.5 GB RAM, port 5000, AES-256 encryption.
 

---

## CloudWatch Monitoring

### Metrics
- `Temperature` — raw °C via log metric filter `$.temperature`
- `Humidity` — % via log metric filter `$.humidity`
- `Smoke` — binary (0 = no smoke, ≥1 = smoke detected)

### Alarms

| Alarm | Metric | Condition | Threshold | 
|---|---|---|---|---|
| Humidity | Humidity | > threshold, avg, 60s | 50% | 
| Temperature | Temperature | > threshold, min, 60s | 30°C | 
| Smoke-Detection | Smoke | ≥ threshold, max, 30s | 1 | 

**Alarm actions:** SNS email notification on ALARM → CloudWatch Logs recovery on OK.

### Dashboard — `SensorData`
Real-time dashboard with 10-second auto-refresh:
- Temperature line chart (threshold line at 30°C)
- Humidity line chart (threshold line at 50%)
- Smoke status numeric widget (0 or 1)
- Alarm status panel (all 3 alarms)

---

## Security Architecture

- **mTLS:** X.509 certificates for all device-to-cloud communication
- **IAM:** Least-privilege roles for Lambda, ECS, DynamoDB, and CloudWatch
- **Encryption in transit:** TLS 1.2+ on all connections
- **Encryption at rest:** AES-256 on DynamoDB and ECR
- **Container security:** Minimal base image (`python:3.12-slim`), ECR image scanning enabled
- **Credentials**: Never committed — stored in `secrets.h`

---

## Key Takeaways

- Serverless architectures (Lambda + Fargate + DynamoDB on-demand) reduce operational cost for variable IoT workloads
- MQTT's lightweight pub/sub model is ideal for resource-constrained edge devices
- CloudWatch alarms enable proactive anomaly detection without polling
- Wokwi eliminates the need for physical hardware during development and testing
- Certificate-based mTLS is essential for production IoT security

---


## Authors

- **Ghimouze Khaoula**
- Hamlaoui Meryem
- Benrabah Roumaissa
- Kemcha Rania Rym

**Supervisor:** Pr. BELGUIDOUM Meriem  
**Institution:** University Constantine 2 – Abdelhamid Mehri  
**Academic Year:** 2024–2025
