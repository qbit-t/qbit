package app.buzzer.mobile;

import android.content.Intent;
import android.os.Bundle;
import org.qtproject.qt5.android.bindings.QtActivity;

import android.app.KeyguardManager;
import android.content.pm.PackageManager;
import android.hardware.fingerprint.FingerprintManager;
import android.Manifest;
import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.graphics.Color;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyPermanentlyInvalidatedException;
import android.security.keystore.KeyProperties;
//import android.support.v7.app.AppCompatActivity;
//import android.support.v4.app.ActivityCompat;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import java.io.IOException;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.KeyStore;
import java.security.Key;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.UnrecoverableKeyException;
import java.security.cert.CertificateException;
import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.SecretKey;
import android.content.Context;
import java.security.KeyPairGenerator;
import java.math.BigInteger;
import javax.security.auth.x500.X500Principal;
import java.security.KeyPair;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.io.File;
import javax.crypto.CipherOutputStream;
import javax.crypto.CipherInputStream;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.nio.charset.StandardCharsets;
import java.security.UnrecoverableEntryException;
import android.util.Log;

public class MainActivity extends QtActivity
{
    private static final String KEY_NAME = "BUZZER-KEY";
    private static final String PIN_NAME = "BUZZER-PAIR-0";
    private Cipher cipher;
    private KeyStore keyStore;
    private KeyGenerator keyGenerator;
    private FingerprintManager.CryptoObject cryptoObject;
    private FingerprintManager fingerprintManager;
    private KeyguardManager keyguardManager;
    private static MainActivity instance_;
    FingerprintHandler helper;

    //  0   - unsupported
    //  1   - initialized
    // -1   - device doesn't support fingerprint authentication
    // -2   - enable the fingerprint permission
    // -3   - no fingerprint configured
    // -4   - enable lockscreen security in your device's settings
    // -100 - common error
    // -101 - cypher init error
    private int fingertipAuthState;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        instance_ = this;
        fingertipAuthState = 0;
        super.onCreate(savedInstanceState);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
        {
            try
            {
                keyguardManager =
                        (KeyguardManager) getSystemService(KEYGUARD_SERVICE);
                fingerprintManager =
                        (FingerprintManager) getSystemService(FINGERPRINT_SERVICE);

                if (keyguardManager == null || fingerprintManager == null) return;

                if (!fingerprintManager.isHardwareDetected())
                {
                    fingertipAuthState = -1;
                }

                /*
                if (ContextCompat.checkSelfPermission(this, Manifest.permission.USE_FINGERPRINT) != PackageManager.PERMISSION_GRANTED)
                {
                    fingertipAuthState = -2;
                }
                */

                if (!fingerprintManager.hasEnrolledFingerprints())
                {
                    fingertipAuthState = -3;
                }

                if (!keyguardManager.isKeyguardSecure())
                {
                    fingertipAuthState = -4;
                }

                if (fingertipAuthState == 0)
                {
                    try
                    {
                        generateKey();
                    }
                    catch (FingerprintException e)
                    {
                        fingertipAuthState = -100;
                        e.printStackTrace();
                    }

                    try
                    {
                        if (initCipher())
                        {
                            fingertipAuthState = 1;
                            cryptoObject = new FingerprintManager.CryptoObject(cipher);
                        }
                        else
                        {
                            fingertipAuthState = -101;
                        }
                    }
                    catch(RuntimeException e)
                    {
                        fingertipAuthState = -102;
                        e.printStackTrace();
                    }
                }
            }
            catch(NoClassDefFoundError | RuntimeException e) // any other exchaption
            {
                fingertipAuthState = -103;
                e.printStackTrace();
            }
        }
    }

    public int getFingertipAuthState()
    {
        return fingertipAuthState;
    }

    public void startFingertipAuth()
    {
        if (fingertipAuthState == 1)
        {
            helper = new FingerprintHandler(this);
            helper.startAuth(fingerprintManager, cryptoObject, extractPin());
        }
    }

    public void stopFingertipAuth()
    {
        if (fingertipAuthState == 1)
        {
            helper.stopAuth();
        }
    }

    public void enablePinStore(String pin)
    {
        try {
            keyStore.load(null);

            // Retrieve the keys
            KeyStore.PrivateKeyEntry privateKeyEntry = (KeyStore.PrivateKeyEntry) keyStore.getEntry(PIN_NAME, null);
            PrivateKey privateKey = privateKeyEntry.getPrivateKey();
            PublicKey publicKey = privateKeyEntry.getCertificate().getPublicKey();

            // Encrypt the text
            String dataDirectory = this.getApplicationInfo().dataDir;
            String filesDirectory = this.getFilesDir().getAbsolutePath();
            String encryptedDataFilePath = filesDirectory + File.separator + "buzzer-key.data";

            Cipher inCipher = Cipher.getInstance("RSA/ECB/PKCS1Padding", "AndroidKeyStoreBCWorkaround");
            inCipher.init(Cipher.ENCRYPT_MODE, publicKey);

            CipherOutputStream cipherOutputStream =
                    new CipherOutputStream(
                            new FileOutputStream(encryptedDataFilePath), inCipher);
            cipherOutputStream.write(pin.getBytes(StandardCharsets.UTF_8));
            cipherOutputStream.close();
        }
        catch (CertificateException | NoSuchAlgorithmException | UnsupportedOperationException | InvalidKeyException | NoSuchPaddingException | UnrecoverableEntryException | NoSuchProviderException | KeyStoreException | IOException e)
        {
            e.printStackTrace();
        }
    }

    public void setBackgroundColor(final String color)
	{
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				View lRoot;
				lRoot = findViewById(android.R.id.content).getRootView();
				lRoot.setBackgroundColor(Color.parseColor(color));
			}
		});
	}

    private String extractPin()
    {
        try {
            keyStore.load(null);

            // Retrieve the keys
            KeyStore.PrivateKeyEntry privateKeyEntry = (KeyStore.PrivateKeyEntry) keyStore.getEntry(PIN_NAME, null);
            PrivateKey privateKey = privateKeyEntry.getPrivateKey();
            PublicKey publicKey = privateKeyEntry.getCertificate().getPublicKey();

            String dataDirectory = this.getApplicationInfo().dataDir;
            String filesDirectory = this.getFilesDir().getAbsolutePath();
            String encryptedDataFilePath = filesDirectory + File.separator + "buzzer-key.data";

            Cipher outCipher = Cipher.getInstance("RSA/ECB/PKCS1Padding", "AndroidKeyStoreBCWorkaround");
            outCipher.init(Cipher.DECRYPT_MODE, privateKey);

            CipherInputStream cipherInputStream =
                    new CipherInputStream(new FileInputStream(encryptedDataFilePath),
                            outCipher);

            byte[] roundTrippedBytes = new byte[1000];

            int index = 0;
            int nextByte;
            while ((nextByte = cipherInputStream.read()) != -1) {
                roundTrippedBytes[index] = (byte) nextByte;
                index++;
            }

            String pinString = new String(roundTrippedBytes, 0, index, StandardCharsets.UTF_8);
            return pinString;
        }
        catch (CertificateException | NoSuchAlgorithmException | UnsupportedOperationException | InvalidKeyException | NoSuchPaddingException | UnrecoverableEntryException | NoSuchProviderException | KeyStoreException | IOException e)
        {
            e.printStackTrace();
            return "none";
        }
    }

    public static void externalStartFingertipAuth()
    {
        instance_.startFingertipAuth();
    }

    public static void externalStopFingertipAuth()
    {
        instance_.stopFingertipAuth();
    }

    public static void externalEnablePinStore(String pin)
    {
        instance_.enablePinStore(pin);
    }

    public static void externalSetBackgroundColor(String color)
	{
		instance_.setBackgroundColor(color);
	}

    public static int externalGetFingertipAuthState()
    {
        return instance_.getFingertipAuthState();
    }

    private void generateKey() throws FingerprintException
    {
        try
        {
            keyStore = KeyStore.getInstance("AndroidKeyStore");
            keyGenerator = KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, "AndroidKeyStore");

            keyStore.load(null);
            keyGenerator.init(new
                    KeyGenParameterSpec.Builder(KEY_NAME,
                    KeyProperties.PURPOSE_ENCRYPT |
                            KeyProperties.PURPOSE_DECRYPT)
                    .setBlockModes(KeyProperties.BLOCK_MODE_CBC)
                    .setUserAuthenticationRequired(true)
                    .setEncryptionPaddings(
                            KeyProperties.ENCRYPTION_PADDING_PKCS7)
                    .build());

            keyGenerator.generateKey();

            // Create the keys if necessary
            if (!keyStore.containsAlias(PIN_NAME)) {
                Log.i("buzzer", "Generate new key...");
                KeyPairGenerator spec = KeyPairGenerator.getInstance(
                        KeyProperties.KEY_ALGORITHM_RSA, "AndroidKeyStore");
                        spec.initialize(new KeyGenParameterSpec.Builder(
                            PIN_NAME, KeyProperties.PURPOSE_DECRYPT | KeyProperties.PURPOSE_ENCRYPT)
                            .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_RSA_PKCS1) //  RSA/ECB/PKCS1Padding
                            .setKeySize(2048)
                            .setCertificateSubject(new X500Principal("CN=test"))
                            .setCertificateSerialNumber(BigInteger.ONE)
                            .build());
                KeyPair keyPair = spec.generateKeyPair();
            }
        }
        catch (KeyStoreException
                | NoSuchAlgorithmException
                | NoSuchProviderException
                | InvalidAlgorithmParameterException
                | CertificateException
                | IOException e)
        {
            e.printStackTrace();
            throw new FingerprintException(e);
        }
    }

    public boolean initCipher()
    {
        try
        {
            cipher = Cipher.getInstance(
                KeyProperties.KEY_ALGORITHM_AES + "/"
                + KeyProperties.BLOCK_MODE_CBC + "/"
                + KeyProperties.ENCRYPTION_PADDING_PKCS7);
        }
        catch (NoSuchAlgorithmException |
                NoSuchPaddingException e)
        {
            throw new RuntimeException("Failed to get Cipher", e);
        }

        try
        {
            keyStore.load(null);

            SecretKey key = (SecretKey) keyStore.getKey(KEY_NAME, null);
            cipher.init(Cipher.ENCRYPT_MODE, key);
            return true;
        }
        catch (KeyPermanentlyInvalidatedException e)
        {
            return false;
        }
        catch (KeyStoreException | CertificateException
            | UnrecoverableKeyException | IOException
            | NoSuchAlgorithmException | InvalidKeyException e)
        {
            throw new RuntimeException("Failed to init Cipher", e);
        }
    }

    private class FingerprintException extends Exception
    {
        public FingerprintException(Exception e)
        {
            super(e);
        }
	}

    @Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == 101) {
			if (data != null) {
				String lFilePath = FileUtils.getPath(this, data.getData());
				Log.i("buzzer", "PATH = " + lFilePath);
				fileSelected(lFilePath);
			}
		}

	    super.onActivityResult(requestCode, resultCode, data);
	}

    public native void fileSelected(String key);

    public void pickImage() {
		Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
		intent.addCategory(Intent.CATEGORY_OPENABLE);
		intent.setType("image/*");
		startActivityForResult(intent, 101);
	}

    public static void externalPickImage() {
		instance_.pickImage();
	}

    @Override
    public void onDestroy()
    {
        System.exit(0);
    }

    public static int isReadStoragePermissionGranted() {
		if (Build.VERSION.SDK_INT >= 23) {
			if (instance_.checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE)
			        == PackageManager.PERMISSION_GRANTED) {
				return 1;
			} else {
			    ActivityCompat.requestPermissions(instance_, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 3);
				return 0;
			}
		}
	    else {
			return 1;
		}
	}

    public static int isWriteStoragePermissionGranted() {
		if (Build.VERSION.SDK_INT >= 23) {
			if (instance_.checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
			        == PackageManager.PERMISSION_GRANTED) {
				return 1;
			} else {
			    ActivityCompat.requestPermissions(instance_, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 2);
				return 0;
			}
		}
	    else {
			return 1;
		}
	}
}

