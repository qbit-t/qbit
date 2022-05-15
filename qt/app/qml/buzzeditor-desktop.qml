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
// NOTICE: native file dialog
import QtQuick.PrivateWidgets 1.0

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
	property bool message_: false;
	property var buzzfeedModel_;
	property var buzzItem_;
	property var pkey_: "";
	property var conversation_: "";
	property var text_: "";

	function initializeRebuzz(item, model) {
		buzzItem_ = item;
		buzzfeedModel_ = model;
		buzz_ = false;
		rebuzz_ = true;
		message_ = false;

		bodyContainer.wrapItem(item);
	}

	function initializeReply(item, model, text) {
		buzzItem_ = item;
		buzzfeedModel_ = model;
		buzz_ = false;
		reply_ = true;
		message_ = false;

		if (text) {
			// buzzText.text = text;
			text_ = text;
			injectText.start();
		}

		bodyContainer.replyItem(item);
	}

	function initializeBuzz(text) {
		buzz_ = true;
		reply_ = false;
		rebuzz_ = false;
		message_ = false;

		if (text) {
			// buzzText.text = text;
			text_ = text;
			injectText.start();
		}
	}

	function initializeMessage(text, pkey, conversation) {
		buzz_ = false;
		reply_ = false;
		rebuzz_ = false;
		message_ = true;
		pkey_ = pkey;
		conversation_ = conversation;

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

	// only once to inject outer text
	Timer {
		id: injectText
		interval: 500
		repeat: false
		running: false

		onTriggered: {
			buzzText.text = text_;
		}
	}

	//
	// toolbar
	//
	QuarkToolBar {
		id: buzzEditorToolBar
		height: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 50) : 45
		width: parent.width

		property int totalHeight: height

		Component.onCompleted: {
		}

		QuarkToolButton	{
			id: cancelButton
			y: topOffset + parent.height / 2 - height / 2
			Material.background: "transparent"
			visible: true
			labelYOffset: buzzerApp.isDesktop ? 0 : 3
			symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.cancelSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : symbolFontPointSize

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
									message_ ? buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.send") :
											   buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.editor.reply")

			textColor: !sending ?
						 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.menu.foreground") :
						 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : fontPointSize

			onClick: {
				if (!sending) {
					sending = true;

					buzzerApp.commitCurrentInput();

					if (rebuzz_) buzzeditor_.createRebuzz(buzzItem_.buzzId);
					else if (reply_) buzzeditor_.createReply(buzzItem_.buzzId);
					else if (buzz_) buzzeditor_.createBuzz();
					else if (message_) buzzeditor_.createMessage();
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
			if (height > 0 && item.y + item.contentHeight + spaceMedia_ > contentY + height) {
				contentY += (item.y + item.contentHeight + spaceMedia_) - (contentY + height);
			}
		}

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
						else if (match.includes('/data/user/') || buzzerApp.isDesktop) {
							var lParts = match.split(".");
							if (lParts.length) {
								if (lParts[lParts.length-1].toLowerCase() === "jpg" || lParts[lParts.length-1].toLowerCase() === "jpeg" ||
									lParts[lParts.length-1].toLowerCase() === "png") {
									console.log(match);
									// inject
									mediaList.addMedia(match);
									// remove
									buzzText.remove(start, start + length);

								}
							}
						}
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
			//height: 1000 //parent.height
			width: parent.width - spaceRight_
			wrapMode: Text.Wrap
			textFormat: Text.RichText
			focus: true
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
			font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 12) : defaultFontPointSize
			selectionColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.selected")
			selectByMouse: true

			onLengthChanged: {
				// TODO: may by too expensive
				var lText = buzzerClient.getPlainText(buzzText.textDocument);
				countProgress.adjust(buzzerClient.getBuzzBodySize(lText) + preeditText.length);
				buzzersList.close();
				tagsList.close();
			}

			property bool buzzerStarted_: false;
			property bool tagStarted_: false;

			onPreeditTextChanged: {
				//
				if (preeditText == " ") {
					buzzerStarted_ = false;
					tagStarted_ = false;
					buzzersList.close();
					tagsList.close();
				} else if (preeditText == "@" || buzzerStarted_) {
					//
					buzzerStarted_ = true;
				} else if (preeditText == "#" || tagStarted_) {
					//
					tagStarted_ = true;
				}

				if (buzzerStarted_ || tagStarted_) {
					var lText = buzzerClient.getPlainText(buzzText.textDocument);
					highlighter.tryHighlightBlock(lText + preeditText, 0);
				}
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
			//
			mediaBox.adjust();
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
			wrapContainer.wrappedItem_.lastUrl_ = buzzerClient.extractLastUrl(item.buzzBody);
			wrapContainer.wrappedItem_.interactive_ = false;
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
			replyContainer.replyItem_.lastUrl_ = buzzerClient.extractLastUrl(item.buzzBody);
			replyContainer.replyItem_.interactive_ = false;
			replyContainer.replyItem_.ago_ = buzzerClient.timestampAgo(item.timestamp);

			bodyContainer.ensureVisible(buzzText);
		}

		function innerHeightChanged(value) {
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
			width: bodyContainer.width - (spaceLeft_ + spaceRight_ - (spaceItems_) * 2)
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

			function adjust() {
				//
				for (var lIdx = 0; lIdx < mediaList.count; lIdx++) {
					var lItem = mediaList.itemAtIndex(lIdx);
					if (lItem) {
						lItem.width = mediaBox.width;
						lItem.adjust();
					}
				}
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

				cacheBuffer: 100000

				function cleanUp() {
					//
					for (var lIdx = 0; lIdx < mediaList.count; lIdx++) {
						var lItem = mediaList.itemAtIndex(lIdx);
						if (lItem && lItem.mediaItem) {
							lItem.mediaItem.terminate();
						}
					}
				}

				onContentXChanged: {
					mediaIndicator.currentIndex = indexAt(contentX, 1);
				}

				add: Transition {
					NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
					//NumberAnimation { property: "height"; from: 0; to: mediaListEditor.width; duration: 400 }
				}

				remove: Transition {
					NumberAnimation { property: "opacity"; from: 1.0; to: 0; duration: 400 }
					NumberAnimation { property: "height"; from: mediaList.width; to: 0; duration: 400 }
				}

				displaced: Transition {
					NumberAnimation { properties: "x,y"; duration: 400; easing.type: Easing.OutElastic }
				}

				model: ListModel { id: mediaModel }

				delegate: Item {
					id: itemDelegate

					property var mediaItem;

					function adjustHeight(proposed) {
						//
						if (index === mediaIndicator.currentIndex) {
							//
							itemDelegate.height = proposed;
						}
					}

					onWidthChanged: {
						if (mediaItem) {
							mediaItem.width = mediaList.width;
							itemDelegate.height = mediaItem.height;
						}
					}

					Component.onCompleted: {
						var lSource = "qrc:/qml/buzzitemmedia-editor-image.qml";
						if (media === "audio") {
							lSource = "qrc:/qml/buzzitemmedia-editor-audio.qml";
						} else if (media === "video") {
							lSource = "qrc:/qml/buzzitemmedia-editor-video.qml";
						}

						var lComponent = Qt.createComponent(lSource);
						mediaItem = lComponent.createObject(itemDelegate);
						if (media === "video") {
							mediaItem.adjustDuration.connect(adjustDuration);
							mediaItem.adjustHeight.connect(adjustHeight);
						}

						mediaItem.width = list.width;
						mediaItem.mediaList = mediaList;
						mediaItem.mediaBox = mediaBox;

						itemDelegate.height = mediaItem.height;
						itemDelegate.width = mediaList.width;
					}

					function adjustDuration(value) {
						//
						for (var lIdx = 0; lIdx < mediaModel.count; lIdx++) {
							if(mediaModel.get(lIdx).key === key) {
								//
								duration = value;
								console.log("[adjustDuration]: vale = " + duration);
							}
						}
					}
				}

				function addMedia(file, info) {
					var lOrientation = 0; // vertical
					if (buzzeditor_.width > buzzeditor_.height) lOrientation = 1; // horizontal

					mediaModel.append({
						key: file,
						path: "file://" + file,
						preview: "none",
						media: "image",
						size: 0,
						duration: 0,
						progress: 0,
						uploaded: 0,
						orientation: lOrientation,
						processing: 0,
						description: info});

					positionViewAtEnd();
				}

				function addAudio(file, duration, info) {
					var lOrientation = 0; // vertical
					if (buzzeditor_.width > buzzeditor_.height) lOrientation = 1; // horizontal

					mediaModel.append({
						key: file,
						path: "file://" + file,
						preview: "none",
						media: "audio",
						size: 0,
						duration: duration,
						progress: 0,
						uploaded: 0,
						orientation: lOrientation,
						processing: 0,
						description: info});

					positionViewAtEnd();
				}

				function addVideo(file, duration, orientation, preview, info) {
					/*
					var lOrientation = 0; // vertical
					if (buzzeditor_.width > buzzeditor_.height) lOrientation = 1; // horizontal
					*/
					mediaModel.append({
						key: file,
						path: "file://" + file,
						preview: preview,
						media: "video",
						size: 0,
						duration: duration,
						progress: 0,
						uploaded: 0,
						orientation: orientation,
						processing: 0,
						description: info});

					positionViewAtEnd();
				}

				function removeMedia(index) {
					mediaModel.remove(index);
				}
			}

			QuarkLabel {
				id: mediaIndicator
				property var defaultFontSize: 11
				font.pointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultFontSize + 1)) : defaultFontSize + 1
				text: "0/0"
				color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
				visible: count > 1

				anchors.bottom: parent.top
				anchors.horizontalCenter: parent.horizontalCenter

				property var count: mediaModel.count
				property var currentIndex: mediaList.currentIndex

				onCurrentIndexChanged: {
					text = (currentIndex + 1) + "/" + count;
				}
			}
		}
	}

	//
	// footer bar
	//
	QuarkToolBar {
		id: buzzFooterBar
		height: controller.bottomBarHeight
		width: parent.width
		y: parent.height - height

		property int totalHeight: height

		Component.onCompleted: {
		}

		QuarkToolButton	{
			id: addFromGalleryButton
			Material.background: "transparent"
			visible: true
			//labelYOffset: 3
			symbolColor: !sending ? buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
								   buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.paperClipSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : symbolFontPointSize

			y: parent.height / 2 - height / 2

			onClicked: {
				if (!sending) {
					imageListing.open();
				}
			}
		}

		QuarkToolButton {
			id: addEmojiButton
			Material.background: "transparent"
			visible: true
			//labelYOffset: 3
			symbolColor: !sending ?
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground") :
							 buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled")
			Layout.alignment: Qt.AlignHCenter
			symbol: Fonts.peopleEmojiSym
			symbolFontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 16) : symbolFontPointSize

			x: addFromGalleryButton.x + addFromGalleryButton.width // + spaceItems_
			y: parent.height / 2 - height / 2

			onClicked: {
				emojiPopup.popup(addEmojiButton.x, buzzFooterBar.y - (emojiPopup.height));
			}
		}

		ProgressBar {
			id: createProgressBar

			x: addFromGalleryButton.x + addFromGalleryButton.width + spaceItems_
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
			size: buzzerClient.scaleFactor * 28
			color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.hidden")
			background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background")
			penWidth: buzzerClient.scaleFactor * 3
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
			lineWidth: buzzerClient.scaleFactor * 3

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

	QuarkEmojiPopup {
		id: emojiPopup
		width: buzzerClient.scaleFactor * 360
		height: buzzerClient.scaleFactor * 300

		onEmojiSelected: {
			//
			buzzText.insert(buzzText.cursorPosition, emoji);
		}

		function popup(nx, ny) {
			//
			x = nx;
			y = ny;

			open();
		}
	}

	QtFileDialog {
		id: imageListing
		nameFilters: [ "Media files (*.jpg *.png *.jpeg *.mp3 *.mp4 *.m4a)" ]
		selectExisting: true
		selectFolder: false
		selectMultiple: false

		onAccepted: {
			//
			var lPath = fileUrl.toString();
			if (Qt.platform.os == "windows") {
				lPath = lPath.replace(/^(file:\/{3})/,"");
			} else {
				lPath = lPath.replace(/^(file:\/{2})/,"");
			}

			//
			var lFile = decodeURIComponent(lPath);
			var lPreview = "";

			//
			if (mediaModel.count < 31) {
				//
				var lSize = buzzerApp.getFileSize(lFile);
				console.log("[onFileSelected]: file = " + lFile + "/" + lSize);
				//
				if ((lFile.includes(".mp4") || lFile.includes(".mp3") || lFile.includes(".m4a")) && preview !== "") {
					mediaList.addVideo(lFile, 0, 0, lPreview, "none");
				} else if (lFile.includes(".mp3") || lFile.includes(".m4a")) {
					mediaList.addAudio(lFile, 0, "none");
				} else if (lFile.includes(".mp4")) {
					handleError("E_MEDIA_PREVIEW_ABSENT", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_MEDIA_PREVIEW_ABSENT"));
				} else {
					//
					mediaList.addMedia(lFile, "none");
				}
			} else {
				handleError("E_MAX_ATTACHMENTS", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_MAX_ATTACHMENTS"));
			}
		}
	}

	QuarkPopupMenu {
		id: buzzersList
		width: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 170) : 170
		visible: false

		model: ListModel { id: buzzersModel }

		property var matched;

		onClick: {
			//
			var lPos = buzzText.cursorPosition; // current
			var lText = buzzerClient.getPlainText(buzzText.textDocument) + "";
			for (var lIdx = lPos; lIdx >= 0; lIdx--) {
				if (lText[lIdx] === '@') {
					break;
				}
			}

			var lNewText = lText.slice(0, lIdx) + key + lText.slice(lPos);

			buzzText.clear();
			buzzText.insert(0, lNewText);

			buzzText.cursorPosition = lIdx + key.length;
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
		width: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 170) : 170
		visible: false

		model: ListModel { id: tagsModel }

		property var matched;

		onClick: {
			//
			var lPos = buzzText.cursorPosition; // current
			var lText = buzzerClient.getPlainText(buzzText.textDocument) + "";
			for (var lIdx = lPos; lIdx >= 0; lIdx--) {
				if (lText[lIdx] === '#') {
					break;
				}
			}

			var lNewText = lText.slice(0, lIdx) + key + lText.slice(lPos);

			buzzText.clear();
			buzzText.insert(0, lNewText);

			buzzText.cursorPosition = lIdx + key.length;
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
	// Create buzz
	//

	BuzzerCommands.UploadMediaCommand {
		id: uploadBuzzMedia
		pkey: pkey_

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
			mediaList.cleanUp();
			createProgressBar.value = 1.0;
			controller.popPage();
		}

		onRetry: {
			// was specific error, retrying...
			createProgressBar.indeterminate = true;
			retryBuzzCommand.start();
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

	Timer {
		id: retryBuzzCommand
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			buzzCommand.reprocess();
		}
	}

	BuzzerCommands.ReBuzzCommand {
		id: rebuzzCommand
		uploadCommand: uploadBuzzMedia
		model: buzzfeedModel_

		onProcessed: {
			mediaList.cleanUp();
			createProgressBar.value = 1.0;
			controller.popPage();
		}

		onRetry: {
			// was specific error, retrying...
			createProgressBar.indeterminate = true;
			retryReBuzzCommand.start();
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

	Timer {
		id: retryReBuzzCommand
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			rebuzzCommand.reprocess();
		}
	}

	BuzzerCommands.ReplyCommand {
		id: replyCommand
		uploadCommand: uploadBuzzMedia
		model: buzzfeedModel_ // TODO: implement property buzzChainId (with buzzfeedModel in parallel)

		onProcessed: {
			mediaList.cleanUp();
			createProgressBar.value = 1.0;
			controller.popPage();
		}

		onRetry: {
			// was specific error, retrying...
			createProgressBar.indeterminate = true;
			retryReplyCommand.start();
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

	Timer {
		id: retryReplyCommand
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			replyCommand.reprocess();
		}
	}

	BuzzerCommands.ConversationMessageCommand {
		id: messageCommand
		uploadCommand: uploadBuzzMedia

		onProcessed: {
			mediaList.cleanUp();
			createProgressBar.value = 1.0;
			controller.popPage();
		}

		onRetry: {
			// was specific error, retrying...
			createProgressBar.indeterminate = true;
			retryConversationMessageCommand.start();
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

	Timer {
		id: retryConversationMessageCommand
		interval: 1000
		repeat: false
		running: false

		onTriggered: {
			messageCommand.reprocess();
		}
	}

	function createBuzz() {
		//
		var lText = buzzerClient.getPlainText(buzzText.textDocument);
		if (lText.length === 0) lText = buzzText.preeditText;

		//
		mediaListcleanUp();

		if (lText.length === 0 && mediaModel.count === 0) {
			handleError("E_BUZZ_IS_EMPTY", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_EMPTY"));
			sending = false;
			return;
		}

		if (buzzerClient.getBuzzBodySize(lText) >= buzzerClient.getBuzzBodyMaxSize()) {
			handleError("E_BUZZ_IS_TOO_BIG", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_TOO_BIG"));
			sending = false;
			return;
		}

		//
		createProgressBar.indeterminate = true;
		createProgressBar.visible = true;
		//
		buzzCommand.clear();
		//
		buzzCommand.buzzBody = lText;
		//
		var lBuzzers = buzzerClient.extractBuzzers(buzzCommand.buzzBody);
		for (var lIdx = 0; lIdx < lBuzzers.length; lIdx++) {
			buzzCommand.addBuzzer(lBuzzers[lIdx]);
		}

		for (lIdx = 0; lIdx < mediaModel.count; lIdx++) {
			console.log("[createBuzz/media]: key = " + mediaModel.get(lIdx).key + ", preview = " + mediaModel.get(lIdx).preview);
			buzzCommand.addMedia(mediaModel.get(lIdx).key + "," +
								 mediaModel.get(lIdx).duration + "," +
								 mediaModel.get(lIdx).preview + "," +
								 mediaModel.get(lIdx).orientation + "," +
								 mediaModel.get(lIdx).description);
		}

		buzzCommand.process();
	}

	function createRebuzz(buzzId) {
		//
		var lText = buzzerClient.getPlainText(buzzText.textDocument);
		if (lText.length === 0) lText = buzzText.preeditText;

		//
		mediaList.cleanUp();

		if (buzzerClient.getBuzzBodySize(lText) >= buzzerClient.getBuzzBodyMaxSize()) {
			handleError("E_BUZZ_IS_TOO_BIG", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_TOO_BIG"));
			sending = false;
			return;
		}

		//
		createProgressBar.indeterminate = true;
		createProgressBar.visible = true;
		//
		rebuzzCommand.clear();
		//
		rebuzzCommand.buzzBody = lText;
		//
		var lBuzzers = buzzerClient.extractBuzzers(rebuzzCommand.buzzBody);
		for (var lIdx = 0; lIdx < lBuzzers.length; lIdx++) {
			rebuzzCommand.addBuzzer(lBuzzers[lIdx]);
		}

		for (lIdx = 0; lIdx < mediaModel.count; lIdx++) {
			rebuzzCommand.addMedia(mediaModel.get(lIdx).key + "," +
								   mediaModel.get(lIdx).duration + "," +
								   mediaModel.get(lIdx).preview + "," +
								   mediaModel.get(lIdx).orientation + "," +
								   mediaModel.get(lIdx).description);
		}

		if (buzzId) rebuzzCommand.buzzId = buzzId;

		rebuzzCommand.process();
	}

	function createReply(buzzId) {
		//
		var lText = buzzerClient.getPlainText(buzzText.textDocument);
		if (lText.length === 0) lText = buzzText.preeditText;

		//
		mediaList.cleanUp();

		if (lText.length === 0 && mediaModel.count === 0) {
			handleError("E_BUZZ_IS_EMPTY", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_EMPTY"));
			sending = false;
			return;
		}

		if (buzzerClient.getBuzzBodySize(lText) >= buzzerClient.getBuzzBodyMaxSize()) {
			handleError("E_BUZZ_IS_TOO_BIG", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_TOO_BIG"));
			sending = false;
			return;
		}

		//
		createProgressBar.indeterminate = true;
		createProgressBar.visible = true;
		//
		replyCommand.clear();
		//
		replyCommand.buzzBody = lText;
		//
		var lBuzzers = buzzerClient.extractBuzzers(replyCommand.buzzBody);
		for (var lIdx = 0; lIdx < lBuzzers.length; lIdx++) {
			replyCommand.addBuzzer(lBuzzers[lIdx]);
		}

		for (lIdx = 0; lIdx < mediaModel.count; lIdx++) {
			replyCommand.addMedia(mediaModel.get(lIdx).key + "," +
								  mediaModel.get(lIdx).duration + "," +
								  mediaModel.get(lIdx).preview + "," +
								  mediaModel.get(lIdx).orientation + "," +
								  mediaModel.get(lIdx).description);
		}

		if (buzzId) replyCommand.buzzId = buzzId;

		replyCommand.process();
	}

	function createMessage() {
		//
		var lText = buzzerClient.getPlainText(buzzText.textDocument);
		if (lText.length === 0) lText = buzzText.preeditText;

		//
		mediaList.cleanUp();

		if (lText.length === 0 && mediaModel.count === 0) {
			handleError("E_BUZZ_IS_EMPTY", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_EMPTY"));
			sending = false;
			return;
		}

		if (buzzerClient.getBuzzBodySize(lText) >= buzzerClient.getBuzzBodyMaxSize()) {
			handleError("E_BUZZ_IS_TOO_BIG", buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.E_BUZZ_IS_TOO_BIG"));
			sending = false;
			return;
		}

		//
		createProgressBar.indeterminate = true;
		createProgressBar.visible = true;
		//
		messageCommand.clear();
		//
		if (conversation_ !== "") messageCommand.conversation = conversation_;
		messageCommand.messageBody = lText;
		//
		var lBuzzers = buzzerClient.extractBuzzers(messageCommand.messageBody);
		for (var lIdx = 0; lIdx < lBuzzers.length; lIdx++) {
			messageCommand.addBuzzer(lBuzzers[lIdx]);
		}

		for (lIdx = 0; lIdx < mediaModel.count; lIdx++) {
			messageCommand.addMedia(mediaModel.get(lIdx).key + "," +
									mediaModel.get(lIdx).duration + "," +
									mediaModel.get(lIdx).preview + "," +
									mediaModel.get(lIdx).orientation + "," +
									mediaModel.get(lIdx).description);
		}

		messageCommand.process();
	}

	function handleError(code, message) {
		if (code === "E_CHAINS_ABSENT") return;
		if (message === "UNKNOWN_REFTX" || code === "E_TX_NOT_SENT" ||
				((code === "E_SENT_TX" || code === "E_UPLOAD_DATA") && message.includes("UNKNOWN_REFTX"))) {
			// a little tricky, but for now
			// we need to RE-process failed upload, but now there is no uploads chain recovery present
			controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Buzzer.error.UNKNOWN_REFTX"), true);
		} else {
			controller.showError(message, true);
		}
	}
}
