#include "emojidata.h"
#include "client.h"

#include <QFile>
#include <QString>
#include <QDebug>

#include <iostream>

using namespace buzzer;

EmojiData::EmojiData(QObject *parent) : QObject(parent) {
	//
}

bool EmojiData::open() {
	//
	QFile lRawFile(":/emoji/emoji-google.json");
	lRawFile.open(QIODevice::ReadOnly | QIODevice::Text);

	QByteArray lRawData = lRawFile.readAll();
	std::string lRawStringData = lRawData.toStdString();

	emojiData_.loadFromString(lRawStringData);

	// fill matrix
	qbit::json::Value lCategories = emojiData_["categories"];
	for (size_t lIdx = 0; lIdx < lCategories.size(); lIdx++) {
		//
		qbit::json::Value lProperties = lCategories[lIdx];
		qbit::json::Value lId = lProperties["id"];
		qbit::json::Value lEmojis = lProperties["emojis"];

		std::vector<std::string> lRow;
		for (size_t lEmojiIdx = 0; lEmojiIdx < lEmojis.size(); lEmojiIdx++) {
			//
			qbit::json::Value lEmoji = lEmojis[lEmojiIdx];
			if (lRow.size() < emojiColumns_) {
				lRow.push_back(lEmoji.getString());
			} else {
				emojiMatrix_[lId.getString()].push_back(lRow);
				lRow.clear();
				lRow.push_back(lEmoji.getString());
			}
		}

		if (lRow.size()) emojiMatrix_[lId.getString()].push_back(lRow);
	}

	updateFavEmojis();

	return true;
}

QString EmojiData::emojiByIndex(const std::string& category, size_t row, size_t column, std::string& name, std::string& caption) {
	//
	std::map<std::string, std::vector<std::vector<std::string>>>::iterator lCategory = emojiMatrix_.find(category);
	if (lCategory != emojiMatrix_.end()) {
		//
		if (lCategory->second.size() > row && lCategory->second[row].size() > column) {
			std::string lEmoji = lCategory->second[row][column];

			qbit::json::Value lRoot = emojiData_["emojis"];
			qbit::json::Value lEmojiObject = lRoot[lEmoji];

			QString lBytesString = QString::fromStdString(lEmojiObject["b"].getString());
			QStringList lParts = lBytesString.split("-");
			bool lOverflow = false;

			int lIdx = 0;
			std::vector<unsigned int> lResult; lResult.resize(lParts.size(), 0);
			for (QStringList::iterator lPart = lParts.begin(); lPart != lParts.end(); lPart++) {
				lResult[lIdx++] = lPart->toUInt(&lOverflow, 16);
			}

			QString lEncodedSymbol = QString::fromUcs4(&lResult[0], lResult.size());
			//qInfo() << lEncodedSymbol << QString::fromStdString(lEmoji) << lResult.size();

			name = lEmoji;
			caption = lEmojiObject["a"].getString();

			return lEncodedSymbol;
		}
	}

	return QString();
}

void EmojiData::categoryDimensions(const std::string& category, size_t& rows, size_t& columns) {
	//
	std::map<std::string, std::vector<std::vector<std::string>>>::iterator lCategory = emojiMatrix_.find(category);
	if (lCategory != emojiMatrix_.end()) {
		//
		rows = lCategory->second.size();
		columns = emojiColumns_;
	}
}

void EmojiData::updateFavEmojis() {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());

	std::vector<std::string> lFavs;
	lClient->extractFavoriteEmojis(lFavs);

	// clean-up
	emojiMatrix_.erase("favorites");

	// fill up
	std::vector<std::string> lRow;
	for (size_t lEmojiIdx = 0; lEmojiIdx < lFavs.size(); lEmojiIdx++) {
		//
		std::string lEmoji = lFavs[lEmojiIdx];
		if (lRow.size() < emojiColumns_) {
			lRow.push_back(lEmoji);
		} else {
			emojiMatrix_["favorites"].push_back(lRow);
			lRow.clear();
			lRow.push_back(lEmoji);
		}
	}

	if (lRow.size()) emojiMatrix_["favorites"].push_back(lRow);
}
