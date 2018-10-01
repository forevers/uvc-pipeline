package com.ess.webcam_pipeline;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;

import java.util.List;

public class VideoModeDialogFragment extends DialogFragment {

    private static final String TAG = "dialog";

    int selectedMode;
    int selectedInterval;

    /* The activity that creates an instance of this dialog fragment must
     * implement this interface in order to receive event callbacks.
     * Each method passes the DialogFragment in case the host needs to query it. */
    public interface VideoModeDialogListener {
        public void onModeSelected(VideoModeDialogFragment dialog);
    }

    // Use this instance of the interface to deliver action events
    VideoModeDialogListener mListener;

    // Override the Fragment.onAttach() method to instantiate the NoticeDialogListener
    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        // Verify that the host activity implements the callback interface
        try {
            // Instantiate the NoticeDialogListener so we can send events to the host
            mListener = (VideoModeDialogListener) activity;
        } catch (ClassCastException e) {
            // The activity doesn't implement the interface, throw exception
            throw new ClassCastException(activity.toString()
                    + " must implement NoticeDialogListener");
        }
    }

    List<Pipeline2ViewActivity.EnumeratedResolution> mode_list;

    public interface NoticeDialogListener {
        public void onSelectMode(DialogFragment dialog);
    }

    public static VideoModeDialogFragment newInstance(int title, List<Pipeline2ViewActivity.EnumeratedResolution> mode_list) {
        VideoModeDialogFragment frag = new VideoModeDialogFragment();
        Bundle args = new Bundle();
        args.putInt("title", title);
        frag.mode_list = mode_list;
        frag.setArguments(args);
        return frag;
    }

    // dialog for mode selection
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());

        int title = getArguments().getInt("title");
        int size = mode_list.size();
        Log.e(TAG, "size : " + size);

        CharSequence[] items = new CharSequence[size];
        for (int i = 0; i< size; i++) {
            Pipeline2ViewActivity.EnumeratedResolution element = mode_list.get(i);
            items[i] = element.width + " x " + element.height + " @ " + element.defaultFrameInterval;
            Log.e(TAG, element.width + " x " + element.height + " @ " + element.defaultFrameInterval);
        }

        builder.setTitle(title)
                .setItems(items, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {

                        selectedMode = which;
                        // TODO long press for interval other than defalut
                        selectedInterval = mode_list.get(which).defaultFrameInterval;
                        mListener.onModeSelected(VideoModeDialogFragment.this);
                    }
                });
        return builder.create();
    }
}
