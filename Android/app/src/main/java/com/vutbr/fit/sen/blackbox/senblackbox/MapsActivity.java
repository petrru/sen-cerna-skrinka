package com.vutbr.fit.sen.blackbox.senblackbox;

import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import android.support.v4.app.FragmentActivity;
import com.google.android.gms.maps.*;
import com.google.android.gms.maps.model.*;

/**
 * Activity showing the crash site of blackbox on map.
 * Uses Google Maps API.
 * @author Andrej Hucko
 */
public class MapsActivity extends FragmentActivity implements OnMapReadyCallback {

    private LatLng crashSite;
    private static boolean shouldStay = true;
    private App app;

    /**
     * Set up the map and parse the extras.
     * @param savedInstanceState unused
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_maps);
        // Obtain the SupportMapFragment and get notified when the map is ready to be used.
        SupportMapFragment mf = (SupportMapFragment) getSupportFragmentManager().findFragmentById(R.id.map);
        mf.getMapAsync(this);

        app = (App) getApplication();
        TextView tv = findViewById(R.id.tv_map);

        String crashInfo = getIntent().getStringExtra("crash");
        if (crashInfo != null) {
            String[] info = crashInfo.split(";");
            if (info.length == 6) {
                crashSite = new LatLng(parseCoord(info[4]), parseCoord(info[5]));

                String acceleration =
                        "acc-x: " + info[1] + "\n" +
                        "acc-y: " + info[2] + "\n" +
                        "acc-z: " + info[3];

                tv.setText(acceleration);
            }
            else {
                crashSite = new LatLng(0.0f, 0.0f);
            }
        }
        else {

            crashInfo = getIntent().getStringExtra("gps");
            if (crashInfo != null) {
                // found that the activity received the map command
                String[] info = crashInfo.split(";");
                if (info.length == 3) {
                    crashSite = new LatLng(parseCoord(info[1]), parseCoord(info[2]));
                }
                else {
                    crashSite = new LatLng(0.0f, 0.0f);
                }
            }
            else {
                crashSite = new LatLng(0.0f, 0.0f);
            }
            tv.setVisibility(View.INVISIBLE);

        }
    }

    /**
     * Manipulates the map once available.
     * This callback is triggered when the map is ready to be used.
     * If Google Play services is not installed on the device, the user will be prompted to install
     * it inside the SupportMapFragment. This method will only be triggered once the user has
     * installed Google Play services and returned to the app.
     */
    @Override
    public void onMapReady(GoogleMap googleMap) {
        googleMap.addMarker(new MarkerOptions().position(crashSite).title("Crash site of SEN blackbox"));
        googleMap.animateCamera(CameraUpdateFactory.newLatLngZoom(crashSite, 15), 2500, null);
    }

    /**
     * Press back 2 times before exiting the map interface.
     */
    @Override
    public void onBackPressed() {
        if (shouldStay) {
            app.popMessage("Press again to exit map interface.");
            shouldStay = false;
            return;
        }
        super.onBackPressed();
    }

    /**
     * Parse latitude/longtitude from string to double value.
     * @param item latitude/longtitude
     * @return the double-precision float value
     */
    private double parseCoord(String item) {
        switch (item.length()) {
            case 8:  return calculate(item, 1); // lat/lon = 1 MM mm mm m
            case 9:  return calculate(item, 2); // lat/lon = (-1 | 16) MM mm mm m
            case 10: return calculate(item, 3); // lat/lon = (-16 | 120) MM mm mm m
            case 11: return calculate(item, 4); // lat/lon = -120 MM mm mm m
            default: return 0.0f;
        }
    }

    /**
     * Calculates the latitude/longtitude from string according to the string's length
     * @param item   the string containing lat/lon
     * @param offset offset determined by its length
     * @return double value of the lat/lon
     */
    private double calculate(String item, int offset) {
        int iItem = Integer.parseInt(item.substring(0, offset));
        String minutes = item.substring(offset, offset + 2);
        String decmins = item.substring(offset + 2);
        return (Float.parseFloat(minutes + "." + decmins) / 60) + iItem;
    }

}
