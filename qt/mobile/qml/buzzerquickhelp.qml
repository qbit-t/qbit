﻿import QtQuick 2.9
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
	id: buzzerquickhelp_
	key: "buzzerquickhelp"

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
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
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
			symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.leftArrowSym

			onClicked: {
				closePage();
			}
		}

		QuarkLabel {
			id: buzzerControl
			x: cancelButton.x + cancelButton.width + 5
			y: parent.height / 2 - height / 2
			width: parent.width - (x)
			elide: Text.ElideRight
			text: "Help"
			font.pointSize: 18
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link")
		}

		QuarkHLine {
			id: bottomLine
			x1: 0
			y1: parent.height
			x2: parent.width
			y2: parent.height
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
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

		Image {
			id: buzzerImage
			x: spaceLeft_
			y: spaceTop_
			width: 50
			height: 50
			fillMode: Image.PreserveAspectFit
			source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "buzzer.icon")
			mipmap: true
		}

		QuarkLabel {
			id: buzzerHeader
			x: buzzerImage.x + buzzerImage.width + spaceLeft_
			y: spaceTop_ + buzzerImage.height / 2 - height / 2
			width: parent.width
			text: "Buzzer v." + buzzerApp.getVersion()
			font.pointSize: 24
			wrapMode: Text.Wrap

			onLinkActivated: {
			}
		}

		QuarkLabel {
			id: buzzerText0
			x: buzzerImage.x
			y: buzzerImage.y + buzzerImage.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.0")
			font.pointSize: 18
			wrapMode: Text.Wrap

			onLinkActivated: {
			}
		}

		QuarkLabel {
			id: buzzerText1
			x: buzzerText0.x
			y: buzzerText0.y + buzzerText0.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.1")
			font.pointSize: 18
			wrapMode: Text.Wrap

			onLinkActivated: {
			}
		}

		QuarkLabel {
			id: buzzerText2
			x: buzzerText1.x
			y: buzzerText1.y + buzzerText1.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.2")
			font.pointSize: 18
			wrapMode: Text.Wrap

			onLinkActivated: {
				//
				if (link[0] === '@') {
					var lId = buzzerClient.getConversationsList().locateConversation(link);
					// conversation found
					if (lId !== "") {
						openBuzzerConversation(lId);
					} else {
						createConversation.process(link);
					}
				}
			}
		}

		QuarkLabel {
			id: buzzerText3
			x: buzzerText2.x
			y: buzzerText2.y + buzzerText2.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.3")
			font.pointSize: 18
			wrapMode: Text.Wrap

			onLinkActivated: {
				//
				if (link[0] === '@') {
					var lId = buzzerClient.getConversationsList().locateConversation(link);
					// conversation found
					if (lId !== "") {
						openBuzzerConversation(lId);
					} else {
						createConversation.process(link);
					}
				}
			}
		}

		QuarkLabel {
			id: buzzerText4
			x: buzzerText3.x
			y: buzzerText3.y + buzzerText3.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.4")
			font.pointSize: 18
			wrapMode: Text.Wrap

			onLinkActivated: {
			}
		}

		QuarkLabel {
			id: buzzerText5
			x: buzzerText4.x
			y: buzzerText4.y + buzzerText4.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.5")
			font.pointSize: 18
			wrapMode: Text.Wrap

			onLinkActivated: {
			}
		}

		QuarkLabel {
			id: buzzerText6
			x: buzzerText5.x
			y: buzzerText5.y + buzzerText5.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.6")
			font.pointSize: 18
			wrapMode: Text.Wrap

			onLinkActivated: {
			}
		}

		QuarkLabel {
			id: buzzerText7
			x: buzzerText6.x
			y: buzzerText6.y + buzzerText6.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.7")
			font.pointSize: 18
			wrapMode: Text.Wrap

			onLinkActivated: {
			}
		}

		QuarkLabel {
			id: buzzerText8
			x: buzzerText7.x
			y: buzzerText7.y + buzzerText7.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_)
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.8")
			font.pointSize: 18
			font.italic: true
			wrapMode: Text.Wrap

			onLinkActivated: {
			}
		}
	}

	BuzzerCommands.CreateConversationCommand {
		id: createConversation

		onProcessed: {
			// open conversation
			openConversation.begin(buzzer);
		}

		onError: {
			// code, message
			handleError(code, message);
		}
	}

	Timer {
		id: openConversation
		interval: 500
		repeat: false
		running: false

		property string buzzer_;

		onTriggered: {
			//
			var lId = buzzerClient.getConversationsList().locateConversation(buzzer_);
			// conversation found
			if (lId !== "") {
				openBuzzerConversation(lId);
			} else {
				start();
			}
		}

		function begin(buzzer) {
			buzzer_ = buzzer;
			start();
		}
	}

	function openBuzzerConversation(conversationId) {
		// open conversation
		var lComponent = null;
		var lPage = null;

		//
		lComponent = Qt.createComponent("qrc:/qml/conversationthread.qml");
		if (lComponent.status === Component.Error) {
			controller.showError(lComponent.errorString());
		} else {
			lPage = lComponent.createObject(controller);
			lPage.controller = controller;

			var lConversation = buzzerClient.locateConversation(conversationId);
			if (lConversation) {
				addPage(lPage);
				lPage.start(conversationId, lConversation, buzzerClient.getConversationsList());
			}
		}
	}

	function handleError(code, message) {
		if (code === "E_CHAINS_ABSENT") return;
		if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
			buzzerClient.resync();
			controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
		} else {
			controller.showError(message);
		}
	}
}