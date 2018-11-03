package com.vutbr.fit.sen.blackbox.senblackbox;

import android.view.*;
import android.os.Bundle;
import android.content.Intent;
import android.widget.TextView;
import android.support.v7.app.*;
import android.text.method.ScrollingMovementMethod;
import java.util.Calendar;
import app.akexorcist.bluetotohspp.library.*;

/**
 * Main class with log console view and three buttons to handle the device.
 * @author Andrej Hucko
 */
public class MainActivity extends AppCompatActivity {

    /**
     * Command constant characters for communication with blackbox.
     */
    final String CMD_GPS = "g";
    final String CMD_ACC = "a";

    static boolean isDebugModeOn;
    BluetoothSPP bluetooth;

    ConnectButton btnConnect;
    ConnectButton btnGPS, btnACC;
    TextView messageView;

    static AlertDialog helpDialog = null;
    static AlertDialog crashDialog = null;
    App a;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        a = (App) getApplication();
        bluetooth = a.bt();

        btnConnect = new ConnectButton(findViewById(R.id.btn_connect));
        btnGPS = new ConnectButton(findViewById(R.id.btn_gps));
        btnACC = new ConnectButton(findViewById(R.id.btn_acc));

        messageView = findViewById(R.id.message_view);
        messageView.setMovementMethod(new ScrollingMovementMethod());

        if (!bluetooth.isBluetoothAvailable()) {
            a.popMessage("Bluetooth is not available");
            finish();
        }

        bluetooth.setBluetoothConnectionListener(new BluetoothSPP.BluetoothConnectionListener() {
            public void onDeviceConnected(String name, String address) {
                // HC-06 -> 98:D3:32:30:78:62
                addMessage("Connected to " + name + ".");
                uiBTConnected();
            }
            public void onDeviceDisconnected() {
                addMessage("Connection lost.");
                uiBTDisconnected();
            }
            public void onDeviceConnectionFailed() {
                addMessage("Unable to connect.");
                uiBTDisconnected();
            }
        });

        bluetooth.setOnDataReceivedListener((data, message) -> {
            addMessage(message);

            if (message.substring(0, message.indexOf(";")).equals("crash")) {
                // Found crash
                String t = "Crash occured!", b = "SHOW";
                String m = "The device received information about crash. Click show to see details.";
                crashDialog = a.genDialog(MainActivity.this, t, m, b, (dialog, which) -> {
                    Intent intent = new Intent(getApplicationContext(), MapsActivity.class);
                    intent.putExtra("crash", message);
                    startActivity(intent);
                    dialog.dismiss();
                    crashDialog = null;
                }, (dialog, which) -> {
                    dialog.dismiss();
                    crashDialog = null;
                });

                crashDialog.show();
            }
            else if (message.substring(0, message.indexOf(";")).equals("gps") && isDebugModeOn) {
                String t = "Debugging: show the location?", b = "SHOW";
                String m = "The device returned its current GPS location, would you like to see it on map?";
                crashDialog = a.genDialog(MainActivity.this, t, m, b, (dialog, which) -> {
                    Intent intent = new Intent(getApplicationContext(), MapsActivity.class);
                    intent.putExtra("gps", message);
                    startActivity(intent);
                    dialog.dismiss();
                    crashDialog = null;
                }, (dialog, which) -> {
                    dialog.dismiss();
                    crashDialog = null;
                });

                crashDialog.show();
            }

        });

        btnConnect.onClick((View view) -> {

            if (!bluetooth.isServiceAvailable()) {
                bluetooth.setupService();
                bluetooth.startService(BluetoothState.DEVICE_OTHER);
                a.popMessage("Starting up bluetooth... press again to connect");
            }
            else {
                bluetooth.connect("98:D3:32:30:78:62");
                btnConnect.setText("Connecting...");
                btnConnect.disable();
            }

        });

        btnGPS.disable();
        btnGPS.onClick((View v) -> bluetooth.send(CMD_GPS, false));

        btnACC.disable();
        btnACC.onClick((View v) -> bluetooth.send(CMD_ACC, false));

        if (crashDialog != null) {
            crashDialog.show();
            if (helpDialog != null) {
                helpDialog = null;
            }
        }
        if (helpDialog != null) helpDialog.show();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        if (item.getItemId() == R.id.menu_disconnect) {
            if (bluetooth.getServiceState() == BluetoothState.STATE_CONNECTED) {
                a.popMessage("Disconnection successfull.");
                bluetooth.disconnect();
                btnConnect.enable();
                btnACC.disable();
                btnGPS.disable();
            }
            else {
                a.popMessage("There is nothing to be disconnected from.");
            }
            return true;
        }

        else if (item.getItemId() == R.id.menu_toggle_debug) {
            isDebugModeOn = !isDebugModeOn;
            a.popMessage("Debug mode is now " + (isDebugModeOn ? "on" : "off"));
            return true;
        }

        else if (item.getItemId() == R.id.menu_help) {
            if (helpDialog == null) {
                String t = "Usage of app", b = "OK";
                String m = "Before connecting to the black box, make sure the box is correctly" +
                        " powered, the device has bluetooth enabled and is paired with the black box." +
                        "\n\nAuthors:\n\nPetr Rusiňák (xrusin03)\nAndrej Hučko (xhucko01)";

                helpDialog = a.genDialog(MainActivity.this, t, m, b, (dialog, which) -> {
                    dialog.dismiss();
                    helpDialog = null;
                }, null);
            }
            helpDialog.show();
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onStart() {
        super.onStart();

        if (!bluetooth.isBluetoothEnabled()) {
            bluetooth.enable();
        } else {
            if (!bluetooth.isServiceAvailable()) {
                bluetooth.setupService();
                bluetooth.startService(BluetoothState.DEVICE_OTHER);
            }
        }

        if (a.restorable()) {
            messageView.setText(a.restoreContent());
            if (a.restoreState()) {
                uiBTConnected();
            }
            else {
                uiBTDisconnected();
            }
        }
    }

    @Override
    public void onDestroy() {
        a.hide(helpDialog, crashDialog);
        a.storeContent(messageView.getText().toString());
        super.onDestroy();
    }

    /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// /// ///  Custom code

    /** @return Time Stamp in format [HH:MM:SS] */
    String getTimeStamp() {
        Calendar c = Calendar.getInstance();
        int h = c.get(Calendar.HOUR_OF_DAY),
            m = c.get(Calendar.MINUTE),
            s = c.get(Calendar.SECOND);

        String time = ((h>9) ? a.s(h) : "0" + h) + ":" +
                      ((m>9) ? a.s(m) : "0" + m) + ":" +
                      ((s>9) ? a.s(s) : "0" + s);

        return "[" + time + "] ";
    }

    /** Add message to text view */
    void addMessage(String message) {

        int len = messageView.getText().toString().length();
        String finmsg;
        if (len == 0) finmsg = getTimeStamp() + message;
        else finmsg = "\n" + getTimeStamp() + message;

        messageView.append(finmsg);
        if (message.length() > 25) messageView.append("\n");
    }

    /** Overrides the buttons after the state change (on successfull connection */
    void uiBTConnected() {
        btnConnect.disable();
        btnConnect.setText("Connected");
        btnACC.enable();
        btnGPS.enable();
        a.storeState(true);
    }

    /** Overrides the buttons after state change (on connection reset / failure) */
    void uiBTDisconnected() {
        btnGPS.disable();
        btnACC.disable();
        btnConnect.enable();
        btnConnect.setText("Connect");
        a.storeState(false);
    }

}

/* Trash:

// 1.) onCreate, btnConnect onclick (before removing the choice list)

//Intent intent = new Intent(getApplicationContext(), DeviceList.class);
//startActivityForResult(intent, BluetoothState.REQUEST_CONNECT_DEVICE);
//bluetooth.autoConnect("HC-06");
//if (bluetooth.getServiceState() == BluetoothState.STATE_CONNECTED)
//    bluetooth.disconnect();
*/

/*
// 2.) testing autoconnection, was not successfull

bluetooth.setAutoConnectionListener(new BluetoothSPP.AutoConnectionListener() {
    @Override
    public void onAutoConnectionStarted() {
        addMessage("Connecting to blackbox...", false);
    }

    @Override
    public void onNewConnection(String name, String address) {
        // HC-06 -> 98:D3:32:30:78:62
        addMessage("Connected to " + name, false);
        uiBTConnected();
    }
});


// 3.) onActivity result when user had to choose the BT device
@Override
public void onActivityResult(int requestCode, int resultCode, Intent data) {

    if (requestCode == BluetoothState.REQUEST_CONNECT_DEVICE) {

        if (resultCode == Activity.RESULT_OK) {

            if (bluetooth == null) {
                popMessage("BT is null! (wtf)");
                //bluetooth = new BluetoothSPP(this);
            }

            bluetooth.connect(data);

        }
    } else if (requestCode == BluetoothState.REQUEST_ENABLE_BT) {

        if (resultCode == Activity.RESULT_OK) {
            bluetooth.setupService();
        } else {
            popMessage("Bluetooth was not enabled.");
            //finish();
        }
    }
}

*/