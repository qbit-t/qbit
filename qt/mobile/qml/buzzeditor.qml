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
	id: buzzeditor_
	key: "buzzeditor"

	Component.onCompleted: {
		closePageHandler = closePage;
		activatePageHandler = activatePage;

		startUp.start();
		buzzText.forceActiveFocus();
	}

	function closePage() {
		stopPage();
		controller.popPage();
		destroy(1000);
	}

	function activatePage() {
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background"));
	}

	function onErrorCallback(error)	{
		controller.showError(error);
	}

	function stopPage() {
	}

	property bool buzz_: true;
	property bool rebuzz_: false;
	property bool reply_: false;
	property var buzzfeedModel_;
	property var buzzItem_;

	function initializeRebuzz(item, model) {
		buzzItem_ = item;
		buzzfeedModel_ = model;
		buzz_ = false;
		rebuzz_ = true;

		bodyContainer.wrapItem(item);
	}

	function initializeReply(item, model, text) {
		buzzItem_ = item;
		buzzfeedModel_ = model;
		buzz_ = false;
		reply_ = true;

		if (text) buzzText.text = text;

		bodyContainer.replyItem(item);
	}

	function initializeBuzz(text) {
		buzz_ = true;
		reply_ = false;
		rebuzz_ = false;

		if (text) buzzText.text = text;
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

	// only once to pop-up system keyboard
	Timer {
		id: startUp
		interval: 300
		repeat: false
		running: false

		onTriggered: {
			buzzText.forceActiveFocus();
		}
	}

	//
	// toolbar
	//
	QuarkToolBar {
		id: buzzEditorToolBar
		height: 45
		width: parent.width

		property int totalHeight: height

		Component.onCompleted: {
		}

		QuarkToolButton	{
			id: cancelButton
			Material.background: "transparent"
			visible: true
			labelYOffset: 3
			symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.cancelSym

			onClicked: {
				controller.popPage();
			}
		}

		QuarkRoundButton {
			id: sendButton
			x: parent.width - width - 12
			y: topOffset + parent.height / 2 - height / 2
			text: buzz_ ? buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.buzz") :
						  rebuzz_ ? buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.rebuzz") :
									buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.reply")

			textColor: !sending ?
						 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground") :
						 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")

			onClick: {
				if (!sending) {
					sending = true;

					buzzerApp.commitCurrentInput();

					if (rebuzz_) buzzeditor_.createRebuzz(buzzItem_.buzzId);
					else if (reply_) buzzeditor_.createReply(buzzItem_.buzzId);
					else if (buzz_) buzzeditor_.createBuzz();
				}
			}
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

	property bool sending: false

	//
	// body
	//
	Flickable {
		id: bodyContainer
		x: 0
		y: buzzEditorToolBar.y + buzzEditorToolBar.height
		width: parent.width
		height: parent.height - (buzzEditorToolBar.height + buzzFooterBar.height)
		contentHeight: buzzText.contentHeight + (mediaList.model.count ? mediaBox.height : 0) + spaceMedia_ + spaceBottom_ * 2 +
							wrapContainer.getHeight() + wrapContainer.getSpace() +
							replyContainer.getHeight() + replyContainer.getSpace()
		clip: true

		onHeightChanged: {
			bodyContainer.ensureVisible(buzzText);
		}

		function ensureVisible(item) {
			//
			//console.log("item.y = " + item.y + ", item.contentHeight = " + item.contentHeight + ", spaceMedia_ = " + spaceMedia_ +
			//			", contentY = " + contentY + ", height = " + height);
			if (height > 0 && item.y + item.contentHeight + spaceMedia_ > contentY + height) {
				//contentY += (item.y + item.contentHeight + spaceMedia_) - (contentY + height);

				//console.log("delta.y = " + ((item.y + item.contentHeight + spaceMedia_) - (contentY + height)));
				contentY += (item.y + item.contentHeight + spaceMedia_) - (contentY + height);

				//ensureVisibleAnimation_.from = contentY;
				//ensureVisibleAnimation_.to = contentY + (item.y + item.contentHeight + spaceMedia_) - (contentY + height);
				//ensureVisibleAnimation_.start();
			}
		}

		/*
		NumberAnimation on contentY {
			id: ensureVisibleAnimation_
			to: 0
			duration: 200
			easing.type: Easing.OutQuad
		}
		*/

		BuzzerComponents.BuzzTextHighlighter {
			id: highlighter
			textDocument: buzzText.textDocument

			onMatched: {
				// start, length, match
				if (match.length) {
					var lPosition = buzzText.cursorPosition;
					if (/*(lPosition === start || lPosition === start + length) &&*/ buzzText.preeditText != " ") {
						if (match[0] === '@')
							searchBuzzers.process(match);
						else if (match[0] === '#')
							searchTags.process(match);
					}
				}
			}
		}

		BuzzerCommands.SearchEntityNamesCommand {
			id: searchBuzzers

			onProcessed: {
				// pattern, entities
				var lRect = buzzText.positionToRectangle(buzzText.cursorPosition);
				buzzersList.popup(pattern, lRect.x, lRect.y + lRect.height, entities);
			}

			onError: {
				controller.showError(message);
			}
		}

		BuzzerCommands.LoadHashTagsCommand {
			id: searchTags

			onProcessed: {
				// pattern, tags
				var lRect = buzzText.positionToRectangle(buzzText.cursorPosition);
				tagsList.popup(pattern, lRect.x, lRect.y + lRect.height, tags);
			}

			onError: {
				controller.showError(message);
			}
		}

		Rectangle {
			id: replyContainer
			x: spaceLeft_
			y: spaceTop_
			width: bodyContainer.width - (spaceLeft_ + spaceRight_)
			height: getHeight()
			color: "transparent"

			property var replyItem_;

			function getHeight() {
				if (replyItem_) {
					return replyItem_.calculatedHeight;
				}

				return 0;
			}

			function getSpace() {
				if (replyItem_) return spaceMedia_;
				return 0;
			}
		}

		QuarkTextEdit {
			id: buzzText
			x: spaceLeft_
			y: replyContainer.y + replyContainer.getHeight() + replyContainer.getSpace()
			height: 1000 //parent.height
			width: parent.width - spaceRight_
			wrapMode: Text.Wrap
			textFormat: Text.RichText
			focus: true
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

			onLengthChanged: {
				countProgress.adjust(length + preeditText.length);
				buzzersList.close();
				tagsList.close();
			}

			onPreeditTextChanged: {
				buzzersList.close();
				tagsList.close();
			}

			onContentHeightChanged: {
				bodyContainer.ensureVisible(buzzText);
			}

			onYChanged: {
				bodyContainer.ensureVisible(buzzText);
			}
		}

		onWidthChanged: {
			//
			if (replyContainer.replyItem_) {
				replyContainer.replyItem_.width = bodyContainer.width - (spaceLeft_ + spaceRight_);
			}
			//
			if (wrapContainer.wrappedItem_) {
				wrapContainer.wrappedItem_.width = bodyContainer.width - (spaceLeft_ + spaceRight_);
			}
		}

		onFlickingChanged: {
		}

		Rectangle {
			id: wrapContainer
			x: spaceLeft_
			y: buzzText.y + buzzText.contentHeight + spaceMedia_
			width: bodyContainer.width - (spaceLeft_ + spaceRight_)
			height: getHeight()
			color: "transparent"

			property var wrappedItem_;

			function getHeight() {
				if (wrappedItem_) {
					return wrappedItem_.calculatedHeight;
				}

				return 0;
			}

			function getSpace() {
				if (wrappedItem_) return spaceMedia_;
				return 0;
			}
		}

		function wrapItem(item) {
			// buzzText
			var lSource = "qrc:/qml/buzzitemlight.qml";
			var lComponent = Qt.createComponent(lSource);
			wrapContainer.wrappedItem_ = lComponent.createObject(wrapContainer);

			wrapContainer.wrappedItem_.x = 0;
			wrapContainer.wrappedItem_.y = 0;
			wrapContainer.wrappedItem_.width = bodyContainer.width - (spaceLeft_ + spaceRight_);
			wrapContainer.wrappedItem_.controller_ = controller;

			wrapContainer.wrappedItem_.timestamp_ = item.timestamp;
			wrapContainer.wrappedItem_.score_ = item.score;
			wrapContainer.wrappedItem_.buzzId_ = item.buzzId;
			wrapContainer.wrappedItem_.buzzChainId_ = item.buzzChainId;
			wrapContainer.wrappedItem_.buzzerId_ = item.buzzerId;
			wrapContainer.wrappedItem_.buzzerInfoId_ = item.buzzerInfoId;
			wrapContainer.wrappedItem_.buzzerInfoChainId_ = item.buzzerInfoChainId;
			wrapContainer.wrappedItem_.buzzBody_ = buzzerClient.decorateBuzzBody(item.buzzBody);
			wrapContainer.wrappedItem_.buzzMedia_ = item.buzzMedia;
			wrapContainer.wrappedItem_.ago_ = buzzerClient.timestampAgo(item.timestamp);
		}

		function replyItem(item) {
			// buzzText
			var lSource = "qrc:/qml/buzzitemlight.qml";
			var lComponent = Qt.createComponent(lSource);
			replyContainer.replyItem_ = lComponent.createObject(replyContainer);
			replyContainer.replyItem_.calculatedHeightModified.connect(innerHeightChanged);

			replyContainer.replyItem_.x = 0;
			replyContainer.replyItem_.y = 0;
			replyContainer.replyItem_.width = bodyContainer.width - (spaceLeft_ + spaceRight_);
			replyContainer.replyItem_.controller_ = controller;

			replyContainer.replyItem_.timestamp_ = item.timestamp;
			replyContainer.replyItem_.score_ = item.score;
			replyContainer.replyItem_.buzzId_ = item.buzzId;
			replyContainer.replyItem_.buzzChainId_ = item.buzzChainId;
			replyContainer.replyItem_.buzzerId_ = item.buzzerId;
			replyContainer.replyItem_.buzzerInfoId_ = item.buzzerInfoId;
			replyContainer.replyItem_.buzzerInfoChainId_ = item.buzzerInfoChainId;
			replyContainer.replyItem_.buzzBody_ = buzzerClient.decorateBuzzBody(item.buzzBody);
			replyContainer.replyItem_.buzzMedia_ = item.buzzMedia;
			replyContainer.replyItem_.ago_ = buzzerClient.timestampAgo(item.timestamp);

			bodyContainer.ensureVisible(buzzText);
		}

		function innerHeightChanged(value) {
			//console.log("replyItem_.height = " + value);
			bodyContainer.ensureVisible(buzzText);
		}

		//
		// media
		//
		Rectangle {
			id: mediaBox
			color: "transparent"
			x: spaceLeft_ - spaceItems_
			y: getY()
			width: parent.width - (spaceLeft_ + spaceRight_ - (spaceItems_) * 2)
			height: Math.max(mediaList.height, calculatedHeight)
			visible: true

			function getY() {
				var lPos = wrapContainer.getHeight();
				if (lPos) { lPos += wrapContainer.y; lPos += wrapContainer.getSpace(); }

				if (buzzText.contentHeight > 0) {
					return lPos ? lPos : (buzzText.y + buzzText.contentHeight + spaceMedia_);
				}

				return lPos ? lPos : y;
			}

			property var calculatedHeight: 400

			QuarkListView {
				id: mediaList
				x: 0
				y: 0
				width: parent.width
				height: parent.height
				clip: true
				orientation: Qt.Horizontal
				layoutDirection:  Qt.LeftToRight
				snapMode: ListView.SnapOneItem

				onContentXChanged: {
					mediaIndicator.currentIndex = indexAt(contentX, 1);
				}

				add: Transition {
					NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
					//NumberAnimation { property: "height"; from: 0; to: mediaList.width; duration: 400 }
				}

				remove: Transition {
					NumberAnimation { property: "opacity"; from: 1.0; to: 0; duration: 400 }
					NumberAnimation { property: "height"; from: mediaList.width; to: 0; duration: 400 }
				}

				displaced: Transition {
					NumberAnimation { properties: "x,y"; duration: 400; easing.type: Easing.OutElastic }
				}

				model: ListModel { id: mediaModel }

				delegate: Rectangle {
					//
					id: imageFrame
					color: "transparent"
					width: mediaImage.width + 2 * spaceItems_
					height: mediaList.height // mediaImage.height

					Image {
						id: mediaImage
						autoTransform: true

						x: getX()
						y: getY()

						fillMode: Image.PreserveAspectFit
						mipmap: true

						source: path

						Component.onCompleted: {
						}

						function getX() {
							return parent.width / 2 - width / 2;
						}

						function getY() {
							return 0;
						}

						function adjustView() {
							width = mediaList.width - 2*spaceItems_;
							mediaList.height = Math.max(mediaBox.calculatedHeight, height);
							mediaBox.calculatedHeight = mediaList.height;
						}

						onStatusChanged: {
							adjustView();
						}

						layer.enabled: true
						layer.effect: OpacityMask {
							id: roundEffect
							maskSource: Item {
								width: roundEffect.getWidth()
								height: roundEffect.getHeight()

								Rectangle {
									x: roundEffect.getX()
									y: roundEffect.getY()
									width: roundEffect.getWidth()
									height: roundEffect.getHeight()
									radius: 8
								}
							}

							function getX() {
								return mediaImage.width / 2 - mediaImage.paintedWidth / 2;
							}

							function getY() {
								return mediaImage.height / 2 - mediaImage.paintedHeight / 2;
							}

							function getWidth() {
								return mediaImage.paintedWidth;
							}

							function getHeight() {
								return mediaImage.paintedHeight;
							}
						}
					}

					QuarkToolButton	{
						id: removeButton

						x: mediaImage.width - (width + spaceItems_)
						y: 2 * spaceItems_
						// Material.background: "transparent"
						visible: true
						labelYOffset: 3
						symbolColor: (!sending || uploaded) ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
														  buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
						symbol: !uploaded ? Fonts.cancelSym : Fonts.checkedSym

						onClicked: {
							if (!sending) {
								mediaList.removeMedia(index);
							}
						}
					}

					QuarkRoundProgress {
						id: mediaUploadProgress
						x: removeButton.x - 1
						y: removeButton.y - 1
						size: removeButton.width + 2
						colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
						colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.link");
						arcBegin: 0
						arcEnd: progress
						lineWidth: 3
					}
				}

				function addMedia(file) {
					mediaModel.append({
						key: file,
						path: "file://" + file,
						progress: 0,
						uploaded: 0,
						processing: 0 });

					positionViewAtEnd();
				}

				function removeMedia(index) {
					mediaModel.remove(index);
				}
			}

			PageIndicator {
				id: mediaIndicator
				count: mediaModel.count
				currentIndex: mediaList.currentIndex

				anchors.bottom: parent.top
				anchors.horizontalCenter: parent.horizontalCenter

				Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
				Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
				Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
				Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
				Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
			}
		}
	}

	//
	// footer bar
	//
	QuarkToolBar {
		id: buzzFooterBar
		height: 45
		width: parent.width
		y: parent.height - 45

		property int totalHeight: height

		Component.onCompleted: {
		}

		QuarkToolButton	{
			id: addFromGalleryButton
			Material.background: "transparent"
			visible: true
			labelYOffset: 3
			symbolColor: !sending ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
								   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.imagesSym

			y: parent.height / 2 - height / 2

			onClicked: {
				if (!sending)
					// imageListing.listImages();
					buzzerApp.pickImageFromGallery();
			}
		}

		QuarkToolButton {
			id: addPhotoButton
			Material.background: "transparent"
			visible: true
			labelYOffset: 3
			symbolColor: !sending ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.cameraSym

			x: addFromGalleryButton.x + addFromGalleryButton.width // + spaceItems_
			y: parent.height / 2 - height / 2

			onClicked: {
				if (sending) return;

				var lComponent = null;
				var lPage = null;

				lComponent = Qt.createComponent("qrc:/qml/camera.qml");
				if (lComponent.status === Component.Error) {
					controller.showError(lComponent.errorString());
				} else {
					lPage = lComponent.createObject(window);
					lPage.controller = controller;
					lPage.dataReady.connect(photoTaken);

					addPage(lPage);
				}
			}

			function photoTaken(path) {
				mediaList.addMedia(path);
			}
		}

		ProgressBar {
			id: createProgressBar

			x: addPhotoButton.x + addPhotoButton.width + spaceItems_
			y: parent.height / 2 - height / 2
			width: countProgress.x - x - spaceRight_
			visible: false
			value: 0.0

			Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
			Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
			Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
			Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
			Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
		}

		QuarkRoundState {
			id: hiddenCountFrame
			x: parent.width - (size + spaceRight_)
			y: parent.height / 2 - size / 2
			size: 28
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.hidden")
			background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
		}

		QuarkRoundProgress {
			id: countProgress
			x: hiddenCountFrame.x
			y: hiddenCountFrame.y
			size: hiddenCountFrame.size
			colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
			colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
			arcBegin: 0
			arcEnd: 0
			lineWidth: 3

			function adjust(length) {
				arcEnd = (length * 360) / buzzerClient.getBuzzBodyMaxSize();
			}
		}

		QuarkHLine {
			id: topLine
			x1: 0
			y1: 0
			x2: parent.width
			y2: 0
			penWidth: 1
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabledHidden")
			visible: true
		}
	}

	//
	// menus and dropdowns
	//

	QuarkPopupMenu {
		id: buzzersList
		width: 170
		visible: false

		model: ListModel { id: buzzersModel }

		property var matched;

		onClick: {
			//
			buzzText.remove(buzzText.cursorPosition - matched.length, buzzText.cursorPosition);
			buzzText.insert(buzzText.cursorPosition, key);
		}

		function popup(match, nx, ny, buzzers) {
			//
			if (buzzers.length === 0) return;
			if (buzzers.length === 1 && match === buzzers[0]) return;
			//
			matched = match;
			//
			if (nx + width > parent.width) x = parent.width - width;
			else x = nx;

			buzzersModel.clear();

			for (var lIdx = 0; lIdx < buzzers.length; lIdx++) {
				buzzersModel.append({
					key: buzzers[lIdx],
					keySymbol: "",
					name: buzzers[lIdx]});
			}

			var lNewY = ny + buzzEditorToolBar.y + buzzEditorToolBar.height + buzzText.y + 5;
			if (lNewY + (buzzers.length * 50/*height*/) > bodyContainer.height)
				y = lNewY - ((buzzers.length + 1) * 50);
			else
				y = lNewY;

			open();
		}
	}

	QuarkPopupMenu {
		id: tagsList
		width: 170
		visible: false

		model: ListModel { id: tagsModel }

		property var matched;

		onClick: {
			//
			buzzText.remove(buzzText.cursorPosition - matched.length, buzzText.cursorPosition);
			buzzText.insert(buzzText.cursorPosition, key);
		}

		function popup(match, nx, ny, tags) {
			//
			if (tags.length === 0) return;
			if (tags.length === 1 && match === tags[0]) return;
			//
			matched = match;
			//
			if (nx + width > parent.width) x = parent.width - width;
			else x = nx;

			tagsModel.clear();

			for (var lIdx = 0; lIdx < tags.length; lIdx++) {
				tagsModel.append({
					key: tags[lIdx],
					keySymbol: "",
					name: tags[lIdx]});
			}

			var lNewY = ny + buzzEditorToolBar.y + buzzEditorToolBar.height + buzzText.y + 5;
			if (lNewY + (tags.length * 50/*height*/) > bodyContainer.height)
				y = lNewY - ((tags.length + 1) * 50);
			else
				y = lNewY;

			open();
		}
	}

	//
	// Image listing - gallery
	//

	BuzzerComponents.ImageListing {
		id: imageListing

		onImageFound:  {
			mediaList.addMedia(file);
		}
	}

	Connections {
		id: selectImage
		target: buzzerApp

		function onFileSelected(file) {
			//
			mediaList.addMedia(file);
		}
	}

	//
	// Create buzz
	//

	BuzzerCommands.UploadMediaCommand {
		id: uploadBuzzMedia

		onProgress: {
			//
			var lPart = 100 / mediaModel.count;

			// file, pos, size
			for (var lIdx = 0; lIdx < mediaModel.count; lIdx++) {
				if(mediaModel.get(lIdx).key === file) {
					//
					if (pos > 0) createProgressBar.indeterminate = false;

					// update media progress
					var lPercent = (pos * 100) / size;
					var lArc = (360 * lPercent) / 100;
					mediaModel.get(lIdx).progress = lArc;

					if (lPercent === 100.0) mediaModel.get(lIdx).uploaded = 1;

					// update global
					var lCurrent = (lIdx * lPart);
					lCurrent += (lPercent * lPart) / 100.0;
					createProgressBar.value = lCurrent / 100.0;
					break;
				}
			}
		}
	}

	BuzzerCommands.BuzzCommand {
		id: buzzCommand
		uploadCommand: uploadBuzzMedia

		onProcessed: {
			createProgressBar.value = 1.0;
			controller.popPage();
		}
		onError: {
			createProgressBar.visible = false;
			sending = false;
			handleError(code, message);
		}

		onMediaUploaded: {
			createProgressBar.indeterminate = true;
		}
	}

	BuzzerCommands.ReBuzzCommand {
		id: rebuzzCommand
		uploadCommand: uploadBuzzMedia
		model: buzzfeedModel_

		onProcessed: {
			createProgressBar.value = 1.0;
			controller.popPage();
		}
		onError: {
			createProgressBar.visible = false;
			sending = false;
			handleError(code, message);
		}

		onMediaUploaded: {
			createProgressBar.indeterminate = true;
		}
	}

	BuzzerCommands.ReplyCommand {
		id: replyCommand
		uploadCommand: uploadBuzzMedia
		model: buzzfeedModel_ // TODO: implement property buzzChainId (with buzzfeedModel in parallel)

		onProcessed: {
			createProgressBar.value = 1.0;
			controller.popPage();
		}
		onError: {
			createProgressBar.visible = false;
			sending = false;
			handleError(code, message);
		}

		onMediaUploaded: {
			createProgressBar.indeterminate = true;
		}
	}

	function createBuzz() {
		//
		var lText = buzzerClient.getPlainText(buzzText.textDocument);
		if (lText.length === 0) lText = buzzText.preeditText;

		if (lText.length === 0 && mediaModel.count === 0) {
			handleError("E_BUZZ_IS_EMPTY", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_EMPTY"));
			sending = false;
			return;
		}

		//
		createProgressBar.indeterminate = true;
		createProgressBar.visible = true;

		//
		buzzCommand.buzzBody = lText;
		//
		var lBuzzers = buzzerClient.extractBuzzers(buzzCommand.buzzBody);
		for (var lIdx = 0; lIdx < lBuzzers.length; lIdx++) {
			buzzCommand.addBuzzer(lBuzzers[lIdx]);
		}

		for (lIdx = 0; lIdx < mediaModel.count; lIdx++) {
			buzzCommand.addMedia(mediaModel.get(lIdx).key);
		}

		buzzCommand.process();
	}

	function createRebuzz(buzzId) {
		//
		var lText = buzzerClient.getPlainText(buzzText.textDocument);
		if (lText.length === 0) lText = buzzText.preeditText;

		//
		createProgressBar.indeterminate = true;
		createProgressBar.visible = true;

		//
		rebuzzCommand.buzzBody = lText;
		var lBuzzers = buzzerClient.extractBuzzers(rebuzzCommand.buzzBody);

		for (var lIdx = 0; lIdx < lBuzzers.length; lIdx++) {
			rebuzzCommand.addBuzzer(lBuzzers[lIdx]);
		}

		for (lIdx = 0; lIdx < mediaModel.count; lIdx++) {
			rebuzzCommand.addMedia(mediaModel.get(lIdx).key);
		}

		if (buzzId) rebuzzCommand.buzzId = buzzId;

		rebuzzCommand.process();
	}

	function createReply(buzzId) {
		//
		var lText = buzzerClient.getPlainText(buzzText.textDocument);
		if (lText.length === 0) lText = buzzText.preeditText;

		if (lText.length === 0 && mediaModel.count === 0) {
			handleError("E_BUZZ_IS_EMPTY", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_EMPTY"));
			sending = false;
			return;
		}

		//
		createProgressBar.indeterminate = true;
		createProgressBar.visible = true;

		//
		replyCommand.buzzBody = lText;
		//
		var lBuzzers = buzzerClient.extractBuzzers(replyCommand.buzzBody);

		for (var lIdx = 0; lIdx < lBuzzers.length; lIdx++) {
			replyCommand.addBuzzer(lBuzzers[lIdx]);
		}

		for (lIdx = 0; lIdx < mediaModel.count; lIdx++) {
			replyCommand.addMedia(mediaModel.get(lIdx).key);
		}

		if (buzzId) replyCommand.buzzId = buzzId;

		replyCommand.process();
	}

	function handleError(code, message) {
		if (code === "E_CHAINS_ABSENT") return;
		if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT") {
			buzzerClient.resync();
			controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"), true);
		} else {
			controller.showError(message, true);
		}
	}
}
