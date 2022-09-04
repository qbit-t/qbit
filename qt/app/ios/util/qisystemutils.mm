#include <QCoreApplication>
#include <UIKit/UIKit.h>
#include <MobileCoreServices/MobileCoreServices.h>
#include <AVFoundation/AVAudioSession.h>
#include <AVKit/AVPlayerViewController.h>
#include <QPointer>
#include <QtCore>
#include <QImage>
#include "iossystemdispatcher.h"
#include "qiviewdelegate.h"
#include "qiaudio.h"

static bool isPad() {
    return ([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad);
}

static QImage fromUIImage(UIImage* image) {
    QImage::Format format = QImage::Format_RGB32;

    CGColorSpaceRef colorSpace = CGImageGetColorSpace(image.CGImage);
    CGFloat width = image.size.width;
    CGFloat height = image.size.height;

    int orientation = [image imageOrientation];
    int degree = 0;

    switch (orientation) {
    case UIImageOrientationLeft:
        degree = -90;
        break;
    case UIImageOrientationDown: // Down
        degree = 180;
        break;
    case UIImageOrientationRight:
        degree = 90;
        break;
    }

    if (degree == 90 || degree == -90)  {
        CGFloat tmp = width;
        width = height;
        height = tmp;
    }

    QSize size(width,height);

    QImage result = QImage(size,format);

    CGContextRef contextRef = CGBitmapContextCreate(result.bits(),                 // Pointer to  data
                                                   width,                       // Width of bitmap
                                                   height,                       // Height of bitmap
                                                   8,                          // Bits per component
                                                   result.bytesPerLine(),              // Bytes per row
                                                   colorSpace,                 // Colorspace
                                                   kCGImageAlphaNoneSkipFirst |
                                                   kCGBitmapByteOrder32Little); // Bitmap info flags

    CGContextDrawImage(contextRef, CGRectMake(0, 0, width, height), image.CGImage);
    CGContextRelease(contextRef);

    if (degree != 0) {
        QTransform myTransform;
        myTransform.rotate(degree);
        result = result.transformed(myTransform,Qt::SmoothTransformation);
    }

    return result;
}

static QString fromNSUrl(NSURL* url) {
    QString result = QString::fromNSString([url absoluteString]);

    return result;
}

static UIViewController* rootViewController() {
    UIApplication* app = [UIApplication sharedApplication];
    return app.keyWindow.rootViewController;
}

static UIAlertView* alertView = nil;

static QIViewDelegate* alertViewDelegate = nil;

static void alertViewDismiss(int buttonIndex) {
    QString name = "alertViewClickedButtonAtIndex";
    QVariantMap data;
    data["buttonIndex"] = buttonIndex;
    QISystemDispatcher* m_instance = QISystemDispatcher::instance();
    QMetaObject::invokeMethod(m_instance,"dispatched",Qt::DirectConnection,
                              Q_ARG(QString , name),
                              Q_ARG(QVariantMap,data));
    alertViewDelegate = nil;
    alertView = nil;
}

static bool alertViewCreate(QVariantMap& data) {
    Q_UNUSED(data);

    alertViewDelegate = [QIViewDelegate alloc];

    alertViewDelegate->alertViewClickedButtonAtIndex = ^(int buttonIndex) {
        alertViewDismiss(buttonIndex);
    };

    NSString* title = data["title"].toString().toNSString();
    NSString* message = data["message"].toString().toNSString();
    QStringList buttons = data["buttons"].toStringList();

    UIAlertView *alert = [UIAlertView alloc];
    alertView = alert;

    [alert initWithTitle:title
        message:message
        delegate:alertViewDelegate
        cancelButtonTitle:nil
        otherButtonTitles:nil
        ];

    for (int i = 0 ; i < buttons.size();i++) {
        NSString *btn = buttons.at(i).toNSString();
        [alert addButtonWithTitle:btn];
    }

    [alert show];

    return true;
}

static bool alertViewDismissWithClickedButtonIndex(QVariantMap& message) {
    if (alertView == nil) {
        return false;
    }

    int index = message["index"].toInt();
    bool animated = message["animated"].toBool();

    [alertView dismissWithClickedButtonIndex:index animated:animated];
    alertViewDismiss(index);
    return true;
}

static bool actionSheetCreate(QVariantMap& data) {
    static QIViewDelegate *delegate;

    delegate = [QIViewDelegate alloc];

    delegate->actionSheetClickedButtonAtIndex = ^(int buttonIndex) {
        QString name = "actionSheetClickedButtonAtIndex";
        QVariantMap data;
        data["buttonIndex"] = buttonIndex;
        QISystemDispatcher* m_instance = QISystemDispatcher::instance();
        QMetaObject::invokeMethod(m_instance,"dispatched",Qt::DirectConnection,
                                  Q_ARG(QString , name),
                                  Q_ARG(QVariantMap,data));
    };

    delegate->actionSheetDidDismissWithButtonIndex = ^(int buttonIndex) {
        QString name = "actionSheetDidDismissWithButtonIndex";
        QVariantMap data;
        data["buttonIndex"] = buttonIndex;

        QISystemDispatcher* m_instance = QISystemDispatcher::instance();
        m_instance->dispatch(name,data);
        delegate = nil;
    };

    NSString* title = nil;
    QString qTitle = data["title"].toString();
    if (!qTitle.isEmpty()) {
        title = qTitle.toNSString();
    }

    NSString* cancelButtonTitle = data["cancelButtonTitle"].toString().toNSString();
    QStringList buttons = data["otherButtonTitles"].toStringList();
    QRect rect = data["sourceRect"].value<QRect>();

    UIActionSheet* actionSheet = [UIActionSheet alloc];

    [actionSheet initWithTitle:title
        delegate:delegate
        cancelButtonTitle: nil
        destructiveButtonTitle:nil
        otherButtonTitles:nil];

    for (int i = 0 ; i < buttons.size();i++) {
        NSString *btn = buttons.at(i).toNSString();
        [actionSheet addButtonWithTitle:btn];
    }

    // Reference: http://stackoverflow.com/questions/1602214/use-nsarray-to-specify-otherbuttontitles

    [actionSheet addButtonWithTitle:cancelButtonTitle];

    actionSheet.cancelButtonIndex = buttons.size();

    if (isPad()) {
        [actionSheet showFromRect:CGRectMake(rect.x(),rect.y(),rect.width(),rect.height())
                inView:[rootViewController() view] animated:YES];

    } else {
        [actionSheet showInView:[UIApplication sharedApplication].keyWindow];
    }

    return true;
}

static UIImagePickerController* imagePickerController = 0;
static UIActivityIndicatorView* imagePickerControllerActivityIndicator = 0;
UIStatusBarStyle statusBarStyle = UIStatusBarStyleDefault;

static bool imagePickerControllerPresent(QVariantMap& data) {

    UIApplication* app = [UIApplication sharedApplication];

    if (app.windows.count <= 0)
        return false;

    UIWindow* rootWindow = app.windows[0];
    UIViewController* rootViewController = rootWindow.rootViewController;
    
    statusBarStyle = rootViewController.preferredStatusBarStyle;

    int sourceType = data["sourceType"].toInt();
    int mediaType = data["mediaTypes"].toInt();
    bool animated = data["animated"].toBool();

    if (![UIImagePickerController isSourceTypeAvailable:(UIImagePickerControllerSourceType) sourceType]) {
        UIAlertView *myAlertView = [[UIAlertView alloc] initWithTitle:@"Error"
                          message:@"The operation is not supported in this device"
                          delegate:nil
                          cancelButtonTitle:@"OK"
                          otherButtonTitles: nil];
        [myAlertView show];
//        [myAlertView release];
        return false;
    }

    UIImagePickerController *picker = [[UIImagePickerController alloc] init];
    imagePickerController = picker;
    picker.sourceType = (UIImagePickerControllerSourceType) sourceType;
    if (mediaType == 1) // images
        picker.mediaTypes =
            [[NSArray alloc] initWithObjects: (NSString *) kUTTypeImage, nil];
    else if (mediaType == 2) // movies
        picker.mediaTypes =
            [[NSArray alloc] initWithObjects: (NSString *) kUTTypeMovie, (NSString *) kUTTypeImage, nil];

    static QIViewDelegate *delegate = 0;
    delegate = [QIViewDelegate alloc];

    delegate->imagePickerControllerDidFinishPickingMediaWithInfo = ^(UIImagePickerController *picker,
                                                                     NSDictionary* info) {
        Q_UNUSED(picker);

        QString name = "imagePickerControllerDisFinishPickingMetaWithInfo";
        QVariantMap data;

        data["mediaType"] = QString::fromNSString(info[UIImagePickerControllerMediaType]);
        data["mediaUrl"] = fromNSUrl(info[UIImagePickerControllerMediaURL]);
        data["referenceUrl"] = fromNSUrl(info[UIImagePickerControllerReferenceURL]);

        // UIImagePickerControllerMediaURL
        UIImage *chosenImage = info[UIImagePickerControllerEditedImage];
        if (!chosenImage) {
            chosenImage = info[UIImagePickerControllerOriginalImage];
        }

        NSURL *chosenVideoURL;
        if (!chosenImage) {
            chosenVideoURL = info[UIImagePickerControllerMediaURL];   
        }

        if (chosenImage) {
            QImage chosenQImage = fromUIImage(chosenImage);
            data["image"] = QVariant::fromValue<QImage>(chosenQImage);
        } else if (chosenVideoURL) {
            // make explicit copy
            NSURL *tempURL =  [NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:true];
            NSURL *tempFileURL = [tempURL URLByAppendingPathComponent:chosenVideoURL.lastPathComponent];
            NSFileManager *fileManager = [NSFileManager defaultManager];
            [fileManager copyItemAtURL:chosenVideoURL.absoluteURL toURL:tempFileURL error:nil];
            
            // pass local copy
            data["absoluteUrl"] = fromNSUrl(tempFileURL);

            //NSString *videoPath = [[info objectForKey:UIImagePickerControllerMediaURL] path];
            //NSURL *myURL = [[NSURL alloc] initFileURLWithPath:videoPath];

            AVURLAsset *asset2 = [[AVURLAsset alloc] initWithURL:chosenVideoURL options:nil];
            AVAssetImageGenerator *generate2 = [[AVAssetImageGenerator alloc] initWithAsset:asset2];
            //generate1.appliesPreferredTrackTransform = YES;
            NSError *err = NULL;
            CMTime time2 = CMTimeMake(1, 2);
            CGImageRef oneRef2 = [generate2 copyCGImageAtTime:time2 actualTime:NULL error:&err];
            UIImage *testImage = [[UIImage alloc] initWithCGImage:oneRef2];
            
            float testWidth = testImage.size.width;
            float testHeight = testImage.size.height;

            AVURLAsset *asset1 = [[AVURLAsset alloc] initWithURL:chosenVideoURL options:nil];
            AVAssetImageGenerator *generate1 = [[AVAssetImageGenerator alloc] initWithAsset:asset1];
            generate1.appliesPreferredTrackTransform = YES;
            CMTime time = CMTimeMake(1, 2);
            CGImageRef oneRef = [generate1 copyCGImageAtTime:time actualTime:NULL error:&err];
            UIImage *thumbImage = [[UIImage alloc] initWithCGImage:oneRef];
            
            QImage thumbQImage = fromUIImage(thumbImage);
            data["image"] = QVariant::fromValue<QImage>(thumbQImage);

            float width = thumbImage.size.width;
            float height = thumbImage.size.height;

            if (testWidth > width) {
                data["orientation"] = 6; // rotate -90
            } else {
                data["orientation"] = 0;
            }

            //
            qWarning() << "Video" << data["mediaType"] << chosenVideoURL.absoluteString << tempFileURL.absoluteString << data["orientation"].toInt() << width << testWidth << testImage.imageOrientation << thumbImage.imageOrientation;
        } else {
            qWarning() << "Image Picker: Failed to take image or video";
            name = "imagePickerControllerDidCancel";
        }

        QISystemDispatcher* m_instance = QISystemDispatcher::instance();

        QMetaObject::invokeMethod(m_instance,"dispatched",Qt::DirectConnection,
                                  Q_ARG(QString , name),
                                  Q_ARG(QVariantMap,data));

        delegate = nil;
    };

    delegate->imagePickerControllerDidCancel = ^(UIImagePickerController *picker) {
        Q_UNUSED(picker);

        QString name = "imagePickerControllerDidCancel";
        QVariantMap data;
        QISystemDispatcher* m_instance = QISystemDispatcher::instance();
        QMetaObject::invokeMethod(m_instance,"dispatched",Qt::DirectConnection,
                                  Q_ARG(QString , name),
                                  Q_ARG(QVariantMap,data));

        delegate = nil;
    };

    picker.delegate = delegate;

    imagePickerControllerActivityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
    imagePickerControllerActivityIndicator.center = picker.view.center;
    [picker.view addSubview:imagePickerControllerActivityIndicator];

    [rootViewController presentViewController:picker animated:animated completion:NULL];

    return true;
}

@interface QIOSViewController2 : UIViewController
@property (nonatomic, assign) BOOL prefersStatusBarHidden;
@property (nonatomic, assign) UIStatusBarAnimation preferredStatusBarUpdateAnimation;
@property (nonatomic, assign) UIStatusBarStyle preferredStatusBarStyle;
@end

bool imagePickerControllerDismiss(QVariantMap& data) {
    Q_UNUSED(data);
    if (!imagePickerController)
        return false;

    bool animated = data["animated"].toBool();

    [imagePickerController dismissViewControllerAnimated:animated completion:NULL];
//    [imagePickerController release];

//    [imagePickerControllerActivityIndicator release];

    imagePickerController = 0;
    imagePickerControllerActivityIndicator = 0;
    
    UIWindow *keyWindow = [[UIApplication sharedApplication] keyWindow];
    if (keyWindow) {
        QIOSViewController2 *viewController = static_cast<QIOSViewController2 *>([keyWindow rootViewController]);
        viewController.preferredStatusBarStyle = statusBarStyle;
        [viewController setNeedsStatusBarAppearanceUpdate];
    }

    return true;
}

bool imagePickerControllerSetIndicator(QVariantMap& data) {
    if (!imagePickerControllerActivityIndicator)
        return false;

    bool active = data["active"].toBool();

    if (active) {
        [imagePickerControllerActivityIndicator startAnimating];
    } else {
        [imagePickerControllerActivityIndicator stopAnimating];
    }

    return true;
}


static bool applicationSetStatusBarStyle(QVariantMap& data) {
    if (!data.contains("style")) {
        qWarning() << "applicationSetStatusBarStyle: Missing argument";
        return false;
    }

    int style = data["style"].toInt();
    [[UIApplication sharedApplication] setStatusBarStyle:(UIStatusBarStyle) style];
    return true;
}

static bool applicationSetStatusBarHidden(QVariantMap& data) {
    bool hidden = data["hidden"].toBool();
    int animation = data["animation"].toInt();

    [[UIApplication sharedApplication] setStatusBarHidden:(bool) hidden withAnimation:(UIStatusBarAnimation) animation];
    return true;
}

static UIActivityIndicatorView* activityIndicator = 0;

static bool activityIndicatorStartAniamtion(QVariantMap& data) {
    Q_UNUSED(data);
    if (!activityIndicator) {
        int style = data["style"].toInt();

        UIViewController* rootView = rootViewController();

        activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:(UIActivityIndicatorViewStyle) style];
        activityIndicator.center = rootView.view.center;
        activityIndicator.hidesWhenStopped = YES;

        [rootView.view addSubview:activityIndicator];
    }

    [activityIndicator startAnimating];
    return true;
}

static bool activityIndicatorStopAnimation(QVariantMap& data) {
    Q_UNUSED(data);

    if (!activityIndicator) {
        return false;
    }

    [activityIndicator stopAnimating];
//    [activityIndicator release];
    activityIndicator = 0;
    return true;
}

static bool makeBackgroundAudioAvailable(QVariantMap& data) {
    //
    Q_UNUSED(data);
    //
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:nil];
    //
    return true;
}

static bool releaseBackgroundAudio(QVariantMap& data) {
    //
    Q_UNUSED(data);
    //
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:nil];
    //
    return true;
}

AVAudioRecorder *audioRecorder;
AVAudioPlayer *audioPlayer;
enum
{
    ENC_AAC = 1,
    ENC_ALAC = 2,
    ENC_IMA4 = 3,
    ENC_ILBC = 4,
    ENC_ULAW = 5,
    ENC_PCM = 6,
} encodingTypes;

int recordEncoding = ENC_PCM;

static bool beginRecordAudio(QVariantMap& data) {
    //
    NSString* localFile = data["localFile"].toString().toNSString();
    
    /*
    [audioPlayer release];
    audioPlayer = nil;

    [audioRecorder release];
    audioRecorder = nil;
    */
    
    // Init audio with record capability
    AVAudioSession *audioSession = [AVAudioSession sharedInstance];
    NSError *err = nil;
    [audioSession setCategory:AVAudioSessionCategoryPlayAndRecord error:&err];
    if(err) {
        qWarning() << [err domain] << (long)[err code] << [[err userInfo] description];
        return false;
    }
    
    err = nil;
    [audioSession setActive:YES error:&err];
    if(err) {
        qWarning() << [err domain] << (long)[err code] << [[err userInfo] description];
        return false;
    }

    NSMutableDictionary *recordSettings = [[NSMutableDictionary alloc] initWithCapacity:10];
    if(recordEncoding == ENC_PCM)
    {
        //[recordSettings setObject:[NSNumber numberWithInt: kAudioFormatLinearPCM] forKey: AVFormatIDKey];
        //[recordSettings setObject:[NSNumber numberWithFloat:44100.0] forKey: AVSampleRateKey];
        //[recordSettings setObject:[NSNumber numberWithInt:2] forKey:AVNumberOfChannelsKey];
        //[recordSettings setObject:[NSNumber numberWithInt:16] forKey:AVLinearPCMBitDepthKey];
        //[recordSettings setObject:[NSNumber numberWithBool:NO] forKey:AVLinearPCMIsBigEndianKey];
        //[recordSettings setObject:[NSNumber numberWithBool:NO] forKey:AVLinearPCMIsFloatKey];
    }
    else
    {
        NSNumber *formatObject;

        switch (recordEncoding) {
            case (ENC_AAC):
                formatObject = [NSNumber numberWithInt: kAudioFormatMPEG4AAC];
                break;
            case (ENC_ALAC):
                formatObject = [NSNumber numberWithInt: kAudioFormatAppleLossless];
                break;
            case (ENC_IMA4):
                formatObject = [NSNumber numberWithInt: kAudioFormatAppleIMA4];
                break;
            case (ENC_ILBC):
                formatObject = [NSNumber numberWithInt: kAudioFormatiLBC];
                break;
            case (ENC_ULAW):
                formatObject = [NSNumber numberWithInt: kAudioFormatULaw];
                break;
            default:
                formatObject = [NSNumber numberWithInt: kAudioFormatAppleIMA4];
        }

        [recordSettings setObject:formatObject forKey: AVFormatIDKey];
        [recordSettings setObject:[NSNumber numberWithFloat:44100.0] forKey: AVSampleRateKey];
        [recordSettings setObject:[NSNumber numberWithInt:2] forKey:AVNumberOfChannelsKey];
        [recordSettings setObject:[NSNumber numberWithInt:12800] forKey:AVEncoderBitRateKey];
        [recordSettings setObject:[NSNumber numberWithInt:16] forKey:AVLinearPCMBitDepthKey];
        [recordSettings setObject:[NSNumber numberWithInt: AVAudioQualityHigh] forKey: AVEncoderAudioQualityKey];
    }

    NSURL *url = [NSURL URLWithString:localFile];

    static QIAudioDelegate *delegate = 0;
    delegate = [QIAudioDelegate alloc];

    delegate->audioRecorderDidFinishRecording = ^(AVAudioRecorder *recorder, bool flag) {
        qWarning() << "audioRecorderDidFinishRecording";
    };

    delegate->audioRecorderEncodeErrorDidOccur = ^(AVAudioRecorder *recorder, NSError *error) {
        qWarning() << "audioRecorderEncodeErrorDidOccur" << [error domain] << (long)[error code] << [[error userInfo] description];
    };

    NSError *error = nil;
    audioRecorder = [[AVAudioRecorder alloc] initWithURL:url settings:recordSettings error:&error];
    audioRecorder.delegate = delegate;
 
    if ([audioRecorder prepareToRecord] == YES) {
        audioRecorder.meteringEnabled = YES;
        
        BOOL audioHWAvailable = audioSession.inputIsAvailable;
        if (!audioHWAvailable) {
            UIAlertView *cantRecordAlert =
            [[UIAlertView alloc] initWithTitle: @"Warning" message: @"Audio input hardware not available"
                                      delegate: nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
            [cantRecordAlert show];
            return false;
        }
        
        if (![audioRecorder recordForDuration:10]) {
            int errorCode = CFSwapInt32HostToBig ([error code]);
            qWarning() << [error localizedDescription] << (char*)&errorCode;
            return false;
        }
    } else {
        int errorCode = CFSwapInt32HostToBig ([error code]);
        qWarning() << [error localizedDescription] << (char*)&errorCode;
        return false;
    }
    
    return true;
}

static bool currentRecordingTime(QVariantMap& data) {
    //
    if (audioRecorder && audioRecorder.recording) {
        NSTimeInterval lTime = audioRecorder.currentTime;
        QVariantMap data1;
        QString name = "currentRecordingTimeCallback";
        
        data1["currentTime"] = QVariant::fromValue<NSTimeInterval>(lTime);
        
        QISystemDispatcher* m_instance = QISystemDispatcher::instance();

        QMetaObject::invokeMethod(m_instance,"dispatched",Qt::DirectConnection,
                                  Q_ARG(QString , name),
                                  Q_ARG(QVariantMap,data1));
    }
    
    return true;
}

static bool endRecordAudio(QVariantMap& data) {
    //
    if (audioRecorder) {
        [audioRecorder stop];
    }
    
    return true;
}

static bool beginPlayAudio(QVariantMap& data) {
    //
    /*
    [audioPlayer release];
    audioPlayer = nil;
    
    [audioRecorder release];
    audioRecorder = nil;
    */

    //
    int position = 0;
    if (data.contains("position"))
        position = data["position"].toInt();
        
    //
    NSString* source = data["source"].toString().toNSString();
    NSURL *url = [NSURL URLWithString:source];
    
    //
    AVAudioSession *audioSession = [AVAudioSession sharedInstance];
    [audioSession setCategory:AVAudioSessionCategoryPlayback error:nil];
    [audioSession setActive:YES error:nil];

    NSError *error;
    audioPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];
    audioPlayer.numberOfLoops = 0;
    data["duration"] = QVariant::fromValue<NSTimeInterval>(audioPlayer.duration);
    //[audioPlayer playAtTime:audioPlayer.deviceCurrentTime+position];
    
    if ([audioPlayer prepareToPlay] == YES) {
        qWarning() << "Source" << data["source"].toString() << audioPlayer.duration << audioPlayer.volume << audioPlayer.url << error << audioPlayer.format;
        if (![audioPlayer play]) {
            int errorCode = CFSwapInt32HostToBig ([error code]);
            //NSLog(@"Error: %@ [%4.4s])", [error localizedDescription], (char*)&errorCode);
            qWarning() << errorCode << [error localizedDescription];
            
            return false;
        }
    } else {
        int errorCode = CFSwapInt32HostToBig ([error code]);
        //NSLog(@"Error: %@ [%4.4s])", [error localizedDescription], (char*)&errorCode);
        qWarning() << errorCode << [error localizedDescription];
        
        return false;
    }
    
    //
    return true;
}

static bool seekPlayAudio(QVariantMap& data) {
    if (audioPlayer) {
        //
        int position = 0;
        if (data.contains("position"))
            position = data["position"].toInt();
        [audioPlayer playAtTime:audioPlayer.deviceCurrentTime+position];
        return true;
    }
    
    return false;
}

static bool pausePlayAudio(QVariantMap& data) {
    //
    if (audioPlayer) {
        [audioPlayer pause];
    }

    return true;
}

static bool endPlayAudio(QVariantMap& data) {
    //
    if (audioPlayer) {
        [audioPlayer stop];
    }

    return true;
}

static bool currentAudioPlayTime(QVariantMap& data) {
    //
    if (audioPlayer) {
        NSTimeInterval lTime = audioPlayer.currentTime;
        qWarning() << "currentTime" << lTime << audioPlayer.playing;
        QVariantMap data1;
        QString name = "currentAudioPlayTimeCallback";
        
        data1["currentTime"] = QVariant::fromValue<NSTimeInterval>(lTime);
        
        QISystemDispatcher* m_instance = QISystemDispatcher::instance();

        QMetaObject::invokeMethod(m_instance,"dispatched",Qt::DirectConnection,
                                  Q_ARG(QString , name),
                                  Q_ARG(QVariantMap,data1));
    }
    
    return true;
}

static void registerTypes() {
    QISystemDispatcher* dispatcher = QISystemDispatcher::instance();

    dispatcher->addListener("alertViewCreate",alertViewCreate);
    dispatcher->addListener("alertViewDismissWithClickedButtonIndex", alertViewDismissWithClickedButtonIndex);

    dispatcher->addListener("applicationSetStatusBarStyle",applicationSetStatusBarStyle);
    dispatcher->addListener("applicationSetStatusBarHidden",applicationSetStatusBarHidden);

    dispatcher->addListener("actionSheetCreate",actionSheetCreate);
    dispatcher->addListener("imagePickerControllerPresent",imagePickerControllerPresent);
    dispatcher->addListener("imagePickerControllerDismiss",imagePickerControllerDismiss);
    dispatcher->addListener("imagePickerControllerSetIndicator",imagePickerControllerSetIndicator);

    dispatcher->addListener("activityIndicatorStartAnimation",activityIndicatorStartAniamtion);
    dispatcher->addListener("activityIndicatorStopAnimation",activityIndicatorStopAnimation);
    
    dispatcher->addListener("makeBackgroundAudioAvailable",makeBackgroundAudioAvailable);
    dispatcher->addListener("releaseBackgroundAudio",releaseBackgroundAudio);

    dispatcher->addListener("beginRecordAudio",beginRecordAudio);
    dispatcher->addListener("endRecordAudio",endRecordAudio);
    dispatcher->addListener("currentRecordingTime",currentRecordingTime);

    dispatcher->addListener("beginPlayAudio",beginPlayAudio);
    dispatcher->addListener("seekPlayAudio",seekPlayAudio);
    dispatcher->addListener("pausePlayAudio",pausePlayAudio);
    dispatcher->addListener("endPlayAudio",endPlayAudio);
    dispatcher->addListener("currentAudioPlayTime",currentAudioPlayTime);

    // UIKeyboardDidShowNotification
    // UIKeyboardWillShowNotification
    // UIKeyboardWillChangeFrameNotification
    [[NSNotificationCenter defaultCenter] addObserverForName:@"UIKeyboardDidShowNotification" object:nil queue:nil usingBlock:^(NSNotification *notification){
        NSDictionary *userInfo = [notification userInfo];
        CGRect keyboardRect = [userInfo[@"UIKeyboardBoundsUserInfoKey"] CGRectValue];
        qWarning() << "aboutShow, keyboardHeight" << keyboardRect.size.height;
        
        QString name = "keyboardHeightChanged";
        QVariantMap data;

        data["keyboardHeight"] = keyboardRect.size.height;

        QISystemDispatcher* m_instance = QISystemDispatcher::instance();

        QMetaObject::invokeMethod(m_instance,"dispatched",Qt::DirectConnection,
                                  Q_ARG(QString , name),
                                  Q_ARG(QVariantMap,data));
    }];

    [[NSNotificationCenter defaultCenter] addObserverForName:@"UIKeyboardWillHideNotification" object:nil queue:nil usingBlock:^(NSNotification *notification){
        NSDictionary *userInfo = [notification userInfo];
        CGRect keyboardRect = [userInfo[@"UIKeyboardBoundsUserInfoKey"] CGRectValue];
        qWarning() << "aboutHide, keyboardHeight" << keyboardRect.size.height;
        
        QString name = "keyboardHeightChanged";
        QVariantMap data;

        data["keyboardHeight"] = 0;

        QISystemDispatcher* m_instance = QISystemDispatcher::instance();

        QMetaObject::invokeMethod(m_instance,"dispatched",Qt::DirectConnection,
                                  Q_ARG(QString , name),
                                  Q_ARG(QVariantMap,data));
    }];
}

Q_COREAPP_STARTUP_FUNCTION(registerTypes)
