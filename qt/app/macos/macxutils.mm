#include <AppKit/AppKit.h>
#include <QWidget>
#include <QDebug>
#include "macxutils.h"

#import <Cocoa/Cocoa.h>

@interface NSColor (HexString)

+ (NSColor *) colorWithHexString: (NSString *) hexString;

@end

@implementation NSColor (HexString)

+ (CGFloat) colorComponentFrom: (NSString *) string start: (NSUInteger) start length: (NSUInteger) length {
	NSString *substring = [string substringWithRange: NSMakeRange(start, length)];
	NSString *fullHex = length == 2 ? substring : [NSString stringWithFormat: @"%@%@", substring, substring];
	unsigned hexComponent;
	[[NSScanner scannerWithString: fullHex] scanHexInt: &hexComponent];
	return hexComponent / 255.0;
}

+ (NSColor *) colorWithHexString: (NSString *) hexString {
	NSString *colorString = [[hexString stringByReplacingOccurrencesOfString: @"#" withString: @""] uppercaseString];
	CGFloat alpha, red, blue, green;
	switch ([colorString length]) {
		case 3: // #RGB
			alpha = 1.0f;
			red   = [self colorComponentFrom: colorString start: 0 length: 1];
			green = [self colorComponentFrom: colorString start: 1 length: 1];
			blue  = [self colorComponentFrom: colorString start: 2 length: 1];
			break;
		case 4: // #ARGB
			alpha = [self colorComponentFrom: colorString start: 0 length: 1];
			red   = [self colorComponentFrom: colorString start: 1 length: 1];
			green = [self colorComponentFrom: colorString start: 2 length: 1];
			blue  = [self colorComponentFrom: colorString start: 3 length: 1];
			break;
		case 6: // #RRGGBB
			alpha = 1.0f;
			red   = [self colorComponentFrom: colorString start: 0 length: 2];
			green = [self colorComponentFrom: colorString start: 2 length: 2];
			blue  = [self colorComponentFrom: colorString start: 4 length: 2];
			break;
		case 8: // #AARRGGBB
			alpha = [self colorComponentFrom: colorString start: 0 length: 2];
			red   = [self colorComponentFrom: colorString start: 2 length: 2];
			green = [self colorComponentFrom: colorString start: 4 length: 2];
			blue  = [self colorComponentFrom: colorString start: 6 length: 2];
			break;
		default:
			return nil;
	}
	return [NSColor colorWithRed: red green: green blue: blue alpha: alpha];
}

@end

void MacXUtils::setStatusBarColor(QQuickWindow* window, QString color) {
	//
	if (window == nullptr) return;
	qInfo() << "MacXUtils::setStatusBarColor" << window;
	QColor lColor(color);
	NSView* lView = (NSView*) window->winId();
	NSWindow* lWindow = [lView window];
	lWindow.titlebarAppearsTransparent = YES;
	lWindow.backgroundColor = [NSColor colorWithHexString:color.toNSString()];
			//colorWithRed:lColor.red() green:lColor.green() blue:lColor.blue() alpha:1.];
}
