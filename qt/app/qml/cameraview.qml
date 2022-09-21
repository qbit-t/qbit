import QtMultimedia 5.15

import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtGraphicalEffects 1.15
import QtQuick.Shapes 1.15

import app.buzzer.components 1.0 as BuzzerComponents

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

Rectangle
{
    id: innerCameraContainer
    color: "transparent"
    anchors.fill: parent

    Camera
    {
        id: cameraDevice

        imageCapture
        {
            onImageSaved:
            {
				innerCameraContainer.stop();

                innerCameraContainer.parentContaner.applyPicture(path);
            }
        }

		focus.focusMode: Camera.FocusContinuous
		captureMode: Camera.CaptureStillImage
		flash.mode: Camera.FlashOff

		exposure {
			exposureMode: Camera.ExposureAuto
		}

		Component.onCompleted:
        {
			// cameraDevice.stop();
        }

		onError: {
			//
			console.log("[CAMERA]: " + errorCode + ": " + errorString);
		}

		onCameraStateChanged: {
		}
    }

	VideoOutput
	{
		id: videoOutput
		source: cameraDevice
		focus : visible // to receive focus and capture key events when visible
		x: 0
		y: 0
		width: innerCameraContainer.width
		height: innerCameraContainer.height
		fillMode: VideoOutput.PreserveAspectCrop
		autoOrientation: true
	}

	QuarkRoundProgress {
		id: roundFrame
		x: -62
		y: -62
		size: innerCameraContainer.width + 124
		colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
		colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
		arcBegin: 0
		arcEnd: 360
		lineWidth: 62
		animationDuration: 0
	}

    property variant parentContaner;

    function setup(container)
    {
        innerCameraContainer.parentContaner = container;
    }

    function start()
    {
		videoOutput.visible = true;
        cameraDevice.start();

		console.log("[CAMERA]: started");
    }

    function stop()
    {
		videoOutput.visible = false;
        cameraDevice.stop();

		console.log("[CAMERA]: STOPPPED!");
	}

    function captureToLocation(location)
    {
        videoOutput.visible = false;
		if (location !== "") cameraDevice.imageCapture.captureToLocation(location);
		else cameraDevice.imageCapture.capture();
    }

    function cameraState()
    {
        return cameraDevice.cameraState;
    }

	function rotate() {
		if (cameraDevice.position === Camera.BackFace) {
			cameraDevice.position = Camera.FrontFace;
		} else {
			cameraDevice.position = Camera.BackFace;
		}
	}
}
