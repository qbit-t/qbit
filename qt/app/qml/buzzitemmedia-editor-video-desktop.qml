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
// NOTICE: native file dialog
import QtQuick.PrivateWidgets 1.0

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
	readonly property real defaultFontSize: 11
	property var frameColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")

	//
	property var mediaList;
	property var mediaBox;

	signal adjustDuration(var dureation);
	signal adjustHeight(var proposed);

	onMediaBoxChanged: {
		adjust();
	}

	onHeightChanged: {
		adjust();
	}

	onWidthChanged: {
		adjust();
	}

	function adjust() {
		videoOut.adjustView();
		previewImage.adjustView();
		previewImage.adjustWidth();
	}

	//
	property var currentOrientation_: orientation;

	onCurrentOrientation_Changed: {
		//
		if (buzzerApp.isDesktop) {
			if (currentOrientation_ === 6) videoOut.orientation = -90;
			else if (currentOrientation_ === 3) videoOut.orientation = -180;
			else if (currentOrientation_ === 8) videoOut.orientation = 90;
			else videoOut.orientation = 0;

			console.info("[onCurrentOrientation_Changed]: orientation = " + currentOrientation_ + ", videoOutput.orientation = " + videoOut.orientation);
			videoOut.adjustView();
		}
	}

	//
	function adjustOrientation(newOrientation) {
		//
	}

	function terminate() {
		//
		player.terminate();
	}

	//
	color: "transparent"
	//width: player.width + 2 * spaceItems_
	height: 600

	//
	VideoOutput {
		id: videoOut

		x: getX()
		y: getY()

		flushMode: VideoOutput.FirstFrame

		function getX() {
			return parent.width / 2 - width / 2;
		}

		function getY() {
			return 0;
		}

		function adjustView() {
			if (mediaList) width = mediaList.width - 4 * spaceItems_;
			if (mediaBox) {
				mediaList.height = Math.max(mediaBox.calculatedHeight, parent.height);
				mediaBox.calculatedHeight = mediaList.height;
			}

			height = videoOut.contentRect.height;
			adjustHeight(height);
		}

		anchors.leftMargin: 2*spaceItems_
		anchors.rightMargin: 2*spaceItems_
		anchors.fill: parent
		//source: player
		fillMode: VideoOutput.PreserveAspectFit

		visible: !previewImage.visible

		onContentRectChanged: {
			//
			if (videoOut.contentRect.height > 0 && videoOut.contentRect.height < parent.height) {
				height = videoOut.contentRect.height;
				adjustHeight(height);
			}
		}

		function push() {
			if (player && !pushed) { player.pushSurface(videoSurface); pushed = true; }
		}

		property bool pushed: false
	}

	//
	QuarkRoundRectangle {
		id: frameContainer
		//anchors.fill: previewImage.visible ? previewImage : videoOutput.contentRect

		x: previewImage.visible ? previewImage.x : videoOut.contentRect.x + 2*spaceItems_
		y: previewImage.visible ? previewImage.y : videoOut.contentRect.y
		width: previewImage.visible ? previewImage.width : videoOut.contentRect.width
		height: previewImage.visible ? previewImage.height : videoOut.contentRect.height

		color: "transparent"
		backgroundColor: "transparent"

		visible: true

		MouseArea {
			anchors.fill: parent
			enabled: true
			cursorShape: Qt.PointingHandCursor

			onClicked: {
				if (player.playing) player.pause();
				else player.play();
			}
		}
	}

	BuzzerComponents.ImageQx {
		id: previewImage
		asynchronous: true
		radius: 8

		function adjustView() {
			if (previewImage.status === Image.Ready && mediaList) {
				if (originalWidth > originalHeight) {
					width = mediaList.width - 4*spaceItems_;
					parent.height = height;
				} else {
					height = parent.parent.height;
				}

				adjustHeight(height);
			}
		}

		function adjustWidth() {
			//
			var lCoeff;
			if (originalWidth > originalHeight) {
				width = mediaList.width - 4*spaceItems_;
				lCoeff = (width * 1.0) / (originalWidth * 1.0);
				var lHeight = originalHeight * 1.0;
				height = lHeight * lCoeff;
				adjustHeight(height);
			}

			if (originalHeight > originalWidth) {
				height = parent.parent.height;
				lCoeff = (height * 1.0) / (originalHeight * 1.0);
				var lWidth = originalWidth * 1.0;
				width = (lWidth * lCoeff);
			}
		}

		onHeightChanged: {
			parent.height = height;
		}

		onWidthChanged: {
			//
		}

		onStatusChanged: {
			if (status == Image.Ready) {
				adjustView();
				adjustWidth();
			}

			if (status == Image.Error) {
				console.info("[onStatusChanged]: error = " + errorString);
			}
		}

		source: preview.startsWith("qrc") ? preview : (Qt.platform.os == "windows" ? "file:///" : "file://") + preview
		fillMode: BuzzerComponents.ImageQx.PreserveAspectFit
		mipmap: true
		anchors.centerIn: parent

		visible: (preview !== "none" || preview !== "") && !(player.hasVideo && player.position > 1)
	}

	Timer {
		id: loadTimer

		interval: 100
		repeat: false
		running: false

		onTriggered: {
			//
			player.play();
			sampleTimer.start();
			videoOut.adjustView();
			previewImage.adjustView();
		}
	}

	Timer {
		id: sampleTimer

		interval: 200
		repeat: false
		running: false

		onTriggered: {
			//
			player.pause();
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
		onStopped: { playing = false; paused = false; }

		Component.onCompleted: videoOut.push();

		function pushSurface(surface) {
			surfaces.pushSurface(surface);
			videoOutput = surfaces;
		}

		//videoOutput: surfaces

		onStatusChanged: {
			//
			switch(status) {
				case MediaPlayer.Loaded:
					if (!intialized) {
						size = buzzerApp.getFileSize(key);
						totalTime.setTotalTime(duration);
						totalSize.setTotalSize(size);
						playSlider.to = duration;
						//videoOutput.fillMode = VideoOutput.PreserveAspectFit;
						//
						player.seek(1);
						videoOut.adjustView();
						previewImage.adjustView();
						loadTimer.start();

						intialized = true;

						//
						adjustDuration(duration);
					}
				break;
				case MediaPlayer.Buffered:
					player.seek(1);
					videoOut.adjustView();
					previewImage.adjustView();
				break;
			}

			console.info("[onStatusChanged(9)]: status = " + status + ", duration = " + duration + ", path = " + path + ", size = " + size + ", preview = " + preview);
		}

		onPositionChanged: {
			elapsedTime.setTime(position);
			playSlider.value = position;
		}

		onPlaybackStateChanged: {
		}

		onErrorStringChanged: {
			console.info("[onErrorStringChanged(9)]: " + errorString);
		}

		property BuzzerComponents.VideoSurfaces surfaces: BuzzerComponents.VideoSurfaces {
			needPreview: true
			onPreviewPrepared: {
				//
				var lPath = buzzerClient.getTempFilesPath() + "/" + buzzerClient.generateRandom() + "-temp.jpeg";
				if (savePreview(lPath)) {
					console.info("[onPreviewPrepared]: preview file = " + lPath);
					previewAvailableDot.visible = true;
					preview = lPath;
				}
			}
		}

		function makePreview() {
			//
			previewAvailableDot.visible = false;
			surfaces.makePreview();
		}

		function terminate() {
			surfaces.clearSurfaces();
			player.stop();
		}
	}

	//
	Rectangle {
		id: controlsBack
		x: frameContainer.visible ? (frameContainer.x + spaceItems_) : (previewImage.x + spaceItems_)
		y: frameContainer.visible ? (frameContainer.y + frameContainer.height - (actionButton.height + 3 * spaceItems_)) :
									(previewImage.y + previewImage.height - (actionButton.height + 3 * spaceItems_))
		width: frameContainer.visible ? (frameContainer.width - (2 * spaceItems_)) : (previewImage.width - (2 * spaceItems_))
		height: actionButton.height + 2 * spaceItems_
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden.uni")
		opacity: 0.9
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
		fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 7)) : 18
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.highlight")
		radius: buzzerApp.isDesktop ? defaultRadius * buzzerClient.scaleFactor : defaultRadius

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
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 11
		text: description
		visible: description != "none"
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
	}

	QuarkLabel {
		id: elapsedTime
		x: actionButton.x + actionButton.width + spaceItems_ + (description != "none" ? 3 : 0)
		y: actionButton.y + (description != "none" ? caption.height + 3 : spaceItems_)
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - (description != "none" ? 3 : 0))) : 11
		text: "00:00"
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

		function setTime(ms) {
			text = DateFunctions.msToTimeString(ms);
		}
	}

	QuarkLabel {
		id: totalTime
		x: elapsedTime.x + elapsedTime.width
		y: elapsedTime.y
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - (description != "none" ? 3 : 0))) : 11
		text: "/00:00"
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

		function setTotalTime(ms) {
			text = "/" + DateFunctions.msToTimeString(ms);
		}
	}

	QuarkLabel {
		id: totalSize
		x: totalTime.x + totalTime.width
		y: elapsedTime.y
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - (description != "none" ? 3 : 0))) : 11
		text: ", 0k"
		visible: size !== 0
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

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
		y: actionButton.y + actionButton.height - (height - 3 * spaceItems_) + (buzzerClient.scaleFactor && description != "none" ? 3 : 0)
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

	QuarkRoundSymbolButton	{
		id: removeButton

		x: frameContainer.x + frameContainer.width - (width + 2*spaceItems_)
		y: frameContainer.y + 2*spaceItems_
		// Material.background: "transparent"
		visible: true
		spaceTop: 1
		textColor: (!sending || uploaded) ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
										  buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		symbol: !uploaded ? Fonts.cancelSym : Fonts.checkedSym
		radius: 20
		opacity: 0.8
		fontPointSize: 16
		enableShadow: true

		onClick: {
			if (!sending) {
				player.terminate();
				mediaList.removeMedia(index);
			}
		}
	}

	QuarkRoundSymbolButton	{
		id: clearDescriptionButton

		x: frameContainer.x + frameContainer.width - (width + 2*spaceItems_)
		y: removeButton.y + removeButton.height + 2*spaceItems_
		// Material.background: "transparent"
		visible: true
		spaceTop: 2
		textColor: (!sending || uploaded) ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
										  buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		symbol: Fonts.eraserSym
		radius: 20
		opacity: 0.8
		fontPointSize: 16
		enableShadow: true

		onClick: {
			if (!sending) {
				description = "none";
			}
		}
	}

	QuarkRoundSymbolButton	{
		id: replaceCoverButton

		x: frameContainer.x + frameContainer.width - (width + 2*spaceItems_)
		y: clearDescriptionButton.y + clearDescriptionButton.height + 2*spaceItems_
		// Material.background: "transparent"
		visible: true
		spaceTop: 2
		//symbolFontPointSize: 14
		textColor: (!sending || uploaded) ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
										  buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		symbol: Fonts.mediaSym
		radius: 20
		opacity: 0.8
		fontPointSize: 16
		enableShadow: true

		onClick: {
			if (!sending) {
				imageListing.open();
			}
		}
	}

	QuarkRoundSymbolButton	{
		id: useFrameForCoverButton

		x: frameContainer.x + frameContainer.width - (width + 2*spaceItems_)
		y: replaceCoverButton.y + replaceCoverButton.height + 2*spaceItems_
		// Material.background: "transparent"
		visible: true
		spaceTop: 2
		//symbolFontPointSize: 14
		textColor: (!sending || uploaded) ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
										  buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
		symbol: Fonts.filmSym
		radius: 20
		opacity: 0.8
		fontPointSize: 16
		enableShadow: true

		onClick: {
			if (!sending) {
				player.makePreview();
			}
		}
	}

	Rectangle {
		id: previewAvailableDot
		x: replaceCoverButton.x + replaceCoverButton.width - (buzzerApp.isDesktop ? buzzerClient.scaleFactor * 5 : 5)
		y: replaceCoverButton.y + (buzzerApp.isDesktop ? buzzerClient.scaleFactor * 5 : 5)
		width: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 6 : 6
		height: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 6 : 6
		radius: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 3 : 3
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Market.tabActive");
		visible: false
	}

	QuarkRoundProgress {
		id: mediaUploadProgress
		x: removeButton.x - 3
		y: removeButton.y - 3
		size: removeButton.width + 6
		colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
		colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
		arcBegin: 0
		arcEnd: progress
		lineWidth: 3
	}

	QtFileDialog {
		id: imageListing
		nameFilters: [ "Media files (*.jpg *.png *.jpeg)" ]
		selectExisting: true
		selectFolder: false
		selectMultiple: false

		onAccepted: {
			//
			var lPath = fileUrl.toString();
			if (Qt.platform.os == "windows") {
				lPath = lPath.replace(/^(file:\/{3})/,"");
			} else {
				lPath = lPath.replace(/^(file:\/{2})/,"");
			}

			//
			preview = decodeURIComponent(lPath);

			//
			player.stop();
			player.seek(1);
		}
	}
}
