#include <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

@interface QIAudioDelegate : NSObject<AVAudioRecorderDelegate> {

    @public

	void ( ^ audioRecorderDidFinishRecording)(AVAudioRecorder *recorder, bool flag);
	void ( ^ audioRecorderEncodeErrorDidOccur)(AVAudioRecorder *recorder, NSError *error); 
}
@end
