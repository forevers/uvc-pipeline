package com.ess.webcam_pipeline;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;

import java.text.DecimalFormat;

public class FrameIntervalDialogFragment extends DialogFragment {

    private static final String TAG = "dialog";

    int selectedFrameInterval;
    int bitsPerPixel;

    /* The activity that creates an instance of this dialog fragment must
     * implement this interface in order to receive event callbacks.
     * Each method passes the DialogFragment in case the host needs to query it. */
    public interface FrameIntervalDialogListener {
        public void onFrameIntervalSelected(FrameIntervalDialogFragment dialog);
    }

    // Use this instance of the interface to deliver action events
    FrameIntervalDialogListener mListener;

    // Override the Fragment.onAttach() method to instantiate the NoticeDialogListener
    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        // Verify that the host activity implements the callback interface
        try {
            // Instantiate the NoticeDialogListener so we can send events to the host
            mListener = (FrameIntervalDialogListener) activity;
        } catch (ClassCastException e) {
            // The activity doesn't implement the interface, throw exception
            throw new ClassCastException(activity.toString()
                    + " must implement NoticeDialogListener");
        }
    }

    Pipeline2ViewActivity.EnumeratedResolution enumeratedResolution;

    public interface NoticeDialogListener {
        public void onSelectFrameInterval(DialogFragment dialog);
    }

    public static FrameIntervalDialogFragment newInstance(int title, Pipeline2ViewActivity.EnumeratedResolution enumeratedResolution, int bitsPerPixel) {

        FrameIntervalDialogFragment frag = new FrameIntervalDialogFragment();
        Bundle args = new Bundle();
        args.putInt("title", title);
        frag.enumeratedResolution = enumeratedResolution;
        frag.bitsPerPixel = bitsPerPixel;
        frag.setArguments(args);
        return frag;
    }

    // dialog for mode selection
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());

        int title = getArguments().getInt("title");
        int size = enumeratedResolution.intervals.size();
        Log.e(TAG, "size : " + size);

        CharSequence[] items = new CharSequence[size];
        for (int i = 0; i< size; i++) {
            int interval = enumeratedResolution.intervals.get(i);
            double fps = 10000000.0/interval;
            double bps = ((double)(enumeratedResolution.height)*enumeratedResolution.width*bitsPerPixel*10000000)/interval;
            items[i] = new DecimalFormat("##0.#####E0").format(bps) + "bps / " + interval/10 + "usec / " + new DecimalFormat("#.##").format(fps) + "fps";
        }

        builder.setTitle(title)
                .setItems(items, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {

                        selectedFrameInterval = enumeratedResolution.intervals.get(which);
                        mListener.onFrameIntervalSelected(FrameIntervalDialogFragment.this);
                    }
                });
        return builder.create();
    }
}
