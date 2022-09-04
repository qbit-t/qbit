import QtQuick 2.15
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1
//import QtGraphicalEffects 1.0
import QtGraphicalEffects 1.15
import Qt.labs.folderlistmodel 2.11
import QtMultimedia 5.15

import app.buzzer.components 1.0 as BuzzerComponents
import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

import "qrc:/lib/numberFunctions.js" as NumberFunctions
import "qrc:/lib/dateFunctions.js" as DateFunctions

Rectangle {
	//
	id: videoFrame

	//
	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceMedia_: 10
	readonly property int spaceSingleMedia_: 8
	readonly property int spaceMediaIndicator_: 15
	readonly property int spaceHeader_: 7
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property int spaceThreaded_: 33
	readonly property int spaceThreadedItems_: 4
	readonly property real defaultFontSize: buzzerApp.defaultFontSize()
	property var frameColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")

	//
	property var mediaList;
	property var mediaBox;

	signal adjustDuration(var dureation);
	signal adjustHeight(var proposed);

	onMediaBoxChanged: {
		videoOutput.adjustView();
	}

	onHeightChanged: {
		videoOutput.adjustView();
	}

	//
	property var currentOrientation_: orientation;

	onCurrentOrientation_Changed: {
		//
		if (buzzerApp.isDesktop) {
			if (currentOrientation_ === 6) videoOutput.orientation = -90;
			else if (currentOrientation_ === 3) videoOutput.orientation = -180;
			else if (currentOrientation_ === 8) videoOutput.orientation = 90;
			else videoOutput.orientation = 0;

			console.log("[onCurrentOrientation_Changed]: orientation = " + currentOrientation_ + ", videoOutput.orientation = " + videoOutput.orientation);
			videoOutput.adjustView();
		}
	}

	//
	function adjustOrientation(newOrientation) {
		//
	}

	function terminate() {
		//
		player.pause();
	}

	//
	color: "transparent"
	width: player.width + 2 * spaceItems_
	height: 600

	//
	VideoOutput {
		id: videoOutput
		//autoOrientation: Qt.platform.os === "ios" ? true : false
		//flushMode: VideoOutput.FirstFrame

		x: getX()
		y: getY()

		function getX() {
			return parent.width / 2 - width / 2;
		}

		function getY() {
			return 0;
		}

		function adjustView() {
			if (mediaList) width = mediaList.width - 2 * spaceItems_;
			if (mediaBox) {
				mediaList.height = Math.max(mediaBox.calculatedHeight, parent.height);
				mediaBox.calculatedHeight = mediaList.height;
			}

			height = videoOutput.contentRect.height;
			adjustHeight(height);
		}

		//width: parent.width
		//height: parent.height
		anchors.fill: parent
		source: player
		fillMode: VideoOutput.PreserveAspectFit

		onContentRectChanged: {
			//
			// console.log("[onContentRectChanged-2]: videoOutput.contentRect = " + videoOutput.contentRect + ", parent.height = " + parent.height);
			if (videoOutput.contentRect.height > 0 && videoOutput.contentRect.height < parent.height) {
				height = videoOutput.contentRect.height;
				adjustHeight(height);
			}
		}
	}

	//
	QuarkRoundRectangle {
		id: frameContainer
		//x: videoOutput.x + spaceItems_ - 1
		//y: videoOutput.contentRect.y - 1
		//width: videoOutput.width + 2
		//height: videoOutput.contentRect.height + 2
		anchors.fill: previewImage

		color: "transparent" // buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
		backgroundColor: "transparent"
		//radius: 14
		//penWidth: 10

		visible: true //!previewImage.visible
	}

	BuzzerComponents.ImageQx {
		id: previewImage
		asynchronous: true
		autoTransform: true
		radius: 8

		function adjustView() {
			if (previewImage.status === Image.Ready && mediaList) {
				width = mediaList.width - 2*spaceItems_;
				parent.height = height;
			}
		}

		onHeightChanged: {
			parent.height = height;
		}

		onStatusChanged: {
			adjustView();
			if (status == Image.Error) {
				// force to reload
				console.info("[image/onStatusChanged/error]: source = " + source + ", error = " + errorString);
			}
		}

		onWidthChanged: {
			//
			if (width != originalWidth) {
				var lCoeff = (width * 1.0) / (originalWidth * 1.0)
				var lHeight = originalHeight * 1.0;
				height = lHeight * lCoeff;

				adjustHeight(height);

				//console.log("[onHeightChanged/image/new height]: height = " + lHeight + ", lCoeff = " + lCoeff + ", width = " + width + ", originalWidth = " + originalWidth);
			}
		}

		source: preview.startsWith("qrc") ? preview : (Qt.platform.os == "windows" ? "file:///" : "file://") + preview
		fillMode: BuzzerComponents.ImageQx.PreserveAspectFit
		mipmap: true
		anchors.centerIn: parent

		visible: true //(preview !== "none" || preview !== "") && !(player.position > 1) 
					//(player.hasVideo && !player.playing && !player.paused))
	}

	Timer {
		id: loadTimer

		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			//
			//player.play();
			//sampleTimer.start();
			//videoOutput.adjustView();
			//previewImage.adjustView();
		}
	}

	Timer {
		id: sampleTimer

		interval: 2000
		repeat: false
		running: false

		onTriggered: {
			//
			//player.pause();
		}
	}

	//
	MediaPlayer {
		id: player
		source: path

		property bool playing: false;
		property bool paused: true;
		property bool intialized: false;

		onPlaying: { playing = true; paused = false; }
		onPaused: { playing = false; paused = true; }
		onStopped: {
			playing = false;
			paused = false; 

			playSlider.value = 0;
			elapsedTime.setTime(0);
			//player.seek(1);

			previewImage.visible = true;
		}

		onStatusChanged: {
			//
			var lStatus = "Unknown";
			switch(status) {
				case MediaPlayer.Loaded:
					lStatus = "Loaded";
					if (!intialized) {
						size = buzzerApp.getFileSize(key);
						totalTime.setTotalTime(duration);
						totalSize.setTotalSize(size);
						playSlider.to = duration;
						//videoOutput.fillMode = VideoOutput.PreserveAspectFit;
						
						//
						//player.seek(1);

						// adjust orientation
						if (orientation !== 0) {
							if (orientation == 6) videoOutput.orientation = -90;
							else if (orientation == 3) videoOutput.orientation = -180;
							else if (orientation == 8) videoOutput.orientation = 90;
							else videoOutput.orientation = 0;
						}

						//						
						videoOutput.adjustView();
						previewImage.adjustView();

						// duration
						adjustDuration(duration);

						//
						intialized = true;
					}
				break;
				case MediaPlayer.Buffered:
					lStatus = "Buffered";
				break;
				case MediaPlayer.EndOfMedia:
					lStatus = "EndOfMedia";
				break;
				case MediaPlayer.InvalidMedia:
					lStatus = "InvalidMedia";
				break;
			}

			console.log("[onStatusChanged(9)]: status = " + lStatus + ", duration = " + duration + ", path = " + path + ", size = " + size);
		}

		onPositionChanged: {
			elapsedTime.setTime(position);
			playSlider.value = position;

			if (position >= 250 && previewImage.visible && playing && hasVideo) previewImage.visible = false;
		}

		onPlaybackStateChanged: {
		}

		onErrorStringChanged: {
			console.log("[onErrorStringChanged(9)]: " + errorString);
		}
	}

	//
	Rectangle {
		id: controlsBack
		//x: frameContainer.x + 2 * spaceItems_
		//y: frameContainer.y + frameContainer.height - (actionButton.height + 4 * spaceItems_)
		//width: frameContainer.width - (4 * spaceItems_)
		x: frameContainer.visible ? (frameContainer.x + spaceItems_) : (previewImage.x + spaceItems_)
		y: frameContainer.visible ? (frameContainer.y + frameContainer.height - (actionButton.height + 3 * spaceItems_)) :
									(previewImage.y + previewImage.height - (actionButton.height + 3 * spaceItems_))
		width: frameContainer.visible ? (frameContainer.width - (2 * spaceItems_)) : (previewImage.width - (2 * spaceItems_))
		height: actionButton.height + 2 * spaceItems_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		opacity: 0.3
		radius: 8
	}

	//
	QuarkRoundSymbolButton {
		id: actionButton
		x: controlsBack.x + spaceItems_
		y: controlsBack.y + spaceItems_
		/*
		spaceLeft: player.playing ? 0 : 3
		spaceTop: 2
		*/
		symbol: player.playing ? Fonts.pauseSym : Fonts.playSym
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 7)) : (buzzerApp.defaultFontSize() + 7)
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.highlight")

		onSymbolChanged: {
			if (symbol !== Fonts.playSym) spaceLeft = 0;
			else spaceLeft = buzzerClient.scaleFactor * 2;
		}

		opacity: 0.6

		onClick: {
			if (!player.playing) {
				player.play();
			} else {
				player.pause();
			}
		}
	}

	QuarkLabel {
		id: caption
		x: actionButton.x + actionButton.width + spaceItems_
		y: actionButton.y + 1
		width: playSlider.width
		elide: Text.ElideRight
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : buzzerApp.defaultFontSize()
		text: description
		visible: description != "none"
	}

	QuarkLabel {
		id: elapsedTime
		x: actionButton.x + actionButton.width + spaceItems_ + (description != "none" ? 3 : 0)
		y: actionButton.y + (description != "none" ? caption.height + 3 : spaceItems_)
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : buzzerApp.defaultFontSize()
		text: "00:00"

		function setTime(ms) {
			text = DateFunctions.msToTimeString(ms);
		}
	}

	QuarkLabel {
		id: totalTime
		x: elapsedTime.x + elapsedTime.width
		y: elapsedTime.y
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : buzzerApp.defaultFontSize()
		text: "/00:00"

		function setTotalTime(ms) {
			text = "/" + DateFunctions.msToTimeString(ms);
		}
	}

	QuarkLabel {
		id: totalSize
		x: totalTime.x + totalTime.width
		y: elapsedTime.y
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : buzzerApp.defaultFontSize()
		text: ", 0k"
		visible: size !== 0

		function getSize() {
			setTotalSize(size)
		}

		function setTotalSize(mediaSize) {
			//
			if (mediaSize < 1000) text = ", " + mediaSize + "b";
			else text = ", " + NumberFunctions.numberToCompact(mediaSize);
		}
	}

	Slider {
		id: playSlider
		x: actionButton.x + actionButton.width // + spaceItems_
		y: actionButton.y + actionButton.height - (height - 3 * spaceItems_)
		from: 0
		to: 1
		orientation: Qt.Horizontal
		stepSize: 0.1
		width: frameContainer.width - (actionButton.width + 5 * spaceItems_)

		onMoved: {
			player.seek(value);
			elapsedTime.setTime(value);
		}
	}

	QuarkToolButton	{
		id: removeButton

		x: frameContainer.x + frameContainer.width - (width + 2*spaceItems_)
		y: 2*spaceItems_
		// Material.background: "transparent"
		visible: true
		labelYOffset: 3
		symbolColor: (!sending || uploaded) ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
										  buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		symbol: !uploaded ? Fonts.cancelSym : Fonts.checkedSym

		onClicked: {
			if (!sending) {
				mediaList.removeMedia(index);
			}
		}
	}

	QuarkToolButton	{
		id: clearDescriptionButton

		x: frameContainer.x + frameContainer.width - (width + 2*spaceItems_)
		y: removeButton.y + removeButton.height
		// Material.background: "transparent"
		visible: true
		labelYOffset: 2
		symbolColor: (!sending || uploaded) ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
										  buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		symbol: Fonts.eraserSym

		onClicked: {
			if (!sending) {
				description = "none";
			}
		}
	}

	QuarkToolButton	{
		id: flipButton

		x: frameContainer.x + frameContainer.width - (width + 2*spaceItems_)
		y: clearDescriptionButton.y + clearDescriptionButton.height
		// Material.background: "transparent"
		visible: true
		labelYOffset: 3
		symbolColor: (!sending || uploaded) ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
										  buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		symbol: Fonts.refreshSym

		onClicked: {
			if (!sending) {
				if (orientation === 0) { orientation = 3; videoOutput.orientation = -180; }
				else { orientation = 0; videoOutput.orientation = 0; }
			}
		}
	}

	QuarkRoundProgress {
		id: mediaUploadProgress
		x: removeButton.x - 1
		y: removeButton.y - 1
		size: removeButton.width + 2
		colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
		colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
		arcBegin: 0
		arcEnd: progress
		lineWidth: 3
	}
}
