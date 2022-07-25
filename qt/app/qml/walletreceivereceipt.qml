import QtQuick 2.15
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1
import QtGraphicalEffects 1.0
import Qt.labs.folderlistmodel 2.11
import QtMultimedia 5.8

import app.buzzer.components 1.0 as BuzzerComponents
import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

import "qrc:/lib/numberFunctions.js" as NumberFunctions

QuarkPage {
	id: walletreceivereceipt_
	key: "walletreceivereceipt"

	property var address_;
	property var amount_;
	property bool private_;

	closePageHandler: function() {
		closePage();
	}

	Component.onCompleted: {
	}

	function closePage() {
		stopPage();
		controller.popPage();
		destroy(1000);
	}

	function activatePage() {
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.background"));
	}

	function onErrorCallback(error)	{
		controller.showError(error);
	}

	function stopPage() {
	}

	function initialize(address, amount, privateSend) {
		//
		address_ = address;
		amount_ = amount;
		private_ = privateSend;

		qrCode.value = address_ + "|" + amount_ + "|" + private_;
		console.log("[initialize]: value = " + qrCode.value);
	}

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

	//
	// toolbar
	//
	QuarkToolBar {
		id: toolBar
		height: 45
		width: parent.width

		property int totalHeight: height

		Component.onCompleted: {
		}

		QuarkRoundSymbolButton {
			id: cancelButton
			x: spaceItems_
			y: parent.height / 2 - height / 2
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

		/*
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
		*/
	}

	//
	// Receipt
	//
	Rectangle {
		id: qrContainer
		x: qrCode.x - 10
		y: toolBar.height + 20
		width: qrCode.getWidth()
		height: qrCode.getWidth()
		border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.border");
		color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
	}

	QRCode {
		id: qrCode
		x: getX()
		y: toolBar.height + 30
		width: (parent.width / 3) * 2 - 20
		height: (parent.width / 3) * 2 - 20
		visible: true

		value: address_
		background: "transparent"

		onActualWidthChanged: {
			qrContainer.width = getWidth();
			qrContainer.height = getWidth();
		}

		function getX() {
			if (!actualWidth) return (parent.width / 3) / 2 + 10;
			return parent.width / 2 - actualWidth / 2;
		}

		function getY() {
			return 30;
		}

		function getWidth() {
			if (!actualWidth) return (parent.width / 3) * 2;
			return actualWidth + 20;
		}
	}

	QuarkInfoBox {
		id: addressBox
		x: 2 * spaceLeft_ - 1
		y: qrContainer.y + qrContainer.height + spaceTop_
		width: parent.width - (2 * spaceLeft_ + 2 * spaceRight_)
		symbol: Fonts.tagSym
		clipboardButton: true
		height: 40

		text: address_
	}

	QuarkInfoBox {
		id: amountBox
		x: 2 * spaceLeft_ - 1
		y: addressBox.y + addressBox.height + spaceItems_*2
		width: parent.width - (2 * spaceLeft_ + 2 * spaceRight_)
		symbol: Fonts.sigmaSym
		clipboardButton: true
		isNumber: true
		height: 40
		fillTo: scaleSend_

		number: amount_
	}

	Rectangle {
		id: privateBox
		x: 2 * spaceLeft_ + 1
		y: amountBox.y + amountBox.height + spaceItems_*2
		width: parent.width - (2 * spaceLeft_ + 2 * spaceRight_) - 3
		height: addressBox.height
		radius: 5
		color: getColor()


		QuarkLabel {
			id: privateLabel
			x: parent.width / 2 - width / 2
			y: parent.height / 2 - height / 2
			text: private_ ? buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.private") :
							 buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.public")
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground"); //Material.highlight
		}

		function getColor() {
			//
			if (private_) {
				return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.0");
			}

			return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.4");
		}
	}

	QuarkSymbolLabel {
		id: qrBeatSymbol
		x: parent.width / 2 - width / 2
		y: privateBox.y + privateBox.height + spaceTop_*2
		font.pointSize: 36
		symbol: Fonts.qrcodeSym

		function beat() {
			if (!animateScaleIn.running && !animateScaleOut.running) {
				animateScaleIn.start();
			}
		}

		NumberAnimation {
			id: animateScaleIn
			target: qrBeatSymbol
			properties: "scale"
			from: 1.0
			to: 1.3
			duration: 500
			easing { type: Easing.InBack; }

			onStopped: {
				animateScaleOut.start();
			}
		}

		NumberAnimation {
			id: animateScaleOut
			target: qrBeatSymbol
			properties: "scale"
			from: 1.3
			to: 1.0
			duration: 800
			easing { type: Easing.OutBack; }
		}
	}

	Timer {
		id: pulser
		interval: 1700
		repeat: true
		running: true

		onTriggered: {
			qrBeatSymbol.beat();
		}
	}
}
