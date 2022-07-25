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
	id: buzzmediaPage_
	key: "buzzmedia"
	stacked: buzzerApp.isDesktop

	//
	property string mediaViewTheme: "Darkmatter"
	property string mediaViewSelector: "dark"

	// mandatory
	property var buzzMedia_;
	property var mediaPlayerController;
	property bool initialized_: false;
	property var pkey_: ""
	property var mediaIndex_: 0
	property var mediaPlayer_: null
	property var buzzId_: null
	property var buzzBody_: ""

	statusBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "MediaView.statusBar")
	navigationBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "MediaView.navigationBar")
	Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "MediaView.pageBackground")
	statusBarTheme: "dark"
	navigatorTheme: "dark"

	Component.onCompleted: {
		closePageHandler = closePage;
		activatePageHandler = activatePage;

		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "MediaView.pageBackground"));
	}

	onMediaPlayerControllerChanged: {
		//
		if (mediaGalleryContainer.buzzMediaItem_)
			mediaGalleryContainer.buzzMediaItem_.sharedMediaPlayer_ = mediaPlayerController;
	}

	onControllerChanged: {
	}

	function closePage() {
		stopPage();
		controller.popPage();
		destroy(1000);

		// set back
		statusBarColor = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.statusBar")
		navigationBarColor = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.navigationBar")
		statusBarTheme = buzzerClient.statusBarTheme;
		navigatorTheme = buzzerClient.themeSelector;
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.background"));
		controller.activePageBackground = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.background");
	}

	function activatePage() {
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "MediaView.pageBackground"));
	}

	function onErrorCallback(error)	{
		controller.showError(error);
	}

	function stopPage() {
		//
		mediaGalleryContainer.checkPlaying();
		//
		playerControl.unlink();
		//
		if (mediaPlayerController) {
			mediaPlayerController.disableContinousPlayback();
			mediaPlayerController.popVideoInstance();
		}
	}

	function initialize(pkey, mediaIndex, player, buzzId, buzzBody) {
		//
		if (key !== undefined) pkey_ = pkey;
		mediaIndex_ = mediaIndex;
		mediaPlayer_ = player;
		buzzBody_ = buzzBody;
		playerControl.key = buzzerApp.isDesktop ? buzzId : null;
		buzzId_ = buzzId;

		console.log("[buzzmedia/initialize]: mediaIndex = " + mediaIndex + ", player = " + player);

		mediaGalleryContainer.initialize();
		//
		initialized_ = true;
		//
		controller.activePageBackground = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "MediaView.pageBackground");
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "MediaView.pageBackground"));
	}

	function adjustToolBar() {
		if (initialized_ && !buzzerApp.isDesktop) {
			//console.log("w = " + width + ", h = " + height);
			if (width > height && height > 0) {
				if (buzzMedia_.length === 1) {
					buzzMediaToolBar.height = 0;
					//bottomLine.visible = false;
					cancelButton.visible = false;
				} else {
					buzzMediaToolBar.height = 20;
					cancelButton.visible = false;
				}
			} else {
				buzzMediaToolBar.height = 45;
				cancelButton.visible = true;
				//bottomLine.visible = true;
			}
		}
	}

	onWidthChanged: {
		//
		buzzerApp.setBackgroundColor(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "MediaView.pageBackground"));
		//
		adjustToolBar();
		orientationChangedTimer.start();
	}

	onHeightChanged: {
		adjustToolBar();
	}

	// to adjust model
	Timer {
		id: orientationChangedTimer
		interval: 100
		repeat: false
		running: false

		onTriggered: {
			mediaGalleryContainer.adjustOrientation();
		}
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
		id: buzzMediaToolBar
		height: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 50) : 45
		width: parent.width
		backgroundColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "MediaView.pageBackground")

		property int totalHeight: height

		Component.onCompleted: {
		}

		QuarkRoundSymbolButton {
			id: cancelButton
			x: spaceItems_
			y: parent.height / 2 - height / 2
			symbol: Fonts.cancelSym
			fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 5)) : buzzerApp.defaultFontSize() + 7
			radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius - 7)) : (defaultRadius - 7)
			color: "transparent"
			textColor: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.foreground")
			opacity: 1.0

			onClick: {
				closePage();
			}
		}

		QuarkRoundSymbolButton {
			id: menuControl
			x: parent.width - width - spaceItems_
			y: parent.height / 2 - height / 2
			symbol: Fonts.elipsisVerticalSym
			fontPointSize: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (buzzerApp.defaultFontSize() + 5)) : buzzerApp.defaultFontSize() + 7
			radius: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * (defaultRadius - 7)) : (defaultRadius - 7)
			color: "transparent"
			textColor: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.foreground")
			opacity: 1.0
			visible: buzzerApp.isDesktop

			onClick: {
				if (headerMenu.visible) headerMenu.close();
				else { headerMenu.prepare(); headerMenu.open(); }
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
			color: buzzerApp.getColor(mediaViewTheme, mediaViewSelector, "Material.disabledHidden")
			visible: false
		}
		*/
	}

	//
	// buzz gallery
	//
	Rectangle {
		id: mediaGalleryContainer

		x: 0
		y: buzzMediaToolBar.y + buzzMediaToolBar.height // + spaceTop_
		width: parent.width
		height: parent.height - buzzMediaToolBar.height // - spaceTop_
		color: "transparent"

		property var buzzMediaItem_;

		function adjustOrientation() {
			if (buzzMediaItem_) {
				buzzMediaItem_.calculatedWidth = mediaGalleryContainer.width;
				buzzMediaItem_.calculatedHeight = mediaGalleryContainer.height;
				buzzMediaItem_.adjust();
			}
		}

		function checkPlaying() {
			//
			if (buzzMediaItem_) {
				buzzMediaItem_.checkPlaying();
			}
		}

		function initialize() {
			//
			var lSource = "qrc:/qml/buzzitemmediaview.qml";
			var lComponent = Qt.createComponent(lSource);

			if (lComponent.status === Component.Error) {
				controller.showError(lComponent.errorString());
			} else {
				buzzMediaItem_ = lComponent.createObject(mediaGalleryContainer);

				buzzMediaItem_.x = 0;
				buzzMediaItem_.y = 0;
				buzzMediaItem_.calculatedWidth = mediaGalleryContainer.width;
				buzzMediaItem_.calculatedHeight = mediaGalleryContainer.height;
				//buzzMediaItem_.width = mediaContainer.width;
				//buzzMediaItem_.height = mediaContainer.height;
				buzzMediaItem_.controller_ = buzzmediaPage_.controller;
				buzzMediaItem_.buzzMedia_ = buzzmediaPage_.buzzMedia_;
				buzzMediaItem_.sharedMediaPlayer_ = buzzmediaPage_.mediaPlayerController;
				buzzMediaItem_.initialize(pkey_, mediaIndex_, mediaPlayer_, buzzId_, buzzBody_);
			}
		}

		onWidthChanged: {
			if (buzzMediaItem_) {
				buzzMediaItem_.calculatedWidth = mediaGalleryContainer.width;
				buzzMediaItem_.adjust();
			}
		}

		onHeightChanged: {
			if (buzzMediaItem_) {
				buzzMediaItem_.calculatedHeight = mediaGalleryContainer.height;
				buzzMediaItem_.adjust();
			}
		}
	}

	//
	BuzzItemMediaPlayer {
		id: playerControl
		x: 0
		y: (mediaGalleryContainer.y + mediaGalleryContainer.height) - height // buzzMediaToolBar.y + buzzMediaToolBar.height
		width: mediaGalleryContainer.width
		showOnChanges: true
		mediaPlayerController: buzzmediaPage_.mediaPlayerController
		gallery: true
	}

	//
	QuarkPopupMenu {
		id: headerMenu
		x: parent.width - width - spaceRight_
		y: menuControl.y + menuControl.height + spaceItems_
		width: buzzerApp.isDesktop ? (buzzerClient.scaleFactor * 350) : 350
		visible: false

		model: ListModel { id: menuModel }

		onAboutToShow: prepare()

		onClick: {
			// key, activate
			controller.activatePage(key);
		}

		function prepare() {
			//
			menuModel.clear();

			//
			var lArray = controller.enumStakedPages();
			for (var lI = 0; lI < lArray.length; lI++) {
				//
				menuModel.append({
					key: lArray[lI].key,
					keySymbol: "",
					name: lArray[lI].alias + " // " + lArray[lI].caption.substring(0, 100)});
			}
		}
	}
}
