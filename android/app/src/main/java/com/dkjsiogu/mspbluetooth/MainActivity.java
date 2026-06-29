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
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Set;
import java.util.UUID;

@SuppressWarnings("deprecation")
public class MainActivity extends Activity {
    private static final UUID SPP_UUID =
            UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    private static final int REQUEST_BLUETOOTH_PERMISSION = 430;

    private final ArrayList<DeviceEntry> devices = new ArrayList<>();
    private Spinner deviceSpinner;
    private TextView stateView;
    private TextView dashboardView;
    private TextView displayView;
    private TextView trackListView;
    private TextView logView;
    private Button connectButton;
    private BluetoothSocket socket;
    private OutputStream outputStream;
    private Thread rxThread;
    private volatile boolean keepReading;
    private final StringBuilder rxLineBuffer = new StringBuilder();
    private final String[] displayLines = new String[]{"--", "--", "--"};

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
        title.setText("MSP430 Player");
        title.setTextSize(22);
        title.setTextColor(0xFF111827);
        title.setGravity(Gravity.CENTER_VERTICAL);
        root.addView(title, new LinearLayout.LayoutParams(-1, dp(40)));

        stateView = new TextView(this);
        stateView.setText("Disconnected");
        stateView.setTextSize(14);
        stateView.setTextColor(0xFF374151);
        root.addView(stateView, new LinearLayout.LayoutParams(-1, dp(28)));

        deviceSpinner = new Spinner(this);
        root.addView(deviceSpinner, new LinearLayout.LayoutParams(-1, dp(48)));

        dashboardView = panelText("Mode: --\nTrack: --\nVolume: --\nOrder: --\nProgress: --");
        root.addView(dashboardView, new LinearLayout.LayoutParams(-1, dp(104)));

        displayView = panelText("Display frame\n--\n--\n--");
        root.addView(displayView, new LinearLayout.LayoutParams(-1, dp(96)));

        trackListView = panelText("Tracks\n1: --  2: --  3: --\n4: --  5: --  6: --\n7: --  8: --  9: --");
        root.addView(trackListView, new LinearLayout.LayoutParams(-1, dp(82)));

        LinearLayout connectionRow = row();
        connectButton = commandButton("Connect", 0xFF0F766E);
        connectButton.setOnClickListener(v -> toggleConnection());
        Button refreshButton = commandButton("Refresh", 0xFF475569);
        refreshButton.setOnClickListener(v -> refreshBondedDevices());
        connectionRow.addView(connectButton, new LinearLayout.LayoutParams(0, dp(44), 1));
        connectionRow.addView(refreshButton, new LinearLayout.LayoutParams(0, dp(44), 1));
        root.addView(connectionRow);

        LinearLayout transportRow = row();
        transportRow.addView(sendButton("Prev", "b"), new LinearLayout.LayoutParams(0, dp(48), 1));
        transportRow.addView(sendButton("Play/Pause", "p"), new LinearLayout.LayoutParams(0, dp(48), 1));
        transportRow.addView(sendButton("Next", "n"), new LinearLayout.LayoutParams(0, dp(48), 1));
        root.addView(transportRow);

        LinearLayout volumeRow = row();
        volumeRow.addView(sendButton("Vol -", "-"), new LinearLayout.LayoutParams(0, dp(48), 1));
        volumeRow.addView(sendButton("Stop", "s"), new LinearLayout.LayoutParams(0, dp(48), 1));
        volumeRow.addView(sendButton("Vol +", "+"), new LinearLayout.LayoutParams(0, dp(48), 1));
        root.addView(volumeRow);

        LinearLayout toolsRow = row();
        toolsRow.addView(sendButton("Replay", "r"), new LinearLayout.LayoutParams(0, dp(44), 1));
        toolsRow.addView(sendButton("Mute", "m"), new LinearLayout.LayoutParams(0, dp(44), 1));
        toolsRow.addView(sendButton("Tone", "t"), new LinearLayout.LayoutParams(0, dp(44), 1));
        root.addView(toolsRow);

        LinearLayout diagRow = row();
        diagRow.addView(sendButton("Info", "i"), new LinearLayout.LayoutParams(0, dp(44), 1));
        diagRow.addView(sendButton("Selftest", "e"), new LinearLayout.LayoutParams(0, dp(44), 1));
        diagRow.addView(sendButton("Display", "d"), new LinearLayout.LayoutParams(0, dp(44), 1));
        root.addView(diagRow);

        LinearLayout queryRow = row();
        queryRow.addView(sendButton("Order", "o"), new LinearLayout.LayoutParams(0, dp(44), 1));
        queryRow.addView(sendButton("Track List", "l"), new LinearLayout.LayoutParams(0, dp(44), 1));
        queryRow.addView(sendButton("Status", "?"), new LinearLayout.LayoutParams(0, dp(44), 1));
        root.addView(queryRow);

        LinearLayout trackGrid = new LinearLayout(this);
        trackGrid.setOrientation(LinearLayout.VERTICAL);
        for (int gridRow = 0; gridRow < 3; gridRow++) {
            LinearLayout trackRow = row();
            for (int col = 0; col < 3; col++) {
                int track = gridRow * 3 + col + 1;
                trackRow.addView(sendButton("Track " + track, String.valueOf(track)),
                        new LinearLayout.LayoutParams(0, dp(42), 1));
            }
            trackGrid.addView(trackRow);
        }
        root.addView(trackGrid);

        logView = new TextView(this);
        logView.setTextSize(13);
        logView.setTextColor(0xFF111827);
        logView.setText("Pair HC-05 in Android Bluetooth settings first.\n");
        ScrollView scrollView = new ScrollView(this);
        scrollView.setBackgroundColor(0xFFFFFFFF);
        scrollView.setPadding(dp(10), dp(10), dp(10), dp(10));
        scrollView.addView(logView);
        root.addView(scrollView, new LinearLayout.LayoutParams(-1, 0, 1));

        setContentView(root);
    }

    private LinearLayout row() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.HORIZONTAL);
        return layout;
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

    private TextView panelText(String text) {
        TextView view = new TextView(this);
        view.setText(text);
        view.setTextSize(14);
        view.setTextColor(0xFF111827);
        view.setBackgroundColor(0xFFFFFFFF);
        view.setPadding(dp(10), dp(8), dp(10), dp(8));
        return view;
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
            setState("Waiting for Bluetooth permission");
            return;
        }

        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        devices.clear();
        if (adapter == null) {
            setState("Bluetooth is not supported on this phone");
        } else if (!adapter.isEnabled()) {
            setState("Enable Bluetooth and pair HC-05 first");
        } else {
            Set<BluetoothDevice> bonded = adapter.getBondedDevices();
            for (BluetoothDevice device : bonded) {
                devices.add(new DeviceEntry(device));
            }
            setState(devices.isEmpty() ? "No paired devices; pair HC-05 first" : "Select a device and connect");
        }

        ArrayAdapter<DeviceEntry> adapterView =
                new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, devices);
        deviceSpinner.setAdapter(adapterView);
    }

    private void toggleConnection() {
        if (socket != null && socket.isConnected()) {
            closeConnection();
            return;
        }

        if (devices.isEmpty()) {
            toast("No paired devices");
            refreshBondedDevices();
            return;
        }

        DeviceEntry entry = (DeviceEntry) deviceSpinner.getSelectedItem();
        if (entry == null) {
            toast("Select a device");
            return;
        }

        connectButton.setEnabled(false);
        setState("Connecting " + entry.label);
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
                connectButton.setText("Disconnect");
                connectButton.setEnabled(true);
                setState("Connected " + device.getName());
                appendLog("connected");
                syncInitialPanels();
            });
        } catch (IOException ex) {
            closeConnection();
            runOnUiThread(() -> {
                connectButton.setEnabled(true);
                setState("Connect failed");
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
                        String text = new String(buffer, 0, count, StandardCharsets.US_ASCII);
                        runOnUiThread(() -> handleIncomingText(text));
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

    private void syncInitialPanels() {
        sendCommand("?");
        sendCommand("l");
        sendCommand("d");
    }

    private void handleIncomingText(String text) {
        appendLog(text);
        rxLineBuffer.append(text);

        int newline;
        while ((newline = rxLineBuffer.indexOf("\n")) >= 0) {
            String line = rxLineBuffer.substring(0, newline).trim();
            rxLineBuffer.delete(0, newline + 1);
            if (line.length() > 0) {
                parseIncomingLine(line);
            }
        }
    }

    private void parseIncomingLine(String line) {
        if (line.startsWith("status=")) {
            updateDashboard(line);
        } else if (line.startsWith("display 1:")) {
            displayLines[0] = line.substring("display 1:".length());
            updateDisplayFrame();
        } else if (line.startsWith("display 2:")) {
            displayLines[1] = line.substring("display 2:".length());
            updateDisplayFrame();
        } else if (line.startsWith("display 3:")) {
            displayLines[2] = line.substring("display 3:".length());
            updateDisplayFrame();
        } else if (line.startsWith("tracks")) {
            updateTrackList(line);
        }
    }

    private void updateDashboard(String statusLine) {
        String mode = fieldValue(statusLine, "status=");
        String track = fieldValue(statusLine, "track=");
        String volume = fieldValue(statusLine, "volume=");
        String order = fieldValue(statusLine, "order=");
        String progress = fieldValue(statusLine, "progress=");
        dashboardView.setText("Mode: " + mode + "\nTrack: " + track +
                "\nVolume: " + volume + "\nOrder: " + order +
                "\nProgress: " + progress + "%");
    }

    private void updateDisplayFrame() {
        displayView.setText("Display frame\n" + displayLines[0] + "\n" +
                displayLines[1] + "\n" + displayLines[2]);
    }

    private void updateTrackList(String tracksLine) {
        String[] parts = tracksLine.split(" ");
        StringBuilder builder = new StringBuilder("Tracks");
        int shown = 0;

        for (String part : parts) {
            if (part.indexOf('=') <= 0) {
                continue;
            }
            String[] item = part.split("=", 2);
            if (shown % 3 == 0) {
                builder.append('\n');
            } else {
                builder.append("  ");
            }
            builder.append(item[0]).append(": ").append(item[1]);
            shown++;
        }

        if (shown == 0) {
            builder.append("\n--");
        }
        trackListView.setText(builder.toString());
    }

    private String fieldValue(String line, String key) {
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

    private void sendCommand(String command) {
        if (outputStream == null) {
            toast("Not connected to HC-05");
            return;
        }

        try {
            outputStream.write(command.getBytes(StandardCharsets.US_ASCII));
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
            connectButton.setText("Connect");
            connectButton.setEnabled(true);
            setState("Disconnected");
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
