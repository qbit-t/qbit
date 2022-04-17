#include "imageqxnode.h"
#include "iapplication.h"

#include <QSGTexture>
#include <QSGTextureMaterial>
#include <QQuickWindow>

//------------------------------------------------------------------------------
buzzer::ImageQxNode::ImageQxNode(int vertexAtCorner)
    : _vertexAtCorner(vertexAtCorner)
    , _segmentCount (4*_vertexAtCorner+3)
{
    QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), _segmentCount);
    geometry->setDrawingMode(QSGGeometry::DrawTriangleFan);
    setGeometry(geometry);

    setFlag(QSGNode::OwnsGeometry);
    setFlag(QSGNode::OwnsOpaqueMaterial);
}

//------------------------------------------------------------------------------
buzzer::ImageQxNode::~ImageQxNode()
{
	if (material()) {
        static_cast<QSGTextureMaterial *>(material())->texture()->deleteLater();
    }
}

void buzzer::ImageQxNode::preprocess() {
	//
}

//------------------------------------------------------------------------------
void buzzer::ImageQxNode::setImage(const QImage &image)
{
	//QSGTexture *texture = nullptr;

	//if (texture) {
	//	texture->deleteLater();
	//}

    if (material()) {
		texture = gApplication->view()->createTextureFromImage(image, QQuickWindow::TextureIsOpaque);
        auto *material = static_cast<QSGTextureMaterial*>(this->material());
        material->texture()->deleteLater();
        material->setTexture(texture);
    } else {
		texture = gApplication->view()->createTextureFromImage(image);
		auto *material = new QSGOpaqueTextureMaterial;
        material->setTexture(texture);
        material->setFiltering(QSGTexture::Linear);
        material->setMipmapFiltering(QSGTexture::Linear);

		setMaterial(material);
		//setOpaqueMaterial(material);
	}
}

//------------------------------------------------------------------------------
void buzzer::ImageQxNode::setMipmap(bool mipmap)
{
    _mipmap = mipmap;

    auto *material = static_cast<QSGTextureMaterial*>(this->material());
    if (material) {
        material->setMipmapFiltering(_mipmap ? QSGTexture::Linear : QSGTexture::None);
    }
}
