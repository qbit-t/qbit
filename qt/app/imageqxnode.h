#pragma once

#include <QSGGeometryNode>
#include <QImage>
#include <QSGTexture>

namespace buzzer {
	class ImageQxNode: public QSGGeometryNode
    {
        public:
			ImageQxNode(int vertexAtCorner = 20);
			~ImageQxNode();

            void setImage(const QImage &image);
            void setMipmap(bool mipmap);

			void preprocess();

        protected:
            int _vertexAtCorner;
            int _segmentCount;
            bool _mipmap = true;

		private:
			QSGTexture* texture = nullptr;
    };
} // namespace sp {
