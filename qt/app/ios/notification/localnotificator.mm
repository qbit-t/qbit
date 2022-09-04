#include "localnotificator.h"
#include <QVariant>

#import <Foundation/Foundation.h>
#import <UserNotifications/UserNotifications.h>
#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>

#include <QDebug>

//
// iOS Objective-C iface for UNUserNotificationCenterDelegate
//

@interface QIOSApplicationDelegate : UIResponder <UIApplicationDelegate, UNUserNotificationCenterDelegate>
@end
//add a category to QIOSApplicationDelegate
@interface QIOSApplicationDelegate (QPushNotificationDelegate)
  @property (nonatomic, strong) void (^contentHandler)(UNNotificationContent *contentToDeliver);
  @property (nonatomic, strong) UNMutableNotificationContent *bestAttemptContent;
@end

@implementation QIOSApplicationDelegate (QPushNotificationDelegate)

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {

    [self registerForRemoteNotification];

    [[UIApplication sharedApplication] setMinimumBackgroundFetchInterval:10.0]; // 10 seconds

    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    center.delegate = self;
    [center requestAuthorizationWithOptions:(UNAuthorizationOptionBadge | UNAuthorizationOptionSound | UNAuthorizationOptionAlert)
           completionHandler:^(BOOL granted, NSError * _Nullable error) {
                  if (!error) {
                      qDebug() << "[iOS/didFinishLaunchingWithOptions]: Authorization request succeeded!";
                  }
    }];

  return YES;
}

- (void)application:(UIApplication *)application performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler {
    // TODO: maybe weneed separate processing
    qDebug() << "performFetchWithCompletionHandler";
    completionHandler(UIBackgroundFetchResultNewData);
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:NULL];
    [[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
}

-(void)userNotificationCenter:(UNUserNotificationCenter *)center willPresentNotification:(UNNotification *)notification withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler{
    NSLog(@"User Info : %@",notification.request.content.userInfo);
    completionHandler(UNAuthorizationOptionSound | UNAuthorizationOptionAlert | UNAuthorizationOptionBadge);
}

-(void)userNotificationCenter:(UNUserNotificationCenter *)center didReceiveNotificationResponse:(UNNotificationResponse *)response withCompletionHandler:(void(^)())completionHandler{
    NSLog(@"User Info : %@",response.notification.request.content.userInfo);
    completionHandler();
}

#pragma mark - Remote Notification Delegate // <= iOS 9.x

- (void)application:(UIApplication *)application didRegisterUserNotificationSettings:(UIUserNotificationSettings *)notificationSettings {
    [application registerForRemoteNotifications];
}

- (void)application:(UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken{
    // for iOS 12 and later
    //NSString *strDevicetoken = [[NSString alloc]initWithFormat:@"%@",[[[deviceToken description] stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"<>"]] stringByReplacingOccurrencesOfString:@" " withString:@""]];
    //qDebug() << "[iOS/DeviceToken]:" << QString::fromNSString(strDevicetoken);
    //graviex::quark::LocalNotificator::instance()->setDeviceToken(QString::fromNSString(strDevicetoken));

    // for iOS 13 and later
    NSUInteger dataLength = deviceToken.length;
    if (dataLength == 0) {
      return;
    }
    const unsigned char *dataBuffer = (const unsigned char *)deviceToken.bytes;
    NSMutableString *hexString  = [NSMutableString stringWithCapacity:(dataLength * 2)];
    for (int i = 0; i < dataLength; ++i) {
        [hexString appendFormat:@"%02x", dataBuffer[i]];
    }

    qDebug() << "[iOS/DeviceToken]:" << QString::fromNSString(hexString);
    graviex::quark::LocalNotificator::instance()->setDeviceToken(QString::fromNSString(hexString));
}

-(void)application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error
{
    NSLog(@"%@ = %@", NSStringFromSelector(_cmd), error);
    NSLog(@"Error = %@",error);
    qDebug() << "[iOS/didFailToRegisterForRemoteNotificationsWithError]:" << QString::fromNSString(error.localizedDescription);
}

#define SYSTEM_VERSION_GRATERTHAN_OR_EQUALTO(v)  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedAscending)

#pragma mark - Class Methods

/**
 Notification Registration
 */
- (void)registerForRemoteNotification {
    if(SYSTEM_VERSION_GRATERTHAN_OR_EQUALTO(@"10.0")) {
        UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
        center.delegate = self;
        [center requestAuthorizationWithOptions:(UNAuthorizationOptionSound | UNAuthorizationOptionAlert | UNAuthorizationOptionBadge) completionHandler:^(BOOL granted, NSError * _Nullable error){
            if( !error ){
                [[UIApplication sharedApplication] registerForRemoteNotifications];
            }
        }];
    }
    else {
        [[UIApplication sharedApplication] registerUserNotificationSettings:[UIUserNotificationSettings settingsForTypes:(UIUserNotificationTypeSound | UIUserNotificationTypeAlert | UIUserNotificationTypeBadge) categories:nil]];
        [[UIApplication sharedApplication] registerForRemoteNotifications];
    }
}

@end

//
// Qt Proxy
//

using namespace graviex::quark;

void LocalNotificator::reset()
{
    //[UIApplication sharedApplication].applicationIconBadgeNumber = 0;
}

void LocalNotificator::notify(QString id, QString title, QString message)
{
    UNMutableNotificationContent *objNotificationContent = [[UNMutableNotificationContent alloc] init];

    objNotificationContent.title = [NSString localizedUserNotificationStringForKey:title.toNSString() arguments:nil];
    objNotificationContent.body = [NSString localizedUserNotificationStringForKey:message.toNSString() arguments:nil];
    objNotificationContent.sound = [UNNotificationSound defaultSound];

    objNotificationContent.badge = @([[UIApplication sharedApplication] applicationIconBadgeNumber] + 1);

    UNTimeIntervalNotificationTrigger *trigger =  [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:1.0 repeats:NO];
    UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:id.toNSString() content:objNotificationContent trigger:trigger];
    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];

    [center addNotificationRequest:request withCompletionHandler:^(NSError * _Nullable error) {
        if (!error) {
            NSLog(@"Local Notification succeeded");
        } else {
            NSLog(@"Local Notification failed");
        }
    }];
}

