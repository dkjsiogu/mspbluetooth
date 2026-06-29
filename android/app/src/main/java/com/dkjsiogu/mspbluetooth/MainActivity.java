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
import android.widget.LinearLayout;
import android.widget.ProgressBar;
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
    private static final String[] ACCEPTANCE_COMMANDS =
            new String[]{"h", "i", "e", "l", "d", "?", "t", "1", "p", "+", "n", "b", "o", "3", "k", "u", "x", "w"};
    private static final String[] DEMO_RX_LINES = new String[]{
            "sd mounted",
            "info name=MSP430F5529-BT-WAV version=1.4.1 profile=bt_wav_player",
            "selftest bt=ok sd=ok wav=ok i2s=ok buttons=ok",
            "tracks 1=ok 2=-- 3=ok 4=-- 5=-- 6=-- 7=-- 8=-- 9=--",
            "display 1:playing T3 V19 ONE",
            "display 2:SD:OK WAV:OPEN",
            "display 3:16000Hz 2ch P42%",
            "status=playing track=3 volume=19 order=repeat_one rate=16000Hz channels=2 progress=42",
            "tone start",
            "tone done",
            "open TRACK03.WAV",
            "link rx=15 status=10 display=9 bad=0 last=k uptime=1234ms",
            "input ecw=3 eccw=1 eb=2 elong=1 s1=2 s1l=1 s2=1 s2l=1 s4=1 s4l=1",
            "trace count=6 1=bt:vol+ 2=bt:next 3=bt:prev 4=bt:order 5=bt:track 6=bt:trace",
            "pin profile=TF:P3.1-3.3 I2S:P4.1-4.3 BT:UCA1",
            "pin tf cs=P4.0 sck=P3.1 mosi=P3.2 miso=P3.3",
            "pin i2s bck=P4.1 lrck=P4.2 din=P4.3",
            "pin ec11 a=P2.1 b=P2.2 sw=P2.3",
            "pin local s1=P1.2 s2=P1.3 s4=P2.6 led=P1.0",
            "pin bt tx=P4.4 rx=P4.5 mode=UCA1 note=no-tf-conflict",
            "pin epaper optional=P6.0-P6.5 default=disabled"
    };

    private final ArrayList<DeviceEntry> devices = new ArrayList<>();
    private Spinner deviceSpinner;
    private TextView stateView;
    private TextView dashboardView;
    private ProgressBar volumeBar;
    private ProgressBar progressBar;
    private TextView healthView;
    private TextView displayView;
    private TextView trackListView;
    private TextView linkView;
    private TextView inputView;
    private TextView traceView;
    private TextView wiringView;
    private TextView acceptanceView;
    private TextView logView;
    private Button connectButton;
    private BluetoothSocket socket;
    private OutputStream outputStream;
    private Thread rxThread;
    private volatile boolean keepReading;
    private final StringBuilder rxLineBuffer = new StringBuilder();
    private final String[] displayLines = new String[]{"--", "--", "--"};
    private String healthSd = "--";
    private String healthInfo = "--";
    private String healthSelftest = "--";
    private String healthTone = "--";
    private String healthFile = "--";
    private String healthError = "--";
    private final String[] wiringLines = new String[]{
            "Profile: --", "TF: --", "I2S: --", "EC11: --", "Local: --", "BT: --", "E-paper: --"};
    private boolean acceptanceSdMounted;
    private boolean acceptanceInfo;
    private boolean acceptanceSelftest;
    private boolean acceptanceTracks;
    private boolean acceptanceDisplay1;
    private boolean acceptanceDisplay2;
    private boolean acceptanceDisplay3;
    private boolean acceptanceStatus;
    private boolean acceptanceToneStart;
    private boolean acceptanceToneDone;
    private boolean acceptanceTrackOpen;
    private boolean acceptanceWiring;

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

        volumeBar = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        volumeBar.setMax(32);
        root.addView(volumeBar, new LinearLayout.LayoutParams(-1, dp(16)));

        progressBar = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        progressBar.setMax(100);
        root.addView(progressBar, new LinearLayout.LayoutParams(-1, dp(16)));

        healthView = panelText("Health\nSD:-- Info:-- Selftest:-- Tone:--\nFile:--\nError:--");
        root.addView(healthView, new LinearLayout.LayoutParams(-1, dp(88)));

        displayView = panelText("Display frame\n--\n--\n--");
        root.addView(displayView, new LinearLayout.LayoutParams(-1, dp(96)));

        trackListView = panelText("Tracks\n1: --  2: --  3: --\n4: --  5: --  6: --\n7: --  8: --  9: --");
        root.addView(trackListView, new LinearLayout.LayoutParams(-1, dp(82)));

        linkView = panelText("Link\nRX: --  Status: --  Display: --\nBad: --  Last: --  Uptime: --");
        root.addView(linkView, new LinearLayout.LayoutParams(-1, dp(70)));

        inputView = panelText("Input\nEC11 CW:-- CCW:-- SW:-- Long:--\nS1:--/-- S2:--/-- S4:--/--");
        root.addView(inputView, new LinearLayout.LayoutParams(-1, dp(70)));

        traceView = panelText("Trace\n--");
        root.addView(traceView, new LinearLayout.LayoutParams(-1, dp(58)));

        wiringView = panelText("Wiring\nProfile: --\nTF: --\nI2S: --\nEC11: --\nLocal: --\nBT: --\nE-paper: --");
        root.addView(wiringView, new LinearLayout.LayoutParams(-1, dp(146)));

        acceptanceView = panelText("Acceptance 0/9\nSD:-- Info:-- Selftest:-- Tracks:-- Wiring:--\nDisplay:-- Status:-- Tone:-- Open:--");
        root.addView(acceptanceView, new LinearLayout.LayoutParams(-1, dp(70)));

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
        diagRow.addView(sendButton("Wiring", "w"), new LinearLayout.LayoutParams(0, dp(44), 1));
        diagRow.addView(sendButton("Input", "u"), new LinearLayout.LayoutParams(0, dp(44), 1));
        root.addView(diagRow);

        LinearLayout queryRow = row();
        queryRow.addView(sendButton("Order", "o"), new LinearLayout.LayoutParams(0, dp(44), 1));
        queryRow.addView(sendButton("Track List", "l"), new LinearLayout.LayoutParams(0, dp(44), 1));
        queryRow.addView(sendButton("Status", "?"), new LinearLayout.LayoutParams(0, dp(44), 1));
        queryRow.addView(sendButton("Link", "k"), new LinearLayout.LayoutParams(0, dp(44), 1));
        queryRow.addView(sendButton("Trace", "x"), new LinearLayout.LayoutParams(0, dp(44), 1));
        root.addView(queryRow);

        LinearLayout acceptanceRow = row();
        Button acceptanceButton = commandButton("Run Acceptance", 0xFF0F766E);
        acceptanceButton.setOnClickListener(v -> runAcceptanceScript());
        Button saveLogButton = commandButton("Save Log", 0xFF334155);
        saveLogButton.setOnClickListener(v -> saveLogToFile());
        Button shareLogButton = commandButton("Share Log", 0xFF334155);
        shareLogButton.setOnClickListener(v -> shareLog());
        Button demoRxButton = commandButton("Demo RX", 0xFF475569);
        demoRxButton.setOnClickListener(v -> runOfflineDemo());
        Button clearLogButton = commandButton("Clear Log", 0xFF475569);
        clearLogButton.setOnClickListener(v -> {
            logView.setText("");
            resetAcceptanceSummary();
        });
        acceptanceRow.addView(acceptanceButton, new LinearLayout.LayoutParams(-1, dp(44)));
        root.addView(acceptanceRow);

        LinearLayout logActionRow = row();
        logActionRow.addView(saveLogButton, new LinearLayout.LayoutParams(0, dp(44), 1));
        logActionRow.addView(shareLogButton, new LinearLayout.LayoutParams(0, dp(44), 1));
        logActionRow.addView(demoRxButton, new LinearLayout.LayoutParams(0, dp(44), 1));
        logActionRow.addView(clearLogButton, new LinearLayout.LayoutParams(0, dp(44), 1));
        root.addView(logActionRow);

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
        root.addView(scrollView, new LinearLayout.LayoutParams(-1, dp(220)));

        ScrollView pageScroll = new ScrollView(this);
        pageScroll.addView(root);
        setContentView(pageScroll);
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

    private void runAcceptanceScript() {
        appendLog("acceptance start");
        for (String command : ACCEPTANCE_COMMANDS) {
            sendCommand(command);
        }
        appendLog("acceptance commands sent");
    }

    private void runOfflineDemo() {
        appendLog("demo rx start");
        StringBuilder builder = new StringBuilder();
        for (String line : DEMO_RX_LINES) {
            builder.append(line).append("\r\n");
        }
        handleIncomingText(builder.toString());
        appendLog("demo rx done");
    }

    private void shareLog() {
        String text = logView.getText().toString();
        if (text.trim().length() == 0) {
            toast("Log is empty");
            return;
        }

        Intent intent = new Intent(Intent.ACTION_SEND);
        intent.setType("text/plain");
        intent.putExtra(Intent.EXTRA_SUBJECT, "MSP430 Bluetooth acceptance log");
        intent.putExtra(Intent.EXTRA_TEXT, text);
        startActivity(Intent.createChooser(intent, "Share acceptance log"));
    }

    private void saveLogToFile() {
        String text = logView.getText().toString();
        if (text.trim().length() == 0) {
            toast("Log is empty");
            return;
        }

        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("text/plain");
        intent.putExtra(Intent.EXTRA_TITLE, "msp430_acceptance_log.txt");
        startActivityForResult(intent, REQUEST_SAVE_LOG);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_SAVE_LOG && resultCode == RESULT_OK && data != null) {
            Uri uri = data.getData();
            if (uri != null) {
                writeLogToUri(uri);
            }
        }
    }

    private void writeLogToUri(Uri uri) {
        try (OutputStream stream = getContentResolver().openOutputStream(uri)) {
            if (stream == null) {
                appendLog("save error: output stream unavailable");
                return;
            }
            stream.write(logView.getText().toString().getBytes(StandardCharsets.UTF_8));
            stream.flush();
            appendLog("log saved");
        } catch (IOException ex) {
            appendLog("save error: " + ex.getMessage());
        }
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
        updateAcceptanceSummary(line);
        if (line.startsWith("status=")) {
            updateDashboard(line);
        } else if (line.startsWith("sd mounted") || line.startsWith("info name=") ||
                line.startsWith("selftest ") || line.startsWith("tone start") ||
                line.startsWith("tone done") || line.startsWith("open TRACK0") ||
                line.startsWith("error: ")) {
            updateHealthPanel(line);
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
        } else if (line.startsWith("link ")) {
            updateLinkPanel(line);
        } else if (line.startsWith("input ")) {
            updateInputPanel(line);
        } else if (line.startsWith("trace ")) {
            updateTracePanel(line);
        } else if (line.startsWith("pin ")) {
            updateWiringPanel(line);
        }
    }

    private void resetAcceptanceSummary() {
        acceptanceSdMounted = false;
        acceptanceInfo = false;
        acceptanceSelftest = false;
        acceptanceTracks = false;
        acceptanceDisplay1 = false;
        acceptanceDisplay2 = false;
        acceptanceDisplay3 = false;
        acceptanceStatus = false;
        acceptanceToneStart = false;
        acceptanceToneDone = false;
        acceptanceTrackOpen = false;
        acceptanceWiring = false;
        renderAcceptanceSummary();
    }

    private void updateAcceptanceSummary(String line) {
        if (line.startsWith("sd mounted")) {
            acceptanceSdMounted = true;
        } else if (line.startsWith("info name=")) {
            acceptanceInfo = true;
        } else if (line.startsWith("selftest bt=ok")) {
            acceptanceSelftest = true;
        } else if (line.startsWith("tracks")) {
            acceptanceTracks = true;
        } else if (line.startsWith("display 1:")) {
            acceptanceDisplay1 = true;
        } else if (line.startsWith("display 2:")) {
            acceptanceDisplay2 = true;
        } else if (line.startsWith("display 3:")) {
            acceptanceDisplay3 = true;
        } else if (line.startsWith("status=")) {
            acceptanceStatus = true;
        } else if (line.startsWith("tone start")) {
            acceptanceToneStart = true;
        } else if (line.startsWith("tone done")) {
            acceptanceToneDone = true;
        } else if (line.startsWith("open TRACK0")) {
            acceptanceTrackOpen = true;
        } else if (line.startsWith("pin bt ")) {
            acceptanceWiring = true;
        }
        renderAcceptanceSummary();
    }

    private void renderAcceptanceSummary() {
        boolean displayOk = acceptanceDisplay1 && acceptanceDisplay2 && acceptanceDisplay3;
        boolean toneOk = acceptanceToneStart && acceptanceToneDone;
        int passed = 0;
        passed += acceptanceSdMounted ? 1 : 0;
        passed += acceptanceInfo ? 1 : 0;
        passed += acceptanceSelftest ? 1 : 0;
        passed += acceptanceTracks ? 1 : 0;
        passed += displayOk ? 1 : 0;
        passed += acceptanceStatus ? 1 : 0;
        passed += toneOk ? 1 : 0;
        passed += acceptanceTrackOpen ? 1 : 0;
        passed += acceptanceWiring ? 1 : 0;

        acceptanceView.setText("Acceptance " + passed + "/9\n" +
                "SD:" + mark(acceptanceSdMounted) + " Info:" + mark(acceptanceInfo) +
                " Selftest:" + mark(acceptanceSelftest) + " Tracks:" + mark(acceptanceTracks) +
                " Wiring:" + mark(acceptanceWiring) + "\n" +
                "Display:" + mark(displayOk) + " Status:" + mark(acceptanceStatus) +
                " Tone:" + mark(toneOk) + " Open:" + mark(acceptanceTrackOpen));
    }

    private String mark(boolean passed) {
        return passed ? "OK" : "--";
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
        volumeBar.setProgress(boundedInt(volume, 0, 32));
        progressBar.setProgress(boundedInt(progress, 0, 100));
    }

    private void updateHealthPanel(String line) {
        if (line.startsWith("sd mounted")) {
            healthSd = "OK";
        } else if (line.startsWith("info name=")) {
            healthInfo = fieldValue(line, "version=");
        } else if (line.startsWith("selftest ")) {
            healthSelftest = line.substring("selftest ".length());
        } else if (line.startsWith("tone start")) {
            healthTone = "running";
        } else if (line.startsWith("tone done")) {
            healthTone = "done";
        } else if (line.startsWith("open TRACK0")) {
            healthFile = line.substring("open ".length());
        } else if (line.startsWith("error: ")) {
            healthError = line.substring("error: ".length());
        }

        healthView.setText("Health\nSD:" + healthSd + " Info:" + healthInfo +
                " Selftest:" + compactHealth(healthSelftest) + " Tone:" + healthTone +
                "\nFile:" + healthFile + "\nError:" + healthError);
    }

    private String compactHealth(String text) {
        if (text.length() <= 24) {
            return text;
        }
        return text.substring(0, 24);
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

    private void updateLinkPanel(String linkLine) {
        String rx = fieldValue(linkLine, "rx=");
        String status = fieldValue(linkLine, "status=");
        String display = fieldValue(linkLine, "display=");
        String bad = fieldValue(linkLine, "bad=");
        String last = fieldValue(linkLine, "last=");
        String uptime = fieldValue(linkLine, "uptime=");
        linkView.setText("Link\nRX: " + rx + "  Status: " + status +
                "  Display: " + display + "\nBad: " + bad +
                "  Last: " + last + "  Uptime: " + uptime);
    }

    private void updateInputPanel(String inputLine) {
        String ecw = fieldValue(inputLine, "ecw=");
        String eccw = fieldValue(inputLine, "eccw=");
        String eb = fieldValue(inputLine, "eb=");
        String elong = fieldValue(inputLine, "elong=");
        String s1 = fieldValue(inputLine, "s1=");
        String s1l = fieldValue(inputLine, "s1l=");
        String s2 = fieldValue(inputLine, "s2=");
        String s2l = fieldValue(inputLine, "s2l=");
        String s4 = fieldValue(inputLine, "s4=");
        String s4l = fieldValue(inputLine, "s4l=");
        inputView.setText("Input\nEC11 CW:" + ecw + " CCW:" + eccw +
                " SW:" + eb + " Long:" + elong + "\nS1:" + s1 + "/" + s1l +
                " S2:" + s2 + "/" + s2l + " S4:" + s4 + "/" + s4l);
    }

    private void updateTracePanel(String traceLine) {
        String[] parts = traceLine.split(" ");
        StringBuilder builder = new StringBuilder("Trace");
        int shown = 0;
        for (String part : parts) {
            if (part.indexOf('=') <= 0 || part.startsWith("count=")) {
                continue;
            }
            if (shown % 3 == 0) {
                builder.append('\n');
            } else {
                builder.append("  ");
            }
            builder.append(part);
            shown++;
        }
        if (shown == 0) {
            builder.append("\n--");
        }
        traceView.setText(builder.toString());
    }

    private void updateWiringPanel(String pinLine) {
        if (pinLine.startsWith("pin profile=")) {
            wiringLines[0] = "Profile: " + pinLine.substring("pin profile=".length());
        } else if (pinLine.startsWith("pin tf ")) {
            wiringLines[1] = "TF: " + pinLine.substring("pin tf ".length());
        } else if (pinLine.startsWith("pin i2s ")) {
            wiringLines[2] = "I2S: " + pinLine.substring("pin i2s ".length());
        } else if (pinLine.startsWith("pin ec11 ")) {
            wiringLines[3] = "EC11: " + pinLine.substring("pin ec11 ".length());
        } else if (pinLine.startsWith("pin local ")) {
            wiringLines[4] = "Local: " + pinLine.substring("pin local ".length());
        } else if (pinLine.startsWith("pin bt ")) {
            wiringLines[5] = "BT: " + pinLine.substring("pin bt ".length());
        } else if (pinLine.startsWith("pin epaper ")) {
            wiringLines[6] = "E-paper: " + pinLine.substring("pin epaper ".length());
        }

        wiringView.setText("Wiring\n" + wiringLines[0] + "\n" + wiringLines[1] + "\n" +
                wiringLines[2] + "\n" + wiringLines[3] + "\n" + wiringLines[4] + "\n" +
                wiringLines[5] + "\n" + wiringLines[6]);
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

    private int boundedInt(String text, int min, int max) {
        try {
            int value = Integer.parseInt(text.replace("%", "").trim());
            if (value < min) {
                return min;
            }
            if (value > max) {
                return max;
            }
            return value;
        } catch (NumberFormatException ex) {
            return min;
        }
    }

    private void sendCommand(String command) {
        if (outputStream == null) {
            toast("Not connected to HC-05");
            return;
        }

        try {
            outputStream.write(command.getBytes(StandardCharsets.US_ASCII));
            outputStream.flush();
            appendLog("TX> " + command);
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
