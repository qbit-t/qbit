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
import android.net.Uri;
import java.util.Date;
import java.text.SimpleDateFormat;
import java.io.OutputStream;
//import android.support.media.ExifInterface;
import java.io.InputStream;
import androidx.exifinterface.media.ExifInterface;
import android.provider.MediaStore;
import android.provider.MediaStore.Images;
import android.graphics.Bitmap;
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
import android.database.Cursor;
import android.util.Size;
import android.media.MediaMetadataRetriever;
import android.graphics.BitmapFactory;

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

		Log.i("buzzer", "============ STARTING JAVA ============");
		if (savedInstanceState == null) Log.i("buzzer", "============ NOT INSTANCE ============");
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
				if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
					//
					try {
						String lFilePath = FileUtils.getPath(this, data.getData());
						if (lFilePath.contains(".mp4") || lFilePath.contains(".MP4")) {
							Cursor ca = getContentResolver().query(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, new String[] { MediaStore.MediaColumns._ID }, MediaStore.MediaColumns.DATA + "=?", new String[] {lFilePath}, null);
							if (ca != null && ca.moveToFirst()) {
								int id = ca.getInt(ca.getColumnIndex(MediaStore.MediaColumns._ID));
								ca.close();

								String format = new SimpleDateFormat("yyyyMMddHHmmss",
								       java.util.Locale.getDefault()).format(new Date());

								File fileThumbnail = new File(getExternalCacheDir(), format + ".jpg");
								OutputStream streamThumbnail = new FileOutputStream(fileThumbnail);
								Bitmap thumbnail = MediaStore.Images.Thumbnails.getThumbnail(getContentResolver(), id, MediaStore.Images.Thumbnails.MICRO_KIND, null);
								thumbnail.compress(Bitmap.CompressFormat.JPEG, 100, streamThumbnail);
								streamThumbnail.flush();
								streamThumbnail.close();

								Log.i("buzzer", "PATH = " + lFilePath);
								fileSelected(lFilePath, fileThumbnail.getAbsolutePath());
							} else {
							    fileSelected(lFilePath, "");
							}
						} else {
						    Log.i("buzzer", "PATH = " + lFilePath);
							fileSelected(lFilePath, "");
						}
					} catch (IOException e) {
					    e.printStackTrace();
					}
				} else {
				    Uri uri = data.getData();
					try {
						String format = new SimpleDateFormat("yyyyMMddHHmmss",
						       java.util.Locale.getDefault()).format(new Date());

						String[] columns = { MediaStore.Images.Media.DATA,
							            MediaStore.Images.Media.MIME_TYPE };

						Cursor cursor = getContentResolver().query(uri, columns, null, null, null);
						cursor.moveToFirst();

						int pathColumnIndex     = cursor.getColumnIndex( columns[0] );
						int mimeTypeColumnIndex = cursor.getColumnIndex( columns[1] );

						String contentPath = cursor.getString(pathColumnIndex);
						String mimeType    = cursor.getString(mimeTypeColumnIndex);
						cursor.close();

						Log.i("buzzer", "mimeType = " + mimeType);

						if (mimeType.startsWith("image")) {
							//
							InputStream inputStream = getContentResolver().openInputStream(uri);
							ExifInterface exif = new ExifInterface(inputStream);

							Bitmap bitmap = MediaStore.Images.Media.getBitmap(getContentResolver(), uri);
							File file = new File(getExternalCacheDir(), format + ".jpg");
							OutputStream stream = new FileOutputStream(file);

							bitmap.compress(Bitmap.CompressFormat.JPEG, 90, stream);
							stream.flush();
							stream.close();
							inputStream.close();

							ExifInterface newExif = new ExifInterface(file.getAbsolutePath());
							newExif.setAttribute(ExifInterface.TAG_ORIENTATION, exif.getAttribute(ExifInterface.TAG_ORIENTATION));
							newExif.saveAttributes();

							Log.i("buzzer", "PATH = " + file.getAbsolutePath());
							fileSelected(file.getAbsolutePath(), "");
						} else if (mimeType.startsWith("video")) {
						    //
							Bitmap thumbnail =
							        getContentResolver().loadThumbnail(uri, new Size(1920, 1080), null);

							File fileThumbnail = new File(getExternalCacheDir(), format + ".jpg");
							OutputStream streamThumbnail = new FileOutputStream(fileThumbnail);

							thumbnail.compress(Bitmap.CompressFormat.JPEG, 100, streamThumbnail);
							streamThumbnail.flush();
							streamThumbnail.close();

							InputStream inputStream = getContentResolver().openInputStream(uri);
							File file = new File(getExternalCacheDir(), format + ".mp4");
							OutputStream stream = new FileOutputStream(file);

							byte[] buffer = new byte[4 * 1024];
							int len;
							while ((len = inputStream.read(buffer, 0, 4 * 1024)) > 0) {
								stream.write(buffer, 0, len);
							}

						    stream.flush();
							stream.close();
							inputStream.close();

							Log.i("buzzer", "PATH = " + file.getAbsolutePath());
							fileSelected(file.getAbsolutePath(), fileThumbnail.getAbsolutePath());

						} else if (mimeType.startsWith("audio/mpeg")) {
						    //
							MediaMetadataRetriever mr = new MediaMetadataRetriever();
							mr.setDataSource(this, uri);

							byte[] byte1 = mr.getEmbeddedPicture();
							mr.release();

							String lAbsolutePath = "";
							if (byte1 != null) {
								Bitmap thumbnail = BitmapFactory.decodeByteArray(byte1, 0, byte1.length);

								File fileThumbnail = new File(getExternalCacheDir(), format + ".jpg");
								OutputStream streamThumbnail = new FileOutputStream(fileThumbnail);

								thumbnail.compress(Bitmap.CompressFormat.JPEG, 100, streamThumbnail);
								streamThumbnail.flush();
								streamThumbnail.close();

								lAbsolutePath = fileThumbnail.getAbsolutePath();
							}

						    InputStream inputStream = getContentResolver().openInputStream(uri);
							File file = new File(getExternalCacheDir(), format + ".mp3");
							OutputStream stream = new FileOutputStream(file);

							byte[] buffer = new byte[4 * 1024];
							int len;
							while ((len = inputStream.read(buffer, 0, 4 * 1024)) > 0) {
								stream.write(buffer, 0, len);
							}

						    stream.flush();
							stream.close();
							inputStream.close();

							Log.i("buzzer", "PATH1 = " + file.getAbsolutePath());
							Log.i("buzzer", "PATH2 = " + lAbsolutePath);
							fileSelected(file.getAbsolutePath(), lAbsolutePath);
						} else if (mimeType.startsWith("audio/mp4")) {
						    //
							MediaMetadataRetriever mr = new MediaMetadataRetriever();
							mr.setDataSource(this, uri);

							byte[] byte1 = mr.getEmbeddedPicture();
							mr.release();

							String lAbsolutePath = "";
							if (byte1 != null) {
								Bitmap thumbnail = BitmapFactory.decodeByteArray(byte1, 0, byte1.length);

								File fileThumbnail = new File(getExternalCacheDir(), format + ".jpg");
								OutputStream streamThumbnail = new FileOutputStream(fileThumbnail);

								thumbnail.compress(Bitmap.CompressFormat.JPEG, 100, streamThumbnail);
								streamThumbnail.flush();
								streamThumbnail.close();

								lAbsolutePath = fileThumbnail.getAbsolutePath();
							}

						    InputStream inputStream = getContentResolver().openInputStream(uri);
							File file = new File(getExternalCacheDir(), format + ".m4a");
							OutputStream stream = new FileOutputStream(file);

							byte[] buffer = new byte[4 * 1024];
							int len;
							while ((len = inputStream.read(buffer, 0, 4 * 1024)) > 0) {
								stream.write(buffer, 0, len);
							}

						    stream.flush();
							stream.close();
							inputStream.close();

							Log.i("buzzer", "PATH1 = " + file.getAbsolutePath());
							Log.i("buzzer", "PATH2 = " + lAbsolutePath);
							fileSelected(file.getAbsolutePath(), lAbsolutePath);
						}
					} catch (IOException e) {
					    e.printStackTrace();
					}
				}
			}
		}

	    super.onActivityResult(requestCode, resultCode, data);
	}

    public native void fileSelected(String key, String preview);

    public void pickImage() {
		Intent intent;
		if(Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT)
		    intent = new Intent(Intent.ACTION_GET_CONTENT);
		else
		    intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);

		intent.addCategory(Intent.CATEGORY_OPENABLE);
		intent.setType("*/*");
		String[] mimetypes = {"image/*", "video/*", "audio/*", "application/pdf"};
		intent.putExtra(Intent.EXTRA_MIME_TYPES, mimetypes);

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

