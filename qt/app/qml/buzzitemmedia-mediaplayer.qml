import QtQuick 2.15
import QtMultimedia 5.15

import app.buzzer.components 1.0 as BuzzerComponents

MediaPlayer {
	id: player

	property bool playing: false;
	property bool paused: false;
	property bool stopped: true;
	property int size: 0;
	property var description: "";
	property var caption: "";

	function pushSurface(surface) {
		if (Qt.platform.os === "ios") {
			videoOutput = surface;
		} else {
			surfaces.pushSurface(surface);
			videoOutput = surfaces;
		}
	}

	function popSurface() {
		if (Qt.platform.os === "ios") {
			videoOutput = surfaces;
		} else surfaces.popSurface();
	}

	function clearSurfaces() {
		if (Qt.platform.os === "ios") {
			videoOutput = surfaces;
		} else surfaces.clearSurfaces();
	}

	//videoOutput: surfaces

	onPlaying: { playing = true; paused = false; stopped = false; }
	onPaused: { playing = false; paused = true; stopped = false; playerPaused(); }
	onStopped: {
		playing = false; paused = false; stopped = true;
		playerStopped();
	}

	onPositionChanged: {
	}

	signal playerStopped();
	signal playerPaused();

	onStatusChanged: {
		switch(status) {
			case MediaPlayer.Buffered:
				size = buzzerApp.getFileSize(source);
			break;
		}

		console.info("[onStatusChanged]: volume = " + volume);
	}

	onError: {
		stop();
	}

	onPlaybackStateChanged: {
	}

	property BuzzerComponents.VideoSurfaces surfaces: BuzzerComponents.VideoSurfaces {}
}
