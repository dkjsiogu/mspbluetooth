package com.dkjsiogu.mspbluetooth;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Set;
import java.util.UUID;

public class MainActivity extends Activity {
    private static final UUID SPP_UUID =
            UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    private static final int REQUEST_BLUETOOTH_PERMISSION = 430;

    private final ArrayList<DeviceEntry> devices = new ArrayList<>();
    private ArrayAdapter<DeviceEntry> deviceAdapter;
    private Spinner deviceSpinner;
    private TextView stateView;
    private TextView logView;
    private Button connectButton;
    private BluetoothSocket socket;
    private OutputStream outputStream;
    private Thread rxThread;
    private volatile boolean keepReading;

    private static final class DeviceEntry {
        final BluetoothDevice device;
        final String label;

        DeviceEntry(BluetoothDevice device) {
            this.device = device;
            String name = device.getName();
            if (name == null || name.length() == 0) {
                name = "Unknown";
            }
            this.label = name + "  " + device.getAddress();
        }

        @Override
        public String toString() {
            return label;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        buildUi();
        ensureBluetoothPermission();
        refreshBondedDevices();
    }

    @Override
    protected void onDestroy() {
        closeConnection();
        super.onDestroy();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_BLUETOOTH_PERMISSION) {
            refreshBondedDevices();
        }
    }

    private void buildUi() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(16), dp(16), dp(16), dp(16));
        root.setBackgroundColor(0xFFF7F8FA);

        TextView title = new TextView(this);
        title.setText("MSP430 音频播放器");
        title.setTextSize(22);
        title.setTextColor(0xFF111827);
        title.setGravity(Gravity.CENTER_VERTICAL);
        root.addView(title, new LinearLayout.LayoutParams(-1, dp(40)));

        stateView = new TextView(this);
        stateView.setText("未连接");
        stateView.setTextSize(14);
        stateView.setTextColor(0xFF374151);
        root.addView(stateView, new LinearLayout.LayoutParams(-1, dp(28)));

        deviceSpinner = new Spinner(this);
        root.addView(deviceSpinner, new LinearLayout.LayoutParams(-1, dp(48)));

        LinearLayout connectionRow = new LinearLayout(this);
        connectionRow.setOrientation(LinearLayout.HORIZONTAL);
        connectButton = commandButton("连接", 0xFF0F766E);
        connectButton.setOnClickListener(v -> toggleConnection());
        Button refreshButton = commandButton("刷新", 0xFF475569);
        refreshButton.setOnClickListener(v -> refreshBondedDevices());
        connectionRow.addView(connectButton, new LinearLayout.LayoutParams(0, dp(44), 1));
        connectionRow.addView(refreshButton, new LinearLayout.LayoutParams(0, dp(44), 1));
        root.addView(connectionRow);

        LinearLayout transportRow = new LinearLayout(this);
        transportRow.setOrientation(LinearLayout.HORIZONTAL);
        transportRow.addView(sendButton("上一曲", "b"), new LinearLayout.LayoutParams(0, dp(48), 1));
        transportRow.addView(sendButton("播放/暂停", "p"), new LinearLayout.LayoutParams(0, dp(48), 1));
        transportRow.addView(sendButton("下一曲", "n"), new LinearLayout.LayoutParams(0, dp(48), 1));
        root.addView(transportRow);

        LinearLayout volumeRow = new LinearLayout(this);
        volumeRow.setOrientation(LinearLayout.HORIZONTAL);
        volumeRow.addView(sendButton("音量 -", "-"), new LinearLayout.LayoutParams(0, dp(48), 1));
        volumeRow.addView(sendButton("停止", "s"), new LinearLayout.LayoutParams(0, dp(48), 1));
        volumeRow.addView(sendButton("音量 +", "+"), new LinearLayout.LayoutParams(0, dp(48), 1));
        root.addView(volumeRow);

        LinearLayout trackGrid = new LinearLayout(this);
        trackGrid.setOrientation(LinearLayout.VERTICAL);
        for (int row = 0; row < 3; row++) {
            LinearLayout trackRow = new LinearLayout(this);
            trackRow.setOrientation(LinearLayout.HORIZONTAL);
            for (int col = 0; col < 3; col++) {
                int track = row * 3 + col + 1;
                trackRow.addView(sendButton("曲目 " + track, String.valueOf(track)),
                        new LinearLayout.LayoutParams(0, dp(42), 1));
            }
            trackGrid.addView(trackRow);
        }
        root.addView(trackGrid);

        Button statusButton = commandButton("查询状态", 0xFF2563EB);
        statusButton.setOnClickListener(v -> sendCommand("?"));
        root.addView(statusButton, new LinearLayout.LayoutParams(-1, dp(44)));

        logView = new TextView(this);
        logView.setTextSize(13);
        logView.setTextColor(0xFF111827);
        logView.setText("等待连接 HC-05...\n");
        ScrollView scrollView = new ScrollView(this);
        scrollView.setBackgroundColor(0xFFFFFFFF);
        scrollView.setPadding(dp(10), dp(10), dp(10), dp(10));
        scrollView.addView(logView);
        root.addView(scrollView, new LinearLayout.LayoutParams(-1, 0, 1));

        setContentView(root);
    }

    private Button commandButton(String text, int color) {
        Button button = new Button(this);
        button.setText(text);
        button.setTextColor(0xFFFFFFFF);
        button.setBackgroundColor(color);
        return button;
    }

    private Button sendButton(String text, String command) {
        Button button = commandButton(text, 0xFF334155);
        button.setOnClickListener(v -> sendCommand(command));
        return button;
    }

    private int dp(int value) {
        return (int) (value * getResources().getDisplayMetrics().density + 0.5f);
    }

    private void ensureBluetoothPermission() {
        if (Build.VERSION.SDK_INT >= 31 &&
                checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{Manifest.permission.BLUETOOTH_CONNECT},
                    REQUEST_BLUETOOTH_PERMISSION);
        }
    }

    private boolean hasBluetoothPermission() {
        return Build.VERSION.SDK_INT < 31 ||
                checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED;
    }

    private void refreshBondedDevices() {
        if (!hasBluetoothPermission()) {
            setState("等待蓝牙权限");
            return;
        }

        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        devices.clear();
        if (adapter == null) {
            setState("此手机不支持蓝牙");
        } else if (!adapter.isEnabled()) {
            setState("请先打开系统蓝牙并配对 HC-05");
        } else {
            Set<BluetoothDevice> bonded = adapter.getBondedDevices();
            for (BluetoothDevice device : bonded) {
                devices.add(new DeviceEntry(device));
            }
            setState(devices.isEmpty() ? "没有已配对设备，请先在系统蓝牙中配对 HC-05" : "选择设备后连接");
        }

        deviceAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, devices);
        deviceSpinner.setAdapter(deviceAdapter);
    }

    private void toggleConnection() {
        if (socket != null && socket.isConnected()) {
            closeConnection();
            return;
        }

        if (devices.isEmpty()) {
            toast("没有已配对设备");
            refreshBondedDevices();
            return;
        }

        DeviceEntry entry = (DeviceEntry) deviceSpinner.getSelectedItem();
        if (entry == null) {
            toast("请选择设备");
            return;
        }

        connectButton.setEnabled(false);
        setState("正在连接 " + entry.label);
        new Thread(() -> connectToDevice(entry.device)).start();
    }

    private void connectToDevice(BluetoothDevice device) {
        try {
            if (!hasBluetoothPermission()) {
                runOnUiThread(() -> {
                    ensureBluetoothPermission();
                    connectButton.setEnabled(true);
                });
                return;
            }

            BluetoothSocket newSocket = device.createRfcommSocketToServiceRecord(SPP_UUID);
            newSocket.connect();
            socket = newSocket;
            outputStream = newSocket.getOutputStream();
            startReader(newSocket.getInputStream());
            runOnUiThread(() -> {
                connectButton.setText("断开");
                connectButton.setEnabled(true);
                setState("已连接 " + device.getName());
                appendLog("connected");
                sendCommand("?");
            });
        } catch (IOException ex) {
            closeConnection();
            runOnUiThread(() -> {
                connectButton.setEnabled(true);
                setState("连接失败");
                appendLog("connect error: " + ex.getMessage());
            });
        }
    }

    private void startReader(InputStream inputStream) {
        keepReading = true;
        rxThread = new Thread(() -> {
            byte[] buffer = new byte[64];
            while (keepReading) {
                try {
                    int count = inputStream.read(buffer);
                    if (count > 0) {
                        String text = new String(buffer, 0, count);
                        runOnUiThread(() -> appendLog(text));
                    }
                } catch (IOException ex) {
                    if (keepReading) {
                        runOnUiThread(() -> appendLog("rx error: " + ex.getMessage()));
                    }
                    break;
                }
            }
        });
        rxThread.start();
    }

    private void sendCommand(String command) {
        if (outputStream == null) {
            toast("未连接 HC-05");
            return;
        }

        try {
            outputStream.write(command.getBytes());
            outputStream.flush();
            appendLog("> " + command);
        } catch (IOException ex) {
            appendLog("tx error: " + ex.getMessage());
            closeConnection();
        }
    }

    private void closeConnection() {
        keepReading = false;
        try {
            if (socket != null) {
                socket.close();
            }
        } catch (IOException ignored) {
        }
        socket = null;
        outputStream = null;
        runOnUiThread(() -> {
            connectButton.setText("连接");
            connectButton.setEnabled(true);
            setState("未连接");
        });
    }

    private void setState(String text) {
        stateView.setText(text);
    }

    private void appendLog(String text) {
        logView.append(text);
        if (!text.endsWith("\n")) {
            logView.append("\n");
        }
    }

    private void toast(String text) {
        Toast.makeText(this, text, Toast.LENGTH_SHORT).show();
    }
}

