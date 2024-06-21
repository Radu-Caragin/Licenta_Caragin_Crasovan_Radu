package com.example.bt_recieve_and;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import java.io.IOException;
import java.io.InputStream;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "ProjLicenta";
    private static final String deviceAddress = "00:22:09:01:0D:79";
    private static final UUID deviceUUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    private static final int REQUEST_BLUETOOTH_PERMISSIONS = 1;

    private BluetoothDevice bluetoothDevice;
    private BluetoothAdapter bluetoothAdapter;
    private BluetoothSocket bluetoothSocket;
    private InputStream inputStream;
    private Thread readThread;
    private boolean isReading;

    private Button buttonConnect;
    private TextView textViewData;
    private Handler handler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        buttonConnect = findViewById(R.id.button_connect);
        textViewData = findViewById(R.id.text_view_data);

        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        handler = new Handler(Looper.getMainLooper());

        buttonConnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                checkPermissionsAndConnect();
            }
        });
    }

    private void checkPermissionsAndConnect() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH) != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_ADMIN) != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {

            ActivityCompat.requestPermissions(this, new String[] {
                    Manifest.permission.BLUETOOTH,
                    Manifest.permission.BLUETOOTH_ADMIN,
                    Manifest.permission.BLUETOOTH_SCAN,
                    Manifest.permission.BLUETOOTH_CONNECT,
                    Manifest.permission.ACCESS_FINE_LOCATION
            }, REQUEST_BLUETOOTH_PERMISSIONS);
        } else {
            connectToBluetoothDevice();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_BLUETOOTH_PERMISSIONS) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                connectToBluetoothDevice();
            } else {
                Log.e(TAG, "Permissions denied");
            }
        }
    }

    @SuppressLint("MissingPermission")
    private void connectToBluetoothDevice() {
        bluetoothDevice = bluetoothAdapter.getRemoteDevice(deviceAddress);
        try {
            bluetoothSocket = bluetoothDevice.createRfcommSocketToServiceRecord(deviceUUID);
            bluetoothSocket.connect();
            inputStream = bluetoothSocket.getInputStream();
            readData();
        } catch (IOException e) {
            Log.e(TAG, "Error connecting", e);
        }
    }

    private void readData() {
        isReading = true;
        readThread = new Thread(() -> {
            final byte[] buffer = new byte[1024];
            int bufferPosition = 0;

            while (isReading) {
                try {
                    if (inputStream.read(buffer, bufferPosition, 1) > 0) {
                        if (buffer[bufferPosition] == '$') {
                            final String readMessage = new String(buffer, 0, bufferPosition).trim();
                            bufferPosition = 0;
                            handler.post(() -> textViewData.setText(readMessage));
                            Thread.sleep(1000);
                        } else {
                            bufferPosition++;
                        }
                    }
                } catch (IOException | InterruptedException e) {
                    Log.e(TAG, "Error reading data", e);
                    isReading = false;
                }
            }
        });
        readThread.start();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        isReading = false;
        if (readThread != null) {
            readThread.interrupt();
        }
        try {
            bluetoothSocket.close();
        } catch (IOException e) {
            Log.e(TAG, "Error socket", e);
        }
    }
}