package arf4.utils;
import java.lang.Runnable;
import android.app.Activity;
class Haptic {
	public static void DoHapticFeedback(final Activity A) {
		A.getWindow().getDecorView().performHapticFeedback(0, 1);
	}
}