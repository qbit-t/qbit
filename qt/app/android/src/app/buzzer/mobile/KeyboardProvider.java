package app.buzzer.mobile;

import android.app.Activity;
import android.view.View;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.view.ViewGroup.LayoutParams;
import android.view.Gravity;
import android.view.WindowManager;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.widget.PopupWindow;
import android.content.res.Configuration;
import android.util.Log;

public class KeyboardProvider extends PopupWindow implements OnGlobalLayoutListener {
    private Activity mActivity;
    private View rootView;
    private KeyboardListener listener;
	private int heightMax = 0; // Record the maximum height of the pop content area
	private int absoluteMax = 0;
	private int lastOrientation = 0;
	private boolean firstTimeMajorChanges = false;

    public KeyboardProvider(Activity activity) {
        super(activity);
        this.mActivity = activity;
		//lastOrientation = mActivity.getResources().getConfiguration().orientation;

        // Basic configuration
        rootView = new View(activity);
        setContentView(rootView);

        // Monitor global Layout changes
        rootView.getViewTreeObserver().addOnGlobalLayoutListener(this);
        setBackgroundDrawable(new ColorDrawable(0));

        // Set width to 0 and height to full screen
        setWidth(0);
        setHeight(LayoutParams.MATCH_PARENT);

        // Set keyboard pop-up mode
        setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        setInputMethodMode(PopupWindow.INPUT_METHOD_NEEDED);
    }

    public KeyboardProvider init() {
        if (!isShowing()) {
            final View view = mActivity.getWindow().getDecorView();
            // Delay loading popupwindow, if not, error will be reported
            view.post(new Runnable() {
                @Override
                public void run() {
                    showAtLocation(view, Gravity.NO_GRAVITY, 0, 0);
                }
            });
        }

        return this;
    }

    public KeyboardProvider setListener(KeyboardListener listener) {
        this.listener = listener;
        return this;
    }

    @Override
    public void onGlobalLayout() {
        Rect rect = new Rect();
        rootView.getWindowVisibleDisplayFrame(rect);
		int height = rect.bottom - rect.top;
		int width = rect.right - rect.left;

		if (listener != null) {
			Log.i("buzzer", "global.width = " + width + ", global.height = " + height);
			listener.onGlobalGeometryChanged(width, height);
		}

		if (height > absoluteMax) absoluteMax = height;

		int lCondition = 0;
		if (heightMax == 0) heightMax = height;
		else lCondition = height * 100 / heightMax;

		Log.i("buzzer", "rect.top = " + rect.top + ", rect.bottom = " + rect.bottom + ", condition = " + lCondition);
		Log.i("buzzer", "before: heightMax = " + heightMax + ", height = " + height);

		if (lCondition > 85 /*&& !firstTimeMajorChanges*/) {
			heightMax = height;
		}

	    /*
		else if (lCondition >= 25 && !firstTimeMajorChanges) {
			// INFO: first time correct adjustements
			// heightMax was previously recorded
			firstTimeMajorChanges = true;
		}
	    */

		Log.i("buzzer", "after: heightMax = " + heightMax + ", height = " + height);

        int orientation = mActivity.getResources().getConfiguration().orientation;
        if (orientation != lastOrientation) {
            heightMax = height;
        }

        lastOrientation = orientation;
        if (height > heightMax) {
            heightMax = height;
        }

        // The difference between the two is the height of the keyboard
        int keyboardHeight = heightMax - height;
        if (listener != null) {
            listener.onHeightChanged(keyboardHeight);
        }
    }

    public interface KeyboardListener {
        void onHeightChanged(int height);
		void onGlobalGeometryChanged(int width, int height);
    }
}
