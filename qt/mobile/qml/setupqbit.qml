import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

QuarkPage
{
	id: setupqbit_
	key: "setupqbit"

	Component.onCompleted: {
		buzzerApp.lockPortraitOrientation();
        closePageHandler = closePage;
		activatePageHandler = activatePage;
    }

	function closePage() {
        stopPage();
        destroy(500);
        controller.popPage();
    }

	function activatePage() {
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
		toolBar.activate();
	}

	function onErrorCallback(error) {
        controller.showError(error);
    }

    //
    // toolbar
    //
	SetupToolBar {
		id: toolBar
		extraOffset: topOffset
	}

	//
	// Text
	//
	QuarkLabel {
		id: welcomeText
		x: 20
		y: toolBar.y + toolBar.totalHeight + 20
		width: parent.width-40
		wrapMode: Label.Wrap
		font.pointSize: 18

		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbit")
	}

	Image {
		id: qbitImage
		fillMode: Image.PreserveAspectFit
		x: parent.width / 2 - qbitImage.width / 2
		y: welcomeText.y + welcomeText.height + 30
		width: parent.width - 200
		Layout.alignment: Qt.AlignCenter
		mipmap: true
		source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "qbit.splash")
	}

	//
	// Switch (i'm already have seed words)
	//

	QuarkSwitchBox {
		id: switchBox
		x: nextButton.x - 10
		y: nextButton.y - height
		width: nextButton.width
		color: "transparent"
		border.color: "transparent"
		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.already.have.qbitKeys")
	}

	QuarkButton {
		id: nextButton
		x: welcomeText.x + 2
		y: parent.height - height - 20
		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.next")
		visible: true
		width: welcomeText.width

		Layout.minimumWidth: 150
		Layout.alignment: Qt.AlignHCenter

		onClicked: {
			var lComponent = null;
			var lPage = null;

			if (!switchBox.checked) {
				lComponent = Qt.createComponent("qrc:/qml/setupqbitaddress.qml");
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(window);
					lPage.controller = controller;

					addPage(lPage);
				}
			} else {
				lComponent = Qt.createComponent("qrc:/qml/buzzerqbitkey.qml");
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(window);
					lPage.controller = controller;
					lPage.setupProcess = true;

					addPage(lPage);
				}
			}
		}
	}
}
