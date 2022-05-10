import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1
import QtGraphicalEffects 1.0
import Qt.labs.folderlistmodel 2.11
import QtMultimedia 5.8
import QtQuick.Window 2.15

import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

Item
{
	id: walletreceive_

	property var controller;

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
	readonly property real defaultFontSize: 11

	function init() {
	}

	Component.onCompleted: {
		//
		if (buzzerClient.buzzerDAppReady) {
			buzzerClient.getWalletReceivedLog(asset_).feed(false);
		}
	}

	Connections {
		target: buzzerClient

		function onBuzzerDAppReadyChanged() {
			if (buzzerClient.buzzerDAppReady) {
				buzzerClient.getWalletReceivedLog(asset_).feed(false);
			}
		}

		function onThemeChanged() {
			receiptButton.adjust();
		}
	}

	//
	// Wallet receive
	//

	Flickable {
		 id: pageContainer
		 x: 0
		 y: 0
		 width: parent.width
		 height: parent.height
		 contentHeight: bottomItem.y
		 clip: true

		 onDragStarted: {
		 }

		 QuarkLabel {
			 id: receiveInfo
			 x: spaceLeft_
			 y: spaceTop_
			 width: parent.width - spaceLeft_*2
			 wrapMode: Label.Wrap
			 text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.receive." + unit_.toLowerCase())
			 font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 2)) : 16
			 Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");

			 onLinkActivated: {
				 if (isValidURL(link)) {
					Qt.openUrlExternally(link);
				 }
			 }

			 function isValidURL(str) {
				var regexp = /(ftp|http|https):\/\/(\w+:{0,1}\w*@)?(\S+)(:[0-9]+)?(\/|\/([\w#!:.?+=&%@!\-\/]))?/
				return regexp.test(str);
			 }
		 }

		 Rectangle {
			id: qrContainer
			x: qrCode.x - 10
			y: receiveInfo.y + receiveInfo.height + 10
			width: qrCode.getWidth()
			height: qrCode.getWidth()
			border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.border");
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background"); //Material.highlight
		}

		QRCode {
			id: qrCode
			x: getX()
			y: receiveInfo.y + receiveInfo.height + 20
			width: (parent.width / 3) * 2 - 20
			height: (parent.width / 3) * 2 - 20
			visible: true

			value: buzzerClient.address // default
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
			x: spaceLeft_ - 1
			y: qrContainer.y + qrContainer.height + spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_) + 2
			symbol: Fonts.tagSym
			clipboardButton: true
			height: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 37) : 42
			textFontSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : 16
			symbolFontSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 2)) : 20


			text: buzzerClient.address // default
		}

		Rectangle {
			id: amountInfo

			x: spaceLeft_
			y: addressBox.y + addressBox.height + 2*spaceItems_
			height: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 35) : 40 //amountEdit.height
			width: (addressBox.width-2) / 3 * 2
			color: "transparent"
			clip: true

			QuarkSymbolTap {
				id: privateButton
				x: amountEdit.width + 1
				y: 0
				symbol: privateSend ? Fonts.eyeSlashSym : Fonts.eyeSym
				border.color: "transparent"
				labelColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.zeroes");
				width: amountInfo.height - 1
				fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : 20

				color: "transparent"

				property bool privateSend: false

				onClicked: {
					//
					privateSend = !privateSend;
				}
			}

			QuarkNumberEdit {
				id: amountEdit
				x: 0 //privateButton.width + 1
				y: 0
				width: parent.width - privateButton.width - 1
				height: amountInfo.height //40
				fillTo: getFixed()
				xButtons: false
				//minDelta: "" + 1.0 / (scaleSend_ > 0 ? scaleSend_ : 1.0)

				fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 2)) : 18
				buzztonsFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 4)) : 20

				function getFixed() {
					return scaleSend_; //
				}

				onNumberStringModified: {
					receiptButton.adjust();
				}

				onPopupClosed: {
				}
			}
		}

		QuarkButton {
			id: receiptButton
			x: amountInfo.x + amountInfo.width //+ spaceItems_
			y: amountInfo.y - spaceItems_ - 1
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.wallet.receipt")
			Material.background: getBackground();
			visible: true
			width: (addressBox.width-2) / 3 //- spaceItems_
			height: amountInfo.height + 2 * spaceItems_ + 2

			Layout.alignment: Qt.AlignHCenter
			Layout.minimumWidth: (addressBox.width-2) / 2 //- spaceItems_

			enabled: !isNaN(amountEdit.numberString) && amountEdit.numberString.length

			contentItem: QuarkText {
				id: buttonText
				text: receiptButton.text
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : 16
				color: receiptButton.enabled ?
						   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground") :
						   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
				horizontalAlignment: Text.AlignHCenter
				verticalAlignment: Text.AlignVCenter
				elide: Text.ElideRight

				function adjust(val) {
					color = val ?
								buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground") :
								buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
				}
			}

			function adjust() {
				enabled = !isNaN(amountEdit.numberString) && amountEdit.numberString.length;
				buttonText.adjust(enabled);
			}

			onClicked: {
				var lAmount = parseFloat(amountEdit.numberString);
				if (lAmount < 0.0001) {
					//
					controller.showError({ code: "E_AMOUNT", message: "Invalid amount", component: "Balance" }, true);
				} else {
					// make receipt with parameters:
					// - address
					// - amount
					// - private
					var lSource;
					var lComponent;

					lSource = buzzerApp.isDesktop ? "qrc:/qml/walletreceivereceipt-desktop.qml" :
													"qrc:/qml/walletreceivereceipt.qml";
					lComponent = Qt.createComponent(lSource);
					if (lComponent.status === Component.Error) {
						controller.showError(lComponent.errorString());
					} else {
						var lPage = lComponent.createObject(controller);
						lPage.initialize(buzzerClient.address, amountEdit.numberString, privateButton.privateSend);
						lPage.controller = controller;
						controller.addPage(lPage);
					}
				}
			}

			function getBackground() {
				return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.4");
			}
		}

		//
		// History
		//

		Rectangle {
			id: historyInfo
			x: spaceLeft_
			y: amountInfo.y + amountInfo.height + spaceTop_
			height: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 35) : 35
			width: parent.width - (spaceLeft_ + spaceRight_)
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.background")

			QuarkLabel {
				id: historyLabel
				x: spaceLeft_
				y: historyInfo.height / 2 - historyLabel.height / 2
				text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.wallet.transactions") //.toUpperCase();
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 2)) : 16
				Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.altAccent");
			}
		}

		WalletTransactions {
			id: transactions
			x: spaceLeft_
			y: historyInfo.y + historyInfo.height + spaceItems_
			width: historyInfo.width
			height: walletreceive_.height - (historyInfo.y + historyInfo.height + spaceItems_)
			walletModel: buzzerClient.getWalletReceivedLog(asset_)
			controller: walletreceive_.controller
		}

		//
		// Bottom item
		//

		Item {
			id: bottomItem
			y: transactions.y + transactions.height + spaceBottom_
		}

		//
		// Flicking
		//

		onAtYBeginningChanged: {
			if (atYBeginning) transactions.height = walletreceive_.height - (historyInfo.y + historyInfo.height + spaceItems_);
			else transactions.height = walletreceive_.height - (historyInfo.height + spaceTop_ + spaceTop_);
		}
	}
}
