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

	Component.onCompleted: {
		buzzerApp.lockPortraitOrientation();
        closePageHandler = closePage;
		activatePageHandler = activatePage;
	}

	property string action_: "CREATE"

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

	function initialize(action) {
		//
		action_ = action; // CREATE | UPDATE
		buzzerInfo.action = action;
	}

	//
	// toolbar
	//

	QuarkToolBar {
		id: toolBar
		height: 45
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

		QuarkToolButton	{
			id: cancelButton
			Material.background: "transparent"
			visible: true
			labelYOffset: 3
			symbolColor: buzzerApp.getColorStatusBar(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.leftArrowSym

			onClicked: {
				closePage();
			}
		}

		QuarkLabelRegular {
			id: buzzerControl
			x: cancelButton.x + cancelButton.width + 5
			y: parent.height / 2 - height / 2
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

			onProcessed: {
				//
				closePage();

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
