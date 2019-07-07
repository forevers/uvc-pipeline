package com.ess.util;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Iterator;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;


/*
	UsbMonitor provides for external USB device connection monitoring and management.
 */
public final class UsbMonitor {

    private static final String TAG = UsbMonitor.class.getSimpleName();

    private static final String ACTION_USB_PERMISSION = "com.ess.util.USB_PERMISSION";

    /* permission granted to usb device */
    WeakReference<UsbDevice> weakUsbDevice;

    /* activity context */
    private final WeakReference<Context> weakContext;

    private final UsbManager usbManager;
    private final OnUsbDeviceConnectListener onUsbDeviceConnectListener;
    private UsbConnectionData usbConnectionData;

    private PendingIntent permissionIntent = null;

    /* async message queue based processing */
    HandlerThread handerlThread;
    Handler handler;
    Looper looper;
    private volatile boolean handlerThreadDestroyed;


    /**
     * constructor passed activity context and usb listener ifc
     * @param context Activity context
     * @param listener USB listener ifc
     */
    public UsbMonitor(final Context context, final OnUsbDeviceConnectListener listener) {

        weakContext = new WeakReference<>(context);

        usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);

        onUsbDeviceConnectListener = listener;

        /* construct async processing message queued thread */
        handerlThread = new HandlerThread(TAG);
        handerlThread.start();
        looper = handerlThread.getLooper();
        /* post async Runnable to handler */
        handler = new Handler(looper);
        handlerThreadDestroyed = false;
    }


    /**
     * client callback interface for USB events
     */
    public interface OnUsbDeviceConnectListener {
	
        /**
         * called when USB device attached
         * @param device attached USB device
         */
        public void onAttach(UsbDevice device);
		
        /**
         * called when USB device detached
         * @param device detached USB device
         */
        public void onDetach(UsbDevice device);
		
        /**
         * called after device opened
         * @param device opened USB device
         * @param usbConnectionData opened USB device connection data
         */
        public void onConnect(UsbDevice device, UsbConnectionData usbConnectionData);

        /**
         * called when USB device is disconnected
         * @param device disconnected USB device
         * @param usbConnectionData disconnected USB device connection data
         */
        public void onDisconnect(UsbDevice device, UsbConnectionData usbConnectionData);

        /**
         * called when requestPermission() permission request fails
         * @param device requestPermission
         */
		public void onCancel(UsbDevice device);
    }


    /**
     * open permissible USB device
     *
     * @param device
     */
	private final void processConnect(final UsbDevice device) {

        Log.i(TAG, "processConnect() entry");

        if (handlerThreadDestroyed) return;

        handler.post(new Runnable() {

            @Override
            public void run() {

                Log.v(TAG, "device : " + device);

                if (usbConnectionData == null) {
                    usbConnectionData = new UsbConnectionData(UsbMonitor.this, device);
                }

                /* create ifc callback if not registered yet */
                if (onUsbDeviceConnectListener != null) {
                    onUsbDeviceConnectListener.onConnect(device, usbConnectionData);
                }
            }
        });

        Log.i(TAG, "processConnect() exit");
    }


    private final void processCancel(final UsbDevice device) {

        Log.i(TAG, "processCancel() entry");

        if (handlerThreadDestroyed) return;

        if (onUsbDeviceConnectListener != null) {

            handler.post(new Runnable() {

                @Override
                public void run() {
					onUsbDeviceConnectListener.onCancel(device);
                }
            });
        }

        Log.i(TAG, "processCancel() exit");
    }


    public void destroy() {

        Log.i(TAG, "destroy() entry");

        unregister();
        if (!handlerThreadDestroyed) {
            handlerThreadDestroyed = true;
            if (usbConnectionData != null) {
                usbConnectionData.close();
            }
            try {
                handler.getLooper().quit();
            } catch (final Exception e) {
                Log.e(TAG, "destroy() : ", e);
            }
        }

        Log.i(TAG, "destroy() exit");
    }


    /**
     * register BroadcastReceiver to monitor USB events
     *
     * @throws IllegalStateException
     */
    public synchronized void register() throws IllegalStateException {

        Log.i(TAG, "register() entry");

        if (handlerThreadDestroyed) throw new IllegalStateException("already destroyed");

        if (permissionIntent == null) {

            Log.i(TAG, "register:");

            final Context context = weakContext.get();
            if (context != null) {

                permissionIntent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), 0);
                final IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);

                filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
                filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);

                context.registerReceiver(usbReceiver, filter);
            }
        }

        Log.i(TAG, "register() exit");
    }


    /**
     * unregister BroadcastReceiver
     *
     * @throws IllegalStateException
     */
    public synchronized void unregister() throws IllegalStateException {

        Log.i(TAG, "unregister() entry");

        if (permissionIntent != null) {

            final Context context = weakContext.get();
            try {
                if (context != null) {
                    context.unregisterReceiver(usbReceiver);
                }
            } catch (final Exception e) {
                Log.w(TAG, e);
            }
            permissionIntent = null;
        }

        Log.i(TAG, "unregister() exit");
    }


    public synchronized boolean isRegistered() {
        return !handlerThreadDestroyed && (permissionIntent != null);
    }


    /**
     * get the first (if any device) in the xml filter which matches pid/vid of connected device
     * @param context
     * @param deviceFilterXmlId
     * @return
     */
    public UsbDevice getDevice(final Context context, final int deviceFilterXmlId) {

        Log.i(TAG, "getDevice() entry");

        UsbDevice device = null;
        HashMap<String, UsbDevice> deviceList = usbManager.getDeviceList();
        String vendorId;
        String productId;

        final XmlPullParser parser = context.getResources().getXml(deviceFilterXmlId);
        try {
            int eventType = parser.getEventType();

            while (eventType != XmlPullParser.END_DOCUMENT && device == null) {

                String tagname;

                switch (eventType) {

                    case (XmlPullParser.START_TAG):

                        tagname = parser.getName();

                        if (tagname.equals("usb-device")) {

                            vendorId = parser.getAttributeValue(null, "vendor-id");
                            productId = parser.getAttributeValue(null, "product-id");

                            Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();
                            while (deviceIterator.hasNext()) {
                                UsbDevice usbDeviceIter = deviceIterator.next();
                                if ((usbDeviceIter.getProductId() == Integer.parseInt(productId)) && (usbDeviceIter.getVendorId() == Integer.parseInt(vendorId))) {
                                    device = usbDeviceIter;
                                    break;
                                }
                            }
                        }
                        break;

                    default:
                        break;
                }
                eventType = parser.next();
            }

        } catch (final XmlPullParserException e) {
            Log.d(TAG, "XmlPullParserException", e);
        } catch (final IOException e) {
            Log.d(TAG, "IOException", e);
        }

        Log.i(TAG, "getDevice() exit");

        return device;
    }


    /**
     * @param device
     * @return true if device has permission
     * @throws IllegalStateException
     */
    public final boolean hasPermission(final UsbDevice device) throws IllegalStateException {

        if (handlerThreadDestroyed) throw new IllegalStateException("already destroyed");

        return (device != null && usbManager.hasPermission(device));
	}


    /**
     * request permission if needed ... connect if device already has permission
     * @param context activity context
     * @param device USB device
     * @return true if success, else false
     */
    public synchronized boolean requestPermission(Context context, final UsbDevice device) {

        Log.i(TAG, "requestPermission() entry");

        boolean result = false;

        if (isRegistered()) {

            if (device != null) {

                if (usbManager.hasPermission(device)) {

                    /* openDevice to enable device endpoint communication */
                    processConnect(device);
                    result = true;

                } else {

                    try {
                        Log.i(TAG,"issue usb permission request intent");

                        if (context != null) {
                            PendingIntent intent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), 0);
                            usbManager.requestPermission(device, intent);
                        }

                    } catch (final Exception e) {
                        Log.e(TAG, e.toString());
                        processCancel(device);
                        result = true;
                    }
                }
            } else {
                processCancel(device);
                result = true;
            }
        } else {
            processCancel(device);
            result = true;
        }

        Log.i(TAG, "requestPermission() exit");

        return result;
    }

    /**
     * open USB device and construct connection data
     * @param device
     * @return UsbConnectionData
     * @throws SecurityException
     */
    public UsbConnectionData openDevice(final UsbDevice device) throws SecurityException
    {
        Log.i(TAG, "openDevice() entry");

        try {
            if (hasPermission(device)) {
                if (null == usbConnectionData) {
                    usbConnectionData = new UsbConnectionData(UsbMonitor.this, device);
                }
                if (null == usbConnectionData.getConnection()) {
                    /* null connection nulls the entire data object */
                    usbConnectionData = null;
                }
            } else {
                Log.i(TAG, "no permissions");
            }
        } catch (final Exception e) {
            Log.e(TAG, "destroy:", e);
        }

        Log.i(TAG, "openDevice() exit");

        return usbConnectionData;
    }

    /**
     * USB device connection data
     */
    public final class UsbConnectionData {

        private UsbDeviceConnection connection;
        private final WeakReference<UsbMonitor> weakMonitor;
        private final WeakReference<UsbDevice> weakDevice;
        private final int busNum;
        private final int devNum;


        protected UsbConnectionData(final UsbMonitor monitor, final UsbDevice device) {
            weakMonitor = new WeakReference<UsbMonitor>(monitor);
            weakDevice = new WeakReference<UsbDevice>(device);
            connection = monitor.usbManager.openDevice(device);
            final String name = device.getDeviceName();
            final String[] v = !TextUtils.isEmpty(name) ? name.split("/") : null;
            int busnum = 0;
            int devnum = 0;
            if (v != null) {
                busnum = Integer.parseInt(v[v.length - 2]);
                devnum = Integer.parseInt(v[v.length - 1]);
            }
            this.busNum = busnum;
            this.devNum = devnum;
        }


        /**
         * get UsbDeviceConnection
         *
         * @return
         */
        public synchronized UsbDeviceConnection getConnection() {
            return connection;
        }


        /* Close device and client connection listener */
        public synchronized void close() {

            Log.i(TAG, "close() entry");

            if (connection != null) {

                connection.close();
                connection = null;
                final UsbMonitor monitor = weakMonitor.get();
                if (monitor != null) {
                    if (monitor.onUsbDeviceConnectListener != null) {
                        monitor.onUsbDeviceConnectListener.onDisconnect(weakDevice.get(), UsbConnectionData.this);
                    }
                    monitor.usbConnectionData = null;
                }
            }

            Log.i(TAG, "close() exit");
        }


        /**
         * get device name
         *
         * @return
         */
        public String getDeviceName() {
            final UsbDevice device = weakDevice.get();
            return device != null ? device.getDeviceName() : "";
        }


        /**
         * get file descriptor to access USB device
         *
         * @return
         * @throws IllegalStateException
         */
        public synchronized int getFileDescriptor() throws IllegalStateException {
            if (connection == null) {
                throw new IllegalStateException("already closed");
            }
            return connection.getFileDescriptor();
        }

        /**
         * @return bus number
         */
        public int getBusNum() {
            return busNum;
        }


        /**
         * @return device number
         */
        public int getDevNum() {
            return devNum;
        }


        /**
         * get product ID
         *
         * @return
         */
        public int getProductId() {
            final UsbDevice device = weakDevice.get();
            return device != null ? device.getProductId() : 0;
        }


        /**
         * get vendor ID
         *
         * @return
         */
        public int getVenderId() {
            final UsbDevice device = weakDevice.get();
            return device != null ? device.getVendorId() : 0;
        }

    };


    /**
     * broadcast receiver fof USB events
     */
    private final BroadcastReceiver usbReceiver = new BroadcastReceiver() {

        public void onReceive(Context context, Intent intent) {

            Log.i(TAG, "onReceive() entry");

            String action = intent.getAction();

            if (ACTION_USB_PERMISSION.equals(action)) {

                Log.i(TAG, "ACTION_USB_PERMISSION");

                synchronized (this) {

                    UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

                    if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {

                        if (device != null) {

                            Log.d(TAG, "permission granted for device : " + device);
                            /* openDevice to enable device endpoint communication */
                            processConnect(device);
                        }

                    } else {
                        Log.d(TAG, "permission denied for device " + device);
                        // https://medium.com/@iiro.krankka/its-time-to-kiss-goodbye-to-your-implicit-broadcastreceivers-eefafd9f4f8a
                        // https://developer.android.com/guide/components/broadcast-exceptions
                        // To allow permission dialog ...
                        //   manually allow permissions in the Settings > Apps & Notifications > App > Advanced > Permissions > Toggle Camera Permission
                        Toast.makeText(context, "Go to App Settings and allow Camera Permissions", Toast.LENGTH_SHORT).show();
                    }
                }

            } else if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {

                Log.i(TAG, "ACTION_USB_DEVICE_ATTACHED");

            } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {

                Log.i(TAG, "ACTION_USB_DEVICE_DETACHED");

                final UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

                if (device != null) {
					if (usbConnectionData != null) {
                        usbConnectionData.close();
					}
                    usbConnectionData = null;
                    processDetach(device);
                }
            }

            Log.i(TAG, "onReceive() exit");
        }
    };

    /**
     * client listener indication when USB device removed
     * @param device
     */
    private final void processDetach(final UsbDevice device) {

        Log.i(TAG, "processDetach() entry");

        if (handlerThreadDestroyed) return;

        if (onUsbDeviceConnectListener != null) {
            handler.post(new Runnable() {
                @Override
                public void run() {
                    onUsbDeviceConnectListener.onDetach(device);
                }
            });
        }

        Log.i(TAG, "processDetach() exit");
    }

}
