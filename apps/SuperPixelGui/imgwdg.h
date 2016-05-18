#ifndef IMGWDG_H
#define IMGWDG_H

#include <memory>
#include <opencv2/core/core.hpp>
#include <QPixmap>
#include <QImage>
#include <QWidget>
#include <QLabel>
#include <array>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QResizeEvent>
#include "UndoManager.h"

namespace priv {
	template<typename T>
		class TripleImageConversionData;

	class TripleImageData {
		public:
			TripleImageData() : reference(0) {
			}

			virtual ~TripleImageData() {
			}

			virtual const std::type_info &type_info() const = 0;

			void increment() {
				reference++;
			}

			void decrement() {
				reference--;
			}

			bool zeroReference() {
				return reference == 0;
			}

			int refCount() const{return reference;}

			template<typename T>
				std::unique_ptr<T> & getData();

			template<typename T>
				const std::unique_ptr<T> & getData() const;

		private:
			int reference;
	};

	template<typename T>
		class TripleImageConversionData : public TripleImageData {
			public:
				TripleImageConversionData(std::unique_ptr<T> && d) {
					data = std::move(d);
				}

				~TripleImageConversionData(){
				}

				std::unique_ptr<T> & getData() {
					return data;
				}

				const std::unique_ptr<T> & getData() const{
					return data;
				}

				virtual const std::type_info &type_info() const {
					return typeid(T);
				}

			private:
				std::unique_ptr<T> data;
		};


	template<typename T>
		std::unique_ptr<T> &TripleImageData::getData() {
			static std::unique_ptr<T> nullp;
			if (type_info() == typeid(T))
				return static_cast<TripleImageConversionData<T> *>(this)->getData();
			return nullp;
		}
	template<typename T>
		const std::unique_ptr<T>& TripleImageData::getData() const{
			const static std::unique_ptr<T> nullp;
			if (type_info() == typeid(T))
				return static_cast<TripleImageConversionData<T> *>(this)->getData();
			return nullp;
		}
}

// Automatically convert between QImage, QPixmap and cv::Mat
class TripleImageConversion {
	public:
		TripleImageConversion();

		TripleImageConversion(const TripleImageConversion &other) : img(nullptr), mat(nullptr), pxm(other.pxm) {
			if (other.img != nullptr) img = new QImage(*other.img);
			if (other.mat != nullptr) mat = new cv::Mat(*other.mat);
			if (other.data != nullptr) {
				data = other.data;
				data->increment();
			}
		}

		TripleImageConversion &operator=(const TripleImageConversion &other) {
			clean();
			pxm = other.pxm.copy();
			if (other.img != nullptr) {
				img = new QImage(*other.img);
			}
			if (other.mat != nullptr) {
				mat = new cv::Mat(*other.mat);
			}
			if (other.data != nullptr) {
				data = other.data;
				data->increment();
			}
			return *this;
		}

		void setImg(const QImage &img);

		void setImg(const QPixmap &px);

		void setImg(const cv::Mat &mat);

		bool isSet() const;

		void clean();

		template<typename T>
			void setMoreData(std::unique_ptr<T> && p) {
				if (data != nullptr) {
					data->decrement();
					if (data->zeroReference()) delete data;
				}
				data = new priv::TripleImageConversionData<T>(std::move(p));
				data->increment();
			};

		template<typename T>
			const std::unique_ptr<T> & moreData() const{
				static std::unique_ptr<T> nullp;
				if (data != nullptr) {
					return data->getData<T>();
				}
				return nullp;
			}

		template<typename T>
			std::unique_ptr<T> & moreData() {
				static std::unique_ptr<T> nullp;
				if (data != nullptr) {
					if (data->refCount()>1){
						data->decrement();
						data = new priv::TripleImageConversionData<T>(
								std::unique_ptr<T>(new T(*data->getData<T>()))
								);
						data->increment();
					}
					return data->getData<T>();
				}
				return nullp;
			}

		QImage getImg();

		cv::Mat getMat();

		QPixmap getPixmap() const;

		virtual ~TripleImageConversion();

	private:
		QPixmap pxm;
		QImage *img;
		cv::Mat *mat;
		priv::TripleImageData *data;
};

// Show images with history (5 images in history)
class ImgWdg : public QGraphicsView {
	Q_OBJECT
	public:
		explicit ImgWdg(QWidget *parent = nullptr);

		ImgWdg(ImgWdg *other, QWidget *parent = nullptr);

		void showImg(const QImage &img);

		void showImg(const QPixmap &px);

		void showImg(const cv::Mat &mat);

		void replaceImg(const QImage & img);

		void replaceImg(const QPixmap & px);

		void replaceImg(const cv::Mat & mat);

		void reloadPixmap();

		QImage getImg();

		cv::Mat getMat();

		QPixmap getPixmap() const;

		virtual ~ImgWdg();

		TripleImageConversion *currentImage();

		const TripleImageConversion *currentImage() const;

		bool hasNext();

		bool hasPrev();

	public slots:
		void goNext();

		void goPrev();

	protected:
		TripleImageConversion *createOne();

		void resizeEvent(QResizeEvent *event);

	private:
		void init();
		QGraphicsScene *scene;
		QGraphicsPixmapItem *pitem;
		UndoManager<TripleImageConversion, 5> history;
};

#endif // IMGWDG_H
