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
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.BitmapFactory;
import androidx.core.app.NotificationCompat;
import android.graphics.PorterDuffXfermode;
import android.graphics.PorterDuff;
import android.graphics.PorterDuff.Mode;
import android.media.AudioAttributes;

public class NotificatorService extends QtService
{
    // Constants
    private static final int ID_SERVICE = 31415;
	private static String CHANNEL_ID = "buzzer_notifications_01";
    private static NotificatorService instance_;
	private static int running_ = 1;

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
	public void onCreate() {
        super.onCreate();

        // Make intent
        Intent notificationIntent = new Intent(instance_, MainActivity.class);
        PendingIntent pendingIntent =
                PendingIntent.getActivity(instance_, 0, notificationIntent, 0);

        // Create the Foreground Service
        NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        Notification.Builder notificationBuilder = null;

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            String channelId = createNotificationChannel(notificationManager);
            notificationBuilder = new Notification.Builder(this, channelId);
		} else {
            notificationBuilder = new Notification.Builder(this);
        }

        Notification notification = notificationBuilder.setOngoing(true)
		        .setSmallIcon(getNotificationIcon(0))
                .setPriority(NotificationManager.IMPORTANCE_HIGH)
                .setCategory(Notification.CATEGORY_SERVICE)
                .setContentIntent(pendingIntent)
                .setContentTitle("Notification service")
                .setContentText("Active")
                .setShowWhen(true)
                .build();

        startForeground(ID_SERVICE, notification);
    }

    public static Bitmap getCircleBitmap(Bitmap bitmap) {
		//
		Bitmap output;
		Rect srcRect, dstRect;
		float r;
		final int width = bitmap.getWidth();
		final int height = bitmap.getHeight();

		if (width > height) {
			output = Bitmap.createBitmap(height, height, Bitmap.Config.ARGB_8888);
			int left = (width - height) / 2;
			int right = left + height;
			srcRect = new Rect(left, 0, right, height);
			dstRect = new Rect(0, 0, height, height);
			r = height / 2;
		} else {
		    output = Bitmap.createBitmap(width, width, Bitmap.Config.ARGB_8888);
			int top = (height - width)/2;
			int bottom = top + width;
			srcRect = new Rect(0, top, width, bottom);
			dstRect = new Rect(0, 0, width, width);
			r = width / 2;
		}

	    Canvas canvas = new Canvas(output);

		final int color = 0xff424242;
		final Paint paint = new Paint();

		paint.setAntiAlias(true);
		canvas.drawARGB(0, 0, 0, 0);
		paint.setColor(color);
		canvas.drawCircle(r, r, r, paint);
		paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_IN));
		canvas.drawBitmap(bitmap, srcRect, dstRect, paint);

		bitmap.recycle();

		return output;
	}

    public static void notify(int type, String id, String alias, String name, String comment,
	                                                        String text, String avatarPath, String mediaPath) {
		//
		//if (running_ == 0) return;

		//
        Intent notificationIntent = new Intent(instance_, MainActivity.class);
        PendingIntent pendingIntent =
                PendingIntent.getActivity(instance_, 0, notificationIntent, 0);

        NotificationManager notificationManager = (NotificationManager)instance_.getSystemService(Context.NOTIFICATION_SERVICE);

		NotificationCompat.Builder builder = null;
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
			builder = new NotificationCompat.Builder(instance_, CHANNEL_ID);
		} else {
		    builder = new NotificationCompat.Builder(instance_);
        }

		builder.setSmallIcon(getNotificationIcon(type));
		builder.setContentTitle(alias + " (" + name + ") " + comment);
		builder.setContentText(text);
		builder.setLargeIcon(getCircleBitmap(BitmapFactory.decodeFile(avatarPath)));

		if (mediaPath.length() > 0) {
			//
			Bitmap lBitmap = BitmapFactory.decodeFile(mediaPath);
			if (lBitmap != null) {
				builder.setLargeIcon(lBitmap);
			}
		}

	    builder.setStyle(new NotificationCompat.BigTextStyle().bigText(text));

		if (mediaPath.length() > 0) {
			builder.setStyle(new NotificationCompat.BigPictureStyle()
			        .bigPicture(BitmapFactory.decodeFile(mediaPath))
					.bigLargeIcon(null));
		}

	    builder.setPriority(NotificationManager.IMPORTANCE_HIGH);
		builder.setContentIntent(pendingIntent);
		builder.setSound(RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION));
		builder.setShowWhen(true);

        long[] vibrate = { 0, 100, 200, 300 };
        builder.setVibrate(vibrate);

		notificationManager.notify(Integer.parseInt(id), builder.build());
    }

    private static int getNotificationIcon(int eventType) {
		// NOTICE: lollipop officially unsupported by google (2014)
		// boolean useWhiteIcon = (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP);

		switch(eventType) {
			case 0x1006: // TX_REBUZZ, TX_REBUZZ_REPLY
			    return R.drawable.rebuzz;
			case 0x1008: // TX_BUZZ_REPLY
			    return R.drawable.reply;			//!
			case 0x1007: // TX_BUZZ_LIKE
			    return R.drawable.like;				//!
			case 0x100c: // TX_BUZZ_REWARD
			    return R.drawable.donate;			//!
			case 0x1002: // TX_BUZZER_SUBSCRIBE
			    return R.drawable.subscribe;		//!
			case 0x1004: // TX_BUZZER_ENDORSE
			    return R.drawable.endorse;			//!
			case 0x1005: // TX_BUZZER_MISTRUST
			    return R.drawable.mistrust;			//!

			case 0x100e: // TX_BUZZER_CONVERSATION
			    return R.drawable.conversation;
			case 0x100f: // TX_BUZZER_ACCEPT_CONVERSATION
			    return R.drawable.conversationaccepted;
			case 0x1010: // TX_BUZZER_DECLINE_CONVERSATION
			    return R.drawable.conversationdeclined;
			case 0x1011: // TX_BUZZER_MESSAGE, TX_BUZZER_MESSAGE_REPLY
			case 0x1012:
			    return R.drawable.message;			//!
		}

	    return R.drawable.contour;
    }

    //@RequiresApi(Build.VERSION_CODES.O)
	private String createNotificationChannel(NotificationManager notificationManager) {
        String channelId = CHANNEL_ID;
		String channelName = "Buzzer notifications";
		NotificationChannel channel = new NotificationChannel(channelId, channelName, NotificationManager.IMPORTANCE_HIGH);
        // omitted the LED color
		channel.setImportance(NotificationManager.IMPORTANCE_HIGH);
        channel.setLockscreenVisibility(Notification.VISIBILITY_PRIVATE);
		channel.enableVibration(true);
		AudioAttributes audioAttributes = new AudioAttributes.Builder()
		              .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
					  .setUsage(AudioAttributes.USAGE_NOTIFICATION)
					  .build();
		channel.setSound(RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION), audioAttributes);
		long[] vibrate = { 0, 100, 200, 300 };
		channel.setVibrationPattern(vibrate);

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

    public static void pauseNotifications(Context ctx) {
		running_ = 0;
	}

    public static void resumeNotifications(Context ctx) {
		running_ = 1;
	}
}

