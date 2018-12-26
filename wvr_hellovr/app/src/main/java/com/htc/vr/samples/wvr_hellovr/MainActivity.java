// "WaveVR SDK 
// © 2017 HTC Corporation. All Rights Reserved.
//
// Unless otherwise required by copyright law and practice,
// upon the execution of HTC SDK license agreement,
// HTC grants you access to and use of the WaveVR SDK(s).
// You shall fully comply with all of HTC’s SDK license agreement terms and
// conditions signed by you and all SDK and API requirements,
// specifications, and documentation provided by HTC to You."

package com.htc.vr.samples.wvr_hellovr;

import com.htc.vr.BuildConfig;
import com.htc.vr.sdk.VRActivity;

import android.Manifest;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.annotation.TargetApi;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;


public class MainActivity extends VRActivity {
    private static final String TAG = "wvr_hellovr";

    private static final String ACTION_SWITCH_DEBUG = "com.htc.vr.samples.wvr_hellovr.ACTION_SWITCH_DEBUG";
    private static final String ACTION_SWITCH_MSAA = "com.htc.vr.samples.wvr_hellovr.ACTION_SWITCH_MSAA";
    private static final String ACTION_SWITCH_SCENE = "com.htc.vr.samples.wvr_hellovr.ACTION_SWITCH_SCENE";
    private static final String SP_DEBUG = "debug";
    private static final int FLAG_DEBUG = 0x01;
    private static final int FLAG_MSAA = 0x02;
    private static final int FLAG_SCENE = 0x04;
    private boolean mDebug = false;
    private int mFlag = FLAG_MSAA;

    static {
        System.loadLibrary("hellovr_jni");
    }
    public MainActivity() {
        super.setUsingRenderBaseActivity(true);
    }

    private BroadcastReceiver receiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.i(TAG,"onReceive: start");
            if (intent.getAction().equals(ACTION_SWITCH_DEBUG)) {
                mDebug = !mDebug;
                SharedPreferences p = PreferenceManager.getDefaultSharedPreferences(getContext());
                p.edit().putBoolean(SP_DEBUG, mDebug).commit();
                mFlag = (mFlag & ~FLAG_DEBUG) | (mDebug ? FLAG_DEBUG : 0);
            } else if (intent.getAction().equals(ACTION_SWITCH_MSAA)) {
                boolean msaa = (mFlag & FLAG_MSAA) == 0;  // invert flag value here
                mFlag = (mFlag & ~FLAG_MSAA) | (msaa ? FLAG_MSAA : 0);
            } else if (intent.getAction().equals(ACTION_SWITCH_SCENE)) {
                boolean scene = (mFlag & FLAG_SCENE) == 0;  // invert flag value here
                mFlag = (mFlag & ~FLAG_SCENE) | (scene ? FLAG_SCENE : 0);
            } else {
                Log.i(TAG,"onReceive: end1");
                return;
            }

            setFlag(mFlag);

            clearNotification();
            setNotification();
            Log.i(TAG,"onReceive: end2");
        }
    };

    static native void setFlag(int flag);

    @Override
    protected void onCreate(Bundle icicle) {
        Log.i(TAG,"onCreate:call init");
        init(getResources().getAssets());
        super.onCreate(icicle);

        // dump verion information
        try {
            Log.i(TAG, "(Native HelloVR) VR_VERSION: " + BuildConfig.VR_VERSION);
            PackageManager pm = getPackageManager();
            PackageInfo info = pm.getPackageInfo(getApplicationInfo().packageName, 0);
            Log.i(TAG, "Native HelloVR version name: " + info.versionName + " code: " + info.versionCode);
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }


        SharedPreferences p = PreferenceManager.getDefaultSharedPreferences(this);
        mDebug = p.getBoolean(SP_DEBUG, false);
        mFlag |= mDebug ? FLAG_DEBUG : 0;

        setFlag(mFlag);
         Log.i(TAG,"onCreate:end");
    }

    // The camera support.
    // If the external camera exist, we use external camera.
    // If not, we use the camera on the phone.  However the phone's camera
    // need the permission to get work.  Please add a right permission in the
    // AndroidManifest.
    private boolean mHasCameraPermission = false;

    private void setNotification() {
    Log.i(TAG,"setNotification:start");
        PendingIntent pIntent = PendingIntent.getBroadcast(this, FLAG_DEBUG,
            new Intent(ACTION_SWITCH_DEBUG), PendingIntent.FLAG_UPDATE_CURRENT);

        int iconid = mDebug ? android.R.drawable.star_on : android.R.drawable.star_off;
        Notification.Action actionDebug = new Notification.Action.Builder(
                    iconid, mDebug ? "Debug On" : "Debug Off", pIntent).build();

        pIntent = PendingIntent.getBroadcast(this, FLAG_MSAA,
            new Intent(ACTION_SWITCH_MSAA), PendingIntent.FLAG_UPDATE_CURRENT);

        boolean msaa = (mFlag & FLAG_MSAA) != 0;
        iconid = msaa ? android.R.drawable.ic_media_next : android.R.drawable.ic_media_ff;
        Notification.Action actionMsaa = new Notification.Action.Builder(
                        iconid, msaa ? "MSAA On" : "MSAA Off", pIntent).build();

        pIntent = PendingIntent.getBroadcast(this, FLAG_SCENE,
            new Intent(ACTION_SWITCH_SCENE), PendingIntent.FLAG_UPDATE_CURRENT);

        boolean scene = (mFlag & FLAG_SCENE) != 0;
        iconid = android.R.drawable.ic_popup_sync;
        Notification.Action actionScene = new Notification.Action.Builder(
                                    iconid, "Scene", pIntent).build();

        Notification n  = new Notification.Builder(this)
                .setContentTitle("WVR HelloVR Flag Switcher")
                .setSmallIcon(android.R.drawable.ic_menu_manage)
                .setOngoing(true)
                .addAction(actionMsaa)
                .addAction(actionScene)
                .addAction(actionDebug)
                .build();
        NotificationManager notificationManager =
                (NotificationManager) getSystemService(NOTIFICATION_SERVICE);

        notificationManager.notify(0, n);
        IntentFilter filter = new IntentFilter();
        filter.addAction(ACTION_SWITCH_DEBUG);
        filter.addAction(ACTION_SWITCH_MSAA);
        filter.addAction(ACTION_SWITCH_SCENE);
        registerReceiver(receiver, filter);
        Log.i(TAG,"setNotification:end");
    }

    private void clearNotification() {
        Log.i(TAG,"clearNotification:start");
        NotificationManager notificationManager =
                (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        notificationManager.cancelAll();
        unregisterReceiver(receiver);
        Log.i(TAG,"clearNotification:end");
    }

    @Override
    protected void onResume() {
        super.onResume();
         Log.i(TAG,"onResume:start");
        setNotification();
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.i(TAG,"onPause:start");
        clearNotification();
    }


    @Override
    public void onRequestPermissionsResult(int requestCode,  String[] permissions,  int[] grantResults) {
        // Avoid crash when user don't confirm the permission after pause/resume.
        if (requestCode == 1 && grantResults != null && grantResults.length > 0) {
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                mHasCameraPermission = true;
            }
        }
    }

    // Pass this acitivty instance to native
    @SuppressWarnings("unused")
    public native void init(AssetManager am);
}
