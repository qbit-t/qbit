import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1

import app.buzzer.components 1.0 as BuzzerComponents
import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

QuarkPage
{
	id: buzzerremove_
	key: "buzzerremove"
	stacked: false

	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceMedia_: 10
	readonly property int spaceSingleMedia_: 8
	readonly property int spaceMediaIndicator_: 15
	readonly property int spaceHeader_: 5
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property int spaceThreaded_: 33
	readonly property int spaceThreadedItems_: 4
	readonly property var hederFontSize_: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 9)) : (buzzerApp.defaultFontSize() + 13)
	readonly property var bodyFontSize_: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 3)) : (buzzerApp.defaultFontSize() + 7)

	Component.onCompleted: {
		buzzerApp.lockPortraitOrientation();
        closePageHandler = closePage;
		activatePageHandler = activatePage;
	}

	function closePage() {
		//
		buzzerApp.unlockOrientation();
		//
        stopPage();
        destroy(500);
        controller.popPage();
    }

	function activatePage() {
		buzzerApp.lockPortraitOrientation();
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.background"));
		toolBar.activate();
	}

	function onErrorCallback(error) {
        controller.showError(error);
    }

	//
	// toolbar
	//

	QuarkToolBar {
		id: toolBar
		height: buzzerApp.isDesktop || buzzerApp.isTablet ? (buzzerClient.scaleFactor * 50) : 45 + topOffset
		width: parent.width

		property int totalHeight: height

		function adjust() {
			if (buzzerApp.isDesktop || buzzerApp.isTablet) return;

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
			y: parent.height / 2 - height / 2  + topOffset / 2
			symbol: Fonts.cancelSym
			fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 5)) : buzzerApp.defaultFontSize() + 7
			radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius - 7)) : (defaultRadius - 7)
			color: "transparent"
			textColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			opacity: 1.0

			onClick: {
				closePage();
			}
		}

		QuarkLabel {
			id: buzzerControl
			x: cancelButton.x + cancelButton.width + 5
			y: parent.height / 2 - height / 2  + topOffset / 2
			width: parent.width - (x)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.remove.button")
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
	Flickable {
		 id: pageContainer
		 x: 0
		 y: toolBar.y + toolBar.height
		 width: parent.width
		 height: parent.height
		 contentHeight: buzzerText8.y + buzzerText8.height + toolBar.height + spaceBottom_
		 clip: true

		 onDragStarted: {
		 }

		QuarkLabel {
			id: buzzerText0
			x: spaceLeft_
			y: spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.remove.info").replace("{buzzer}", buzzerClient.name)
			font.pointSize: bodyFontSize_
			wrapMode: Text.Wrap
		}

		QuarkLabel {
			id: buzzerText1
			x: buzzerText0.x
			y: buzzerText0.y + buzzerText0.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.remove.info.1")
			font.pointSize: bodyFontSize_
			wrapMode: Text.Wrap
		}

		QuarkLabel {
			id: buzzerText2
			x: buzzerText1.x
			y: buzzerText1.y + buzzerText1.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.remove.info.2")
			font.pointSize: bodyFontSize_
			wrapMode: Text.Wrap
		}

		QuarkButton {
			id: removeButton
			x: parent.width / 2 - width / 2
			y: buzzerText2.y + buzzerText2.height + spaceTop_
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.remove.button")
			Material.background: getBackground()
			visible: true
			width: 100
			height: 70

			contentItem: QuarkText {
				id: buttonText
				text: removeButton.text
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : (buzzerApp.defaultFontSize() + 7)
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
				horizontalAlignment: Text.AlignHCenter
				verticalAlignment: Text.AlignVCenter
				elide: Text.ElideRight
			}

			Layout.alignment: Qt.AlignHCenter

			enabled: true

			onClicked: {
				//
				buzzerHideCommand.process();
			}

			function getBackground() {
				return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.wallet.send.enabled");
			}
		}
	}

	BuzzerCommands.BuzzerHideCommand {
		id: buzzerHideCommand

		onProcessed: {
			// unlink current buzzer
			buzzerClient.unlink();

			//
			var lComponent = null;
			var lPage = null;

			//
			lComponent = Qt.createComponent("qrc:/qml/buzzerexit.qml");
			if (lComponent.status === Component.Error) {
				handleError(lComponent.errorString());
			} else {
				lPage = lComponent.createObject(controller);
				lPage.controller = controller;

				controller.addPage(lPage);
			}
		}

		onError: {
			// code, message
			handleError(code, message);
		}
	}

	function handleError(code, message) {
		if (code === "E_CHAINS_ABSENT") return;
		if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
			//buzzerClient.resync();
			controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
		} else {
			controller.showError(message);
		}
	}
}
