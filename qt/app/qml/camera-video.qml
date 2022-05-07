import QtMultimedia 5.5

import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1
import StatusBar 0.1
import QtSensors 5.15

import app.buzzer.components 1.0 as BuzzerComponents

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

import "qrc:/lib/dateFunctions.js" as DateFunctions

QuarkPage {
	id: cameraVideoPage_

	property var localPath: buzzerClient.getTempFilesPath() + "/" + buzzerClient.generateRandom()
	signal videoReady(var path, var duration, var orientation, var preview);

    width: parent.width
    height: parent.height

	Component.onCompleted: {
		orientationSensor.start();
		buzzerApp.lockPortraitOrientation();
        closePageHandler = closePage;
    }

    statusBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.statusBar")
    navigationBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.navigationBar")

	function closePage() {
		//
		buzzerApp.unlockOrientation();
		orientationSensor.stop();
		//
		stopPage();
        controller.popPage();
		destroy(1000);
	}

	// recording control
	Timer {
		id: recordingVideo
		interval: 500
		repeat: true
		running: false

		onTriggered: {
			//
			if (cameraDevice.videoRecorder.duration >= videoRecorder.maxDuration) {
				// we are done
				buzzerApp.wakeRelease(); // release
				cameraDevice.videoRecorder.stop();
				recordingVideo.start();
			}
		}
	}

	OrientationSensor {
		id: orientationSensor
		onReadingChanged: {
			// console.log("[onReadingChanged]: " + reading.orientation);
			videoRecorder.orientation = reading.orientation;
		}
	}

	Camera {
		id: cameraDevice

        focus.focusMode: Camera.FocusContinuous
		captureMode: Camera.CaptureVideo
		exposure {
			exposureMode: Camera.ExposureAuto
            exposureCompensation: expoSlider.value
        }

		imageCapture.resolution: Qt.size(1920, 1080)
		viewfinder.resolution: Qt.size(1920, 1080)

		flash.mode: Camera.FlashOff
        digitalZoom: zoomSlider.value
		position: Camera.BackFace
    }

	BuzzerComponents.VideoRecorder {
		id: videoRecorder
		localPath: buzzerClient.getTempFilesPath()

		Component.onCompleted: {
			resolution = "1080p";
		}

		onDurationChanged: {
			// max 5 min
			var lPart = (duration * 100.0) / maxDuration;
			videoCaptureProgress.progress = lPart * 360.0 / 100.0;
			elapsedAudioTime.setTime(duration);
		}

		onStopped: {
			//
			if (actualFileLocation !== "" && previewLocation !== "") {
				// we have location and content saved
				videoReady(actualFileLocation, duration, unifiedOrientation, previewLocation);
				closePage();
			}
		}

		onActualFileLocationChanged: {
			//
			if (isStopped && actualFileLocation !== "" && previewLocation !== "") {
				// we have location and content saved
				videoReady(actualFileLocation, duration, unifiedOrientation, previewLocation);
				closePage();
			}
		}

		onIsRecordingChanged: {
		}

		onResolutionChanged: {
			elapsedAudioTime.setTotalTime(maxDuration);
		}

		onMaxDurationChanged: {
			elapsedAudioTime.setTotalTime(maxDuration);
		}
	}

	VideoOutput {
        id: viewfinder
		source: cameraDevice
        anchors.fill: parent
        focus : visible // to receive focus and capture key events when visible
        autoOrientation: true
		fillMode: VideoOutput.PreserveAspectCrop
    }

	QuarkToolButton {
		id: backButton
		symbol: Fonts.cancelSym
		visible: true
		labelYOffset: 3
		x: 10
		y: topOffset + 10

		onClicked: {
			cameraDevice.stop();
			closePage();
		}
    }

	//
	// resolution menu
	//

	QuarkSimpleComboBox {
		id: resolutionCombo
		x: backButton.x + backButton.width + 10
		y: backButton.y
		width: 65
		itemLeftPadding: 12
		itemTopPadding: 16
		itemHorizontalAlignment: Text.AlignHCenter

		// Material.background: "transparent"

		model: ListModel { id: resolutionModel_ }

		Component.onCompleted: {
			prepare();
		}

		indicator: Canvas {
		}

		clip: false

		onActivated: {
			var lEntry = resolutionModel_.get(resolutionCombo.currentIndex);
			if (lEntry !== undefined) {
				videoRecorder.resolution = lEntry.id;

				if (lEntry.id === "1080p") {
					cameraDevice.imageCapture.resolution = Qt.size(1920, 1080);
					cameraDevice.viewfinder.resolution = Qt.size(1920, 1080);
				} else if (lEntry.id === "720p") {
					cameraDevice.imageCapture.resolution = Qt.size(1280, 720);
					cameraDevice.viewfinder.resolution = Qt.size(1280, 720);
				} else if (lEntry.id === "480p") {
					cameraDevice.imageCapture.resolution = Qt.size(720, 480);
					cameraDevice.viewfinder.resolution = Qt.size(720, 480);
				}
			}
		}

		function prepare() {
			if (resolutionModel_.count) return;

			// very common dimensions for most cameras
			resolutionModel_.append({ id: "1080p", name: "1080p" });
			resolutionModel_.append({ id: "720p", name: "720p" });
			resolutionModel_.append({ id: "480p", name: "480p" });

			resolutionCombo.currentIndex = 0;
		}
	}

	QuarkToolButton {
		id: flashButton
		symbol: Fonts.flashLightSym
		visible: true
		labelYOffset: 3
		symbolRotation: -90.0
		x: rotateButton.x - (width + 5)
		y: topOffset + 10

		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");

		onClicked: {
			if (cameraDevice.flash.mode === Camera.FlashOff) {
				cameraDevice.flash.mode = Camera.FlashVideoLight;
				symbolColor = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
			} else {
				cameraDevice.flash.mode = Camera.FlashOff;
				symbolColor = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
			}
		}
	}

	QuarkToolButton {
		id: rotateButton
		symbol: Fonts.rotateSym
		visible: true
		labelYOffset: 3
		x: parent.width - (width + 10)
		y: topOffset + 10

		onClicked: {
			if (cameraDevice.position === Camera.BackFace) {
				cameraDevice.position = Camera.FrontFace;
			} else {
				cameraDevice.position = Camera.BackFace;
			}
		}
	}

	QuarkToolButton {
		id: photoButton
		x: parent.width / 2 - width / 2
		y: parent.height - (height + 50)
		width: 100
		height: width
		visible: true
		Layout.alignment: Qt.AlignHCenter
		radius: width / 2 //- 10

		Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background.transparent");

		Image {
			id: buzzImage
			x: parent.width / 2 - width / 2 + 3
			y: parent.height / 2 - height / 2 + 3
			width: 60
			height: 60
			source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "lightning.logo")
			visible: true
			fillMode: Image.PreserveAspectFit
			mipmap: true
		}

		onClicked: {
			//
			if (videoRecorder.isRecording) {
				buzzerApp.wakeRelease(); // release
				videoRecorder.stop();
				recordingVideo.stop();
			} else {
				//
				videoRecorder.camera = cameraDevice;
				buzzerApp.wakeLock(); // lock from sleep
				videoRecorder.record();
				recordingVideo.start();
			}
		}

		Rectangle {
			id: fill
			x: photoButton.contentItem.x
			y: photoButton.contentItem.y
			width: photoButton.contentItem.width
			height: width
			radius: width / 2
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.peer.banned")
			visible: videoRecorder.isRecording
		}
	}

	QuarkRoundProgress {
		id: videoCaptureProgress
		x: photoButton.x + 8
		y: photoButton.y + 8
		size: photoButton.width - 16
		colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
		colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
		arcBegin: 0
		arcEnd: progress
		lineWidth: 3
		visible: videoRecorder.isRecording

		property var progress: 0;
	}

	QuarkLabel {
		id: elapsedAudioTime
		x: photoButton.x + (photoButton.width / 2 - width / 2)
		y: photoButton.y + photoButton.height + (parent.height - (photoButton.y + photoButton.height)) / 2 - height / 2
		font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 14) : 14
		text: "00:00 / 00:00"
		visible: true
		color: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

		property string currentTime_: "00:00"
		property string totalTime_: "00:00"

		function setTime(ms) {
			currentTime_ = DateFunctions.msToTimeString(ms);
			text = currentTime_ + " / " + totalTime_;
		}

		function setTotalTime(ms) {
			totalTime_ = DateFunctions.msToTimeString(ms);
			text = currentTime_ + " / " + totalTime_;
		}
	}

	Column {
        id: exposureControl
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        height: expoSlider.height + expoInLabel.height + expoOutLabel.height

		QuarkSymbolLabel {
            id: expoInLabel
            symbol: Fonts.adjustSym
            x: exposureControl.width / 2 - width / 2
			color: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

			QuarkSymbolLabel {
                symbol: Fonts.plusSym
                font.pointSize: 9
                x: expoInLabel.width + 3
                y: 0
				color: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
            }
        }

		Slider {
            id: expoSlider
            to: 3
            from: -3
            orientation: Qt.Vertical
            stepSize: 0.5
			height: cameraVideoPage_.height/3
        }

		QuarkSymbolLabel {
            id: expoOutLabel
            symbol: Fonts.adjustSym
            x: exposureControl.width / 2 - width / 2
			color: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

			QuarkSymbolLabel {
                symbol: Fonts.minusSym
                font.pointSize: 9
                x: expoInLabel.width + 3
                y: 0
				color: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
            }
        }
    }

	Column {
        id: zoomControl
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: zoomSlider.height + zoomOurLabel.height + zoomInLabel.height

		QuarkSymbolLabel {
            id: zoomInLabel
            symbol: Fonts.zoomInSym
            x: zoomControl.width / 2 - width / 2 + 2
			color: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
        }

		Slider {
            id: zoomSlider
			to: cameraDevice.maximumOpticalZoom * cameraDevice.maximumDigitalZoom
            from: 1
            orientation: Qt.Vertical
            stepSize: 0.5
			height: cameraVideoPage_.height/3
        }

		QuarkSymbolLabel {
            id: zoomOurLabel
            symbol: Fonts.zoomOutSym
            x: zoomControl.width / 2 - width / 2 + 2
			color: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
        }
    }
}
