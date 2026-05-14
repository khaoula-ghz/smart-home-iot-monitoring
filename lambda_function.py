import boto3
import json
import time 

dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('IoTSensorData') 

def lambda_handler(event, context):
    # Extract data from the incoming IoT event
    try:
        temperature = event['temperature']
        humidity = event['humidity']
        smoke = event['smoke']

        # Create a dictionary for the item to be stored
        data = {
            'timestamp': str(int(time.time())),  # Unique primary key
            'Temperature': temperature,
            'Humidity': humidity,
            'Smoke': smoke
        }

        # Store the data in the DynamoDB table
        table.put_item(Item=data)
        print(f"Data stored successfully: {data}")

    except Exception as e:
        print(f"Error storing data in DynamoDB: {str(e)}")
        raise e
