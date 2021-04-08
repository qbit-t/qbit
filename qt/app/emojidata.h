#ifndef EMOJIDATA_H
#define EMOJIDATA_H

#include <QObject>
#include "json.h"

namespace buzzer {

class EmojiData : public QObject
{
	Q_OBJECT
public:
	explicit EmojiData(QObject *parent = nullptr);

	bool open();

	QString emojiByIndex(const std::string& /*category*/, size_t /*row*/, size_t /*column*/, std::string& /*name*/, std::string& /*caption*/);
	void categoryDimensions(const std::string& /*category*/, size_t& /*rows*/, size_t& /*columns*/);
	void updateFavEmojis();

private:
	qbit::json::Document emojiData_;
	std::map<std::string, std::vector<std::vector<std::string>>> emojiMatrix_;
	size_t emojiColumns_ = 6;
};

} // buzzer

#endif // EMOJIDATA_H
