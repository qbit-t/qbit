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

	property bool setupProcess: false

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
			labelYOffset: buzzerApp.isDesktop ? 0 : 3
			symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.leftArrowSym
			x: buzzerApp.isDesktop ? 10 : 0

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
			text: buzzerClient.name
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
			font.pointSize: buzzerApp.isDesktop ? 14 : 18

			text: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbitKeys.manage")
		}

		QuarkInfoBox {
			id: addressBox
			x: 20
			y: qbitKeysText.y + qbitKeysText.height + 15
			width: parent.width - 20 * 2
			symbol: Fonts.tagSym
			clipboardButton: true
			helpButton: true

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
			font.pointSize: buzzerApp.isDesktop ? 14 : 18

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

			onTextChanged: {
			}

			onAddClicked: {
				//
				if (text !== "") {
					//
					seedView.model.append({ name: text });
					text = "";
				}
			}

			onHelpClicked: {
				var lMsgs = [];
				lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.qbitKeys.word.help"));
				infoDialog.show(lMsgs);
			}

			onHeadClicked: {
				seedView.model.clear();
			}
		}

		Rectangle {
			id: backRect
			x: 21
			y: wordEditBox.y + wordEditBox.height
			height: 250 // nameEditBox.y - y - 15
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

				contentItem: Rectangle	{
					height: 40
					width: seedView.width
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.background");

					QuarkText {
						text: name
						x: 10
						y: parent.height / 2 - height / 2
						font.pointSize: buzzerApp.isDesktop ? 14 : font.pointSize
					}
				}

				swipe.right: Rectangle {
					id: swipeRect
					width: listDelegate.height
					height: listDelegate.height
					color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.downBackground")

					QuarkSymbolLabel {
						symbol: Fonts.trashSym
						font.pointSize: 16
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

			onHelpClicked:  {
				if (enabled) {
					var lMsgs = [];
					lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.name"));
					infoDialog.show(lMsgs);
				}
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
				font.pointSize: 16
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
					// if ok - remove all & reimport
					progressBar.visible = true;
					progressBar.indeterminate = true;
					linkButton.enabled = false;

					// WARNING: remove all previous keys
					buzzerClient.removeAllKeys();

					// import
					buzzerClient.importKey(lWords);
					addressBox.text = buzzerClient.firstPKey();
					keyBox.text = buzzerClient.firstSKey();

					// try to load buzzer and check pkey
					loadBuzzerInfo.process(nameEditBox.text);
				}
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
						closePage();
					} else {
						// setup completed
						buzzerClient.setProperty("Client.configured", "true");

						// open main app
						var lComponent = null;
						var lPage = null;

						lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzer-main-desktop.qml") : Qt.createComponent("qrc:/qml/buzzer-main.qml");
						if (lComponent.status === Component.Error) {
							controller.showError(lComponent.errorString());
						} else {
							lPage = lComponent.createObject(controller);
							lPage.controller = controller;

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
				closePage();
			} else {
				// setup completed
				buzzerClient.setProperty("Client.configured", "true");

				// open main app
				var lComponent = null;
				var lPage = null;

				lComponent = buzzerApp.isDesktop ? Qt.createComponent("qrc:/qml/buzzer-main-desktop.qml") : Qt.createComponent("qrc:/qml/buzzer-main.qml");
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(controller);
					lPage.controller = controller;

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
			buzzerClient.resync();
			controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"));
		} else {
			controller.showError(message);
		}
	}
}
