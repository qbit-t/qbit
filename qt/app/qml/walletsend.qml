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
import QtQuick.Controls.Styles 1.2

import app.buzzer.commands 1.0 as BuzzerCommands

import "qrc:/lib/numberFunctions.js" as NumberFunctions
import "qrc:/fonts"
import "qrc:/components"
import "qrc:/qml"

Item
{
	id: walletsend_

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
	property double balance_: 0.0;
	readonly property real defaultFontSize: 11

	function init() {
		sendToAddress.rollback();
		balanceCommand.process();
	}

	function initSendTo(buzzer) {
		addressBox.prevText_ = buzzer;
		addressBox.text = buzzer;
	}

	Component.onCompleted: {
		//
		if (buzzerClient.buzzerDAppReady) {
			buzzerClient.getWalletSentLog().feed(false);
			addressBox.prepare();
		}
	}

	Connections {
		target: buzzerClient

		function onBuzzerDAppReadyChanged() {
			if (buzzerClient.buzzerDAppReady) {
				buzzerClient.getWalletSentLog().feed(false);
				addressBox.prepare();
			}
		}

		function onWalletTransactionReceived(chain, tx) {
			// refresh balance
			balanceCommand.process();
		}

		function onThemeChanged() {
			sendButton.adjust();
		}
	}

	BuzzerCommands.SearchEntityNamesCommand {
		id: searchBuzzers

		onProcessed: {
			// pattern, entities
			buzzersList.popup(pattern, entities);
		}

		onError: {
			controller.showError(message);
		}
	}

	//
	// Wallet send
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

		QuarkAddressBox {
			id: addressBox
			x: spaceLeft_ - 1
			y: spaceTop_
			width: parent.width - (spaceLeft_ + spaceRight_) + 2
			placeholderText: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.wallet.address")
			editor: true
			scan: buzzerApp.isDesktop ? false : true
			textFontSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : 16
			symbolFontSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 4)) : 18 //20
			height: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 50) : 50

			model: ListModel { id: contactsModel_ }

			property var prevText_: ""

			onAddressTextChanged: {
				//
				if (address[0] === '@' && prevText_ !== address) {
					prevText_ = address;
					if (!contactExists(address))
						searchBuzzers.process(address);
					else
						amountInfo.prepare();
				}

				sendButton.adjust();
			}

			onEditTextChanged: {
				prevText_ = address;
				if (address === "") addressBox.address = "";
			}

			onAdjustAddress: {
				sendButton.adjust();
			}

			onAddAddress: {
				addAddressDialog.show(addressBox.text, addressBox.address);
			}

			onRemoveAddress: {
				// id = buzzer
				buzzerClient.removeContact(id);
				prepare();
			}

			onScanButtonClicked: {
				var lComponent = Qt.createComponent("qrc:/qml/qrscanner.qml");
				var lPage = lComponent.createObject(controller);
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
				} else {
					lPage.controller = controller;
					lPage.caption = buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.wallet.scan.address")
					lPage.dataReady.connect(addressScanned);

					controller.addPage(lPage);
				}
			}

			onCleared: {
				sendButton.adjust();
			}

			function addressScanned(data) {
				if (data.indexOf("|") >= 0) {
					//
					var lParts = data.split("|");
					addressBox.text = lParts[0]; // address | amount | false, true
					amountEdit.numberString = lParts[1];
					if (lParts[2] === "true") {
						privateButton.privateSend = true;
					} else {
						privateButton.privateSend = false;
					}
					sendToAddress.privateSend = privateButton.privateSend;

					//
					amountInfo.prepare();
				} else {
					addressBox.text = data; // address
					amountInfo.prepare();
				}
			}

			function prepare() {
				//
				if (contactsModel_.count) contactsModel_.clear();

				var lContacts = buzzerClient.contacts;
				for (var lIdx = 0; lIdx < lContacts.length; lIdx++) {
					//
					contactsModel_.append({
					  id: lContacts[lIdx].buzzer(),
					  label: lContacts[lIdx].buzzer(),
					  address: lContacts[lIdx].pkey()});
				}
			}

			function contactExists(buzzer) {
				//
				if (!contactsModel_.count) return false;
				//
				var lContacts = buzzerClient.contacts;
				for (var lIdx = 0; lIdx < lContacts.length; lIdx++) {
					//
					if (lContacts[lIdx].buzzer() === buzzer)
						return true;
				}

				return false;
			}
		}

		Rectangle {
			id: amountInfo

			x: spaceLeft_
			y: addressBox.y + addressBox.height + 2*spaceItems_
			height: amountEdit.height + spaceItems_ + feeRateEdit.height + 3 //!
			width: (addressBox.width-2) / 3 * 2
			color: "transparent"
			clip: true

			QuarkNumberEdit {
				id: amountEdit
				x: 0
				y: 0
				width: parent.width - 1
				height: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 35) : 35
				fillTo: getFixed()
				xButtons: false
				//percentButtons: true

				fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : 18
				buzztonsFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 3)) : 20

				function getFixed() {
					return 4; //
				}

				onNumberStringModified: {
					//console.log("text = '" + addressBox.text + "', address = '" + addressBox.address + "'");
					if (addressBox.text !== "" && addressBox.text[0] === '@') {
						loadEntity.process(addressBox.text);
					} else {
						amountInfo.prepare();
					}
				}

				onClearClicked: {
					//
					sendToAddress.rollback();
					balanceCommand.process();
					//amountInfo.recalculate();
				}

				onPopupClosed: {
				}

				onPercentClicked: {
					// percent
				}
			}

			QuarkNumberEdit {
				id: feeRateEdit
				x: 0
				y: amountEdit.height + spaceItems_ + 3 //!
				width: parent.width - 1
				height: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 35) : 35
				fillTo: 0
				xButtons: false
				units: "qBIT/b"
				numberString: NumberFunctions.scientificToDecimal(parseFloat(buzzerClient.getQbitRate()))
				useLimits: true
				leftLimit: 1.0
				rightLimit: 100.0

				fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : 18
				buzztonsFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 2)) : 20
				unitsFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize - 4)) : 10

				onNumberStringModified: {
					amountInfo.prepare();
					//balanceCommand.process();
				}

				onPopupClosed: {
				}
			}

			function recalculate() {
				//
				var lSend = NumberFunctions.scientificToDecimal(sendToAddress.finalAmount());
				if (balance_ > 0.0 && lSend > 0.0) {
					sendAmount.number = lSend;
					availableAmount.number = balance_ - lSend;
				} else {
					sendAmount.number = 0.0;
					availableAmount.number = balance_;

					// try prepare
					prepare();
				}

				//balanceCommand.process();
				sendButton.adjust();
			}

			function prepare() {
				// NOTICE: manual broadcast
				if (!isNaN(amountEdit.numberString) && amountEdit.numberString.length &&
						NumberFunctions.scientificToDecimal(amountEdit.numberString) > 0.0 &&
					!isNaN(feeRateEdit.numberString) && feeRateEdit.numberString.length &&
						NumberFunctions.scientificToDecimal(feeRateEdit.numberString) > 0.0) {
					//
					var lAddress = addressBox.text;
					if (addressBox.address !== "") lAddress = addressBox.address;
					if (lAddress !== "" && lAddress[0] !== '@')
						sendToAddress.process("*", lAddress, amountEdit.numberString, feeRateEdit.numberString);
				} else {
					sendAmount.number = 0.0;
					availableAmount.number = balance_;
				}

				sendButton.adjust();
			}
		}

		Rectangle {
			id: totalInfo

			x: amountInfo.x
			y: amountInfo.y + amountInfo.height + spaceItems_ + 3 //!
			height: sendAmount.calculatedHeight + availableAmount.calculatedHeight + spaceItems_ * 5
			width: amountInfo.width
			clip: true

			QuarkNumberLabel {
				id: sendAmount
				x: totalInfo.width / 2 - sendAmount.calculatedWidth / 2
				y: amountEdit.height / 2 - sendAmount.height / 2 + (Qt.platform.os == "android" ? 0 : 1)
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : 16

				zeroesColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.zeroes")

				units: ""
				number: 0.0
				fillTo: 8

				QuarkSymbolLabel {
					id: totalSym
					x: -width - spaceItems_
					y: y - (Qt.platform.os == "android" ? 0 : 0)
					symbol: Fonts.sigmaSym
					font.pointSize: sendAmount.font.pointSize + 1
					color: sendAmount.getColor()
				}

				function getColor() {
					return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.up");
				}
			}

			QuarkHLine {
				id: middleLine
				x1: sendAmount.x - (spaceLeft_ + amountSym.width)
				y1: sendAmount.y + sendAmount.calculatedHeight + spaceItems_
				x2: sendAmount.x + sendAmount.calculatedWidth + spaceLeft_
				y2: sendAmount.y + sendAmount.calculatedHeight + spaceItems_
				penWidth: 1
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
				visible: true
			}

			QuarkNumberLabel {
				id: availableAmount
				x: sendAmount.x + sendAmount.calculatedWidth - calculatedWidth
				y: sendAmount.y + sendAmount.calculatedHeight + 2 * spaceItems_
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : 16

				zeroesColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.zeroes")

				units: ""
				number: 0.0
				fillTo: 8

				QuarkSymbolLabel {
					id: amountSym
					x: -width - spaceItems_
					y: y - (Qt.platform.os == "android" ? 0 : 0)
					symbol: Fonts.coinsSym
					font.pointSize: sendAmount.font.pointSize + 1
				}
			}

			border.color: "transparent"
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.background");
		}

		QuarkButton {
			id: sendButton
			x: amountInfo.x + amountInfo.width + spaceItems_
			y: amountInfo.y - spaceItems_ - 1
			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.wallet.send")
			Material.background: getBackground();
			visible: true
			width: (addressBox.width-2) / 3 - spaceItems_
			height: amountInfo.height + 2 * spaceItems_ + totalInfo.height + 2 * spaceItems_

			contentItem: QuarkText {
				id: buttonText
				text: sendButton.text
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : 18
				color: sendButton.enabled ?
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

			Layout.alignment: Qt.AlignHCenter
			Layout.minimumWidth: (addressBox.width-2) / 2

			enabled: ready()

			onClicked: {
				//
				var lBuzzer = addressBox.text;
				var lAddress = addressBox.address;

				if (lBuzzer !== "" && lAddress !== "")
					confirmSendDialog.show(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.wallet.confirm.send").
										   replace("{amount}", amountEdit.numberString).
										   replace("{buzzer}", lBuzzer).
										   replace("{address}", lAddress));
				else if (lBuzzer !== "" && lBuzzer[0] !== '@')
					confirmSendDialog.show(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.wallet.confirm.send.address").
										   replace("{amount}", amountEdit.numberString).
										   replace("{address}", lBuzzer));
			}

			function ready() {
				//console.log("[sendReady]: text = '" + addressBox.text + "', address = '" + addressBox.address + "'");
				var lAddress = addressBox.text;
				if (addressBox.address !== "") lAddress = addressBox.address;
				if (lAddress !== "" && lAddress[0] === '@') lAddress = "";
				var lEnabled = (lAddress !== "" && !isNaN(amountEdit.numberString) && amountEdit.numberString.length &&
						NumberFunctions.scientificToDecimal(amountEdit.numberString) > 0.0 &&
					!isNaN(feeRateEdit.numberString) && feeRateEdit.numberString.length &&
						NumberFunctions.scientificToDecimal(feeRateEdit.numberString) > 0.0 &&
					sendAmount.number > 0.0);

				buttonText.adjust(lEnabled);
				return lEnabled;
			}

			function getBackground() {
				return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.wallet.send.enabled");
			}

			function adjust() {
				enabled = ready();
			}
		}

		QuarkSymbolTap {
			id: privateButton
			x: totalInfo.x + totalInfo.width - (width + 2)
			y: totalInfo.y + amountEdit.height / 2 - height / 2
			symbol: privateSend ? Fonts.eyeSlashSym : Fonts.eyeSym
			border.color: "transparent"
			labelColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.zeroes");
			width: amountEdit.height - 1
			height: amountEdit.height - 1
			fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : 18

			color: "transparent"

			property bool privateSend: false

			onClicked: {
				//
				privateSend = !privateSend;
				sendToAddress.privateSend = privateSend;

				//
				amountInfo.prepare();
				//balanceCommand.process();
			}
		}

		//
		// History
		//

		Rectangle {
			id: historyInfo
			x: spaceLeft_
			y: totalInfo.y + totalInfo.height + spaceTop_
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
			height: walletsend_.height - (historyInfo.y + historyInfo.height + spaceItems_)
			walletModel: buzzerClient.getWalletSentLog()
			controller: walletsend_.controller
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
			if (atYBeginning) transactions.height = walletsend_.height - (historyInfo.y + historyInfo.height + spaceItems_);
			else transactions.height = walletsend_.height - (historyInfo.height + spaceTop_ + spaceTop_);
		}
	}

	//
	// Backend
	//

	//
	BuzzerCommands.BalanceCommand {
		id: balanceCommand

		onProcessed: {
			// amount, pending, scale
			availableAmount.number = amount;
			balance_ = amount;
			//
			amountInfo.recalculate();
		}

		onError: {
			controller.showError({ code: code, message: message, component: "Balance" });
		}
	}

	BuzzerCommands.SendToAddressCommand {
		id: sendToAddress
		manualProcessing: true

		onProcessed: {
			//
			if (broadcasted) {
				amountEdit.numberString = "";
				sendAmount.number = 0.0;
				privateButton.privateSend = false;
				sendToAddress.privateSend = false;
				addressBox.reset();
				balanceCommand.process();
			}

			amountInfo.recalculate();
		}

		onError: {
			if (code === "E_CHAINS_ABSENT") return;
			if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
				//buzzerClient.resync();
				controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"), true);
			} else if (code === "E_ADDRESS_IS_INVALID") {
				controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_ADDRESS_IS_INVALID"), true);
			} else if (code === "E_AMOUNT") {
				controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_AMOUNT"), true);
			} else {
				controller.showError(message, true);
			}

			sendToAddress.rollback();
			amountEdit.numberString = "";
			amountInfo.recalculate();
		}
	}

	BuzzerCommands.LoadEntityCommand {
		id: loadEntity
		onProcessed: {
			// tx, chain
			addressBox.address = loadEntity.extractAddress();
			amountInfo.prepare();
		}

		onTransactionNotFound: {
			amountInfo.prepare();
		}

		onError: {
			amountInfo.prepare();
			if (code === "E_CHAINS_ABSENT") return;
			if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
				//buzzerClient.resync();
				controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
			} else {
				controller.showError(message);
			}
		}
	}

	AddAddressDialog {
		id: addAddressDialog
		y: addressBox.y + addressBox.height + 10

		onAccepted: {
			//
			if (addAddressDialog.label !== "") {
				buzzerClient.addContact(addAddressDialog.label /*buzzer*/,
										(addressBox.address === "" ? addressBox.text : addressBox.address) /*pkey*/);
				addressBox.prepare();
				addressBox.synchronize();
				addAddressDialog.close();
			}
		}

		onRejected: {
			addAddressDialog.close();
		}
	}

	ConfirmSendDialog {
		id: confirmSendDialog
		y: addressBox.y + addressBox.height + 10

		onAccepted: {
			//
			// NOTICE: tx was created by command with manual_broadcast = true
			sendToAddress.broadcast();
		}

		onRejected: {
			confirmSendDialog.close();
		}
	}

	QuarkPopupMenu {
		id: buzzersList
		width: 170
		visible: false

		model: ListModel { id: buzzersModel }

		property var matched;

		onClick: {
			//
			addressBox.prevText_ = key;
			addressBox.text = key;
			loadEntity.process(key);
		}

		onClosed: {
			//
			if (addressBox.address === "")
				loadEntity.process(addressBox.text);
		}

		function popup(match, buzzers) {
			//
			addressBox.address = "";
			amountInfo.prepare();
			//
			if (buzzers.length === 0) return;
			if (buzzers.length === 1 && match === buzzers[0]) return;
			//
			matched = match;
			//
			buzzersModel.clear();

			for (var lIdx = 0; lIdx < buzzers.length; lIdx++) {
				buzzersModel.append({
					key: buzzers[lIdx],
					keySymbol: "",
					name: buzzers[lIdx]});
			}

			x = addressBox.x + addressBox.height;
			y = addressBox.y + addressBox.height;

			open();
		}
	}
}
