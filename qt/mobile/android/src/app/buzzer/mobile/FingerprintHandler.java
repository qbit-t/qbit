package app.buzzer.mobile;

import android.content.Context;
import android.content.pm.PackageManager;
import android.hardware.fingerprint.FingerprintManager;
import android.Manifest;
import android.os.CancellationSignal;
import android.widget.Toast;

public class FingerprintHandler extends FingerprintManager.AuthenticationCallback
{
    private CancellationSignal cancellationSignal;
    private Context context;
    private String key;

    public FingerprintHandler(Context mContext)
    {
        context = mContext;
    }

    public void startAuth(FingerprintManager manager, FingerprintManager.CryptoObject cryptoObject, String encodedKey)
    {
        cancellationSignal = new CancellationSignal();
        key = encodedKey;
        manager.authenticate(cryptoObject, cancellationSignal, 0, this, null);
    }

    public void stopAuth()
    {
        if (cancellationSignal != null) cancellationSignal.cancel();
    }

    @Override
    public void onAuthenticationError(int errMsgId, CharSequence errString)
    {
    }

    @Override
    public void onAuthenticationFailed()
    {
        authenticationFailed();
    }

    @Override
    public void onAuthenticationHelp(int helpMsgId, CharSequence helpString)
    {
    }

    @Override
    public void onAuthenticationSucceeded(FingerprintManager.AuthenticationResult result)
    {
        authenticationSucceeded(key);
    }

    public native void authenticationFailed();
    public native void authenticationSucceeded(String key);
}
