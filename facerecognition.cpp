/**
*     Yet Another Sign Up - A new sign up system for class managements
 *     Copyright (C) 2025  知念夏世 <chart11from21@outlook.com>

 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.

 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.

 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "facerecognition.h"
#include <QDir>
#include <QFile>
#include <QDebug>

FaceRecognition::FaceRecognition(QObject *parent) : QObject(parent), modelTrained(false) {
    // Load the face cascade classifier
    QString cascadePath = QCoreApplication::applicationDirPath() + 
                         QDir::separator() + "haarcascade_frontalface_default.xml";
    
    if (!faceCascade.load(cascadePath.toStdString())) {
        qDebug() << "Error loading face cascade. Please ensure haarcascade_frontalface_default.xml is in the application directory.";
        // You can download this file from OpenCV GitHub repository
    }
    
    // Initialize the face recognizer
    model = cv::face::LBPHFaceRecognizer::create();
}

FaceRecognition::~FaceRecognition() {
    releaseCamera();
}

bool FaceRecognition::initializeCamera() {
    if (!capture.isOpened()) {
        capture.open(0); // Open default camera
        if (!capture.isOpened()) {
            qDebug() << "Failed to open camera";
            return false;
        }
        // Set camera properties
        capture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        capture.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
        capture.set(cv::CAP_PROP_FPS, 30);
    }
    return true;
}

void FaceRecognition::releaseCamera() {
    if (capture.isOpened()) {
        capture.release();
    }
}

QImage FaceRecognition::captureFrame() {
    cv::Mat frame;
    if (capture.isOpened() && capture.read(frame)) {
        // Convert BGR to RGB for QImage
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
        return QImage(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
    }
    return QImage();
}

int FaceRecognition::recognizeFace() {
    if (!modelTrained) {
        qDebug() << "Face recognition model not trained";
        return -1;
    }

    cv::Mat frame;
    if (!capture.read(frame)) {
        return -1;
    }

    // Convert to grayscale for face detection
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    // Detect faces
    std::vector<cv::Rect> faces;
    faceCascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(30, 30));

    if (faces.empty()) {
        return -1;
    }

    // For each detected face, try to recognize
    for (const auto& face : faces) {
        cv::Mat faceROI = gray(face);
        cv::resize(faceROI, faceROI, cv::Size(100, 100));
        
        int predictedLabel = -1;
        double confidence = 0.0;
        model->predict(faceROI, predictedLabel, confidence);
        
        // If confidence is good, return the predicted ID
        if (confidence < 100.0) { // Lower confidence is better
            return predictedLabel;
        }
    }

    return -1;
}

void FaceRecognition::trainModel() {
    // This should load training data from a directory
    // For now, we'll implement a basic training mechanism
    QString trainingDir = QCoreApplication::applicationDirPath() + 
                         QDir::separator() + "faces";
    
    QDir dir(trainingDir);
    if (!dir.exists()) {
        dir.mkpath(trainingDir);
        qDebug() << "Created faces directory for training data";
        return;
    }

    std::vector<cv::Mat> images;
    std::vector<int> labels;

    QStringList filters;
    filters << "*.jpg" << "*.png" << "*.jpeg";
    dir.setNameFilters(filters);

    QFileInfoList fileList = dir.entryInfoList();
    for (const QFileInfo& fileInfo : fileList) {
        cv::Mat image = cv::imread(fileInfo.absoluteFilePath().toStdString(), cv::IMREAD_GRAYSCALE);
        if (!image.empty()) {
            // Extract ID from filename (format: id_number.jpg)
            QString filename = fileInfo.baseName();
            QStringList parts = filename.split('_');
            if (parts.size() >= 2 && parts[0] == "id") {
                bool ok;
                int id = parts[1].toInt(&ok);
                if (ok) {
                    cv::resize(image, image, cv::Size(100, 100));
                    images.push_back(image);
                    labels.push_back(id);
                }
            }
        }
    }

    if (!images.empty()) {
        model->train(images, labels);
        modelTrained = true;
        qDebug() << "Face recognition model trained with" << images.size() << "images";
    } else {
        qDebug() << "No training images found in faces directory";
    }
}

void FaceRecognition::addFace(int id, const QImage &faceImage) {
    // Convert QImage to cv::Mat
    cv::Mat mat(faceImage.height(), faceImage.width(), CV_8UC3, 
                const_cast<uchar*>(faceImage.bits()), faceImage.bytesPerLine());
    
    // Convert RGB to BGR
    cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
    
    // Convert to grayscale
    cv::Mat gray;
    cv::cvtColor(mat, gray, cv::COLOR_BGR2GRAY);
    
    // Save the face image
    QString filename = QCoreApplication::applicationDirPath() + 
                      QDir::separator() + "faces" + 
                      QDir::separator() + "id_" + QString::number(id) + ".jpg";
    
    cv::imwrite(filename.toStdString(), gray);
    
    // Retrain the model
    trainModel();
}

bool FaceRecognition::isModelTrained() const {
    return modelTrained;
}
