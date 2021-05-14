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
	id: walletbalance_

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
		balanceCommand.process();
	}

	Component.onCompleted: {
		//
		if (buzzerClient.buzzerDAppReady) {
			balanceCommand.process();
			buzzerClient.getWalletLog().feed(false);
		}
	}

	Connections {
		target: buzzerClient

		function onBuzzerDAppReadyChanged() {
			if (buzzerClient.buzzerDAppReady) {
				balanceCommand.process();
				buzzerClient.getWalletLog().feed(false);
			}
		}

		function onMainChainStateUpdated(block, height, time, ago) {
			//
			heightControl.text = height;
			agoControl.text = ago;
			blockControl.text = block;
			timeControl.text = time;
			heartBeatSymbol.beat();
		}

		function onWalletTransactionReceived(chain, tx) {
			// refresh balance
			balanceCommand.process();
		}
	}

	Connections {
		target: buzzerClient.getWalletLog()

		function onOutUpdatedSignal() {
			// refresh balance
			balanceCommand.process();
		}
	}

	//
	BuzzerCommands.BalanceCommand {
		id: balanceCommand

		onProcessed: {
			// amount, pending, scale
			availableNumber.number = amount;
			pendingNumber.number = pending;

			if (amount < 0.00000001)
				checkBalance.start();
		}

		onError: {
			controller.showError({ code: code, message: message, component: "Balance" });
		}
	}

	//
	Timer {
		id: checkBalance
		interval: 2000
		repeat: false
		running: false

		onTriggered: {
			balanceCommand.process();
		}
	}


	//
	// QBIT balance
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

		 Image {
			 id: qbitBackImage

			 x: parent.width * 5 / 100
			 y: spaceTop_
			 width: parent.width - (parent.width * 10 / 100)
			 fillMode: Image.PreserveAspectFit
			 source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "wallet.balance")
			 mipmap: true

			 QuarkNumberLabel {
				 id: availableNumber
				 number: 0.00000000
				 font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 22) : 30
				 fillTo: 8
				 mayCompact: true
				 x: parent.width / 2 - calculatedWidth / 2
				 y: parent.height / 2 - (calculatedHeight + middleLine.penWidth + spaceItems_ * 2 + pendingControl.height) / 2
				 color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
				 numberColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
				 mantissaColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
				 zeroesColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
				 unitsColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
			 }

			 MouseArea {
				 x: availableNumber.x
				 y: availableNumber.y
				 width: availableNumber.calculatedWidth
				 height: availableNumber.calculatedHeight

				 onClicked: {
					 availableNumber.mayCompact = !availableNumber.mayCompact;
				 }
			 }

			 QuarkLabel {
				 id: qbitText1
				 x: availableNumber.x + availableNumber.calculatedWidth + spaceItems_
				 y: availableNumber.y
				 text: "QBIT"
				 font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * defaultFontSize) : 16
				 color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
			 }

			 QuarkHLine {
				 id: middleLine
				 x1: availableNumber.x - spaceLeft_
				 y1: availableNumber.y + availableNumber.calculatedHeight + spaceItems_
				 x2: availableNumber.x + availableNumber.calculatedWidth + spaceLeft_
				 y2: availableNumber.y + availableNumber.calculatedHeight + spaceItems_
				 penWidth: 2
				 color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
				 visible: true
			 }

			 QuarkLabel {
				 id: pendingControl
				 x: parent.width / 2 - (width + pendingNumber.calculatedWidth + /*qbitText2.width + 2*/ + spaceItems_)/ 2
				 y: middleLine.y1 + spaceItems_
				 text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.balance.pending")
				 font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : 14
				 color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

				 QuarkNumberLabel {
					 id: pendingNumber
					 number: 0.00000000
					 font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : 14
					 fillTo: 8
					 x: pendingControl.width + spaceItems_
					 y: 0
					 numberColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
					 mantissaColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
					 zeroesColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
					 unitsColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
				 }
			 }
		 }

		 QuarkToolButton {
			 id: refreshControl
			 x: qbitBackImage.x + qbitBackImage.width - width - spaceItems_
			 y: qbitBackImage.y + spaceItems_
			 symbol: Fonts.rotateSym
			 visible: true
			 labelYOffset: buzzerApp.isDesktop ? 1 : 3
			 symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")
			 Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.background");
			 Layout.alignment: Qt.AlignHCenter
			 opacity: 0.7
			 symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 14) : symbolFontPointSize

			 onClicked: {
				 // refeed balance
				 balanceCommand.process();
				 // refeed transactions
				 buzzerClient.getWalletLog().feed(false);
			 }
		 }

		 //
		 // QBIT information & heartbeat
		 //

		 QuarkSymbolLabel {
			 id: heartBeatSymbol
			 x: qbitBackImage.x
			 y: qbitBackImage.y + qbitBackImage.height + spaceTop_
			 font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 3)) : 22
			 symbol: Fonts.heartBeatSym

			 function beat() {
				 if (!animateScaleIn.running && !animateScaleOut.running) {
					 animateScaleIn.start();
				 }
			 }

			 NumberAnimation {
				 id: animateScaleIn
				 target: heartBeatSymbol
				 properties: "scale"
				 from: 1.0
				 to: 1.3
				 duration: buzzerApp.isDesktop ? 600 : 200
				 easing { type: Easing.InBack; }

				 onStopped: {
					 animateScaleOut.start();
				 }
			 }

			 NumberAnimation {
				 id: animateScaleOut
				 target: heartBeatSymbol
				 properties: "scale"
				 from: 1.3
				 to: 1.0
				 duration: buzzerApp.isDesktop ? 1000 : 600
				 easing { type: Easing.OutBack; }
			 }
		 }

		 QuarkSymbolLabel {
			 id: blockSymbol
			 x: qbitBackImage.x + 1
			 y: blockControl.y - 1
			 font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 3)) : 20
			 symbol: Fonts.blockSym
		 }

		 QuarkSymbolLabel {
			 id: blockTimeSymbol
			 x: qbitBackImage.x + 1
			 y: timeControl.y - 1
			 font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 3)) : 20
			 symbol: Fonts.clockSym
		 }

		 QuarkLabel {
			 id: heightControl
			 x: heartBeatSymbol.x + heartBeatSymbol.width + spaceLeft_
			 y: heartBeatSymbol.y + 3
			 text: "0000000000"
			 font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : defaultFontPointSize

			 onTextChanged: {
				 transactions.updateAgo();
			 }
		 }

		 Rectangle {
			 id: point0

			 x: heightControl.x + heightControl.width + spaceItems_
			 y: heightControl.y + heightControl.height / 2 - height / 2
			 width: spaceItems_
			 height: spaceItems_
			 radius: width / 2

			 color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
			 border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
		 }

		 QuarkLabel {
			 id: agoControl
			 x: point0.x + point0.width + spaceItems_
			 y: heightControl.y
			 text: "0s"
			 font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : defaultFontPointSize
		 }

		 QuarkLabelRegular {
			 id: blockControl
			 x: heartBeatSymbol.x + heartBeatSymbol.width + spaceLeft_
			 y: heightControl.y + heightControl.height + spaceItems_
			 width: parent.width - (x + spaceRight_)
			 elide: Text.ElideRight
			 text: "00000000000000000000000000000000000000000000000000"
			 font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : defaultFontPointSize
		 }

		 QuarkLabelRegular {
			 id: timeControl
			 x: heartBeatSymbol.x + heartBeatSymbol.width + spaceLeft_
			 y: blockControl.y + blockControl.height + spaceItems_
			 width: parent.width - (x + spaceRight_)
			 elide: Text.ElideRight
			 text: "00:00:00 00/00/0000"
			 font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize)) : defaultFontPointSize
		 }

		 //
		 // History
		 //

		 Rectangle {
			 id: historyInfo
			 x: spaceLeft_
			 y: timeControl.y + timeControl.height + spaceTop_
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
			 height: walletbalance_.height - (historyInfo.y + historyInfo.height + spaceItems_)
			 walletModel: buzzerClient.getWalletLog()
			 controller: walletbalance_.controller
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
			 if (atYBeginning) transactions.height = walletbalance_.height - (historyInfo.y + historyInfo.height + spaceItems_);
			 else transactions.height = walletbalance_.height - (historyInfo.height + spaceTop_ + spaceTop_);
		 }
	}
}
