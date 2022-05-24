// Fonts.qml

pragma Singleton

import QtQuick 2.0

Item 
{
	id: fonts

	/*
	readonly property FontLoader fontAwesomeRegular: FontLoader
	{
		//source: "Font Awesome 5 Regular-400.ttf";
		source: "fa-regular-400.ttf";
	}
	*/

	/*readonly*/ property FontLoader fontAwesomeLight: FontLoader
	{
		// source: "Font Awesome 5 Light-300.ttf";
		source: "fa-light-300.ttf";
	}

	/*readonly*/ property FontLoader fontAwesomeSolid: FontLoader
	{
		// source: "Font Awesome 5 Light-300.ttf";
		source: "fa-solid-900.ttf";
	}

	/*
	readonly property FontLoader fontAwesomeSolid: FontLoader
	{
		source: "Font Awesome 5 Solid-900.ttf";
	}
	readonly property FontLoader fontAwesomeBrands: FontLoader
	{
		source: "Font Awesome 5 Brands-400.ttf";
	}

	readonly property FontLoader fontAwesomeRegularFree: FontLoader
	{
		source: "Font Awesome 5 Regular Free-400.ttf";
	}
	*/

	readonly property string icons: fonts.fontAwesomeLight.name;
	//readonly property string iconsSolid: fonts.fontAwesomeSolid.name;
	//readonly property string iconsLight: fonts.fontAwesomeLight.name;
	//readonly property string iconsFree: fonts.fontAwesomeRegularFree.name;

	readonly property string backspaceSym: "\uf55a";
	readonly property string drawerSym: "\uf0c9";
	readonly property string qrcodeSym: "\uf029";
	readonly property string accountSym: "\uf007";
	readonly property string emailSym: "\uf0e0";
	readonly property string keySym: "\uf084";
	readonly property string secretSym: "\uf084";
	readonly property string userTagSym: "\uf507";
	readonly property string tagSym: "\uf02b";
	readonly property string starHalfSym: "\uf5c0";
	readonly property string starSym: "\uf005";
	readonly property string heartSym: "\uf004";
	readonly property string circleAddSym: "\uf055";
	readonly property string thumbsSym: "\uf164";
	readonly property string linkSym: "\uf0c1";
	readonly property string wifiSym: "\uf1eb";
	readonly property string wifiOffSym: "\uf6ac";
	readonly property string closeSym: "\uf057";
	readonly property string arrowUpSym: "\uf062";
	readonly property string arrowDownSym: "\uf063";
	readonly property string userSym: "\uf007";
	readonly property string walletSym: "\uf555";
	readonly property string caseSym: "\uf0f2";
	readonly property string listSym: "\uf022";
	readonly property string historySym: "\uf1da";
	readonly property string circleArrowDownSym: "\uf358";
	readonly property string circleArrowUpSym: "\uf35b";
	readonly property string shevronLeftSym: "\uf053";
	readonly property string minusSym: "\uf068";
	readonly property string plusSym: "\uf067";
	readonly property string lockedSym: "\uf30d";
	readonly property string unlockedSym: "\uf3c2";
	readonly property string checkedCircleSym: "\uf058";
	readonly property string coinsSym: "\uf51e";
	readonly property string sigmaSym: "\uf68b";
	readonly property string pieSym: "\uf64e";
	readonly property string circleSym: "\uf111";
	readonly property string trashSym: "\uf2ed";
	readonly property string unlinkSym: "\uf127";
	readonly property string zoomInSym: "\uf00e";
	readonly property string zoomOutSym: "\uf010";
	readonly property string adjustSym: "\uf042";
	readonly property string languageSym: "\uf1ab";
	readonly property string colorSym: "\uf53f";
	readonly property string usersSym: "\uf500";
	readonly property string fingerPrintSym: "\uf577";
	readonly property string globe2Sym: "\uf57d";
	readonly property string globe0Sym: "\uf57c";
	readonly property string globe1Sym: "\uf57e";
	readonly property string refreshSym: "\uf021";
	readonly property string helpSym: "\uf059";
	readonly property string banSym: "\uf05e";
	readonly property string codeSym: "\uf121";
	readonly property string eyeSym: "\uf06e";
	readonly property string eyeSlashSym: "\uf070";
	readonly property string exclamationSym: "\uf071";
	readonly property string clockSym: "\uf017";
	readonly property string clipboardSym: "\uf328";
	readonly property string questionSym: "\uf128";
	readonly property string questionCircleSym: "\uf059";
	readonly property string addressBookSym: "\uf2b9";

	readonly property string userPlusSym: "\uf234";
	readonly property string userEditSym: "\uf4ff";
	readonly property string userSecretSym: "\uf21b";

	readonly property string conversationStartedSym: "\uf234";
	readonly property string conversationPendingSym: "\uf4fd";
	readonly property string conversationAcceptedSym: "\uf4fc";
	readonly property string conversationDeclinedSym: "\uf235";
	readonly property string conversationMessageSym: "\uf4a6";

	readonly property string chevronCircleSym: "\uf13a";
	readonly property string pasteSym: "\uf0ea";
	readonly property string levelDownSym: "\uf149";
	readonly property string infinitySym: "\uf534";
	readonly property string squareCancelSym: "\uf2d3";
	readonly property string siteMapSym: "\uf0e8";
	readonly property string blockExplorerSym: "\uf542";
	readonly property string homeSym: "\uf015";
	readonly property string chatSym: "\uf27a";
	readonly property string commentSym: "\uf075";
	readonly property string tagsSym: "\uf02c";
	readonly property string circleMinusSym: "\uf056";
	readonly property string chartBarSym: "\uf080";
	readonly property string calculatorSym: "\uf1ec";
	readonly property string exchangeSym: "\uf362";
	readonly property string dateSym: "\uf073";
	readonly property string startSym: "\uf08b";
	readonly property string endSym: "\uf090";
	readonly property string filterSym: "\uf0b0";
	readonly property string arrowDownHollowSym: "\uf354";
	readonly property string botSym: "\uf544";
	readonly property string inboxInSym: "\uf310";
	readonly property string downloadSym: "\uf019";
	readonly property string inboxOutSym: "\uf311";
	readonly property string exchangeLightSym: "\uf0ec";
	readonly property string shevronUpSym: "\uf077";
	readonly property string shevronDownSym: "\uf078";
	readonly property string playCircleSym: "\uf144";
	readonly property string pauseCircleSym: "\uf28b";
	readonly property string analyticsSym: "\uf643";
	readonly property string speedNormalSym: "\uf62c";
	readonly property string speedFastestSym: "\uf62b";

	readonly property string sunSym: "\uf185";
	readonly property string moonSym: "\uf186";
	readonly property string cameraSym: "\uf030";
	readonly property string folderSym: "\uf07b";
	readonly property string cancelSym: "\uf00d";
	readonly property string userAliasSym: "\uf504";
	readonly property string featherSym: "\uf56b";
	readonly property string userFrameSym: "\uf2bd";

	readonly property string hashSym: "\uf292";
	readonly property string searchSym: "\uf002";
	readonly property string bellSym: "\uf0f3";

	readonly property string chatsSym: "\uf4b6";

	readonly property string replySym: "\uf075";
	readonly property string rebuzzSym: "\uf364"; //f079
	readonly property string rebuzzWithCommantSym: "\uf044";
	readonly property string likeSym: "\uf004";
	readonly property string tipSym: "\uf51e";
	readonly property string heartBeatSym: "\uf21e";

	readonly property string endorseSym: "\uf164";
	readonly property string mistrustSym: "\uf165";
	readonly property string subscribeSym: "\uf055";
	readonly property string unsubscribeSym: "\uf056";

	readonly property string coinSym: "\uf85c";
	readonly property string sendSym: "\uf1d8";

	readonly property string imageSym: "\uf03e";
	readonly property string imagesSym: "\uf302";

	readonly property string flashSym: "\uf0e7";
	readonly property string rotateSym: "\uf2f1";

	readonly property string checkedSym: "\uf00c";
	readonly property string externalLinkSym: "\uf08e";

	readonly property string moveSym: "\uf0b2";
	readonly property string zoomSym: "\uf002";

	readonly property string caretUpSym: "\uf0d8";
	readonly property string autoFitSym: "\uf78c";

	readonly property string richEditSym: "\uf1ea";
	readonly property string leftArrowSym: "\uf060";
	readonly property string elipsisVerticalSym: "\uf142";

	readonly property string blockSym: "\uf1b2";
	readonly property string hourglassHalfSym: "\uf252";
	readonly property string hash2Sym: "\uf3ef";
	readonly property string expandDownSym: "\uf32d";
	readonly property string expandSimpleDownSym: "\uf0d7";
	readonly property string targetMapSym: "\uf276";

	readonly property string userClockSym: "\uf4fd";
	readonly property string userBanSym: "\uf235";

	readonly property string copySym: "\uf0c5";

	readonly property string serverSym: "\uf233";
	readonly property string passportSym: "\uf5ab";
	readonly property string queuesSym: "\uf5fd";
	readonly property string stopwatchSym: "\uf2f2";
	readonly property string networkSym: "\uf78a";
	readonly property string chainsSym: "\uf1b3";

	readonly property string collapsedSym: "\uf105";
	readonly property string expandedSym: "\uf107";

	readonly property string globeSym: "\uf0ac";

	readonly property string peopleEmojiSym: "\uf118";
	readonly property string natureEmojiSym: "\uf708";
	readonly property string foodEmojiSym: "\uf7f1";
	readonly property string activityEmojiSym: "\uf45e";
	readonly property string placesEmojiSym: "\uf82b";
	readonly property string objectsEmojiSym: "\uf0eb";
	readonly property string symbolsEmojiSym: "\uf004";
	readonly property string flagsEmojiSym: "\uf74c";

	readonly property string paperClipSym: "\uf0c6";

	readonly property string maximizeSym: "\uf2d0";
	readonly property string minimizeSym: "\uf2d1";
	readonly property string restoreSym: "\uf2d2";

	readonly property string microphoneSym: "\uf130";
	readonly property string dotCircleSym: "\uf192";
	readonly property string dotCircle2Sym: "\uf111";
	readonly property string dotCircle3Sym: "\u10f111";

	readonly property string playSym: "\uf04b";
	readonly property string pauseSym: "\uf04c";
	readonly property string stopSym: "\uf04d";
	readonly property string backwardSym: "\uf048";
	readonly property string forwardSym: "\uf051";

	readonly property string videoSym: "\uf03d";
	readonly property string flashLightSym: "\uf8b8";

	readonly property string eraserSym: "\uf12d";
	readonly property string arrowBottomSym: "\uf33d";

	readonly property string shareSym: "\uf1e0";
	readonly property string mediaSym: "\uf87c";
	readonly property string filmSym: "\uf008";
}

