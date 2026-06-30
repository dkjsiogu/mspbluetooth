package com.dkjsiogu.mspbluetooth;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.view.Gravity;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Set;
import java.util.UUID;

@SuppressWarnings("deprecation")
public class MainActivity extends Activity {
    private static final UUID SPP_UUID =
            UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    private static final int REQUEST_BLUETOOTH_PERMISSION = 430;
    private static final int REQUEST_SAVE_LOG = 431;
    private static final String[] DEMO_RX_LINES = new String[]{
            "MSP430F5529 ENV monitor",
            "cmd ? i w x T+ T- SETT=32.0 HIST? HIST n DUMP CLRLOG",
            "info name=MSP430F5529-ENV-MON version=2.0.0 profile=DHT:P1.0 MQ2:P6.0 US:P1.2/1.3 OLED:P3.0/3.1 BT:UCA1",
            "DATA T=26.3 H=58 GAS=1250 L=0 D=342 TH=30.0 ALM=0",
            "TH=30.5",
            "TH=32.0 SAVED",
            "HIST COUNT=2",
            "REC 1 U=10 T=25.0 H=55 GAS=1200 D=410 TH=30.0 ALM=0",
            "REC 2 U=20 T=32.1 H=61 GAS=2300 D=280 TH=30.0 ALM=2",
            "pin dht11 data=P1.0",
            "pin mq2 ao=P6.0 adc=A0",
            "pin hcsr04 trig=P1.2 echo=P1.3 note=echo-divide-to-3v3",
            "pin oled scl=P3.1 sda=P3.0 mode=soft-i2c",
            "pin bt tx=P4.4 rx=P4.5 mode=UCA1",
            "pin buzzer pwm=P2.0",
            "pin ec11 a=P2.1 b=P2.2 sw=P2.3",
            "trace count=4 1=bt:SET 2=bt:T+ 3=bt:H? 4=bt:wire",
            "LOG CLEARED"
    };

    private final ArrayList<DeviceEntry> devices = new ArrayList<>();
    private final StringBuilder rxLineBuffer = new StringBuilder();
    private final StringBuilder logBuffer = new StringBuilder();
    private Spinner deviceSpinner;
    private TextView stateView;
    private TextView dataView;
    private TextView thresholdView;
    private TextView historyView;
    private TextView wiringView;
    private TextView traceView;
    private TextView logView;
    private EditText thresholdEdit;
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

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_SAVE_LOG && resultCode == RESULT_OK && data != null && data.getData() != null) {
            saveLogToUri(data.getData());
        }
    }

    private void buildUi() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(16), dp(16), dp(16), dp(16));
        root.setBackgroundColor(0xFFF7F8FA);

        TextView title = new TextView(this);
        title.setText("MSP430 Env Monitor");
        title.setTextSize(22);
        title.setTextColor(0xFF111827);
        title.setGravity(Gravity.CENTER_VERTICAL);
        root.addView(title, new LinearLayout.LayoutParams(-1, dp(40)));

        stateView = label("Disconnected");
        root.addView(stateView, new LinearLayout.LayoutParams(-1, dp(28)));

        deviceSpinner = new Spinner(this);
        root.addView(deviceSpinner, new LinearLayout.LayoutParams(-1, dp(48)));

        dataView = panel("DATA\nT: --  H: --\nGas: --  L: --\nDistance: --  Alarm: --");
        root.addView(dataView, new LinearLayout.LayoutParams(-1, dp(104)));

        thresholdView = panel("Threshold\nTemp: --");
        root.addView(thresholdView, new LinearLayout.LayoutParams(-1, dp(62)));

        historyView = panel("History\n--");
        root.addView(historyView, new LinearLayout.LayoutParams(-1, dp(106)));

        wiringView = panel("Wiring\n--");
        root.addView(wiringView, new LinearLayout.LayoutParams(-1, dp(126)));

        traceView = panel("Trace\n--");
        root.addView(traceView, new LinearLayout.LayoutParams(-1, dp(70)));

        LinearLayout connectionRow = row();
        connectButton = button("Connect", 0xFF0F766E);
        connectButton.setOnClickListener(v -> toggleConnection());
        Button refreshButton = button("Refresh", 0xFF475569);
        refreshButton.setOnClickListener(v -> refreshBondedDevices());
        connectionRow.addView(connectButton, new LinearLayout.LayoutParams(0, dp(44), 1));
        connectionRow.addView(refreshButton, new LinearLayout.LayoutParams(0, dp(44), 1));
        root.addView(connectionRow);

        LinearLayout commandRow = row();
        commandRow.addView(sendButton("Data", "?"), new LinearLayout.LayoutParams(0, dp(44), 1));
        commandRow.addView(sendButton("Info", "i"), new LinearLayout.LayoutParams(0, dp(44), 1));
        commandRow.addView(sendButton("Wiring", "w"), new LinearLayout.LayoutParams(0, dp(44), 1));
        commandRow.addView(sendButton("Trace", "x"), new LinearLayout.LayoutParams(0, dp(44), 1));
        root.addView(commandRow);

        LinearLayout thresholdRow = row();
        thresholdRow.addView(sendButton("T-", "T-"), new LinearLayout.LayoutParams(0, dp(44), 1));
        thresholdRow.addView(sendButton("T+", "T+"), new LinearLayout.LayoutParams(0, dp(44), 1));
        thresholdEdit = new EditText(this);
        thresholdEdit.setSingleLine(true);
        thresholdEdit.setText("32.0");
        thresholdEdit.setTextSize(14);
        thresholdEdit.setGravity(Gravity.CENTER);
        thresholdRow.addView(thresholdEdit, new LinearLayout.LayoutParams(0, dp(44), 1));
        Button setButton = button("Set", 0xFF2563EB);
        setButton.setOnClickListener(v -> sendCommand("SETT=" + thresholdEdit.getText().toString().trim()));
        thresholdRow.addView(setButton, new LinearLayout.LayoutParams(0, dp(44), 1));
        root.addView(thresholdRow);

        LinearLayout historyRow = row();
        historyRow.addView(sendButton("Count", "HIST?"), new LinearLayout.LayoutParams(0, dp(44), 1));
        historyRow.addView(sendButton("First", "HIST 1"), new LinearLayout.LayoutParams(0, dp(44), 1));
        historyRow.addView(sendButton("Dump", "DUMP"), new LinearLayout.LayoutParams(0, dp(44), 1));
        historyRow.addView(sendButton("Clear", "CLRLOG"), new LinearLayout.LayoutParams(0, dp(44), 1));
        root.addView(historyRow);

        LinearLayout localRow = row();
        Button demoButton = button("Demo RX", 0xFF7C3AED);
        demoButton.setOnClickListener(v -> runDemoRx());
        Button helpButton = button("Help", 0xFF475569);
        helpButton.setOnClickListener(v -> sendCommand("h"));
        Button saveButton = button("Save Log", 0xFF334155);
        saveButton.setOnClickListener(v -> createLogDocument());
        localRow.addView(demoButton, new LinearLayout.LayoutParams(0, dp(44), 1));
        localRow.addView(helpButton, new LinearLayout.LayoutParams(0, dp(44), 1));
        localRow.addView(saveButton, new LinearLayout.LayoutParams(0, dp(44), 1));
        root.addView(localRow);

        logView = panel("Log\n");
        ScrollView logScroll = new ScrollView(this);
        logScroll.addView(logView);
        root.addView(logScroll, new LinearLayout.LayoutParams(-1, 0, 1));

        setContentView(root);
    }

    private TextView label(String text) {
        TextView view = new TextView(this);
        view.setText(text);
        view.setTextSize(14);
        view.setTextColor(0xFF374151);
        return view;
    }

    private TextView panel(String text) {
        TextView view = new TextView(this);
        view.setText(text);
        view.setTextSize(14);
        view.setTextColor(0xFF111827);
        view.setPadding(dp(10), dp(8), dp(10), dp(8));
        view.setBackgroundColor(0xFFFFFFFF);
        return view;
    }

    private Button button(String text, int color) {
        Button button = new Button(this);
        button.setText(text);
        button.setTextColor(0xFFFFFFFF);
        button.setTextSize(13);
        button.setBackgroundColor(color);
        return button;
    }

    private Button sendButton(String text, String command) {
        Button button = button(text, 0xFF0F766E);
        button.setOnClickListener(v -> sendCommand(command));
        return button;
    }

    private LinearLayout row() {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setPadding(0, dp(4), 0, dp(4));
        return row;
    }

    private int dp(int value) {
        return (int) (value * getResources().getDisplayMetrics().density + 0.5f);
    }

    private void ensureBluetoothPermission() {
        if (Build.VERSION.SDK_INT >= 31 &&
                checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{Manifest.permission.BLUETOOTH_CONNECT}, REQUEST_BLUETOOTH_PERMISSION);
        }
    }

    private boolean hasBluetoothPermission() {
        return Build.VERSION.SDK_INT < 31 ||
                checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED;
    }

    private void refreshBondedDevices() {
        devices.clear();
        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        if (adapter == null) {
            stateView.setText("Bluetooth unavailable");
            return;
        }
        if (!adapter.isEnabled()) {
            stateView.setText("Bluetooth disabled");
        }
        if (!hasBluetoothPermission()) {
            stateView.setText("Bluetooth permission required");
            return;
        }
        Set<BluetoothDevice> bonded = adapter.getBondedDevices();
        if (bonded != null) {
            for (BluetoothDevice device : bonded) {
                devices.add(new DeviceEntry(device));
            }
        }
        ArrayAdapter<DeviceEntry> adapterView = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, devices);
        adapterView.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        deviceSpinner.setAdapter(adapterView);
        stateView.setText(devices.isEmpty() ? "No paired HC-05" : "Paired devices: " + devices.size());
    }

    private void toggleConnection() {
        if (socket != null) {
            closeConnection();
            return;
        }
        if (devices.isEmpty() || deviceSpinner.getSelectedItem() == null) {
            toast("No paired device");
            return;
        }
        if (!hasBluetoothPermission()) {
            ensureBluetoothPermission();
            return;
        }
        DeviceEntry entry = (DeviceEntry) deviceSpinner.getSelectedItem();
        stateView.setText("Connecting " + entry.label);
        new Thread(() -> connectDevice(entry.device)).start();
    }

    private void connectDevice(BluetoothDevice device) {
        try {
            BluetoothSocket nextSocket = device.createRfcommSocketToServiceRecord(SPP_UUID);
            nextSocket.connect();
            socket = nextSocket;
            outputStream = nextSocket.getOutputStream();
            keepReading = true;
            rxThread = new Thread(() -> readLoop(nextSocket));
            rxThread.start();
            runOnUiThread(() -> {
                stateView.setText("Connected " + device.getName());
                connectButton.setText("Disconnect");
                appendLog("connected " + device.getAddress());
                sendCommand("?");
            });
        } catch (IOException ex) {
            runOnUiThread(() -> {
                stateView.setText("Connect failed");
                appendLog("connect error: " + ex.getMessage());
            });
            closeQuietly();
        }
    }

    private void readLoop(BluetoothSocket activeSocket) {
        byte[] buffer = new byte[64];
        try {
            InputStream inputStream = activeSocket.getInputStream();
            while (keepReading) {
                int count = inputStream.read(buffer);
                if (count < 0) {
                    break;
                }
                for (int index = 0; index < count; index++) {
                    char ch = (char) (buffer[index] & 0xFF);
                    handleRxChar(ch);
                }
            }
        } catch (IOException ex) {
            runOnUiThread(() -> appendLog("rx error: " + ex.getMessage()));
        }
        runOnUiThread(() -> stateView.setText("Disconnected"));
    }

    private void handleRxChar(char ch) {
        if (ch == '\r' || ch == '\n') {
            if (rxLineBuffer.length() > 0) {
                String line = rxLineBuffer.toString();
                rxLineBuffer.setLength(0);
                runOnUiThread(() -> parseIncomingLine(line));
            }
        } else if (rxLineBuffer.length() < 160) {
            rxLineBuffer.append(ch);
        }
    }

    private void sendCommand(String command) {
        if (command == null || command.length() == 0) {
            return;
        }
        appendLog("TX> " + command);
        OutputStream out = outputStream;
        if (out == null) {
            return;
        }
        try {
            out.write((command + "\r\n").getBytes(StandardCharsets.US_ASCII));
            out.flush();
        } catch (IOException ex) {
            appendLog("tx error: " + ex.getMessage());
            closeConnection();
        }
    }

    private void parseIncomingLine(String line) {
        appendLog("RX> " + line);
        if (line.startsWith("DATA ")) {
            updateDataPanel(line);
        } else if (line.startsWith("TH=")) {
            thresholdView.setText("Threshold\nTemp: " + valueAfter(line, "TH="));
        } else if (line.startsWith("HIST COUNT=") || line.startsWith("REC ") || line.startsWith("DUMP END") || line.startsWith("ERR HIST") || line.startsWith("LOG CLEARED")) {
            appendPanelLine(historyView, "History", line, 8);
        } else if (line.startsWith("pin ")) {
            appendPanelLine(wiringView, "Wiring", line, 8);
        } else if (line.startsWith("trace ")) {
            traceView.setText("Trace\n" + line);
        } else if (line.startsWith("info ") || line.startsWith("cmd ")) {
            appendPanelLine(traceView, "Trace", line, 6);
        }
    }

    private void updateDataPanel(String line) {
        String temp = valueAfter(line, "T=");
        String humidity = valueAfter(line, "H=");
        String gas = valueAfter(line, "GAS=");
        String level = valueAfter(line, "L=");
        String distance = valueAfter(line, "D=");
        String threshold = valueAfter(line, "TH=");
        String alarm = valueAfter(line, "ALM=");
        dataView.setText(
                "DATA\nT: " + temp + " C  H: " + humidity + " %\n" +
                        "Gas: " + gas + "  L: " + level + "\n" +
                        "Distance: " + distance + " mm  Alarm: " + alarm);
        thresholdView.setText("Threshold\nTemp: " + threshold);
    }

    private String valueAfter(String line, String key) {
        int start = line.indexOf(key);
        if (start < 0) {
            return "--";
        }
        start += key.length();
        int end = line.indexOf(' ', start);
        if (end < 0) {
            end = line.length();
        }
        return line.substring(start, end);
    }

    private void appendPanelLine(TextView view, String title, String line, int maxLines) {
        String current = view.getText().toString();
        String body = current.startsWith(title + "\n") ? current.substring(title.length() + 1) : "";
        String[] existing = body.length() == 0 || body.equals("--") ? new String[0] : body.split("\n");
        ArrayList<String> lines = new ArrayList<>();
        lines.add(line);
        for (String item : existing) {
            if (lines.size() >= maxLines) {
                break;
            }
            lines.add(item);
        }
        StringBuilder next = new StringBuilder(title).append('\n');
        for (int index = 0; index < lines.size(); index++) {
            if (index > 0) {
                next.append('\n');
            }
            next.append(lines.get(index));
        }
        view.setText(next.toString());
    }

    private void appendLog(String line) {
        logBuffer.append(line).append('\n');
        logView.setText("Log\n" + logBuffer);
    }

    private void runDemoRx() {
        appendLog("demo rx start");
        for (String line : DEMO_RX_LINES) {
            parseIncomingLine(line);
        }
        appendLog("demo rx done");
    }

    private void createLogDocument() {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("text/plain");
        intent.putExtra(Intent.EXTRA_TITLE, "msp430_env_log.txt");
        startActivityForResult(intent, REQUEST_SAVE_LOG);
    }

    private void saveLogToUri(Uri uri) {
        try (OutputStream out = getContentResolver().openOutputStream(uri)) {
            if (out != null) {
                out.write(logBuffer.toString().getBytes(StandardCharsets.UTF_8));
            }
            toast("Log saved");
        } catch (IOException ex) {
            toast("Save failed");
            appendLog("save error: " + ex.getMessage());
        }
    }

    private void closeConnection() {
        keepReading = false;
        closeQuietly();
        stateView.setText("Disconnected");
        connectButton.setText("Connect");
    }

    private void closeQuietly() {
        try {
            if (socket != null) {
                socket.close();
            }
        } catch (IOException ignored) {
        }
        socket = null;
        outputStream = null;
    }

    private void toast(String text) {
        Toast.makeText(this, text, Toast.LENGTH_SHORT).show();
    }
}
