import QtQuick 2.15
import QtMultimedia 5.15

MediaPlayer {
	id: player

	property bool playing: false;
	property bool paused: false;
	property bool stopped: true;
	property int size: 0;

	onPlaying: { playing = true; paused = false; stopped = false; adjustControls(); }
	onPaused: { playing = false; paused = true; stopped = false; adjustControls(); }
	onStopped: {
		playing = false; paused = false; stopped = true;
		adjustControls();
		//player.seek(1);
	}

	signal adjustControls();

	onStatusChanged: {
		switch(status) {
			case MediaPlayer.Buffered:
				size = buzzerApp.getFileSize(source);
			break;
		}
	}

	onErrorStringChanged: {
		stop();
	}

	onPositionChanged: {
	}

	onPlaybackStateChanged: {
	}
}
