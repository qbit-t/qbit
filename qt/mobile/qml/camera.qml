import QtMultimedia 5.5

import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1
import StatusBar 0.1

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

QuarkPage {
	id: cameraPage_

    width: parent.width
    height: parent.height

	Component.onCompleted: {
        buzzerApp.lockPortraitOrientation();
        closePageHandler = closePage;
    }

    statusBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.statusBar")
    navigationBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.navigationBar")

	function closePage() {
        stopPage();
        controller.popPage();
		destroy(1000);
	}

	Camera {
        id: camera

        imageProcessing.whiteBalanceMode: CameraImageProcessing.WhiteBalanceFlash

        focus.focusMode: Camera.FocusContinuous
		captureMode: Camera.CaptureStillImage

		imageCapture {
			onImageSaved: {
				camera.stop();
				dataReady(path);
				closePage();
			}
		}

		exposure {
            exposureMode: Camera.ExposureAuto
            exposureCompensation: expoSlider.value
        }

		flash.mode: Camera.FlashOff

        digitalZoom: zoomSlider.value

		position: Camera.BackFace
    }

	VideoOutput {
        id: viewfinder
        source: camera
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
			camera.stop();
			closePage();
		}
    }

	QuarkToolButton {
		id: flashButton
		symbol: Fonts.flashSym
		visible: true
		labelYOffset: 3
		x: rotateButton.x - (width + 5)
		y: topOffset + 10

		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");

		onClicked: {
			if (camera.flash.mode === Camera.FlashOff) {
				camera.flash.mode = Camera.FlashAuto;
				symbolColor = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
			} else {
				camera.flash.mode = Camera.FlashOff;
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
			if (camera.position === Camera.BackFace) {
				camera.position = Camera.FrontFace;
			} else {
				camera.position = Camera.BackFace;
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
			camera.imageCapture.capture();
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

			QuarkSymbolLabel {
                symbol: Fonts.plusSym
                font.pointSize: 9
                x: expoInLabel.width + 3
                y: 0
            }
        }

		Slider {
            id: expoSlider
            to: 3
            from: -3
            orientation: Qt.Vertical
            stepSize: 0.5
			height: cameraPage_.height/3
        }

		QuarkSymbolLabel {
            id: expoOutLabel
            symbol: Fonts.adjustSym
            x: exposureControl.width / 2 - width / 2

			QuarkSymbolLabel {
                symbol: Fonts.minusSym
                font.pointSize: 9
                x: expoInLabel.width + 3
                y: 0
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
        }

		Slider {
            id: zoomSlider
            to: camera.maximumOpticalZoom * camera.maximumDigitalZoom
            from: 1
            orientation: Qt.Vertical
            stepSize: 0.5
			height: cameraPage_.height/3
        }

		QuarkSymbolLabel {
            id: zoomOurLabel
            symbol: Fonts.zoomOutSym
            x: zoomControl.width / 2 - width / 2 + 2
        }
    }
}
