import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Dialogs 1.3
import Qt.labs.settings 1.0
import QtGraphicalEffects 1.0
import QtMultimedia 5.8
// NOTICE: native file dialog
import QtQuick.PrivateWidgets 1.0

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
	property string action: "CREATE"; // UPDATE
	property bool buzzerCreated: false
	property var controller;

	property string buzzerName_: action !== "CREATE" ? buzzerClient.name : "";
	property string buzzerAlias_: action !== "CREATE" ? buzzerClient.alias : "";
	property string buzzerDescription_: action !== "CREATE" ? buzzerClient.description : "";
	property string buzzerAvatar_: action !== "CREATE" ? buzzerClient.avatar : "";
	property string buzzerHeader_: action !== "CREATE" ? buzzerClient.header : "";
	property string buzzerFinalName_: "";

	signal processed();

	//
	// Buzzer avatar
	//

	/*
	QuarkToolButton {
		id: rotateButton
		symbol: Fonts.rotateSym
		visible: false
		//labelYOffset: 3
		x: parent.width / 2 - avatarImage.width / 2 - (takePhotoButton.width + 10)
		y: takePhotoButton.y - (width + 5)

		topInset: 10
		leftInset: 10
		rightInset: 10
		bottomInset: 10
		padding: 20
		spacing: 10

		onClicked: {
			imageContainer.rotate();
		}
	}

	QuarkToolButton	{
		id: takePhotoButton
		symbol: Fonts.cameraSym
		visible: true
		//labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		topInset: 10
		leftInset: 10
		rightInset: 10
		bottomInset: 10
		padding: 20
		spacing: 10

		x: parent.width / 2 - avatarImage.width / 2 - (takePhotoButton.width + 10)
		y: avatarImage.y + avatarImage.displayHeight / 2 - height / 2

		onClicked: {
			makeAction();
		}

		function takeImage() {
			cancelPhotoButton.visible = false;
			rotateButton.visible = false;
			imageContainer.captureToLocation("");
		}

		function makeAction() {
			if (imageContainer.cameraState() !== Camera.ActiveState) {
				imageContainer.start();
				avatarImage.mipmap = true;
				cancelPhotoButton.visible = true;
				rotateButton.visible = true;
			} else if (imageContainer.cameraState() === Camera.ActiveState) {
				cancelPhotoButton.visible = false;
				rotateButton.visible = false;
				imageContainer.captureToLocation("");
			}
		}
	}

	QuarkToolButton {
		id: cancelPhotoButton
		x: parent.width / 2 - avatarImage.width / 2 - (takePhotoButton.width + 10)
		y: takePhotoButton.y + takePhotoButton.height + 5
		symbol: Fonts.cancelSym
		visible: true
		//labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		topInset: 10
		leftInset: 10
		rightInset: 10
		bottomInset: 10
		padding: 20
		spacing: 10

		onClicked: {
			cancelPhotoButton.visible = false;
			rotateButton.visible = false;

			if (imageContainer.cameraState() === Camera.ActiveState) {
				imageContainer.stop();
				avatarImage.mipmap = false;
				avatarImage.source = "file://" + buzzerAvatar_;
			}
		}

		Component.onCompleted: {
			cancelPhotoButton.visible = false;
			rotateButton.visible = false;
		}
	}
	*/

	Image {
		id: avatarImage

		property bool rounded: true
		property bool adapt: true
		property int displayWidth: parent.width * 65 / 100 > 300 ? 300 : parent.width * 65 / 100
		property int displayHeight: displayWidth

		autoTransform: true

		MouseArea {
			x: parent.x
			y: parent.y
			width: parent.width
			height: parent.height

			onClicked: {
				//takePhotoButton.takeImage();
			}
		}

		x: parent.width / 2 - avatarImage.width / 2
		width: avatarImage.displayWidth
		height: avatarImage.displayHeight
		fillMode: Image.PreserveAspectCrop

		source: action !== "CREATE" ? "file://" + buzzerAvatar_ : ""

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

	Rectangle {
		id: imageContainer

		x: parent.width / 2 - avatarImage.width / 2
		width: avatarImage.displayWidth
		height: avatarImage.displayHeight
		clip: true

		color: "transparent"

		property variant inner;

		function cameraState() {
			if (inner !== undefined) return inner.cameraState();
			return Camera.Unavailable;
		}
		function start() {
			// TODO: create camera
			if(inner === undefined) {
				var lComponent = Qt.createComponent("qrc:/qml/cameraview.qml");
				if (lComponent.status === Component.Error) {
					console.log("[CAMERAVIEW]: " + lComponent.errorString());
				} else {
					inner = lComponent.createObject(imageContainer);
					inner.setup(imageContainer /*cameraDevice*/);
				}
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
			buzzerAvatar_ = path;
			avatarImage.source = "file://" + buzzerAvatar_;
		}
		function rotate() {
			if (inner !== undefined) {
				inner.rotate();
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

	/*
	Connections {
		id: selectImage
		target: buzzerApp

		property var destination: "none";

		function onFileSelected(file) {
			//
			if (destination == "avatar") {
				avatarImage.mipmap = false;
				buzzerAvatar_ = file;
				avatarImage.source = "file://" + buzzerAvatar_;
				createBuzzer.enabled = createBuzzer.getEnabled();
			} else if (destination == "header") {
				buzzerHeader_ = file;
				headerImage.source = "file://" + buzzerHeader_;
				createBuzzer.enabled = createBuzzer.getEnabled();
			}

			selectImage.destination = "none";
		}
	}
	*/

	QtFileDialog {
		id: avatarFileSelector
		nameFilters: [ "Image files (*.jpg *.png *.jpeg)" ]
		selectExisting: true
		selectFolder: false
		selectMultiple: false

		onAccepted: {
			avatarImage.mipmap = false;
			avatarImage.source = fileUrl;

			var lPath = fileUrl.toString();
			if (Qt.platform.os == "windows") {
				lPath = lPath.replace(/^(file:\/{3})/,"");
			} else {
				lPath = lPath.replace(/^(file:\/{2})/,"");
			}

			buzzerAvatar_ = decodeURIComponent(lPath);

			//createBuzzer.enabled = createBuzzer.getEnabled();
		}
	}

	QuarkToolButton {
		id: chooseAvatarButton
		x: parent.width / 2 + avatarImage.width / 2 + 10
		y: avatarImage.y + avatarImage.displayHeight / 2 - height / 2
		symbol: Fonts.folderSym
		visible: true
		//labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		topInset: 10
		leftInset: 10
		rightInset: 10
		bottomInset: 10
		padding: 20
		spacing: 10

		onClicked: {
			avatarFileSelector.open();
		}
	}

	QuarkToolButton {
		id: clearAvatarButton
		x: parent.width / 2 + avatarImage.width / 2 + 10
		y: chooseAvatarButton.y + chooseAvatarButton.height + 5
		symbol: Fonts.trashSym
		visible: (buzzerAvatar_ !== "")
		//labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		topInset: 10
		leftInset: 10
		rightInset: 10
		bottomInset: 10
		padding: 20
		spacing: 10

		property var imageIndex: 1

		onClicked: {
			buzzerAvatar_ = "";
			avatarImage.source = "";
			//createBuzzer.enabled = createBuzzer.getEnabled();
		}
	}

	QuarkToolButton {
		id: nextAvatarButton
		x: parent.width / 2 + avatarImage.width / 2 - 42
		y: avatarImage.y + avatarImage.height - 42
		symbol: Fonts.rightArrowSym
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		topInset: 10
		leftInset: 10
		rightInset: 10
		bottomInset: 10

		onClicked: {
			//
			if (prevAvatarButton.imageIndex + 1 > 6) prevAvatarButton.imageIndex = 1;
			else prevAvatarButton.imageIndex += 1;
			//
			setupAvatar();
		}

		Component.onCompleted: {
			//
			if (buzzerClient.avatar === "") setupAvatar();
		}

		function setupAvatar() {
			//
			var lPath = "qrc://res/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "avatar0" + prevAvatarButton.imageIndex);
			var lNewPath = buzzerApp.tempFilesPath() + "/" + buzzerApp.getFileName(lPath);
			buzzerApp.copyFile(lPath, lNewPath);
			avatarImage.source = "file://" + lNewPath;
			buzzerAvatar_ = lNewPath;
		}
	}

	QuarkToolButton {
		id: prevAvatarButton
		x: (parent.width / 2 - avatarImage.width / 2 - (prevAvatarButton.width)) + 42
		y: avatarImage.y + avatarImage.height - 42
		symbol: Fonts.leftArrowSym
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		topInset: 10
		leftInset: 10
		rightInset: 10
		bottomInset: 10

		property var imageIndex: 5

		onClicked: {
			//
			if (prevAvatarButton.imageIndex - 1 < 1) prevAvatarButton.imageIndex = 6;
			else prevAvatarButton.imageIndex -= 1;
			//
			nextAvatarButton.setupAvatar();
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
		placeholderText: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.placeholder.name")
		text: action !== "CREATE" ? buzzerName_ : ""
		enabled: action === "CREATE"
		visible: action === "CREATE"
		textFontSize: 14

		onHelpClicked:  {
			if (enabled) {
				var lMsgs = [];
				lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.name"));
				infoDialog.show(lMsgs);
			}
		}

		onTextChanged: {
			buzzerName_ = text;
			//createBuzzer.enabled = createBuzzer.getEnabled();
		}
	}

	QuarkEditBox {
		id: aliasEditBox
		x: 20
		y: nameEditBox.visible ? nameEditBox.y + nameEditBox.height + 10 :
								 avatarImage.y + avatarImage.displayHeight + 20
		width: parent.width - 20 * 2
		symbol: Fonts.userAliasSym
		clipboardButton: false
		helpButton: true
		placeholderText: buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.placeholder.alias")
		text: action !== "CREATE" ? buzzerAlias_ : ""
		textFontSize: 14

		onHelpClicked: {
			var lMsgs = [];
			lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.alias"));
			infoDialog.show(lMsgs);
		}

		onTextChanged: {
			buzzerAlias_ = text;
			//createBuzzer.enabled = createBuzzer.getEnabled();
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
		text: action !== "CREATE" ? buzzerDescription_ : ""
		textFontSize: 14

		onHelpClicked: {
			var lMsgs = [];
			lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.description"));
			infoDialog.show(lMsgs);
		}

		onTextChanged: {
			buzzerDescription_ = text;
			//createBuzzer.enabled = createBuzzer.getEnabled();
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
			source: action !== "CREATE" ? "file://" + buzzerHeader_ : ""
		}
	}

	QtFileDialog {
		id: headerFileSelector
		nameFilters: [ "Image files (*.jpg *.png *.jpeg)" ]
		selectExisting: true
		selectFolder: false
		selectMultiple: false

		onAccepted: {
			headerImage.source = fileUrl;

			var lPath = fileUrl.toString();
			if (Qt.platform.os == "windows") {
				lPath = lPath.replace(/^(file:\/{3})/,"");
			} else {
				lPath = lPath.replace(/^(file:\/{2})/,"");
			}

			buzzerHeader_ = decodeURIComponent(lPath);

			//createBuzzer.enabled = createBuzzer.getEnabled();
		}
	}

	QuarkToolButton {
		id: prevHeaderButton
		x: headerContainer.x + 5
		y: headerContainer.y + 5
		symbol: Fonts.leftArrowSym
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		topInset: 10
		leftInset: 10
		rightInset: 10
		bottomInset: 10
		padding: 20
		spacing: 10

		property var imageIndex: 5

		onClicked: {
			//
			if (prevHeaderButton.imageIndex - 1 < 1) prevHeaderButton.imageIndex = 6;
			else prevHeaderButton.imageIndex -= 1;
			//
			nextHeaderButton.setupHeader();
		}
	}

	QuarkToolButton {
		id: nextHeaderButton
		x: prevHeaderButton.x + prevHeaderButton.width + 5
		y: headerContainer.y + 5
		symbol: Fonts.rightArrowSym
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		topInset: 10
		leftInset: 10
		rightInset: 10
		bottomInset: 10
		padding: 20
		spacing: 10

		onClicked: {
			//
			if (prevHeaderButton.imageIndex + 1 > 6) prevHeaderButton.imageIndex = 1;
			else prevHeaderButton.imageIndex += 1;
			//
			setupHeader();
		}

		Component.onCompleted: {
			//
			if (buzzerClient.header === "") setupHeader();
		}

		function setupHeader() {
			//
			var lPath = "qrc://res/" + buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "header0" + prevHeaderButton.imageIndex);
			var lNewPath = buzzerApp.tempFilesPath() + "/" + buzzerApp.getFileName(lPath);
			buzzerApp.copyFile(lPath, lNewPath);
			headerImage.source = "file://" + lNewPath;
			buzzerHeader_ = lNewPath;
		}
	}

	QuarkToolButton {
		id: chooseHeaderButton
		x: headerContainer.x + headerContainer.width - width - 5
		y: headerContainer.y + 5
		symbol: Fonts.folderSym
		visible: true
		//labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		topInset: 10
		leftInset: 10
		rightInset: 10
		bottomInset: 10
		padding: 20
		spacing: 10

		onClicked: {
			headerFileSelector.open();
		}
	}

	QuarkToolButton {
		id: clearHeaderButton
		x: chooseHeaderButton.x
		y: chooseHeaderButton.y + chooseHeaderButton.height + 5
		symbol: Fonts.trashSym
		visible: true
		//labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		topInset: 10
		leftInset: 10
		rightInset: 10
		bottomInset: 10
		padding: 20
		spacing: 10

		onClicked: {
			buzzerHeader_ = "";
			headerImage.source = "";
			//createBuzzer.enabled = createBuzzer.getEnabled();
		}
	}

	QuarkToolButton {
		id: helpHeaderButton
		x: clearHeaderButton.x
		y: clearHeaderButton.y + clearHeaderButton.height + 5
		symbol: Fonts.helpSym
		visible: true
		//labelYOffset: 3
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter

		topInset: 10
		leftInset: 10
		rightInset: 10
		bottomInset: 10
		padding: 20
		spacing: 10

		onClicked: {
			var lMsgs = [];
			lMsgs.push(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.help.header"));
			infoDialog.show(lMsgs);
		}
	}

	Connections {
		target: buzzerClient

		function onBuzzerDAppReadyChanged() {
			createBuzzer.enabled = buzzerClient.buzzerDAppReady;
		}

		function onBuzzerDAppSuspended() {
			createBuzzer.enabled = false;
		}
	}

	//
	// Create/update buzzer/buzzer info
	//
	QuarkToolButton {
		id: createBuzzer
		x: parent.width / 2 - width / 2
		y: headerContainer.y + headerContainer.height + 15
		width: parent.width * 45 / 100 > 200 ? 200 : parent.width * 45 / 100
		height: width
		visible: true
		labelYOffset: height / 2 //- metrics.tightBoundingRect.height
		symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
		Layout.alignment: Qt.AlignHCenter
		radius: width / 2 //- 10

		enabled: true

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
			if (buzzerinfo_.action === "CREATE" && !buzzerinfo_.buzzerCreated) {
				//
				if (buzzerName_ === "" /*|| buzzerName_[0] !== '@'*/) {
					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_NAME_INCORRECT"), true);
					return;
				}

				if (buzzerAlias_ === "") {
					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_ALIAS_INCORRECT"), true);
					return;
				}

				if (buzzerDescription_ === "") {
					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_DESCRIPTION_INCORRECT"), true);
					return;
				}

				if (buzzerAvatar_ === "") {
					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_AVATAR_ABSENT"), true);
					return;
				}

				if (buzzerHeader_ === "") {
					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_HEADER_ABSENT"), true);
					return;
				}

				//
				waitTimer.start();

				var lBuzzerName = buzzerName_.trim();
				if (lBuzzerName[0] !== '@') buzzerFinalName_ = "@" + lBuzzerName;
				else buzzerFinalName_ = lBuzzerName;

				console.info("[createBuzzer]: creating buzzer = '" + buzzerFinalName_ + "'...");

				createBuzzerCommand.name = buzzerFinalName_;
				createBuzzerCommand.alias = buzzerAlias_;
				createBuzzerCommand.description = buzzerDescription_;
				createBuzzerCommand.avatar = buzzerAvatar_;
				createBuzzerCommand.header = buzzerHeader_;

				createBuzzerCommand.process();
			} else {
				//
				if (buzzerAlias_ === "") {
					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_ALIAS_INCORRECT"), true);
					return;
				}

				if (buzzerDescription_ === "") {
					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_DESCRIPTION_INCORRECT"), true);
					return;
				}

				if (buzzerAvatar_ === "") {
					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_AVATAR_ABSENT"), true);
					return;
				}

				if (buzzerHeader_ === "") {
					controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_HEADER_ABSENT"), true);
					return;
				}

				//
				waitTimer.start();

				createBuzzerInfoCommand.alias = buzzerAlias_;
				createBuzzerInfoCommand.description = buzzerDescription_;
				createBuzzerInfoCommand.avatar = buzzerAvatar_;
				createBuzzerInfoCommand.header = buzzerHeader_;

				createBuzzerInfoCommand.process();
			}

			enabled = false;
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

			console.info("[createBuzzerCommand]: buzzer = '" + buzzerFinalName_ + "' created, saving...");

			buzzerClient.name = buzzerFinalName_;
			buzzerClient.alias = buzzerAlias_;
			buzzerClient.description = buzzerDescription_;
			buzzerClient.avatar = buzzerAvatar_;
			buzzerClient.header = buzzerHeader_;

			// update local info
			buzzerClient.save();

			// processed
			buzzerinfo_.processed();

			// setup completed
			buzzerClient.setProperty("Client.configured", "true");
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
			createBuzzer.enabled = true;

			waitTimer.stop();
			progressBar.arcEnd = 0;

			// insufficient amount
			if (code === "E_AMOUNT") {
				// clean-up and refill cache
				buzzerClient.cleanUpBuzzerCache();
				// check info and try again
				controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_AMOUNT_INSTALL"), true);
			} else if (code === "E_BUZZER_EXISTS") {
				//
				controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZER_EXISTS"), true);
			} else if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
				// NOTICE: probably buzzer is sucessfully was created, so just try to create info
				buzzerinfo_.buzzerCreated = true;
				buzzerClient.name = buzzerName_;
				//
				buzzerClient.resync();
				controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"), true);
			} else {
				// NOTICE: probably buzzer is sucessfully was created, so just try to create info
				buzzerinfo_.buzzerCreated = true;
				buzzerClient.name = buzzerName_;
				//
				controller.showError(message, true);
			}
		}
	}

	BuzzerCommands.CreateBuzzerInfoCommand {
		//
		id: createBuzzerInfoCommand

		onProcessed: {
			// buzzer, buzzerChain, buzzerInfo, buzzerInfoChain
			waitTimer.stop();
			progressBar.arcEnd = 360;

			buzzerStartImage.visible = false;
			buzzerStopImage.visible = true;

			buzzerClient.alias = buzzerAlias_;
			buzzerClient.description = buzzerDescription_;
			buzzerClient.avatar = buzzerAvatar_;
			buzzerClient.header = buzzerHeader_;

			// update local info
			buzzerClient.save();

			// processed
			buzzerinfo_.processed();
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
			createBuzzer.enabled = true;

			waitTimer.stop();
			progressBar.arcEnd = 0;

			// insufficient amount
			if (code === "E_AMOUNT") {
				// clean-up and refill cache
				buzzerClient.cleanUpBuzzerCache();
				// check info and try again
				controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_AMOUNT_INSTALL"), true);
			} else if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
				//
				buzzerClient.resync();
				controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"), true);
			} else {
				//
				controller.showError(message, true);
			}
		}
	}
}
