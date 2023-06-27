# -*- coding: utf-8 -*-
import requests
import openai
import json
import subprocess
import time

# OpenWeatherMap API key
weather_api_key = 'ADD_API_KEY'

# ChatGPT API key
chatgpt_api_key = 'ADD_API_KEY'

def get_weather(city):
    url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + weather_api_key + "&units=metric"
    response = requests.get(url)
    data = json.loads(response.text)
    temperature = data['main']['temp']
    description = data['weather'][0]['description']
    return temperature, description

def get_outfit_recommendation(temperature):
    if temperature < 10:
        return "It's cold outside. Wear a warm coat, hat, and gloves."
    elif temperature < 20:
        return "It's cool outside. Wear a light jacket or sweater."
    else:
        return "It's warm outside. Wear something light and comfortable."

def create_outfit_file(outfit_suggestions):
    with open('ootdPT_gpt.txt', 'w') as file:
        file.write(outfit_suggestions)
    print("Outfit suggestions saved to ootdPT_gpt.txt")

def push_file_to_device():
    subprocess.run(['adb', 'push', 'ootdPT_gpt.txt', '/data/local/tmp'])
    print("File pushed to the FPGA device.")

# Main program loop
while True:
    # Get weather information for Seoul
    city = "Seoul"
    temperature, description = get_weather(city)
    print("Weather in {}: {}, Temperature: {}°C".format(city, description, temperature))

    # Get outfit recommendation based on weather information
    outfit_recommendation = get_outfit_recommendation(temperature)
    print("Outfit Recommendation:", outfit_recommendation)

    # Call ChatGPT API to generate outfit suggestions
    openai.api_key = chatgpt_api_key

    question = "I need outfit suggestions for the {} weather in {}.".format(description, city)

    response = openai.Completion.create(
        model="text-davinci-003",
        prompt=question,
        max_tokens=2048
    )

    outfit_suggestions = response.choices[0].text.strip()
    print("Outfit Suggestions:", outfit_suggestions)

    # Save the outfit suggestions to a file
    create_outfit_file(outfit_suggestions)

    # Push the file to the FPGA device
    push_file_to_device()

    # Wait for 5 seconds before repeating the loop
    time.sleep(5)

