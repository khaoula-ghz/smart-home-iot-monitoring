from flask import Flask, render_template
import boto3
import time

app = Flask(__name__)

# Initialize CloudWatch client
cloudwatch_logs = boto3.client('logs', region_name='us-east-1')  # Replace with your AWS region

# Define log group and log stream
log_group = '/aws/lambda/ProcessIoTData'

# Fetch CloudWatch logs
def fetch_cloudwatch_logs():
    try:
        # Get logs for a specific log stream (you can adjust this to your needs)
        response = cloudwatch_logs.filter_log_events(
            logGroupName=log_group,
            limit=10,  # Limit number of logs to retrieve
            startTime=int(time.time() - 3600) * 1000,  # Filter logs from the past hour
            endTime=int(time.time()) * 1000
        )
        
        logs = response['events']
        log_data = []
        
        # Process the log data (assuming logs contain JSON-like structure)
        for event in logs:
            log_message = event['message']
            timestamp = event['timestamp']
            # Assuming your logs are structured in JSON format, you can parse them like so:
            log_data.append({
                'message': log_message,
                'timestamp': time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime(timestamp / 1000))  # Convert timestamp
            })

        return log_data

    except Exception as e:
        print(f"Error fetching logs: {e}")
        return []

@app.route('/')
def index():
    # Fetch logs from CloudWatch
    logs = fetch_cloudwatch_logs()

    return render_template('index.html', logs=logs)

if __name__ == "__main__":
    app.run(debug=True)
