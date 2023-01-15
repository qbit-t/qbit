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
	id: buzzerqbitkey_
	key: "buzzerqbitkey"
	stacked: false
	followKeyboard: true

	property bool setupProcess: false

	property var menuHighlightColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.highlight")
	property var menuBackgroundColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.background")
	property var menuForegroundColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

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

		// prepare list
		seedView.prepare();

		// pre-register buzzer info
		if (setupProcess)
			buzzerClient.preregisterBuzzerInstance();
    }

	onSetupProcessChanged: {
		// pre-register buzzer info
		if (setupProcess) {
			buzzerClient.preregisterBuzzerInstance();
			seedView.clear();
		}
	}

	onKeyboardHeightChanged: {
	}

	onHeightChanged: {
		var lOffset = pageContainer.contentHeight > buzzerqbitkey_.height ? pageContainer.contentHeight - buzzerqbitkey_.height : 0;
		if (lOffset > 0 && keyboardHeight) pageContainer.flick(0.0, (-1.0)*5000);
		console.info("[onHeightChanged]: lOffset = " + lOffset + ", keyboardHeight = " + keyboardHeight);
	}

	function closePage() {
		//
		if (!setupProcess) buzzerApp.unlockOrientation();

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
			y: parent.height / 2 - height / 2 + topOffset / 2
			symbol: Fonts.leftArrowSym //cancelSym
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
			y: parent.height / 2 - height / 2 + topOffset / 2 //
			width: parent.width - (x)
			elide: Text.ElideRight
			text: !setupProcess ? buzzerClient.name : ""
			font.pointSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 14 : 18
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
			visible: buzzerApp.isDesktop ? false : true
		}
	}

	Flickable {
		 id: pageContainer
		 x: 0
		 y: toolBar.y + toolBar.height
		 width: parent.width
		 height: parent.height - (toolBar.y + toolBar.height)
		 contentHeight: linkButton.y + linkButton.height + 15
		 clip: true

		 onDragStarted: {
		 }

		//
		// Text
		//
		QuarkLabel {
			id: qbitKeysText
			x: 20
			y: 10
			width: parent.width-40
			wrapMode: Label.Wrap
			font.pointSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 14 : 18

			text: !setupProcess ? buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbitKeys.manage") :
								  buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbitKeys.link")
		}

		QuarkInfoBox {
			id: addressBox
			x: 20
			y: qbitKeysText.y + qbitKeysText.height + 15
			width: parent.width - 20 * 2
			symbol: Fonts.tagSym
			clipboardButton: true
			helpButton: true
			textFontSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 14 : 16
			symbolFontSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 16 : 20

			text: !setupProcess ? buzzerClient.firstPKey() : ""

			onHelpClicked: {
				var lMsgs = [];
				lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbitKeys.help"));
				infoDialog.show(lMsgs);
			}
		}

		QuarkInfoBox {
			id: keyBox
			x: 20
			y: addressBox.y + addressBox.height + 10
			width: parent.width - 20 * 2
			symbol: Fonts.keySym
			clipboardButton: false
			pasteButton: false
			helpButton: true
			textFontSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 14 : 16
			symbolFontSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 16 : 20

			text: !setupProcess ? buzzerClient.firstSKey() : ""

			onHelpClicked: {
				var lMsgs = [];
				lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbitKeys.secret.help"));
				infoDialog.show(lMsgs);
			}
		}

		QuarkText {
			id: qbitSeedText
			x: 20
			y: keyBox.y + keyBox.height + 15
			width: parent.width-40
			wrapMode: Text.Wrap
			font.pointSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 14 : 18

			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbitSeed")
		}

		QuarkEditBox {
			id: wordEditBox
			x: 20
			y: qbitSeedText.y + qbitSeedText.height + 15
			width: parent.width-40
			symbol: Fonts.userSecretSym
			clipboardButton: false
			helpButton: true
			addButton: true
			placeholderText: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbitKeys.word")
			textFontSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 14 : 16
			symbolFontSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 16 : 20

			onTextChanged: {
			}

			onAddClicked: {
				//
				if (text !== "") {
					//
					var lWords = text.split(",");
					for (var lIdx = 0; lIdx < lWords.length; lIdx++) {
						seedView.model.append({ name: lWords[lIdx] });
					}

					text = "";
				}
			}

			onEditingFinished: {
				if (!buzzerApp.isDesktop && text != "") wordEditBox.forceFocus();
			}

			onHelpClicked: {
				var lMsgs = [];
				lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbitKeys.word.help"));
				infoDialog.show(lMsgs);
			}

			onHeadClicked: {
				//
				clipboard.setText(buzzerClient.firstSeedWords());
				//
				seedView.model.clear();
			}
		}

		Rectangle {
			id: backRect
			x: 21
			y: wordEditBox.y + wordEditBox.height
			height: buzzerqbitkey_.height - (y + toolBar.height + linkButton.height + nameEditBox.height + bottomOffset + 15 + 15 + 10) < 180 ?
						180 :
						buzzerqbitkey_.height - (y + toolBar.height + linkButton.height + nameEditBox.height + bottomOffset + 15 + 15 + 10)
			width: parent.width - 43
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.background");
		}

		QuarkListView {
			id: seedView
			x: 21
			y: backRect.y
			height: backRect.height
			width: backRect.width
			model: ListModel { id: model_ }
			clip: true

			Material.background: "transparent"

			delegate: SwipeDelegate
			{
				id: listDelegate
				width: seedView.width
				height: 40
				leftPadding: 0
				rightPadding: 0
				topPadding: 0
				bottomPadding: 0

				onClicked: {
					//
					swipe.open(SwipeDelegate.Right);
				}

				Binding {
					target: background
					property: "color"
					value: listDelegate.highlighted ?
							   menuHighlightColor:
							   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.background");
				}

				contentItem: Rectangle	{
					height: 40
					width: seedView.width
					color: "transparent"

					QuarkText {
						text: name
						x: 10
						y: parent.height / 2 - height / 2
						font.pointSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 14 : defaultFontPointSize
					}
				}

				swipe.right: Rectangle {
					id: swipeRect
					width: listDelegate.height
					height: listDelegate.height
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.downBackground")

					QuarkSymbolLabel {
						symbol: Fonts.trashSym
						font.pointSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 14 : 16
						color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground")

						x: parent.width / 2 - width / 2
						y: parent.height / 2 - height / 2
					}

					MouseArea {
						anchors.fill: swipeRect
						onPressed: {
							swipeRect.color = Qt.darker(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.downBackground"), 1.1);
						}
						onReleased: {
							swipeRect.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.downBackground");
						}
						onClicked: {
							listDelegate.swipe.close();
							seedView.model.remove(index);
						}
					}

					anchors.right: parent.right
				}
			}

			function prepare() {
				if (model_.count) return;

				var lWords = buzzerClient.firstSeedWords();
				for (var lIdx = 0; lIdx < lWords.length; lIdx++) {
					model_.append({ name: lWords[lIdx] });
				}
			}

			function clear() {
				model_.clear();
			}
		}

		InfoDialog {
			id: infoDialog
			bottom: 50
		}

		QuarkEditBox {
			id: nameEditBox
			x: 20
			y: seedView.y + seedView.height + 15 //linkButton.y - (height + 15)
			width: parent.width - 20 * 2
			symbol: Fonts.userTagSym
			clipboardButton: false
			helpButton: true
			placeholderText: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.placeholder.name")
			textFontSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 14 : 16
			symbolFontSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 16 : 20
			imFilter: true

			onHelpClicked:  {
				if (enabled) {
					var lMsgs = [];
					lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.name"));
					infoDialog.show(lMsgs);
				}
			}

			onEditingFinished: {
				//if (!buzzerApp.isDesktop && text != "") nameEditBox.forceFocus();
			}

			onTextChanged: {
			}
		}

		ProgressBar {
			id: progressBar

			x: 22
			y: nameEditBox.y + nameEditBox.height + 8
			width: nameEditBox.width - 5
			visible: false
			value: 0.0

			Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
			Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
			Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
			Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
			Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
		}

		QuarkButton	{
			id: linkButton
			x: qbitKeysText.x + 1
			y: nameEditBox.y + nameEditBox.height + 15 // parent.height - height - 15
			visible: true
			enabled: seedView.model.count
			width: seedView.width - 1
			Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Buzzer.trustScore.4")

			Layout.minimumWidth: 150
			Layout.alignment: Qt.AlignHCenter

			contentItem: QuarkText {
				id: buttonText
				text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.link")
				font.pointSize: buzzerApp.isDesktop ? buzzerClient.scaleFactor * 16 : 16
				color: linkButton.enabled ?
						   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground") :
						   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
				horizontalAlignment: Text.AlignHCenter
				verticalAlignment: Text.AlignVCenter

				function adjust(val) {
					color = val ?
								buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground") :
								buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
				}
			}

			onClicked: {
				// pre-ceck
				if (nameEditBox.text === "" || nameEditBox.text[0] !== '@') {
					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_NAME_INCORRECT"), true);
					return;
				}

				// prepare new key
				var lWords = [];
				for (var lIdx = 0; lIdx < model_.count; lIdx++) {
					lWords.push(model_.get(lIdx).name);
				}

				// try to import key
				if (buzzerClient.checkKey(lWords)) {
					// starting...
					progressBar.visible = true;
					progressBar.indeterminate = true;
					linkButton.enabled = false;

					// push timer
					tryLink.start();
				}
			}
		}
	}

	Timer {
		id: tryLink
		interval: 200
		repeat: false
		running: false

		onTriggered: {
			//
			console.log("[tryLink]: trying to link key...");
			//
			if (buzzerClient.buzzerDAppReady) {
				// prepare new key
				var lWords = [];
				for (var lIdx = 0; lIdx < model_.count; lIdx++) {
					lWords.push(model_.get(lIdx).name);
				}

				// import
				var lPKey = buzzerClient.importKey(lWords);
				addressBox.text = lPKey;
				keyBox.text = buzzerClient.findSKey(lPKey);

				// set current key
				buzzerClient.setCurrentKey(lPKey);

				// try to load buzzer and check pkey
				loadBuzzerInfo.process(nameEditBox.text);
			} else {
				tryLink.start();
			}
		}
	}

	BuzzerCommands.LoadBuzzerInfoCommand {
		id: loadBuzzerInfo
		loadUtxo: true

		onProcessed: {
			// check
			if (pkey === addressBox.text) {
				// state
				progressBar.indeterminate = false;
				progressBar.value = 0.3;

				// save
				buzzerClient.name = name;
				buzzerClient.alias = alias;
				buzzerClient.description = description;

				// reset wallet cache
				buzzerClient.resetWalletCache();
				// start re-sync, i.e. load owned utxos
				buzzerClient.resyncWalletCache();

				// update local buzzer info
				if (!loadBuzzerInfo.updateLocalBuzzer()) {
					//
					progressBar.visible = false;
					progressBar.indeterminate = false;
					linkButton.enabled = true;

					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_INFO_INCORRECT"), true);
					return;
				}

				// update local info
				buzzerClient.save();

				if (avatarId !== "0000000000000000000000000000000000000000000000000000000000000000") {
					avatarDownloadCommand.url = avatarUrl;
					avatarDownloadCommand.localFile = buzzerClient.getTempFilesPath() + "/" + avatarId;
					avatarDownloadCommand.process();
				} else {
					// clean-up cache
					buzzerClient.cleanUpBuzzerCache();
					// notify changes
					buzzerClient.notifyBuzzerChanged();
					//
					if (!setupProcess) {
						controller.popNonStacked();
					} else {
						// setup completed
						buzzerClient.setProperty("Client.configured", "true");

						// open main app
						var lComponent = null;
						var lPage = null;

						lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzer-main-desktop.qml") :
														   Qt.createComponent(buzzerApp.isTablet ? "qrc:/qml/buzzer-main-tablet.qml" : "qrc:/qml/buzzer-main.qml");
						if (lComponent.status === Component.Error) {
							controller.showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(controller);
							lPage.controller = controller;

							if (buzzerApp.isDesktop || buzzerApp.isTablet)
								addPageLocal(lPage);
							else
								addPage(lPage);
						}
					}
				}
			} else {
				progressBar.visible = false;
				progressBar.indeterminate = false;
				linkButton.enabled = true;

				controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_KEY_INCORRECT"), true);
			}
		}

		onError: {
			progressBar.visible = false;
			progressBar.indeterminate = false;
			linkButton.enabled = true;

			handleError(code, message);
		}
	}

	BuzzerCommands.DownloadMediaCommand {
		id: avatarDownloadCommand
		preview: false
		skipIfExists: false

		onProcessed: {
			// tx, previewFile, originalFile
			buzzerClient.avatar = originalFile;

			// update local info
			buzzerClient.save();

			// start header
			headerDownloadCommand.url = loadBuzzerInfo.headerUrl;
			headerDownloadCommand.localFile = buzzerClient.getTempFilesPath() + "/" + loadBuzzerInfo.headerId;
			headerDownloadCommand.process();
		}

		onProgress: {
			// pos, size
			var lPercent = (pos * 100.0) / size;
			progressBar.value = 0.3 + (0.3 * lPercent) / 100.0;

			if (progressBar.value >= 0.6)
				progressBar.indeterminate = true;
			else
				progressBar.indeterminate = false;
		}

		onError: {
			progressBar.visible = false;
			progressBar.indeterminate = false;
			linkButton.enabled = true;

			handleError(code, message);
		}
	}

	BuzzerCommands.DownloadMediaCommand {
		id: headerDownloadCommand
		preview: false
		skipIfExists: false

		onProcessed: {
			// tx, previewFile, originalFile
			buzzerClient.header = originalFile;

			// update local info
			buzzerClient.save();
			// clean-up cache
			buzzerClient.cleanUpBuzzerCache();
			// notify changes
			buzzerClient.notifyBuzzerChanged();

			//
			progressBar.visible = false;
			progressBar.indeterminate = false;
			linkButton.enabled = true;

			//
			if (!setupProcess) {
				controller.popNonStacked();
			} else {
				// setup completed
				buzzerClient.setProperty("Client.configured", "true");

				// open main app
				var lComponent = null;
				var lPage = null;

				lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzer-main-desktop.qml") :
												   Qt.createComponent(buzzerApp.isTablet ? "qrc:/qml/buzzer-main-tablet.qml" : "qrc:/qml/buzzer-main.qml");
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(controller);
					lPage.controller = controller;

					if (buzzerApp.isDesktop || buzzerApp.isTablet)
						addPageLocal(lPage);
					else
						addPage(lPage);
				}
			}
		}

		onProgress: {
			// pos, size
			var lPercent = (pos * 100.0) / size;
			progressBar.value = 0.6 + (0.3 * lPercent) / 100.0;

			if (progressBar.value >= 0.9)
				progressBar.indeterminate = true;
			else
				progressBar.indeterminate = false;
		}

		onError: {
			progressBar.visible = false;
			progressBar.indeterminate = false;
			linkButton.enabled = true;

			handleError(code, message);
		}
	}

	function handleError(code, message) {
		if (code === "E_CHAINS_ABSENT") return;
		if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
			//buzzerClient.resync();
			controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
		} else {
			controller.showError(message, true);
		}
	}
}
