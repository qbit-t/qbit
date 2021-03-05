#ifndef EMOJIMODEL_H
#define EMOJIMODEL_H

#include <QObject>
#include <QAbstractTableModel>

namespace buzzer {

class EmojiModel : public QAbstractTableModel {
	Q_OBJECT

	Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)

public:
	enum EmojiDataRoles {
		SymbolRole = Qt::UserRole + 1,
		NameRole,
		CaptionRole
	};

public:
	EmojiModel(QObject *parent = nullptr);

	Q_INVOKABLE int rows();

	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
	bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;

	Q_INVOKABLE void resetModel() {
		beginResetModel();
		endResetModel();
	}

	QString category() {
		return category_;
	}

	void setCategory(const QString& category) {
		category_ = category;
		emit categoryChanged();
	}

signals:
	void categoryChanged();

protected:
	QHash<int, QByteArray> roleNames() const override;

private:
	QString category_;
};

} // buzzer

#endif // EMOJIMODEL_H
