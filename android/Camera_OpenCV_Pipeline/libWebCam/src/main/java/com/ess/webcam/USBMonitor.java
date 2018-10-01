/*
 *  UVCCamera
 *  library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2017 saki t_saki@serenegiant.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 *  All files in the folder are under this Apache License, Version 2.0.
 *  Files in the libjpeg-turbo, libusb, libuvc, rapidjson folder
 *  may have a different license, see the respective files.
 */

/*
 * Modifications copyright (C) 2018 Microvision apps@microvision.com
 */

package com.ess.webcam;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Locale;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.os.Handler;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;

import com.serenegiant.utils.HandlerThreadHandler;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

public final class USBMonitor {

    private static final boolean DEBUG = true;
    //  private static final String TAG = "USBMonitor";
    private static final String TAG = "test";

    private static final String ACTION_USB_PERMISSION = "com.ess.webcam.USB_PERMISSION";

//  public static final String ACTION_USB_DEVICE_ATTACHED = "android.hardware.usb.action.USB_DEVICE_ATTACHED";

    private UsbControlBlock mCtrlBlock;
    /* permission granted to usb device */
    WeakReference<UsbDevice> mHasPermission;

    private final WeakReference<Context> mWeakContext;
    private final UsbManager mUsbManager;
    private final OnDeviceConnectListener mOnDeviceConnectListener;
    private PendingIntent mPermissionIntent = null;

    private final Handler mAsyncHandler;
    private volatile boolean destroyed;

    public interface OnDeviceConnectListener {
        /**
         * called when device attached
         * @param device
         */
        public void onAttach(UsbDevice device);
        /**
         * called when device dettach(after onDisconnect)
         * @param device
         */
        public void onDetach(UsbDevice device);
        /**
         * called after device opened
         * @param device
         * @param ctrlBlock
         */
//        public void onConnect(UsbDevice device, UsbControlBlock ctrlBlock, boolean createNew);
        public void onConnect(UsbDevice device, UsbControlBlock ctrlBlock);

        /**
         * called when USB device removed or its power off (this callback is called after device closing)
         *
         * @param device
         * @param ctrlBlock
         */
        public void onDisconnect(UsbDevice device, UsbControlBlock ctrlBlock);
        /**
         * called when canceled or could not get permission from user
         * @param device
         */
//		public void onCancel(UsbDevice device);
    }

    public USBMonitor(final Context context, final OnDeviceConnectListener listener) {
//	public USBMonitor(final Context context, UsbDevice usbDevice) {
//		if (listener == null)
//			throw new IllegalArgumentException("OnDeviceConnectListener should not null.");
        mWeakContext = new WeakReference<Context>(context);
        mUsbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        mOnDeviceConnectListener = listener;
        mAsyncHandler = HandlerThreadHandler.createHandler(TAG);
        destroyed = false;
    }

    public void destroy() {
        unregister();
        if (!destroyed) {
            destroyed = true;
            if (mCtrlBlock != null) {
                mCtrlBlock.close();
            }
            try {
                mAsyncHandler.getLooper().quit();
            } catch (final Exception e) {
                Log.e(TAG, "destroy:", e);
            }
        }
    }

    /**
     * register BroadcastReceiver to monitor USB events
     *
     * @throws IllegalStateException
     */
    public synchronized void register() throws IllegalStateException {
        if (destroyed) throw new IllegalStateException("already destroyed");
        if (mPermissionIntent == null) {
            if (DEBUG) Log.i(TAG, "register:");
            final Context context = mWeakContext.get();
            if (context != null) {
                mPermissionIntent = PendingIntent.getBroadcast(context, 0, new Intent(ACTION_USB_PERMISSION), 0);
                final IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
                // ACTION_USB_DEVICE_ATTACHED never comes on some devices so it should not be added here
                filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
                context.registerReceiver(mUsbReceiver, filter);
            }
        }
    }

    /**
     * unregister BroadcastReceiver
     *
     * @throws IllegalStateException
     */
    public synchronized void unregister() throws IllegalStateException {
        if (DEBUG) Log.i(TAG, "unregister:");
        mDeviceCounts = 0;
        if (!destroyed) {
//			mAsyncHandler.removeCallbacks(mDeviceCheckRunnable);
        }
        if (mPermissionIntent != null) {
            if (DEBUG) Log.i(TAG, "unregister:");
            final Context context = mWeakContext.get();
            try {
                if (context != null) {
                    context.unregisterReceiver(mUsbReceiver);
                }
            } catch (final Exception e) {
                Log.w(TAG, e);
            }
            mPermissionIntent = null;
        }
    }

    public synchronized boolean isRegistered() {
        return !destroyed && (mPermissionIntent != null);
    }

    public UsbDevice getDevice(final Context context, final int deviceFilterXmlId) {

        UsbDevice usbDevice = null;
        HashMap<String, UsbDevice> deviceList = mUsbManager.getDeviceList();
        String vendorId = null;
        String productId = null;

        final XmlPullParser parser = context.getResources().getXml(deviceFilterXmlId);
        try {
            int eventType = parser.getEventType();

            while (eventType != XmlPullParser.END_DOCUMENT && usbDevice == null) {

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
                                    usbDevice = usbDeviceIter;
                                    break;
                                }
                            }
                        } else if (tagname.equals("vendor-id")) {
                            vendorId = parser.getAttributeValue(0);
                        } else if (tagname.equals("product-id")) {
                            productId = parser.getAttributeValue(0);
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

        return usbDevice;
    }

    /**
     * return whether the specific Usb device has permission
     * @param device
     * @return true:
     * @throws IllegalStateException
     */
//	public final boolean hasPermission(final UsbDevice device) throws IllegalStateException {
//		if (destroyed) throw new IllegalStateException("already destroyed");
//		return updatePermission(device, device != null && mUsbManager.hasPermission(device));
//	}

    /**
     * @param device
     * @param hasPermission
     * @return hasPermission
     */
    private boolean updatePermission(final UsbDevice device, final boolean hasPermission) {
//		final int deviceKey = getDeviceKey(device, true);
        synchronized (mHasPermission) {
            if (hasPermission) {
                mHasPermission = new WeakReference<UsbDevice>(device);
//				if (mHasPermissions.get(deviceKey) == null) {
//					mHasPermissions.put(deviceKey, new WeakReference<UsbDevice>(device));
//				}
            } else {
//				mHasPermissions.remove(deviceKey);
                mHasPermission = null;
            }
        }
        return hasPermission;
    }
//	private boolean updatePermission(final UsbDevice device, final boolean hasPermission) {
//		final int deviceKey = getDeviceKey(device, true);
//		synchronized (mHasPermissions) {
//			if (hasPermission) {
//				if (mHasPermissions.get(deviceKey) == null) {
//					mHasPermissions.put(deviceKey, new WeakReference<UsbDevice>(device));
//				}
//			} else {
//				mHasPermissions.remove(deviceKey);
//			}
//		}
//		return hasPermission;
//	}

    /**
     * request permission to access to USB device
     *
     * @param device
     * @return true if fail to request permission
     */
    public synchronized boolean requestPermission(final UsbDevice device) {
        boolean result = false;
        if (isRegistered()) {
            if (device != null) {
                if (mUsbManager.hasPermission(device)) {
                    // call onConnect if app already has permission
                    processConnect(device);
                } else {
                    try {
                        mUsbManager.requestPermission(device, mPermissionIntent);
                    } catch (final Exception e) {
                        Log.w(TAG, e);
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
        return result;
    }

//	public synchronized boolean requestPermission(final UsbDevice device) {
////		if (DEBUG) Log.v(TAG, "requestPermission:device=" + device);
//		boolean result = false;
//		if (isRegistered()) {
//			if (device != null) {
//				if (mUsbManager.hasPermission(device)) {
//					// call onConnect if app already has permission
//					processConnect(device);
//				} else {
//					try {
//						// パーミッションがなければ要求する
//						mUsbManager.requestPermission(device, mPermissionIntent);
//					} catch (final Exception e) {
//						// Android5.1.xのGALAXY系でandroid.permission.sec.MDM_APP_MGMTという意味不明の例外生成するみたい
//						Log.w(TAG, e);
//						processCancel(device);
//						result = true;
//					}
//				}
//			} else {
//				processCancel(device);
//				result = true;
//			}
//		} else {
//			processCancel(device);
//			result = true;
//		}
//		return result;
//	}

    /**
     * @param device
     * @return
     * @throws SecurityException
     */
    public UsbControlBlock openDevice(final UsbDevice device) throws SecurityException {
//		if (hasPermission(device)) {
//			UsbControlBlock result = mCtrlBlocks.get(device);
//			if (result == null) {
        if (mCtrlBlock == null) {
//				result = new UsbControlBlock(USBMonitor.this, device);
            mCtrlBlock = new UsbControlBlock(USBMonitor.this, device);
//				mCtrlBlocks.put(device, result);
        }
//			return result;
        return mCtrlBlock;

//		} else {
//			throw new SecurityException("has no permission");
//		}
    }

    /**
     * BroadcastReceiver for USB permission
     */
//	TODO - register this method to obtain permission to com with device

    private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {

        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ACTION_USB_PERMISSION.equals(action)) {
                synchronized (this) {
                    UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

                    if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                        if (device != null) {
                            //call method to set up device communication
                            processConnect(device);
                        }
                    } else {
                        Log.d(TAG, "permission denied for device " + device);
                    }
                }

            } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {

                // when device removed
                final UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                if (device != null) {
//					UsbControlBlock ctrlBlock = mCtrlBlocks.remove(device);
//					if (ctrlBlock != null) {
//						// cleanup
//						ctrlBlock.close();
//					}
//					mDeviceCounts = 0;
                    processDetach(device);
                }
            }
        }
    };

//	private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
//
//		@Override
//		public void onReceive(final Context context, final Intent intent) {
//			if (destroyed) return;
//			final String action = intent.getAction();
//			if (ACTION_USB_PERMISSION.equals(action)) {
//				// when received the result of requesting USB permission
//				synchronized (USBMonitor.this) {
//					final UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
//					if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
//						if (device != null) {
//							// get permission, call onConnect
//							processConnect(device);
//						}
//					} else {
//						// failed to get permission
//						processCancel(device);
//					}
//				}
//			} else if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
//				final UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
//				updatePermission(device, hasPermission(device));
//				processAttach(device);
//			} else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
//				// when device removed
//				final UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
//				if (device != null) {
//					UsbControlBlock ctrlBlock = mCtrlBlocks.remove(device);
//					if (ctrlBlock != null) {
//						// cleanup
//						ctrlBlock.close();
//					}
//					mDeviceCounts = 0;
//					processDetach(device);
//				}
//			}
//		}
//	};

    /**
     * number of connected & detected devices
     */
    private volatile int mDeviceCounts = 0;

    /**
     * periodically check connected devices and if it changed, call onAttach
     */
//	private final Runnable mDeviceCheckRunnable = new Runnable() {
//		@Override
//		public void run() {
//			if (destroyed) return;
//			final List<UsbDevice> devices = getDeviceList();
//			final int n = devices.size();
//			final int hasPermissionCounts;
//			final int m;
//			synchronized (mHasPermissions) {
//				hasPermissionCounts = mHasPermissions.size();
//				mHasPermissions.clear();
//				for (final UsbDevice device: devices) {
//					hasPermission(device);
//				}
//				m = mHasPermissions.size();
//			}
//			if ((n > mDeviceCounts) || (m > hasPermissionCounts)) {
//				mDeviceCounts = n;
//				if (mOnDeviceConnectListener != null) {
//					for (int i = 0; i < n; i++) {
//						final UsbDevice device = devices.get(i);
//						mAsyncHandler.post(new Runnable() {
//							@Override
//							public void run() {
//								mOnDeviceConnectListener.onAttach(device);
//							}
//						});
//					}
//				}
//			}
//			mAsyncHandler.postDelayed(this, 2000);	// confirm every 2 seconds
//		}
//	};

    /**
     * open specific USB device
     *
     * @param device
     */
//	private final void processConnect(final UsbDevice device) {
    public final void processConnect(final UsbDevice device) {
        if (destroyed) return;
//		updatePermission(device, true);
        mAsyncHandler.post(new Runnable() {
            @Override
            public void run() {
                if (DEBUG) Log.v(TAG, "processConnect:device=" + device);
                UsbControlBlock ctrlBlock;
                final boolean createNew;
//				ctrlBlock = mCtrlBlocks.get(device);
//				if (ctrlBlock == null) {
                if (mCtrlBlock == null) {
//					ctrlBlock = new UsbControlBlock(USBMonitor.this, device);
                    mCtrlBlock = new UsbControlBlock(USBMonitor.this, device);
//					mCtrlBlocks.put(device, ctrlBlock);
                    createNew = true;
                } else {
                    createNew = false;
                }
                if (mOnDeviceConnectListener != null) {
//					mOnDeviceConnectListener.onConnect(device, ctrlBlock, createNew);
                    mOnDeviceConnectListener.onConnect(device, mCtrlBlock);
                }
            }
        });
    }

    private final void processCancel(final UsbDevice device) {
        if (destroyed) return;
        if (DEBUG) Log.v(TAG, "processCancel:");
        updatePermission(device, false);
        if (mOnDeviceConnectListener != null) {
            mAsyncHandler.post(new Runnable() {
                @Override
                public void run() {
//					mOnDeviceConnectListener.onCancel(device);
                }
            });
        }
    }

//	private final void processAttach(final UsbDevice device) {
//		if (destroyed) return;
//		if (DEBUG) Log.v(TAG, "processAttach:");
//		if (mOnDeviceConnectListener != null) {
//			mAsyncHandler.post(new Runnable() {
//				@Override
//				public void run() {
//					mOnDeviceConnectListener.onAttach(device);
//				}
//			});
//		}
//	}

	private final void processDetach(final UsbDevice device) {
		if (destroyed) return;
		if (DEBUG) Log.v(TAG, "processDetach:");
		if (mOnDeviceConnectListener != null) {
			mAsyncHandler.post(new Runnable() {
				@Override
				public void run() {
					mOnDeviceConnectListener.onDetach(device);
				}
			});
		}
	}

    public static class UsbDeviceInfo {
        public String usb_version;
        public String manufacturer;
        public String product;
        public String version;
        public String serial;

        private void clear() {
            usb_version = manufacturer = product = version = serial = null;
        }

        @Override
        public String toString() {
            return String.format("UsbDevice:usb_version=%s,manufacturer=%s,product=%s,version=%s,serial=%s",
                    usb_version != null ? usb_version : "",
                    manufacturer != null ? manufacturer : "",
                    product != null ? product : "",
                    version != null ? version : "",
                    serial != null ? serial : "");
        }
    }

    private static final int USB_DIR_IN = 0x80;
    private static final int USB_TYPE_STANDARD = (0x00 << 5);
    private static final int USB_RECIP_DEVICE = 0x00;
    private static final int USB_REQ_GET_DESCRIPTOR = 0x06;

    private static final int USB_REQ_STANDARD_DEVICE_GET = (USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE);            // 0x90

    private static final int USB_DT_STRING = 0x03;

    /**
     * @param connection
     * @param id
     * @param languageCount
     * @param languages
     * @return
     */
    private static String getString(final UsbDeviceConnection connection, final int id, final int languageCount, final byte[] languages) {
        final byte[] work = new byte[256];
        String result = null;
        for (int i = 1; i <= languageCount; i++) {
            int ret = connection.controlTransfer(
                    USB_REQ_STANDARD_DEVICE_GET, // USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE
                    USB_REQ_GET_DESCRIPTOR,
                    (USB_DT_STRING << 8) | id, languages[i], work, 256, 0);
            if ((ret > 2) && (work[0] == ret) && (work[1] == USB_DT_STRING)) {
                // skip first two bytes(bLength & bDescriptorType), and copy the rest to the string
                try {
                    result = new String(work, 2, ret - 2, "UTF-16LE");
                    if (!"Љ".equals(result)) {
                        break;
                    } else {
                        result = null;
                    }
                } catch (final UnsupportedEncodingException e) {
                    // ignore
                }
            }
        }
        return result;
    }

    /**
     * @param device
     * @return
     */
    public UsbDeviceInfo getDeviceInfo(final UsbDevice device) {
        return updateDeviceInfo(mUsbManager, device, null);
    }

    /**
     * #updateDeviceInfo(final UsbManager, final UsbDevice, final UsbDeviceInfo)
     *
     * @param context
     * @param device
     * @return
     */
    public static UsbDeviceInfo getDeviceInfo(final Context context, final UsbDevice device) {
        return updateDeviceInfo((UsbManager) context.getSystemService(Context.USB_SERVICE), device, new UsbDeviceInfo());
    }

    /**
     * @param manager
     * @param device
     * @param _info
     * @return
     */
    public static UsbDeviceInfo updateDeviceInfo(final UsbManager manager, final UsbDevice device, final UsbDeviceInfo _info) {
        final UsbDeviceInfo info = _info != null ? _info : new UsbDeviceInfo();
        info.clear();

        if (device != null) {
//			if (BuildCheck.isLollipop()) {
//				info.manufacturer = device.getManufacturerName();
//				info.product = device.getProductName();
//				info.serial = device.getSerialNumber();
//			}
//			if (BuildCheck.isMarshmallow()) {
//				info.usb_version = device.getVersion();
//			}
            if ((manager != null) && manager.hasPermission(device)) {
                final UsbDeviceConnection connection = manager.openDevice(device);
                final byte[] desc = connection.getRawDescriptors();

                if (TextUtils.isEmpty(info.usb_version)) {
                    info.usb_version = String.format("%x.%02x", ((int) desc[3] & 0xff), ((int) desc[2] & 0xff));
                }
                if (TextUtils.isEmpty(info.version)) {
                    info.version = String.format("%x.%02x", ((int) desc[13] & 0xff), ((int) desc[12] & 0xff));
                }
                if (TextUtils.isEmpty(info.serial)) {
                    info.serial = connection.getSerial();
                }

                final byte[] languages = new byte[256];
                int languageCount = 0;
                // controlTransfer(int requestType, int request, int value, int index, byte[] buffer, int length, int timeout)
                try {
                    int result = connection.controlTransfer(
                            USB_REQ_STANDARD_DEVICE_GET, // USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE
                            USB_REQ_GET_DESCRIPTOR,
                            (USB_DT_STRING << 8) | 0, 0, languages, 256, 0);
                    if (result > 0) {
                        languageCount = (result - 2) / 2;
                    }
                    if (languageCount > 0) {
                        if (TextUtils.isEmpty(info.manufacturer)) {
                            info.manufacturer = getString(connection, desc[14], languageCount, languages);
                        }
                        if (TextUtils.isEmpty(info.product)) {
                            info.product = getString(connection, desc[15], languageCount, languages);
                        }
                        if (TextUtils.isEmpty(info.serial)) {
                            info.serial = getString(connection, desc[16], languageCount, languages);
                        }
                    }
                } finally {
                    connection.close();
                }
            }
//			if (TextUtils.isEmpty(info.manufacturer)) {
//				info.manufacturer = USBVendorId.vendorName(device.getVendorId());
//			}
            if (TextUtils.isEmpty(info.manufacturer)) {
                info.manufacturer = String.format("%04x", device.getVendorId());
            }
            if (TextUtils.isEmpty(info.product)) {
                info.product = String.format("%04x", device.getProductId());
            }
        }
        return info;
    }

    /**
     * control class
     * never reuse the instance when it closed
     */
    public static final class UsbControlBlock implements Cloneable {
        private final WeakReference<USBMonitor> mWeakMonitor;
        private final WeakReference<UsbDevice> mWeakDevice;
        protected UsbDeviceConnection mConnection;
        protected final UsbDeviceInfo mInfo;
        private final int mBusNum;
        private final int mDevNum;
        private final SparseArray<SparseArray<UsbInterface>> mInterfaces = new SparseArray<SparseArray<UsbInterface>>();

        /**
         * this class needs permission to access USB device before constructing
         *
         * @param monitor
         * @param device
         */
        protected UsbControlBlock(final USBMonitor monitor, final UsbDevice device) {
//		private UsbControlBlock(final USBMonitor monitor, final UsbDevice device) {
            if (DEBUG) Log.i(TAG, "UsbControlBlock:constructor");
            mWeakMonitor = new WeakReference<USBMonitor>(monitor);
            mWeakDevice = new WeakReference<UsbDevice>(device);
            mConnection = monitor.mUsbManager.openDevice(device);
            mInfo = updateDeviceInfo(monitor.mUsbManager, device, null);
            final String name = device.getDeviceName();
            final String[] v = !TextUtils.isEmpty(name) ? name.split("/") : null;
            int busnum = 0;
            int devnum = 0;
            if (v != null) {
                busnum = Integer.parseInt(v[v.length - 2]);
                devnum = Integer.parseInt(v[v.length - 1]);
            }
            mBusNum = busnum;
            mDevNum = devnum;
            if (DEBUG) {
                if (mConnection != null) {
                    final int desc = mConnection.getFileDescriptor();
                    final byte[] rawDesc = mConnection.getRawDescriptors();
                    Log.i(TAG, String.format(Locale.US, "name=%s,desc=%d,busnum=%d,devnum=%d,rawDesc=", name, desc, busnum, devnum) + rawDesc);
                } else {
                    Log.e(TAG, "could not Connect to device " + name);
                }
            }
        }

        /**
         * copy constructor
         *
         * @param src
         * @throws IllegalStateException
         */
        private UsbControlBlock(final UsbControlBlock src) throws IllegalStateException {
            final USBMonitor monitor = src.getUSBMonitor();
            final UsbDevice device = src.getDevice();
            if (device == null) {
                throw new IllegalStateException("device may already be removed");
            }
            mConnection = monitor.mUsbManager.openDevice(device);
            if (mConnection == null) {
                throw new IllegalStateException("device may already be removed or have no permission");
            }
            mInfo = updateDeviceInfo(monitor.mUsbManager, device, null);
            mWeakMonitor = new WeakReference<USBMonitor>(monitor);
            mWeakDevice = new WeakReference<UsbDevice>(device);
            mBusNum = src.mBusNum;
            mDevNum = src.mDevNum;
        }

        /**
         * duplicate by clone
         * need permission
         * USBMonitor never handle cloned UsbControlBlock, you should release it after using it.
         *
         * @return
         * @throws CloneNotSupportedException
         */
        @Override
        public UsbControlBlock clone() throws CloneNotSupportedException {
            final UsbControlBlock ctrlblock;
            try {
                ctrlblock = new UsbControlBlock(this);
            } catch (final IllegalStateException e) {
                throw new CloneNotSupportedException(e.getMessage());
            }
            return ctrlblock;
        }

        public USBMonitor getUSBMonitor() {
            return mWeakMonitor.get();
        }

        public final UsbDevice getDevice() {
            return mWeakDevice.get();
        }

        /**
         * get device name
         *
         * @return
         */
        public String getDeviceName() {
            final UsbDevice device = mWeakDevice.get();
            return device != null ? device.getDeviceName() : "";
        }

        /**
         * get UsbDeviceConnection
         *
         * @return
         */
        public synchronized UsbDeviceConnection getConnection() {
            return mConnection;
        }

        /**
         * get file descriptor to access USB device
         *
         * @return
         * @throws IllegalStateException
         */
        public synchronized int getFileDescriptor() throws IllegalStateException {
            checkConnection();
            return mConnection.getFileDescriptor();
        }

        /**
         * get vendor id
         *
         * @return
         */
        public int getVenderId() {
            final UsbDevice device = mWeakDevice.get();
            return device != null ? device.getVendorId() : 0;
        }

        /**
         * get product id
         *
         * @return
         */
        public int getProductId() {
            final UsbDevice device = mWeakDevice.get();
            return device != null ? device.getProductId() : 0;
        }

        public int getBusNum() {
            return mBusNum;
        }

        public int getDevNum() {
            return mDevNum;
        }

        /**
         * Close device
         * This also close interfaces if they are opened in Java side
         */
        public synchronized void close() {
            if (DEBUG) Log.i(TAG, "UsbControlBlock#close:");

            if (mConnection != null) {
                final int n = mInterfaces.size();
                for (int i = 0; i < n; i++) {
                    final SparseArray<UsbInterface> intfs = mInterfaces.valueAt(i);
                    if (intfs != null) {
                        final int m = intfs.size();
                        for (int j = 0; j < m; j++) {
                            final UsbInterface intf = intfs.valueAt(j);
                            mConnection.releaseInterface(intf);
                        }
                        intfs.clear();
                    }
                }
                mInterfaces.clear();
                mConnection.close();
                mConnection = null;
                final USBMonitor monitor = mWeakMonitor.get();
                if (monitor != null) {
                    if (monitor.mOnDeviceConnectListener != null) {
                        monitor.mOnDeviceConnectListener.onDisconnect(mWeakDevice.get(), UsbControlBlock.this);
                    }
//					monitor.mCtrlBlocks.remove(getDevice());
                    monitor.mCtrlBlock = null;
                }
            }
        }

        @Override
        public boolean equals(final Object o) {
            if (o == null) return false;
            if (o instanceof UsbControlBlock) {
                final UsbDevice device = ((UsbControlBlock) o).getDevice();
                return device == null ? mWeakDevice.get() == null
                        : device.equals(mWeakDevice.get());
            } else if (o instanceof UsbDevice) {
                return o.equals(mWeakDevice.get());
            }
            return super.equals(o);
        }


        private synchronized void checkConnection() throws IllegalStateException {
            if (mConnection == null) {
                throw new IllegalStateException("already closed");
            }
        }
    }

}
