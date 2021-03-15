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
	id: setupqbitaddress_
	key: "setupqbitadress"

	Component.onCompleted: {
		buzzerApp.lockPortraitOrientation();
        closePageHandler = closePage;
		activatePageHandler = activatePage;

		seedView.prepare();
    }

	function closePage() {
        stopPage();
        destroy(500);
        controller.popPage();
    }

	function activatePage() {
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
		id: qbitKeysText
		x: 20
		y: toolBar.y + toolBar.totalHeight + 20
		width: parent.width-40
		wrapMode: Label.Wrap
		font.pointSize: 14

		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbitKeys")
	}

	QuarkInfoBox {
		id: addressBox
		x: 20
		y: qbitKeysText.y + qbitKeysText.height + 20
		width: parent.width - 20 * 2
		symbol: Fonts.tagSym
		clipboardButton: true

		text: buzzerClient.firstPKey()
	}

	QuarkInfoBox {
		id: keyBox
		x: 20
		y: addressBox.y + addressBox.height + 10
		width: parent.width - 20 * 2
		symbol: Fonts.keySym
		clipboardButton: false
		helpButton: true
		textFontSize: 14

		text: buzzerClient.firstSKey()

		onHelpClicked: {
			var lMsgs = [];
			lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbitKeys.secret.help"));
			infoDialog.show(lMsgs);
		}
	}

	QuarkText {
		id: qbitSeedText
		x: 20
		y: keyBox.y + keyBox.height + 20
		width: parent.width-40
		wrapMode: Text.Wrap
		font.pointSize: 14

		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbitSeed")
	}

	QuarkListView {
		id: seedView
		x: 20
		y: qbitSeedText.y + qbitSeedText.height + 20
		height: nextButton.y - y - 20
		width: parent.width-40
		model: ListModel { id: model_ }
		clip: true

		delegate: Rectangle	{
			height: 40
			width: seedView.width
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.background");
			//color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");

			QuarkText {
				text: name
				x: 10
				y: parent.height / 2 - height / 2
				font.pointSize: 14
			}
		}

		function prepare() {
			if (model_.count) return;

			var lWords = buzzerClient.firstSeedWords();
			for (var lIdx = 0; lIdx < lWords.length; lIdx++) {
				model_.append({ name: lWords[lIdx] });
			}
		}
	}

	QuarkButton	{
		id: nextButton
		x: qbitKeysText.x + 2
		y: parent.height - height - 20
		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.next")
		visible: true
		width: seedView.width

		Layout.minimumWidth: 150
		Layout.alignment: Qt.AlignHCenter

		onClicked: {
			var lComponent = null;
			var lPage = null;

			lComponent = Qt.createComponent("qrc:/qml/setupaskqbit-desktop.qml");
			if (lComponent.status === Component.Error) {
				controller.showError(lComponent.errorString());
			} else {
				lPage = lComponent.createObject(window);
				lPage.controller = controller;

				addPageLocal(lPage);
			}
		}
	}

	InfoDialog {
		id: infoDialog
		bottom: 50
	}
}
