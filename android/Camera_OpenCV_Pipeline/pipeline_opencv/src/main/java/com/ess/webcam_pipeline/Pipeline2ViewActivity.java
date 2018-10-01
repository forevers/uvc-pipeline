package com.ess.webcam_pipeline;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.support.constraint.ConstraintLayout;
import android.support.constraint.ConstraintSet;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v4.view.GestureDetectorCompat;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import com.ess.webcam.ICamera;
import com.ess.webcam.IRenderer;
import com.ess.webcam.Opencv;
import com.ess.webcam.Renderer;
import com.ess.utils.ThreadedHandler;
import com.ess.webcam.Camera;
import com.ess.webcam.USBMonitor;
import com.ess.webcam.IStatusCallback;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import static android.view.MotionEvent.ACTION_DOWN;

public class Pipeline2ViewActivity
        extends AppCompatActivity
        implements VideoModeDialogFragment.VideoModeDialogListener,
        FrameIntervalDialogFragment.FrameIntervalDialogListener {

    ConstraintSet constraintSet = new ConstraintSet();

    private ICamera camera = null;
    private Opencv opencv = null;

    ConstraintLayout constraintLayout;
    ViewTreeObserver viewTreeObserver;
    int constraintLayoutWidth;
    int constraintLayoutHeight;

    String resolutionString;

    boolean keepScreenOn = false;

    /* render surfaces */
    ArrayList<RenderSurface> renderSurfaceList;

    private class RenderSurface implements SurfaceHolder.Callback {

        public RenderSurface() {}

        @Override
        public void surfaceCreated(final SurfaceHolder holder) {
            if (DEBUG) Log.e(TAG, "surfaceCreated()");
        }

        @Override
        public void surfaceChanged(final SurfaceHolder holder, final int format, final int width, final int height) {
            if (DEBUG) Log.e(TAG, "surfaceChanged()");

            if ((width == 0) || (height == 0)) {
                Log.e(TAG, "(width == 0) || (height == 0)");
                Log.e(TAG, "width = " + width + " height = " + height);
            }

            previewSurface = holder.getSurface();
            if (DEBUG) Log.e(TAG, "previewSurface " + previewSurface);

            if (previewSurface != null) {
                synchronized (pipelineSync) {
                    if (isActive && (renderer != null) && (camera != null) ) {
                        renderer.setSurface(previewSurface);
                        isPreview = true;
                    }
                }
            } else {
                Log.e(TAG, "previewSurface == null");
            }
        }

        @Override
        public void surfaceDestroyed(final SurfaceHolder holder) {
            if (DEBUG) Log.e(TAG, "surfaceDestroyed()");

            previewSurface = null;
            if (DEBUG) Log.e(TAG, "previewSurface null");
        }

        private SurfaceView cameraView;
        private Surface previewSurface;
//        /* surface bound to jni */
//        private boolean isPreview;

        private int shift;

        /* overlay view text */
        private TextView overlayTextView;

        private IRenderer renderer = null;
    }

    /* start stop preview button */
    private ImageButton cameraButton;

    /* state */
    private boolean isActive;
    private boolean isPreview;

    /* uvc encoding mode */
    class EnumeratedResolution {
        public EnumeratedResolution(int width, int height, int defaultFrameInterval, int frameIntervalType, List<Integer> intervals) {

            this.width = width;
            this.height = height;
            this.defaultFrameInterval = defaultFrameInterval;
            this.frameIntervalType = frameIntervalType;
            if (frameIntervalType == 0) {
                this.intervals = null;
            } else {
                this.intervals = intervals;
            }
        }
        int width;
        int height;
        int defaultFrameInterval;
        int frameIntervalType;
        List<Integer> intervals = new ArrayList<Integer>();
    }

    class NegotiatedResolution {

        NegotiatedResolution(EnumeratedResolution enumeratedResolution, int negotiatedFrameInterval, int frameFormat) {
            this.enumeratedResolution = enumeratedResolution;
            this.negotiatedFrameInterval = negotiatedFrameInterval;
            this.frameFormat = frameFormat;
        }

        EnumeratedResolution enumeratedResolution;
        int negotiatedFrameInterval;
        int frameFormat;
    }
    NegotiatedResolution negotiatedResolution = null;

    Map<String, List<EnumeratedResolution>> fourccMultiMap = new HashMap<String, List<EnumeratedResolution>>();

    /* camera async metadata callback display */
    private TextView cameraMetadata;
    private ScrollView cameraMetadataScroll;

    /* SW version */
    private TextView version;

    /* camera enumeration data */
    private TextView uvcMode;

    // view overlay test
    private TextView overlayView_1;

    /* JSON parser */
    private CameraStatusParser statusParser;

    private static final boolean DEBUG = true;

    //    private static final String TAG = Pipeline2ViewActivity.class.getSimpleName();
    private static final String TAG = "test";

    /* camera sync object */
    private final Object pipelineSync = new Object();

    /* async handler thread */
    private final Handler uiHandler = new Handler(Looper.getMainLooper());
    private final Thread uiThread = uiHandler.getLooper().getThread();
    private ThreadedHandler threadedHandler;

    /* USB monitor for attach/detach and permission management */
    private USBMonitor usbMonitor;

    UsbManager usbManager = null;

    private static final String ACTION_USB_PERMISSION = "com.android.example.USB_PERMISSION";
    private static int vendorId;
    private static int productId;
    private UsbDevice usbDevice;

    public final int PERMISSIONS_REQUEST_SDCARD_ACCESS = 1;

    // TODO versioning in seperate class
    String versionName = "versionName"; //BuildConfig.VERSIONNAME;
    String versionCode = "versionCode"; //BuildConfig.VERSIONCODE;
    String versionBranch = "versionBranch"; // BuildConfig.VERSIONBRANCH;
    String versionHash = "versionHash"; //BuildConfig.VERSIONHASHSHORT;

    RenderSurface renderSurface;
//    RenderSurface renderSurfaceDepth;
//    RenderSurface renderSurfaceAmplitude;

    private GestureDetectorCompat mDetector;

    class MyGestureListener extends GestureDetector.SimpleOnGestureListener {
        private static final String DEBUG_TAG = "Gestures";

        @Override
        public boolean onDown(MotionEvent event) {
            Log.d(TAG,"onDown: " + event.toString());
            return true;
        }

        @Override
        public boolean onSingleTapUp(MotionEvent event) {
            Log.d(DEBUG_TAG, "onSingleTapUp: " + event.toString());

            synchronized (pipelineSync) {
                if (isActive && isPreview && (camera != null) && (renderSurface != null)) {
                    Log.e(TAG, "ACTION_DOWN");
                    float yAxisVal = event.getY();
                    int viewHeight = renderSurface.cameraView.getHeight();
                    if (yAxisVal < viewHeight / 3) {
                        if (usbDevice != null) {
                            if (isActive == true && isPreview == true) {
                                opencv.cycleProcessingMode();
                            }
                        }
                    } else {
                        if (renderSurface.cameraView.getVisibility() == View.VISIBLE) {
                            renderSurface.cameraView.setVisibility(View.GONE);
                            renderSurface.overlayTextView.setVisibility(View.GONE);
                            renderSurface.renderer.setSurface(null);
                        } else {
                            renderSurface.cameraView.setVisibility(View.VISIBLE);
                            renderSurface.overlayTextView.setVisibility(View.VISIBLE);
                            renderSurface.renderer.setSurface(renderSurface.previewSurface);
                        }
                    }
                }
            }
            return true;
        }

        @Override
        public boolean onFling(MotionEvent event1, MotionEvent event2, float velocityX, float velocityY) {
            Log.d(TAG, "onFling: " + event1.toString() + event2.toString());

            synchronized (pipelineSync) {
                if (isActive && isPreview && (camera != null) && (renderSurface != null)) {
                    if (Math.abs(velocityX) > Math.abs(velocityY)) {
                        boolean positiveVelocity = (velocityX > 0) ? true : false;
                        int channel_scale = opencv.scaleChannelIfGray16(positiveVelocity);
                        renderSurface.overlayTextView.setText("Depth [" + (7 + channel_scale) + ":" + channel_scale + "]");
                    }
                }
            }

            return true;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // hide status bar
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        View decorView = getWindow().getDecorView();
        int uiOptions = View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
        decorView.setSystemUiVisibility(uiOptions);

        constraintLayout = (ConstraintLayout)findViewById(R.id.container);
        viewTreeObserver = constraintLayout.getViewTreeObserver();

        constraintSet.clone(this, R.layout.activity_main);

        cameraMetadata = (TextView)findViewById(R.id.camera_metadata);
        cameraMetadataScroll = (ScrollView)findViewById(R.id.camera_metadata_scroll);
        cameraMetadata.setVisibility(View.GONE);
        cameraMetadataScroll.setVisibility(View.GONE);

        version = (TextView)findViewById(R.id.version);
        version.setText("Pipeline App\nversion:" + getResources().getString(R.string.app_version) +"\nbranch:" + versionBranch + "\nhash:" + versionHash);
        version.setVisibility(View.VISIBLE);
        version.setOnTouchListener(new View.OnTouchListener() {

            @Override
            public boolean onTouch(View v, MotionEvent event) {

                boolean touchHandled = false;
                int action = event.getAction();

                switch(action) {
                    case ACTION_DOWN:
                        touchHandled = true;
                        int visibility = Pipeline2ViewActivity.this.cameraMetadataScroll.getVisibility();
                        Pipeline2ViewActivity.this.cameraMetadataScroll.setVisibility((visibility == View.VISIBLE) ? View.GONE : View.VISIBLE);
                        Pipeline2ViewActivity.this.cameraMetadata.setVisibility((visibility == View.VISIBLE) ? View.GONE : View.VISIBLE);

                        break;
                    default:
                        break;
                }

                return touchHandled;
            }
        });

        uvcMode = (TextView)findViewById(R.id.uvc_mode);
        uvcMode.setVisibility(View.INVISIBLE);

        cameraButton = (ImageButton)findViewById(R.id.camera_button);
        cameraButton.setOnClickListener(onClickListener);

        renderSurfaceList = new ArrayList<>();

        // surface gesture detectors
        mDetector = new GestureDetectorCompat(this, new MyGestureListener());

        // camera sensor
        renderSurface = new RenderSurface();
        renderSurface.cameraView = (SurfaceView) findViewById(R.id.camera_surface_view_1);
        renderSurface.cameraView.getHolder().addCallback(renderSurface);
        renderSurface.cameraView.setVisibility(View.VISIBLE);
        renderSurface.previewSurface = null;
        renderSurface.cameraView.setOnTouchListener(new View.OnTouchListener() {

            @Override
            public boolean onTouch(View v, MotionEvent event) {

                boolean touchHandled = false;
                int action = event.getAction();

                return Pipeline2ViewActivity.this.mDetector.onTouchEvent(event);
            }
        });

        renderSurface.overlayTextView = (TextView)findViewById(R.id.overlay_view_1);
        renderSurface.overlayTextView.setText("Depth");
        renderSurface.overlayTextView.setVisibility(View.INVISIBLE);
        renderSurface.shift = 0;
        renderSurfaceList.add(renderSurface);

        isPreview = false;

        viewTreeObserver.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN) {
                    constraintLayout.getViewTreeObserver().removeGlobalOnLayoutListener(this);
                } else {
                    constraintLayout.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                }
                constraintLayoutWidth  = constraintLayout.getMeasuredWidth();
                constraintLayoutHeight = constraintLayout.getMeasuredHeight();
            }
        });

        if (threadedHandler == null) {
            threadedHandler = ThreadedHandler.createHandler();
        }

        // usb detection
        usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        usbMonitor = new USBMonitor(this, onDeviceConnectListener);

        // obtain first USB of device registered in the device_filter.xml list
        usbDevice = usbMonitor.getDevice(this, R.xml.device_filter);

        if (usbDevice == null) {
            Toast.makeText(Pipeline2ViewActivity.this, "No UVC Camera Attached", Toast.LENGTH_SHORT).show();
        } else {
            productId = usbDevice.getProductId();
            vendorId = usbDevice.getVendorId();
            Log.d(TAG, usbDevice.getDeviceName());
        }

        // permission required for SDCARD access
        int permissionCheck = ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE);
        if (permissionCheck != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    PERMISSIONS_REQUEST_SDCARD_ACCESS);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    // If the activity has already been created/instantiated, the ACTION_USB_DEVICE_ATTACHED event will arrive through the 'onNewIntent()' method:
    @Override
    protected void onNewIntent(Intent intent) {
        if (DEBUG) Log.v(TAG, "onNewIntent() enter");

        super.onNewIntent(intent);

        if(intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_ATTACHED)){

            HashMap<String, UsbDevice> deviceList = usbManager.getDeviceList();
            if (!deviceList.isEmpty()) {
                for (UsbDevice device : deviceList.values()) {
                    if (device.getVendorId() == vendorId && device.getProductId() == productId) {

                        if (!usbManager.hasPermission(device)) {

                        } else {
                            Log.i("UsbStateReceiver", "We have the USB permission! ");
                            Toast.makeText(Pipeline2ViewActivity.this, "UVC Camera Attached", Toast.LENGTH_SHORT).show();

                            usbDevice = device;
                        }
                    }
                }
            }
        }

        if (DEBUG) Log.v(TAG, "onNewIntent() exit");
    }

    @Override
    protected void onStart() {
        if (DEBUG) Log.v(TAG, "onStart:");

        super.onStart();

        usbMonitor.register();

        synchronized (pipelineSync) {

            if (camera != null && renderSurfaceList != null ) {

                camera.start();
                opencv.start();
//                opencvDepth.start();
//                opencvAmplitude.start();
                for (RenderSurface renderSurface : renderSurfaceList) {
                    if (renderSurface.renderer != null) {
                        renderSurface.renderer.start();
                    }
                }
            }
        }
    }

    @Override
    protected void onStop() {
        if (DEBUG) Log.v(TAG, "onStop() entry");

        // release native pipeline resources
        releaseCamera();

        synchronized (pipelineSync) {

            if (usbMonitor != null) {
                usbMonitor.unregister();
            }
        }

        super.onStop();

        if (DEBUG) Log.v(TAG, "onStop() exit");
    }

    @Override
    protected void onDestroy() {

        if (DEBUG) Log.v(TAG, "onDestroy:");

        synchronized (pipelineSync) {
            for (RenderSurface renderSurface : renderSurfaceList) {
//                renderSurface.isPreview = false;
            }

            isActive = isPreview = false;
        }

        if (usbMonitor != null) {
            usbMonitor.destroy();
            usbMonitor = null;
        }

        for (RenderSurface renderSurface : renderSurfaceList) {
            renderSurface.cameraView = null;
        }

        cameraButton = null;

        /* terminate async handler thread */
        if (threadedHandler != null) {
            try {
                threadedHandler.getLooper().quit();
            } catch (final Exception e) {
                //
            }
            threadedHandler = null;
        }

        super.onDestroy();
    }

    private class CameraStatusParser {

        private String statusString;

        CameraStatusParser(int cameraStatusSchema, String jsonString) {
            try {
                JSONObject reader = null;
                switch (cameraStatusSchema) {
                    case 3:
                        reader = new JSONObject(jsonString);
                        int numFifoEnqueueOverflows = reader.getInt("ovflw");
                        double ovflwPcnt = reader.getInt("ovflw_pcnt");
                        statusString =  new String("Camera Q Overflows: " + numFifoEnqueueOverflows + "\nCamera Q Overflow %: " + ovflwPcnt + "\n");
                        break;

                    case 6:
                        reader = new JSONObject(jsonString);
                        int numDropFrames = reader.getInt("num_drop_frames");
                        int outFrames = reader.getInt("out_frames");
                        statusString = new String("Drop " + numDropFrames + " of " + outFrames + " frames\n");
                        break;

                    case 7:
                        reader = new JSONObject(jsonString);
                        int numDroppedFrames = reader.getInt("num_dropped_frames");
                        int numTotalFrames = reader.getInt("num_total_frames");
                        statusString = new String("Drop " + numDroppedFrames + " of " + numTotalFrames + " frames\n");
                        break;

                    default:
                        statusString = new String("invalid status schema");
                        break;
                }

                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {

                        cameraMetadata.append("\n" + statusString);
                        Log.d(TAG, statusString);
                        Pipeline2ViewActivity.this.cameraMetadataScroll.setVisibility(View.VISIBLE);
                        Pipeline2ViewActivity.this.cameraMetadata.setVisibility(View.VISIBLE);

                        cameraMetadataScroll.post(new Runnable() {
                            @Override
                            public void run() {
                                cameraMetadataScroll.fullScroll(View.FOCUS_DOWN);
                            }
                        });
                    }
                });

            } catch(JSONException ex) {
                Log.e(TAG, ex.getMessage());
            }

        }

    };

    private void TerminatePipeline() {

        synchronized (pipelineSync) {

            if (camera != null) {
                camera.setStatusCallback(null);
                camera.stop();
                camera = null;
            }

            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    for (RenderSurface renderSurface : renderSurfaceList) {
                        renderSurface.overlayTextView.setVisibility(View.INVISIBLE);
                    }
                }
            });

            for (RenderSurface renderSurface : renderSurfaceList) {
                if (renderSurface.renderer != null) {
                    renderSurface.renderer.stop();
                    renderSurface.renderer = null;
                }
            }

            if (opencv != null) {
                opencv.stop();
                opencv = null;
            }

            isActive = isPreview = false;
        }
    }

    /**
     * Keep screen on during camera render
     * @parameter keepOn boolean indicating screen keep on state
     */
    private void ScreenControl(boolean keepOn) {

        keepScreenOn = keepOn;

        runOnUiThread(new Runnable() {

            @Override
            public void run() {
                if (keepScreenOn == true) {
                    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                } else {
                    getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                }
            }
        });
    }

    private void SelectMode(final USBMonitor.UsbControlBlock ctrlBlock) {

        TerminatePipeline();

        runOnUiThread(new Runnable() {

            @Override
            public void run() {

                camera = new Camera();

                String sizes = camera.prepare(ctrlBlock);
                Log.d(TAG, sizes);
                try {
                    ScreenControl(true);

                    JSONObject object = new JSONObject(sizes);
                    Iterator keys = object.keys();
                    while (keys.hasNext()) {
                        String dynamicKey = (String)keys.next();
                        JSONArray fourccArray = object.getJSONArray(dynamicKey);
                        for (int i = 0; i < fourccArray.length(); i++) {
                            JSONObject fourccObject = fourccArray.getJSONObject(i);
                            String fourcc = fourccObject.getString("fourcc");
                            List<EnumeratedResolution> sizeList = new ArrayList<EnumeratedResolution>();
                            EnumeratedResolution fourccSize;

                            JSONArray sizeArray = fourccObject.getJSONArray("sizes");
                            for (int j = 0; j < sizeArray.length(); j++) {
                                JSONObject sizeObject = sizeArray.getJSONObject(j);
                                String sizeString = sizeObject.getString("resolution");
                                String[] sizeSplit = sizeString.split("x", 2);

                                int defaultFrameInterval = sizeObject.getInt("default_frame_interval");

                                int interval_type = sizeObject.getInt("interval_type");
                                JSONArray intervalArray = sizeObject.getJSONArray("interval_type_array");
                                if (intervalArray.length() != 0) {
                                    List<Integer> intervals = new ArrayList<Integer>();
                                    for (int k = 0; k < intervalArray.length(); k++) {
                                        intervals.add(intervalArray.getInt(k));
                                    }
                                    fourccSize = new EnumeratedResolution( Integer.parseInt(sizeSplit[0]), Integer.parseInt(sizeSplit[1]), defaultFrameInterval, interval_type, intervals);
                                } else {
                                    List<Integer> intervals = new ArrayList<Integer>();
                                    // TODO add 0 - dwMaxFrameInterval and 1 - dwFrameIntervalStep
                                    intervals.add(0);
                                    intervals.add(1);
                                    fourccSize = new EnumeratedResolution( Integer.parseInt(sizeSplit[0]), Integer.parseInt(sizeSplit[1]), defaultFrameInterval, interval_type, intervals);
                                }

                                sizeList.add(fourccSize);
                            }
                            fourccMultiMap.put(fourcc, sizeList);
                        }
                    }

                } catch (JSONException ex) {
                    Log.e(TAG, ex.getMessage());
                    ScreenControl(false);
                }

                if (DEBUG) Log.i(TAG, "supportedSize:" + camera.getSupportedVideoModes());
                negotiatedResolution = null;
                try {

                    if (fourccMultiMap.containsKey("YUY2")) {

                        VideoModeDialogFragment frag = VideoModeDialogFragment.newInstance(R.string.select_mode, fourccMultiMap.get("YUY2"));
                        frag.show(getFragmentManager(), "dialog");
                    }

                } catch (final IllegalArgumentException e) {
                    Log.e(TAG, e.getMessage());
                }

            } // end run()
        });
    }

    boolean bm22Hijack = false;

    public void onModeSelected(VideoModeDialogFragment dialog) {

        Log.e(TAG, "call StartPreviewModal() here");

//        // am i running on ui thread ?
//        if (Looper.getMainLooper().getThread() == Thread.currentThread()) {
//            Log.e(TAG, "running on ui thread");
//        }

        int selected_mode = dialog.selectedMode;
        EnumeratedResolution mode = fourccMultiMap.get("YUY2").get(selected_mode);
        int bitsPerPixel = 2;

        // steal the BM22 mode
        if (vendorId == 1204 && productId == 249 && mode.width == 240 && mode.height == 720) {
            bm22Hijack = true;
            bitsPerPixel = 4;
        }
        negotiatedResolution = new NegotiatedResolution(mode, selected_mode, Camera.CAMERA_FRAME_FORMAT_YUYV);

        if (negotiatedResolution.enumeratedResolution.frameIntervalType != 0) {
            // select frame interval
            FrameIntervalDialogFragment frag = FrameIntervalDialogFragment.newInstance(R.string.select_fps, negotiatedResolution.enumeratedResolution, bitsPerPixel);
            frag.show(getFragmentManager(), "dialog");
        } else {
            StartPreviewModal();
        }
    }

    public void onFrameIntervalSelected(FrameIntervalDialogFragment dialog) {

        negotiatedResolution.negotiatedFrameInterval = dialog.selectedFrameInterval;
        StartPreviewModal();
    }

    private void StartPreviewModal() {

        cameraButton.setImageResource(R.drawable.ic_videocam_off_white_48dp);

        // allows async status platform callbacks
        // TODO best location for callback ???
        camera.setStatusCallback(new IStatusCallback() {

            public void onStatus(final int StatusSchema, String jsonString) {

                statusParser = new CameraStatusParser(StatusSchema, jsonString);
            }
        });

        if (DEBUG) Log.i(TAG, "supportedSize:" + camera.getSupportedVideoModes());
        try {

            if (negotiatedResolution != null) {

                // camera driver/stack image resolution and format
                int fps = 10000000/negotiatedResolution.negotiatedFrameInterval;
                camera.setCaptureMode(negotiatedResolution.enumeratedResolution.width, negotiatedResolution.enumeratedResolution.height, fps, fps, negotiatedResolution.frameFormat);

                if (fourccMultiMap.containsKey("YUY2")) {

                    List<EnumeratedResolution> resolutions = fourccMultiMap.get("YUY2");
                    resolutionString = new String();
                    for (EnumeratedResolution resolution : resolutions) {
                        resolutionString = resolutionString + new String("H " + resolution.height + ", W " + resolution.width + "  @" + resolution.frameIntervalType + "\n");
                    }

                    int bps = (int)((double)(negotiatedResolution.enumeratedResolution.defaultFrameInterval) * negotiatedResolution.enumeratedResolution.height * negotiatedResolution.enumeratedResolution.width * 2);

                    uvcMode.setText(new String("UVC Mode: YUY2\n" + "H " + negotiatedResolution.enumeratedResolution.height + ", W " + negotiatedResolution.enumeratedResolution.width + " @ " + bps + " bps\n"));

                    uvcMode.setVisibility(View.VISIBLE);

                    // TODO why this visibility not effective
                    Pipeline2ViewActivity.this.cameraMetadataScroll.setVisibility(View.VISIBLE);
                    //int visibility = Pipeline2ViewActivity.this.cameraMetadataScroll.getVisibility();
                    Pipeline2ViewActivity.this.cameraMetadata.setVisibility(View.VISIBLE);
                    //visibility = Pipeline2ViewActivity.this.cameraMetadata.getVisibility();
                    cameraMetadata.append("\nUVC Enumeration: YUY2\n" + resolutionString);
                    Log.d(TAG, resolutionString);

                    /* async message console from native code */
//                    cameraMetadataScroll.post(new Runnable() {
//                        @Override
//                        public void run() {
//                            cameraMetadataScroll.fullScroll(View.FOCUS_DOWN);
//                        }
//                    });

                    for (RenderSurface renderSurface : renderSurfaceList) {
                        renderSurface.overlayTextView.setVisibility(View.VISIBLE);
                    }
                }
            } else {
                Log.e(TAG, "negotiatedResolution == null");
            }

        } catch (final IllegalArgumentException e) {
            Log.e(TAG, e.getMessage());
        }

        if (fourccMultiMap.containsKey("YUY2") &&
                renderSurfaceList.get(0).previewSurface != null /*&&
                renderSurfaceList.get(1).previewSurface != null */ ) {

            // register opencv element on depth channel of first sensor
            // BM22 override
            // TODO make this pid/vid driven
            int width_acquisition = negotiatedResolution.enumeratedResolution.width;
            int width_render = negotiatedResolution.enumeratedResolution.width;
            if (bm22Hijack) {
                width_acquisition = 120;
                width_render = 1280;
            }
            opencv = new Opencv(camera.getFrameAccessIfc(0), negotiatedResolution.enumeratedResolution.height, width_acquisition, renderSurfaceList.get(0).shift);

//            // register opencv element on depth channel of first sensor
//            opencvDepth = new Opencv(demux.getFrameAccessIfc(0), negotiatedResolution.height, negotiatedResolution.width, TOF_SENSOR_1, TOF_CHANNEL_FORMAT_DEPTH);
//
//            // register opencv element on amplitude channel of second sensor
//            opencvAmplitude = new Opencv(demux.getFrameAccessIfc(1), negotiatedResolution.height, negotiatedResolution.width, TOF_SENSOR_1, TOF_CHANNEL_FORMAT_AMPLITUDE);
//
//            renderSurfaceList.get(0).isPreview = true;
//            renderSurfaceList.get(1).isPreview = true;

            RenderSurface surface;

            // opencv rendering
            surface = renderSurfaceList.get(0);
            surface.renderer = new Renderer(opencv.getFrameAccessIfc(0), negotiatedResolution.enumeratedResolution.height, width_acquisition);
            surface.renderer.setSurface(surface.previewSurface);
            surface.overlayTextView.setText("Depth [" + (7+surface.shift) + ":" + surface.shift + "]");

            // configure aspect ratio
            android.view.ViewGroup.LayoutParams lp = surface.cameraView.getLayoutParams();
            String ar_string = "w," + Integer.toString(width_render) + ":" + Integer.toString(negotiatedResolution.enumeratedResolution.height);
            Log.e(TAG, ar_string);
            constraintSet.setDimensionRatio(R.id.camera_surface_view_1, ar_string);
            constraintSet.applyTo(constraintLayout);

//            // opencv depth rendering
//            surface = renderSurfaceList.get(0);
//            surface.renderer = new TofRenderer(opencvDepth.getFrameAccessIfc(0), negotiatedResolution.height, negotiatedResolution.width, surface.tofSensor, surface.tofChannelFormat, surface.shift);
//            surface.renderer.setSurface(surface.previewSurface);
//            surface.overlayTextView.setText("Depth [" + (7+surface.shift) + ":" + surface.shift + "]");
//
//            // opencv amplitude rendering
//            surface = renderSurfaceList.get(1);
//            surface.renderer = new TofRenderer(opencvAmplitude.getFrameAccessIfc(0), negotiatedResolution.height, negotiatedResolution.width, surface.tofSensor, surface.tofChannelFormat, surface.shift);
//            surface.renderer.setSurface(surface.previewSurface);
//            surface.overlayTextView.setText("Amplitude [" + (7+surface.shift) + ":" + surface.shift + "]");

            isActive = true;
            camera.start();
            opencv.start();
//            opencvDepth.start();
//            opencvAmplitude.start();
            for (RenderSurface renderSurface : renderSurfaceList) {
                if (renderSurface.cameraView.getVisibility() == View.VISIBLE) {
                    if (renderSurface.renderer != null) {
                        renderSurface.renderer.start();
                    }
                }
            }
            isPreview = true;
        }

        synchronized (pipelineSync) {
            Pipeline2ViewActivity.this.camera = camera;
        }

    }

    private final USBMonitor.OnDeviceConnectListener onDeviceConnectListener = new USBMonitor.OnDeviceConnectListener() {

        @Override
        public void onAttach(final UsbDevice device) {
            if (DEBUG) Log.v(TAG, "onAttach()");

            Toast.makeText(Pipeline2ViewActivity.this, "USB_DEVICE_ATTACHED", Toast.LENGTH_SHORT).show();

            if (usbDevice == null) {

                HashMap<String, UsbDevice> deviceList = usbManager.getDeviceList();
                if (!deviceList.isEmpty()) {
                    for (UsbDevice allowedDevice : deviceList.values()) {
                        if (device.getVendorId() == vendorId && device.getProductId() == productId) {

                            if (usbManager.hasPermission(allowedDevice)) {
                                Log.i("UsbStateReceiver", "We have the USB permission! ");
                                Toast.makeText(Pipeline2ViewActivity.this, "UVC Camera Attached", Toast.LENGTH_SHORT).show();

                                usbDevice = allowedDevice;
                            }
                        }
                    }
                }
            }
        }

        @Override
        public void onDetach(final UsbDevice device) {
            if (DEBUG) Log.v(TAG, "onDetach()");
            Toast.makeText(Pipeline2ViewActivity.this, "USB_DEVICE_DETACHED", Toast.LENGTH_SHORT).show();
            releaseCamera();
            usbDevice = null;
        }

        @Override
        public void onConnect(final UsbDevice device, final USBMonitor.UsbControlBlock ctrlBlock) {
            if (DEBUG) Log.v(TAG, "onConnect() entry");

            TerminatePipeline();

            postAsyncEvent(new Runnable() {
                @Override
                public void run() {
                    synchronized (pipelineSync) {

                        SelectMode(ctrlBlock);
                    }
                }
            }, 0);

        }

        @Override
        public void onDisconnect(final UsbDevice device, final USBMonitor.UsbControlBlock ctrlBlock) {
            if (DEBUG) Log.v(TAG, "onDisconnect()");
        }
    };

    private final View.OnClickListener onClickListener = new View.OnClickListener() {

        @Override
        public void onClick(final View view) {

            synchronized (pipelineSync) {

                if (usbDevice != null) {

                    if (isActive == true && isPreview == true) {

                        releaseCamera();
                        for (RenderSurface renderSurface : renderSurfaceList) {
//                            renderSurface.isPreview = false;
                            renderSurface.overlayTextView.setVisibility(View.INVISIBLE);
                        }
                        isActive = isPreview = false;

                        for (RenderSurface renderSurface : renderSurfaceList) {
                            renderSurface.cameraView.setVisibility(View.VISIBLE);
                        }

                    } else if (usbMonitor.requestPermission(usbDevice)) {
                        // returns false if permissions not currently given and issues permission dialog

                        TerminatePipeline();

                        postAsyncEvent(new Runnable() {
                            @Override
                            public void run() {

                                synchronized (pipelineSync) {

                                    USBMonitor.UsbControlBlock ctrlBlock = usbMonitor.openDevice(usbDevice);

                                    SelectMode(ctrlBlock);
                                }
                            }
                        }, 0);
                    }

                } else {
                    releaseCamera();
                }
            }
        }
    };

    private synchronized void releaseCamera() {
        if (DEBUG) Log.d(TAG, "releaseCamera() entry");

        cameraButton.setImageResource(R.drawable.ic_videocam_white_48dp);

        TerminatePipeline();

        synchronized (pipelineSync) {

            ScreenControl(false);

            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    cameraMetadata.setText("");
                    uvcMode.setText("");
                    fourccMultiMap.clear();
                }
            });

        }

        if (DEBUG) Log.d(TAG, "releaseCamera() exit");
    }

    // from UVCLib BaseActivity
    public final void runOnUiThread(final Runnable task, final long duration) {
        if (task == null) return;
        uiHandler.removeCallbacks(task);
        if ((duration > 0) || Thread.currentThread() != uiThread) {
            uiHandler.postDelayed(task, duration);
        } else {
            try {
                task.run();
            } catch (final Exception e) {
                Log.w(TAG, e);
            }
        }
    }

    public final void removeFromUiThread(final Runnable task) {
        if (task == null) return;
        uiHandler.removeCallbacks(task);
    }

    protected final synchronized void postAsyncEvent(final Runnable task, final long delayMillis) {
        if ((task == null) || (threadedHandler == null)) return;
        try {
            threadedHandler.removeCallbacks(task);
            if (delayMillis > 0) {
                threadedHandler.postDelayed(task, delayMillis);
            } else {
                threadedHandler.post(task);
            }
        } catch (final Exception e) {
            Log.e(TAG, e.toString());
        }
    }

}
