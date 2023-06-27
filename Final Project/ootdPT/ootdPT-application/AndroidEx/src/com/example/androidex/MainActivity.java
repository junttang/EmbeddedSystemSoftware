package com.example.androidex;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.IBinder;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.text.SimpleDateFormat;
import java.util.Date;

public class MainActivity extends Activity {

    private LinearLayout linearLayout;
    private TurnOnOffThread turnOnOffThread;
    private MyService myService;
    private boolean isServiceBound = false;

    class Args {
        String date;
        int humid;
        double tempCelsius;
        String weatherDescription;
    }

    public native static int openDevice(Args args);
    public native static void closeDevice(int fd);

    private int fd;

    static {
        System.loadLibrary("dev_driver");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        linearLayout = (LinearLayout) findViewById(R.id.container);

        // Bind to the service
        Intent serviceIntent = new Intent(this, MyService.class);
        bindService(serviceIntent, serviceConnection, Context.BIND_AUTO_CREATE);

        // Call the method to fetch weather data
        fetchWeatherData();

        // Set up the button click listener
        Button openFileButton = (Button) findViewById(R.id.openFileButton);
        openFileButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openFile();
            }
        });
    }

    private String getCurrentDate() {
        SimpleDateFormat dateFormat = new SimpleDateFormat("MMdd");
        Date currentDate = new Date();
        return dateFormat.format(currentDate);
    }

    private void fetchWeatherData() {
        String openWeatherMapApiKey = "ADD_API_KEY_HERE";
        String city = "Seoul";

        String apiUrl = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + openWeatherMapApiKey;

        WeatherTask weatherTask = new WeatherTask();
        weatherTask.execute(apiUrl);
    }

    private class WeatherTask extends AsyncTask<String, Void, String> {

        @Override
        protected String doInBackground(String... urls) {
            try {
                URL url = new URL(urls[0]);
                HttpURLConnection connection = (HttpURLConnection) url.openConnection();
                connection.connect();

                InputStream inputStream = connection.getInputStream();
                BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
                StringBuilder result = new StringBuilder();
                String line;
                while ((line = reader.readLine()) != null) {
                    result.append(line);
                }

                return result.toString();

            } catch (IOException e) {
                e.printStackTrace();
                return null;
            }
        }

        @Override
        protected void onPostExecute(String result) {
            super.onPostExecute(result);
            if (result != null) {
                try {
                    JSONObject json = new JSONObject(result);
                    JSONObject main = json.getJSONObject("main");
                    double temperature = main.getDouble("temp");
                    int humidity = main.getInt("humidity");
                    String weatherDescription = json.getJSONArray("weather").getJSONObject(0).getString("description");

                    double temperatureCelsius = temperature - 273.15; // Convert temperature from Kelvin to Celsius
                    int roundedTemperature = (int) Math.round(temperatureCelsius); // Round the temperature to the nearest integer

                    Args temp = new Args();
                    temp.date = getCurrentDate();
                    temp.humid = humidity;
                    temp.tempCelsius = temperatureCelsius;
                    temp.weatherDescription = weatherDescription;
                    fd = openDevice(temp); // Open the driver with humidity, temperature, and weather information

                    // Call the method to get outfit recommendations
                    recommendOutfit(weatherDescription, roundedTemperature, humidity);

                } catch (JSONException e) {
                    e.printStackTrace();
                    showErrorToast("JSON parsing error");
                }
            } else {
                showErrorToast("Failed to fetch weather data");
            }
        }
    }

    private void recommendOutfit(String weatherDescription, int temperature, int humidity) {
        // Perform outfit recommendation based on weather description, temperature, and humidity
        String outfit;

        if (weatherDescription.contains("rain")) {
            if (temperature < 10) {
                outfit = "Raincoat, boots, and a warm jacket";
            } else if (temperature < 15) {
                outfit = "Raincoat and boots";
            } else {
                outfit = "Rain jacket and waterproof shoes";
            }
        } else if (weatherDescription.contains("cloud") || weatherDescription.contains("overcast") || weatherDescription.contains("mist")) {
            if (temperature < 10) {
                outfit = "Light jacket, pants, and a sweater";
            } else if (temperature < 15) {
                outfit = "Light jacket and pants";
            } else if (temperature < 20) {
                if (humidity < 50) {
                    outfit = "Long-sleeved shirt and jeans";
                } else {
                    outfit = "Long-sleeved shirt, jeans, and a light sweater";
                }
            } else {
                if (humidity < 50) {
                    outfit = "T-shirt, jeans, and a light jacket";
                } else {
                    outfit = "T-shirt, jeans, light jacket, and an umbrella";
                }
            }
        } else if (weatherDescription.contains("clear")) {
            if (temperature > 30) {
                if (humidity < 40) {
                    outfit = "T-shirt, shorts, sunglasses, and sunscreen";
                } else {
                    outfit = "T-shirt, shorts, sunglasses, sunscreen, and a hat";
                }
            } else if (temperature > 25) {
                if (humidity < 60) {
                    outfit = "T-shirt, shorts, sunglasses, and a hat";
                } else {
                    outfit = "T-shirt, shorts, and a light hat";
                }
            } else if (temperature > 20) {
                if (humidity < 70) {
                    outfit = "T-shirt, shorts, and a light jacket";
                } else {
                    outfit = "T-shirt, shorts, light jacket, and a small fan";
                }
            } else if (temperature > 15) {
                outfit = "T-shirt and shorts";
            } else {
                outfit = "T-shirt and jeans";
            }
        } else {
            outfit = "Outfit recommendation not available";
        }

        // Display the outfit recommendation
        addOutfitRecommendation(outfit);
    }

    private void addOutfitRecommendation(String outfitRecommendation) {
        TextView textView = new TextView(this);
        textView.setText(outfitRecommendation);
        textView.setTextColor(getResources().getColor(android.R.color.white));
        textView.setTextSize(42);
        textView.setTypeface(null, Typeface.ITALIC);
        textView.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));

        linearLayout.addView(textView);
    }

    private void openFile() {
        String filePath = "/data/local/tmp/ootdPT_gpt.txt"; // Path to the txt file
        File file = new File(filePath);

        try {
            FileInputStream fis = new FileInputStream(file);
            InputStreamReader isr = new InputStreamReader(fis);
            BufferedReader br = new BufferedReader(isr);

            StringBuilder stringBuilder = new StringBuilder();
            String line;
            while ((line = br.readLine()) != null) {
                stringBuilder.append(line).append("\n");
            }

            br.close();

            // Display the contents of the text file
            TextView textView = (TextView) findViewById(R.id.textViewFileContents);
            textView.setText(stringBuilder.toString());

        } catch (IOException e) {
            e.printStackTrace();
            showErrorToast("Error opening file");
        }
    }

    private void showErrorToast(String errorMessage) {
        Toast.makeText(this, errorMessage, Toast.LENGTH_SHORT).show();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        // Stop the turn on/off thread
        if (turnOnOffThread != null) {
            turnOnOffThread.stopThread();
        }

        // Unbind from the service
        if (isServiceBound) {
            unbindService(serviceConnection);
            isServiceBound = false;
        }
        
        closeDevice(fd);
    }

    private void startMyService() {
        // Start the bound service
        Intent intent = new Intent(this, MyService.class);
        bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE);
    }

    private void stopMyService() {
        // Stop the bound service
        unbindService(serviceConnection);
    }

    private ServiceConnection serviceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            MyService.MyBinder binder = (MyService.MyBinder) service;
            myService = binder.getService();
            isServiceBound = true;

            // Start the turn on/off thread after the service is bound
            turnOnOffThread = new TurnOnOffThread(myService);
            turnOnOffThread.start();
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            // This method is called when the service is disconnected
        }
    };
}
