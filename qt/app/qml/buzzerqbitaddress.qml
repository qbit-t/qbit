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
	id: buzzerqbitaddress_
	key: "buzzerqbitadress"
	stacked: false

	property var pKey_: buzzerClient.firstPKey()
	property var sKey_: buzzerClient.firstSKey()
	property var seedWords_: buzzerClient.firstSeedWords()
	property bool activateNewKey_: true
	property var nextPageTransition_: null

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

	function initialize(makeNewKey, activateNewKey, nextPageTransition) {
		//
		if (makeNewKey) {
			// get prev
			var lPrevPKey = buzzerClient.firstPKey();

			// make new
			pKey_ = buzzerClient.newKeyPair();
			sKey_ = buzzerClient.findSKey(pKey_);
			seedWords_ = buzzerClient.keySeedWords(pKey_);
			seedView.prepare();

			activateNewKey_ = activateNewKey;
			nextPageTransition_ = nextPageTransition;

			if (activateNewKey_) {
				// back-up current keys
				buzzerClient.setProperty("Client.previousKey", lPrevPKey);
				// set new key
				buzzerClient.setCurrentKey(pKey_);
				// reset wallet cache
				buzzerClient.resetWalletCache();
				// start re-sync, i.e. load owned utxos
				buzzerClient.resyncWalletCache();
				// broadcast dapp instance
				buzzerClient.broadcastCurrentState();
			}
		}
	}

	//
	// toolbar
	//
	QuarkToolBar {
		id: toolBar
		height: (buzzerApp.isDesktop || buzzerApp.isTablet ? (buzzerClient.scaleFactor * 50) : 45) + topOffset
		width: parent.width

		property int totalHeight: height

		function adjust() {
			if (parent.width > parent.height) {
				visible = false;
				height = 0;
			} else {
				visible = true;
				height = 45;
			}
		}

		Component.onCompleted: {
		}

		QuarkRoundSymbolButton {
			id: cancelButton
			x: spaceItems_
			y: parent.height / 2 - height / 2
			symbol: activateNewKey_ ? Fonts.leftArrowSym : Fonts.cancelSym
			fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 5)) : buzzerApp.defaultFontSize() + 7
			radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius - 7)) : (defaultRadius - 7)
			color: "transparent"
			textColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			opacity: 1.0

			onClick: {
				//
				if (activateNewKey_) {
					// restore key
					var lPrevKey = buzzerClient.getProperty("Client.previousKey");
					// set back
					buzzerClient.setCurrentKey(lPrevKey);
					// reset wallet cache
					buzzerClient.resetWalletCache();
					// start re-sync, i.e. load owned utxos
					buzzerClient.resyncWalletCache();
					// broadcast dapp instance
					buzzerClient.broadcastCurrentState();
				}

				// close
				closePage();
			}
		}

		QuarkLabel {
			id: buzzerControl
			x: cancelButton.x + cancelButton.width + 5
			y: parent.height / 2 - height / 2  + topOffset / 2
			width: parent.width - (x)
			text: buzzerApp.getLocalization(buzzerClient.locale, activateNewKey_ ? "Buzzer.create.new.address.caption" :
																				   "Buzzer.group.new.address.caption")
			color: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.link")
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 5)) : (buzzerApp.defaultFontSize() + 7)
		}

		QuarkHLine {
			id: bottomLine
			x1: 0
			y1: parent.height
			x2: parent.width
			y2: parent.height
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.bottom.separator")
			visible: true
		}
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

		text: buzzerApp.getLocalization(buzzerClient.locale, activateNewKey_ ? "Buzzer.qbitKeys" :
																			   "Buzzer.group.new.address")
	}

	QuarkInfoBox {
		id: addressBox
		x: 20
		y: qbitKeysText.y + qbitKeysText.height + 20
		width: parent.width - 20 * 2
		symbol: Fonts.tagSym
		clipboardButton: true

		text: pKey_
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

		text: sKey_

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
			//
			model_.clear();
			//
			var lWords = seedWords_;
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
		font.capitalization: Font.AllUppercase

		Layout.minimumWidth: 150
		Layout.alignment: Qt.AlignHCenter

		onClicked: {
			if (nextPageTransition_) nextPageTransition_();
			else {
				var lComponent = null;
				var lPage = null;

				lComponent = Qt.createComponent("qrc:/qml/buzzeraskqbit.qml");
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(window);
					lPage.controller = controller;

					addPage(lPage);
				}
			}
		}
	}

	InfoDialog {
		id: infoDialog
		bottom: 50
	}
}
