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

public class NotificatorBroadcastReceiver extends BroadcastReceiver
{
    @Override
	public void onReceive(Context context, Intent intent) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
			Intent lIntent = new Intent(context, NotificatorService.class);
			lIntent.putExtra("buzzer", "none");
			context.startForegroundService(lIntent);
		} else {
		    Intent lIntent = new Intent(context, NotificatorService.class);
			lIntent.putExtra("buzzer", "none");
			context.startService(lIntent);
		}
    }
}
