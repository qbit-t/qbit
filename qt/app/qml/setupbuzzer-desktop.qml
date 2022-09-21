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
	id: setupbuzzer_
	key: "setupbuzzer"

	Component.onCompleted: {
		buzzerApp.lockPortraitOrientation();
        closePageHandler = closePage;
		activatePageHandler = activatePage;

		//
		if (!buzzerApp.isReadStoragePermissionGranted()) {
		}

		if (!buzzerApp.isWriteStoragePermissionGranted()) {
		}
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
		id: welcomeText
		x: 20
		y: toolBar.y + toolBar.totalHeight + 20
		width: parent.width-40
		wrapMode: Label.Wrap
		font.pointSize: 14

		text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.setupbuzzer")
	}

	Flickable {
		 id: pageContainer
		 x: 0
		 y: welcomeText.y + welcomeText.height + 10
		 width: parent.width
		 height: parent.height - (welcomeText.y + welcomeText.height + 10 + nextButton.height + 20 + 10)
		 contentHeight: buzzerInfo.calculatedHeight
		 clip: true

		 onDragStarted: {
		 }

		BuzzerInfoDesktop {
			id: buzzerInfo
			x: 0
			y: 10
			infoDialog: rootInfoDialog
			width: parent.width
			controller: setupbuzzer_.controller

			onProcessed: {
				nextButton.enabled = true;
			}
		}
	}

	InfoDialog {
		id: rootInfoDialog
		bottom: 50
	}

	QuarkButton	{
		id: nextButton
		x: 20
		y: parent.height - height - 20
		text: buttonAction === "down" ? Fonts.arrowDownSym : buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.next")
		visible: buzzerInfo.calculatedHeight > pageContainer.height && pageContainer.contentHeight - pageContainer.contentY > pageContainer.height ||
				 forceVisible
		enabled: buttonAction === "down" || forceEnabled ? true : false
		width: parent.width - 40

		font.family: buttonAction === "down" ? Fonts.icons : ""
		font.pointSize: buttonAction === "down" ? defaultFontPointSize + 5 : defaultFontPointSize
		font.capitalization: Font.AllUppercase

		property bool forceVisible: false
		property bool forceEnabled: false
		property string buttonAction: buzzerInfo.calculatedHeight > pageContainer.height ? "down" : "next" // "down|next"

		Layout.minimumWidth: 150
		Layout.alignment: Qt.AlignHCenter

		onClicked: {
			//
			if (buttonAction === "down") {
				//
				pageContainer.flick(0.0, (-1.0)*5000);
			} else {
				var lComponent = null;
				var lPage = null;

				lComponent = Qt.createComponent("qrc:/qml/buzzer-main-desktop.qml");
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(window);
					lPage.controller = controller;

					addPageLocal(lPage);
				}
			}
		}
	}
}
