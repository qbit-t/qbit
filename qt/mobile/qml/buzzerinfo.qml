﻿import QtQuick 2.9
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

Item
{
	id: buzzerinfo_

	property var infoDialog;
	property int calculatedHeight: progressBar.y + progressBar.height + 15;
	property string action: "CREATE";
	property var controller;

	signal processed();

	//
	// Buzzer avatar
	//

	QuarkToolButton {
		id: rotateButton
		symbol: Fonts.rotateSym
		visible: false
		labelYOffset: 3
		x: parent.width / 2 - avatarImage.width / 2 - (takePhotoButton.width + 10)
		y: takePhotoButton.y - (width + 5)

		onClicked: {
			camera.rotate();
		}
	}

	QuarkToolButton	{
		id: takePhotoButton
		symbol: Fonts.cameraSym
		visible: true
		labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		x: parent.width / 2 - avatarImage.width / 2 - (takePhotoButton.width + 10)
		y: avatarImage.y + avatarImage.displayHeight / 2 - height / 2

		onClicked: {
			makeAction();
		}

		function takeImage() {
			cancelPhotoButton.visible = false;
			rotateButton.visible = false;
			camera.captureToLocation("");
		}

		function makeAction() {
			if (camera.cameraState() !== Camera.ActiveState)
			{
				camera.start();
				avatarImage.mipmap = true;
				cancelPhotoButton.visible = true;
				rotateButton.visible = true;
			} else if (camera.cameraState() === Camera.ActiveState) {
				cancelPhotoButton.visible = false;
				rotateButton.visible = false;
				camera.captureToLocation("");
			}
		}
	}

	QuarkToolButton {
		id: cancelPhotoButton
		x: parent.width / 2 - avatarImage.width / 2 - (takePhotoButton.width + 10)
		y: takePhotoButton.y + takePhotoButton.height + 5
		symbol: Fonts.cancelSym
		visible: true
		labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		onClicked: {
			cancelPhotoButton.visible = false;
			rotateButton.visible = false;

			if (camera.cameraState() === Camera.ActiveState) {
				camera.stop();
				avatarImage.mipmap = false;
				avatarImage.source = "file://" + buzzerClient.avatar;
			}
		}

		Component.onCompleted: {
			cancelPhotoButton.visible = false;
			rotateButton.visible = false;
		}
	}

	Image {
		id: avatarImage

		property bool rounded: true
		property bool adapt: true
		property int displayWidth: parent.width * 65 / 100
		property int displayHeight: displayWidth

		autoTransform: true

		Rectangle {
			id: imageContainer
			x: 0
			y: 0
			width: avatarImage.displayWidth // 256
			height: avatarImage.displayHeight // 256

			color: "transparent"

			// camera proxy
			Item {
				id: camera

				property variant inner;

				function cameraState() {
					if (inner !== undefined) return inner.cameraState();
					return Camera.Unavailable;
				}
				function start() {
					// TODO: create camera
					if(inner === undefined) {
						var lComponent = Qt.createComponent("qrc:/qml/cameraview.qml");
						inner = lComponent.createObject(imageContainer);
						inner.setup(camera);
					}

					if (inner !== undefined) {
						inner.start();
					}
				}
				function stop() {
					if (inner !== undefined) {
						inner.stop();
					}
				}
				function captureToLocation(location) {
					if (inner !== undefined) {
						inner.captureToLocation(location);
					}
				}
				function applyPicture(path) {
					avatarImage.mipmap = false;
					buzzerClient.avatar = path;
					avatarImage.source = "file://" + buzzerClient.avatar;
				}
				function rotate() {
					if (inner !== undefined) {
						inner.rotate();
					}
				}
			}
		}

		MouseArea {
			x: parent.x
			y: parent.y
			width: parent.width
			height: parent.height

			onClicked: {
				takePhotoButton.takeImage();
			}
		}

		x: parent.width / 2 - avatarImage.width / 2
		width: avatarImage.displayWidth
		height: avatarImage.displayHeight
		fillMode: Image.PreserveAspectCrop

		source: "file://" + buzzerClient.avatar

		layer.enabled: rounded
		layer.effect: OpacityMask {
			maskSource: Item {
				width: avatarImage.displayWidth
				height: avatarImage.displayHeight

				Rectangle {
					anchors.centerIn: parent
					width: avatarImage.displayWidth
					height: avatarImage.displayHeight
					radius: avatarImage.displayWidth
				}
			}
		}
	}

	QuarkRoundProgress {
		id: imageFrame
		x: avatarImage.x - 10
		y: avatarImage.y - 10
		size: avatarImage.displayWidth + 20
		colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
		colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
		arcBegin: 0
		arcEnd: 360
		lineWidth: 3
	}

	BuzzerComponents.ImageListing {
		id: imageListing

		onImageFound:  {
			avatarImage.mipmap = false;
			buzzerClient.avatar = file;
			avatarImage.source = "file://" + buzzerClient.avatar;
		}
	}

	Connections {
		id: selectImage
		target: buzzerApp

		property var destination: "none";

		function onFileSelected(file) {
			//
			if (destination == "avatar") {
				avatarImage.mipmap = false;
				buzzerClient.avatar = file;
				avatarImage.source = "file://" + buzzerClient.avatar;
				createBuzzer.enabled = createBuzzer.getEnabled();
			} else if (destination == "header") {
				buzzerClient.header = file;
				headerImage.source = "file://" + buzzerClient.header;
				createBuzzer.enabled = createBuzzer.getEnabled();
			}

			selectImage.destination = "none";
		}
	}

	QuarkToolButton {
		id: chooseAvatarButton
		x: parent.width / 2 + avatarImage.width / 2 + 10
		y: avatarImage.y + avatarImage.displayHeight / 2 - height / 2
		symbol: Fonts.folderSym
		visible: true
		labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		onClicked: {
			//imageListing.listImages();
			selectImage.destination = "avatar";
			buzzerApp.pickImageFromGallery();
		}
	}

	QuarkToolButton {
		id: clearAvatarButton
		x: parent.width / 2 + avatarImage.width / 2 + 10
		y: takePhotoButton.y + takePhotoButton.height + 5
		symbol: Fonts.trashSym
		visible: (buzzerClient.avatar !== "")
		labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		onClicked: {
			buzzerClient.avatar = "";
			avatarImage.source = "";
			createBuzzer.enabled = createBuzzer.getEnabled();
		}
	}

	//
	// Buzzer data
	//
	QuarkEditBox {
		id: nameEditBox
		x: 20
		y: avatarImage.y + avatarImage.displayHeight + 20
		width: parent.width - 20 * 2
		symbol: Fonts.userTagSym
		clipboardButton: false
		helpButton: true
		placeholderText: "@" + buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.placeholder.name")
		text: buzzerClient.name

		onHelpClicked:  {
			var lMsgs = [];
			lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.name"));
			infoDialog.show(lMsgs);
		}

		onTextChanged: {
			buzzerClient.name = text;
			createBuzzer.enabled = createBuzzer.getEnabled();
		}
	}

	QuarkEditBox {
		id: aliasEditBox
		x: 20
		y: nameEditBox.y + nameEditBox.height + 10
		width: parent.width - 20 * 2
		symbol: Fonts.userAliasSym
		clipboardButton: false
		helpButton: true
		placeholderText: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.placeholder.alias")
		text: buzzerClient.alias

		onHelpClicked: {
			var lMsgs = [];
			lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.alias"));
			infoDialog.show(lMsgs);
		}

		onTextChanged: {
			buzzerClient.alias = text;
			createBuzzer.enabled = createBuzzer.getEnabled();
		}
	}

	QuarkEditBox {
		id: descriptionEditBox
		x: 20
		y: aliasEditBox.y + aliasEditBox.height + 10
		width: parent.width - 20 * 2
		symbol: Fonts.featherSym
		clipboardButton: false
		helpButton: true
		placeholderText: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.placeholder.description")
		text: buzzerClient.description

		onHelpClicked: {
			var lMsgs = [];
			lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.description"));
			infoDialog.show(lMsgs);
		}

		onTextChanged: {
			buzzerClient.description = text;
			createBuzzer.enabled = createBuzzer.getEnabled();
		}
	}

	//
	// Buzzer header
	//
	Rectangle {
		id: headerContainer
		x: 22
		y: descriptionEditBox.y + descriptionEditBox.height + 12
		width: parent.width - 44
		height: (width / 16) * 9

		border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
		border.width: 3
		color: "transparent"

		Image {
			id: headerImage
			x: 10
			y: 10
			width: parent.width - 20
			height: parent.height - 20
			fillMode: Image.PreserveAspectCrop
			autoTransform: true
			source: "file://" + buzzerClient.header
		}
	}

	BuzzerComponents.ImageListing {
		id: headerListing

		onImageFound: {
			buzzerClient.header = file;
			headerImage.source = "file://" + buzzerClient.header;
		}
	}

	QuarkToolButton {
		id: chooseHeaderButton
		x: headerContainer.x + headerContainer.width - width - 5
		y: headerContainer.y + 5
		symbol: Fonts.folderSym
		visible: true
		labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		onClicked: {
			//headerListing.listImages();
			selectImage.destination = "header";
			buzzerApp.pickImageFromGallery();
		}
	}

	QuarkToolButton {
		id: clearHeaderButton
		x: chooseHeaderButton.x
		y: chooseHeaderButton.y + chooseHeaderButton.height + 5
		symbol: Fonts.trashSym
		visible: true
		labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		onClicked: {
			buzzerClient.header = "";
			headerImage.source = "";
			createBuzzer.enabled = createBuzzer.getEnabled();
		}
	}

	QuarkToolButton {
		id: helpHeaderButton
		x: clearHeaderButton.x
		y: clearHeaderButton.y + clearHeaderButton.height + 5
		symbol: Fonts.helpSym
		visible: true
		labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		onClicked: {
			var lMsgs = [];
			lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.header"));
			infoDialog.show(lMsgs);
		}
	}

	//
	// Create/update buzzer/buzzer info
	//
	QuarkToolButton {
		id: createBuzzer
		x: parent.width / 2 - width / 2
		y: headerContainer.y + headerContainer.height + 15
		width: parent.width * 45 / 100
		height: width
		visible: true
		labelYOffset: height / 2 - metrics.tightBoundingRect.height
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter
		radius: width / 2 //- 10

		enabled: getEnabled()

		Image {
			id: buzzerStartImage
			x: parent.width / 2 - width / 2 + 3
			y: parent.height / 2 - height / 2 + 5
			source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "lightning-light.logo")
		}

		Image {
			id: buzzerStopImage
			x: parent.width / 2 - width / 2 + 3
			y: parent.height / 2 - height / 2 + 5
			source: "../images/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "lightning.logo")
			visible: false
		}

		onClicked: {
			// create buzzer
			if (buzzerinfo_.action === "CREATE") {
				//
				if (buzzerClient.name === "" || buzzerClient.name[0] !== '@') {
					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_NAME_INCORRECT"), true);
					return;
				}

				if (buzzerClient.alias === "") {
					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_ALIAS_INCORREC"), true);
					return;
				}

				if (buzzerClient.avatar === "") {
					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_AVATAR_ABSENT"), true);
					return;
				}

				if (buzzerClient.header === "") {
					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_HEADER_ABSENT"), true);
					return;
				}

				//
				waitTimer.start();

				createBuzzerCommand.name = buzzerClient.name;
				createBuzzerCommand.alias = buzzerClient.alias;
				createBuzzerCommand.description = buzzerClient.description;
				createBuzzerCommand.avatar = buzzerClient.avatar;
				createBuzzerCommand.header = buzzerClient.header;

				createBuzzerCommand.process();
			} else {
				// create new buzzer info
			}

			enabled = false;
		}

		function getEnabled() {
			return true; //buzzerClient.avatar !== "" && buzzerClient.header !== "" && buzzerClient.name !== "" && buzzerClient.alias !== "";
		}
	}

	QuarkRoundProgress {
		id: progressBar
		x: parent.width / 2 - (createBuzzer.width + 10) / 2
		y: createBuzzer.y - (size - createBuzzer.height) / 2
		size: createBuzzer.width + 10
		colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
		colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
		arcBegin: 0
		arcEnd: 0
		lineWidth: 3

		Timer {
			id: waitTimer
			running: false
			repeat: true
			interval: 100

			onTriggered: {
				progressBar.arcEnd += 1;
				if (progressBar.arcEnd >= 360) {
					running = false;
				}
			}
		}
	}

	//
	// Commands
	//

	BuzzerCommands.CreateBuzzerCommand {
		//
		id: createBuzzerCommand

		onProcessed: {
			// buzzer, buzzerChain, buzzerInfo, buzzerInfoChain
			waitTimer.stop();
			progressBar.arcEnd = 360;

			buzzerStartImage.visible = false;
			buzzerStopImage.visible = true;

			// update local info
			buzzerClient.save();

			// processed
			buzzerinfo_.processed();

			// setup completed
			buzzerClient.setProperty("Client.configured", "true");

			console.log("[buzzer]: buzzer = " + buzzer + ", chain = " + buzzerChain + ", info = " + buzzerInfo + ", info_chain = " + buzzerInfoChain);
		}

		onAvatarProcessed: {
			// tx, chain
			if (progressBar.arcEnd < 120) progressBar.arcEnd = 120;

			console.log("[avatar]: tx = " + tx + ", chain = " + chain);
		}

		onHeaderProcessed: {
			// tx, chain
			if (progressBar.arcEnd < 240) progressBar.arcEnd = 240;

			console.log("[header]: tx = " + tx + ", chain = " + chain);
		}

		onAvatarProgress: {
			// pos, size
			console.log("[avatar/progress]: pos = " + pos + ", size = " + size);

			var lPrcent = (pos * 100) / size;
			var lArc = (120 * lPrcent) / 100;
			progressBar.arcEnd = lArc;
		}

		onHeaderProgress: {
			// pos, size
			console.log("[header/progress]: pos = " + pos + ", size = " + size);

			var lPrcent = (pos * 100) / size;
			var lArc = 120 + (120 * lPrcent) / 100;
			progressBar.arcEnd = lArc;
		}

		onError: {
			console.log("[error]: code = " + code + ", message = " + message);
			//
			createBuzzer.enabled = createBuzzer.getEnabled();

			waitTimer.stop();
			progressBar.arcEnd = 0;

			//
			controller.showError(message, true);
		}
	}
}