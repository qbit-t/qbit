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
	id: buzzercreateupdate_
	key: "buzzercreateupdate"

	// spacing
	readonly property int spaceLeft_: 15
	readonly property int spaceTop_: 12
	readonly property int spaceRight_: 15
	readonly property int spaceBottom_: 12
	readonly property int spaceAvatarBuzz_: 10
	readonly property int spaceItems_: 5
	readonly property int spaceHeader_: 5
	readonly property int spaceRightMenu_: 15
	readonly property int spaceStats_: -5
	readonly property int spaceLine_: 4
	readonly property int spaceMedia_: 20

	Component.onCompleted: {
		buzzerApp.lockPortraitOrientation();
        closePageHandler = closePage;
		activatePageHandler = activatePage;
	}

	property string action_: "CREATE"
	property string source_: "setup"

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

	function initialize(action, source) {
		//
		action_ = action; // CREATE | UPDATE
		if (source) source_ = source;
		console.info("[initialize]: source_ = " + source_);
		buzzerInfo.action = action;
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
			y: parent.height / 2 - height / 2 +  + topOffset / 2
			symbol: Fonts.leftArrowSym
			fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 5)) : buzzerApp.defaultFontSize() + 7
			radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius - 7)) : (defaultRadius - 7)
			color: "transparent"
			textColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			opacity: 1.0

			onClick: {
				closePage();
			}
		}

		QuarkLabelRegular {
			id: buzzerControl
			x: cancelButton.x + cancelButton.width + 5
			y: parent.height / 2 - height / 2 +  + topOffset / 2
			width: parent.width - (x)
			elide: Text.ElideRight
			text: action_ !== "CREATE" ? buzzerClient.name : ""
			font.pointSize: 18
			color: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.link")
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
		id: warningText
		x: 20
		y: toolBar.y + toolBar.totalHeight + 20
		width: parent.width-40
		wrapMode: Label.Wrap
		font.pointSize: 18

		text: action_ !== "CREATE" ?
				  buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.updateBuzzer") :
				  buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.createBuzzer")
	}

	Flickable {
		 id: pageContainer
		 x: 0
		 y: warningText.y + warningText.height + 10
		 width: parent.width
		 height: parent.height - (warningText.y + warningText.height + 10)
		 contentHeight: buzzerInfo.calculatedHeight
		 clip: true

		 onDragStarted: {
		 }

		BuzzerInfo {
			id: buzzerInfo
			x: 0
			y: 10
			infoDialog: rootInfoDialog
			width: parent.width
			controller: buzzercreateupdate_.controller
			action: action_
			source: source_

			onProcessed: {
				//
				if (source_ === "wizard") controller.popNonStacked();
				else closePage();

				//
				if (action_ === "CREATE") {
					buzzerClient.getBuzzfeedList().clear();
					buzzerClient.getBuzzfeedList().resetModel();

					buzzerClient.getEventsfeedList().clear();
					buzzerClient.getEventsfeedList().resetModel();

					buzzerClient.getConversationsList().clear();
					buzzerClient.getConversationsList().resetModel();
				}
			}
		}
	}

	InfoDialog {
		id: rootInfoDialog
		bottom: 50
	}
}
