package com.vutbr.fit.sen.blackbox.senblackbox;

import android.widget.Button;

/**
 * Wrapper class for {@link Button}, with background drawables for fast style change.
 * @author Andrej Hucko
 */
class ConnectButton {

    private Button button;

    ConnectButton(Button button) {
        this.button = button;
    }

    void setText(String text) {
        button.setText(text);
    }

    void enable() {
        button.setClickable(true);
        button.setBackgroundResource(R.drawable.btn_shape);
    }

    void disable() {
        button.setClickable(false);
        button.setBackgroundResource(R.drawable.btn_shape_disabled);
    }

    void onClick(Button.OnClickListener listener) {
        button.setOnClickListener(listener);
    }

}
