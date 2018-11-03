package com.vutbr.fit.sen.blackbox.senblackbox;

import android.content.*;
import android.widget.Toast;
import android.app.Application;
import android.support.v7.app.AlertDialog;
import app.akexorcist.bluetotohspp.library.*;

/**
 * Top level class handling bluetooth connection and activity state.
 * @author Andrej Hucko
 */
public class App extends Application {

    private String consoleContent;
    private boolean connectedState;
    private BluetoothSPP bluetooth;

    @Override
    public void onCreate() {
        super.onCreate();
        consoleContent = null;
        connectedState = false;
        bluetooth = new BluetoothSPP(getApplicationContext());

        if (!bluetooth.isBluetoothEnabled()) {
            bluetooth.enable();
        } else {
            if (!bluetooth.isServiceAvailable()) {
                bluetooth.setupService();
                bluetooth.startService(BluetoothState.DEVICE_OTHER);
            }
        }
    }

    /**
     * Application holding reference to managing the bluetooth.
     * The bluetooth connection is not interrupted by exiting the main activity.
     * @return reference to bluetooth management object
     */
    BluetoothSPP bt() {
        return bluetooth;
    }

    /**
     * MainActivity state storage method.
     * @param consoleContent content from textview "console"
     */
    void storeContent(String consoleContent) {
        this.consoleContent = consoleContent;
    }

    /**
     * MainActivity state storage method.
     * @param connectedState whether the device is connected to blackbox
     */
    void storeState(boolean connectedState) {
        this.connectedState = connectedState;
    }

    /**
     * @return whether anything is stored
     */
    boolean restorable() {
        return consoleContent != null;
    }

    /**
     * @return the textview previous content
     */
    String restoreContent() {
        return consoleContent;
    }

    /**
     * @return previous connection state
     */
    boolean restoreState() {
        return connectedState;
    }

    // Code shortening methods

    /**
     * calls Integer.toString(). Yes I'm that lazy.
     * @param value the int to be converted
     * @return the string representation
     */
    String s(int value) {
        return Integer.toString(value);
    }

    /**
     * Hides all dialogs if they aren't null
     * @param dialogs varag param
     */
    void hide(AlertDialog... dialogs) {
        for (AlertDialog dialog : dialogs) {
            if (dialog != null) dialog.dismiss();
        }
    }

    /**
     * Generates dialog for the app.
     * @param context     for the builder
     * @param title       title of the dialog
     * @param message     message of the dialog
     * @param btnText     positive button text
     * @param posListener listener for positive button
     * @param negListener listener for negative button, if null, the negative button is not set
     * @return the actual built dialog
     */
    AlertDialog genDialog(Context context, String title, String message, String btnText,
                          DialogInterface.OnClickListener posListener,
                          DialogInterface.OnClickListener negListener) {

        AlertDialog.Builder builder =  new AlertDialog.Builder(context)
                .setCancelable(false)
                .setPositiveButton(btnText, posListener)
                .setTitle(title)
                .setMessage(message);

        if (negListener != null) {
            builder.setNegativeButton("Cancel", negListener);
        }
        return builder.create();
    }

    /** Toast shortcut */
    void popMessage(String message) {
        Toast.makeText(getApplicationContext(), message, Toast.LENGTH_LONG).show();
    }

}
