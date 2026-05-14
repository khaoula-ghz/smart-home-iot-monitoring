# Use an official Python runtime as a parent image
FROM python:3.9-slim

# Set the working directory in the container
WORKDIR /app

# Copy the current directory contents into the container at /app
COPY . /app

# Install required Python dependencies
RUN pip install boto3 flask

# Expose port 5000 for Flask application (if needed)
EXPOSE 5000

# Define the environment variable for the application
ENV FLASK_APP=src/main.py

# Command to run the Flask app
CMD ["python", "src/main.py"]
