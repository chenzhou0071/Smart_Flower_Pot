package com.irrigation.app;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;

public class BluetoothHelper {
    private static final String TAG = "BluetoothHelper";
    private static final UUID SPP_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

    private static BluetoothSocket socket;
    private static InputStream inputStream;
    private static OutputStream outputStream;
    private static long nativePointer;

    // Native callbacks
    public static native void onDataReceived(byte[] data);
    public static native void onConnectionStateChanged(int state);

    public static void startDiscovery(long pointer) {
        nativePointer = pointer;

        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        if (adapter == null) {
            Log.e(TAG, "Bluetooth not available");
            return;
        }

        if (!adapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            // Activity should handle this
            return;
        }

        // Start discovery
        if (adapter.isDiscovering()) {
            adapter.cancelDiscovery();
        }

        adapter.startDiscovery();

        // Register receiver for found devices
        IntentFilter filter = new IntentFilter(BluetoothDevice.ACTION_FOUND);
        // Register this in your activity
    }

    public static void onDeviceFound(BluetoothDevice device) {
        if (device.getName() != null && device.getName().contains("HC-05")) {
            Log.d(TAG, "Found HC-05: " + device.getAddress());
            connectToDevice(device.getAddress(), nativePointer);
        }
    }

    public static void connectToDevice(String macAddress, long pointer) {
        nativePointer = pointer;

        new Thread(() -> {
            try {
                BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
                BluetoothDevice device = adapter.getRemoteDevice(macAddress);

                socket = device.createRfcommSocketToServiceRecord(SPP_UUID);
                socket.connect();

                inputStream = socket.getInputStream();
                outputStream = socket.getOutputStream();

                onConnectionStateChanged(2); // Connected

                // Start reading thread
                startReadingThread();

            } catch (IOException e) {
                Log.e(TAG, "Connection failed", e);
                onConnectionStateChanged(0); // Disconnected
            }
        }).start();
    }

    private static void startReadingThread() {
        new Thread(() -> {
            byte[] buffer = new byte[1024];
            int bytes;

            while (true) {
                try {
                    bytes = inputStream.read(buffer);
                    if (bytes > 0) {
                        byte[] data = new byte[bytes];
                        System.arraycopy(buffer, 0, data, 0, bytes);
                        onDataReceived(data);
                    }
                } catch (IOException e) {
                    Log.e(TAG, "Read error", e);
                    onConnectionStateChanged(0);
                    break;
                }
            }
        }).start();
    }

    public static void sendData(byte[] data) {
        if (outputStream == null) {
            Log.w(TAG, "Not connected");
            return;
        }

        try {
            outputStream.write(data);
            outputStream.flush();
        } catch (IOException e) {
            Log.e(TAG, "Send error", e);
        }
    }

    public static void disconnect() {
        try {
            if (inputStream != null) inputStream.close();
            if (outputStream != null) outputStream.close();
            if (socket != null) socket.close();
        } catch (IOException e) {
            Log.e(TAG, "Disconnect error", e);
        }
    }
}
