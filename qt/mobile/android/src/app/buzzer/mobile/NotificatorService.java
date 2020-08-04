package app.buzzer.mobile;

import org.qtproject.qt5.android.bindings.QtService;
import org.qtproject.qt5.android.bindings.QtActivity;
import android.util.Log;
import android.content.BroadcastReceiver;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.app.PendingIntent;
import android.app.NotificationChannel;
import android.os.Build;
import android.content.IntentFilter;
import android.media.RingtoneManager;
import android.app.ActivityManager;
import android.app.Notification.BigTextStyle;

public class NotificatorService extends QtService
{
    // Constants
    private static final int ID_SERVICE = 31415;
    private static String CHANNEL_ID = "buzzer_notification_channel";
    private static NotificatorService instance_;

    public NotificatorService()
    {
        instance_ = this;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId)
    {
        super.onStartCommand(intent, flags, startId);
        return START_STICKY;
    }

    @Override
    public void onCreate()
    {
        super.onCreate();

        // Make intent
        Intent notificationIntent = new Intent(instance_, MainActivity.class);
        PendingIntent pendingIntent =
                PendingIntent.getActivity(instance_, 0, notificationIntent, 0);

        // Create the Foreground Service
        NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        Notification.Builder notificationBuilder = null;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
        {
            String channelId = createNotificationChannel(notificationManager);
            notificationBuilder = new Notification.Builder(this, channelId);
        }
        else
        {
            notificationBuilder = new Notification.Builder(this);
        }

        Notification notification = notificationBuilder.setOngoing(true)
                .setSmallIcon(getNotificationIcon())
                .setPriority(NotificationManager.IMPORTANCE_HIGH)
                .setCategory(Notification.CATEGORY_SERVICE)
                .setContentIntent(pendingIntent)
                .setContentTitle("Notification service")
                .setContentText("Active")
                .setShowWhen(true)
                .build();

        startForeground(ID_SERVICE, notification);
    }

    public static void notify(String id, String title, String message)
    {
        Intent notificationIntent = new Intent(instance_, MainActivity.class);
        PendingIntent pendingIntent =
                PendingIntent.getActivity(instance_, 0, notificationIntent, 0);

        NotificationManager notificationManager = (NotificationManager)instance_.getSystemService(Context.NOTIFICATION_SERVICE);

        Notification.Builder builder = null;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
        {
            builder = new Notification.Builder(instance_, CHANNEL_ID);
        }
        else
        {
            builder = new Notification.Builder(instance_);
        }

        Notification.BigTextStyle textStyle = new Notification.BigTextStyle();
        textStyle = textStyle.bigText(message).setBigContentTitle(title);
        builder.setSmallIcon(getNotificationIcon());
        builder.setContentTitle(title);
        builder.setStyle(new Notification.BigTextStyle().bigText(message));
        builder.setPriority(NotificationManager.IMPORTANCE_HIGH);
        builder.setContentIntent(pendingIntent);
        builder.setStyle(textStyle);
        builder.setContentText(message);
        builder.setSound(RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION));
        builder.setShowWhen(true);

        long[] vibrate = { 0, 100, 200, 300 };
        builder.setVibrate(vibrate);

        notificationManager.notify(Integer.parseInt(id) /* may be some event id?*/, builder.build());
    }

    private static int getNotificationIcon()
    {
        boolean useWhiteIcon = (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP);
        return useWhiteIcon ? R.drawable.contour : R.drawable.icon;
    }

    //@RequiresApi(Build.VERSION_CODES.O)
    private String createNotificationChannel(NotificationManager notificationManager)
    {
        String channelId = CHANNEL_ID;
        String channelName = "Buzzer Notification Channel";
        NotificationChannel channel = new NotificationChannel(channelId, channelName, NotificationManager.IMPORTANCE_HIGH);
        // omitted the LED color
        channel.setImportance(NotificationManager.IMPORTANCE_NONE);
        channel.setLockscreenVisibility(Notification.VISIBILITY_PRIVATE);
        notificationManager.createNotificationChannel(channel);
        return channelId;
    }

    public static boolean isServiceRunning(Context ctx, String className)
    {
            ActivityManager manager = (ActivityManager) ctx.getSystemService(Context.ACTIVITY_SERVICE);
            for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE))
            {
                if (className.equals(service.service.getClassName()))
                {
                    return true;
                }
            }
            return false;
    }

    public static void startNotificatorService(Context ctx)
    {
        if (!isServiceRunning(ctx, "app.buzzer.mobile.NotificatorService"))
            ctx.startService(new Intent(ctx, NotificatorService.class));
    }

    public static void stopNotificatorService(Context ctx)
    {
        if (isServiceRunning(ctx, "app.buzzer.mobile.NotificatorService"))
            ctx.stopService(new Intent(ctx, NotificatorService.class));
    }
}

