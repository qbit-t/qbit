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
	id: setupinfo_
	key: "setupinfo"

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

	function activatePage()	{
		buzzerApp.lockPortraitOrientation();
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
		toolBar.activate();
	}

	function onErrorCallback(error) {
        controller.showError(error);
    }

    //
    // toolbar
    //
	SetupToolBarDesktop {
		id: toolBar
		extraOffset: topOffset
		x: 10
		width: parent.width - 20
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
		font.pointSize: 14

		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.welcome")

		onLinkActivated: {
			Qt.openUrlExternally(link);
		}
	}

	Image {
		id: decentralized
		fillMode: Image.PreserveAspectFit
		x: parent.width / 2 - decentralized.width / 2
		y: welcomeText.y + welcomeText.height
		width: (parent.width + 20) > 700 ? 700 : parent.width + 20
		height: (nextButton.y - (welcomeText.y + welcomeText.height)) - 30
		Layout.alignment: Qt.AlignCenter
		mipmap: true
		source: "../images/decentralized.png"
	}

	QuarkButton	{
		id: nextButton
		x: welcomeText.x + 2
		y: parent.height - height - 20
		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.next")
		visible: true
		width: welcomeText.width
		font.capitalization: Font.AllUppercase

		Layout.minimumWidth: 150
		Layout.alignment: Qt.AlignHCenter

		onClicked: {
			var lComponent = null;
			var lPage = null;

			lComponent = Qt.createComponent(buzzerApp.getLimited() ? "qrc:/qml/setupeula-desktop.qml" : "qrc:/qml/setupqbit-desktop.qml");
			lPage = lComponent.createObject(window);
			lPage.controller = controller;

			addPageLocal(lPage);
		}
	}
}
