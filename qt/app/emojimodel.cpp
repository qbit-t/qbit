#include "emojimodel.h"
#include "client.h"
#include "iapplication.h"

using namespace buzzer;

EmojiModel::EmojiModel(QObject *parent): QAbstractTableModel(parent) {
	//
}

QHash<int, QByteArray> EmojiModel::roleNames() const {
	//
	QHash<int, QByteArray> lRoles;
	lRoles[SymbolRole] = "symbol";
	lRoles[NameRole] = "name";
	lRoles[CaptionRole] = "caption";

	return lRoles;
}

int EmojiModel::rows() {
	//
	QModelIndex lIndex;
	return rowCount(lIndex);
}

int EmojiModel::rowCount(const QModelIndex &/*parent*/) const {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	EmojiData* lEmojiData = lClient->emojiData();

	size_t lRows, lCols;
	lEmojiData->categoryDimensions(category_.toStdString(), lRows, lCols);
	return lRows;
}

int EmojiModel::columnCount(const QModelIndex &/*parent*/) const {
	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	EmojiData* lEmojiData = lClient->emojiData();

	size_t lRows, lCols;
	lEmojiData->categoryDimensions(category_.toStdString(), lRows, lCols);
	return lCols;
}

QVariant EmojiModel::data(const QModelIndex &index, int role) const {
	//
	if (!index.isValid())
		return QVariant();

	//
	Client* lClient = static_cast<Client*>(gApplication->getClient());
	EmojiData* lEmojiData = lClient->emojiData();

	size_t lRows, lCols;
	lEmojiData->categoryDimensions(category_.toStdString(), lRows, lCols);

	if (index.row() >= (int)lRows || index.row() < 0)
		return QVariant();

	std::string lName;
	std::string lCaption;
	QString lEmoji = lEmojiData->emojiByIndex(category_.toStdString(), index.row(), index.column(), lName, lCaption);

	if (role == SymbolRole) {
		//
		return lEmoji;
	} else if (role == NameRole) {
		//
		return QString::fromStdString(lName);
	} else if (role == CaptionRole) {
		//
		return QString::fromStdString(lCaption);
	}

	return QVariant();
}

QVariant EmojiModel::headerData(int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const {
	return QVariant();
}

bool EmojiModel::insertRows(int /*position*/, int /*rows*/, const QModelIndex &/*index*/) {
	return true;
}

bool EmojiModel::removeRows(int /*position*/, int /*rows*/, const QModelIndex &/*index*/) {
	return true;
}

bool EmojiModel::setData(const QModelIndex &/*index*/, const QVariant &/*value*/, int /*role*/) {
	return true;
}

Qt::ItemFlags EmojiModel::flags(const QModelIndex &index) const {
	if (!index.isValid())
		return Qt::ItemIsEnabled;

	return QAbstractTableModel::flags(index)/* | Qt::ItemIsEditable*/;
}
