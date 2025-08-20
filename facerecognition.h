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

#ifndef FACERECOGNITION_H
#define FACERECOGNITION_H

#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <QObject>
#include <QString>
#include <QImage>
#include <QCoreApplication>

class FaceRecognition : public QObject {
    Q_OBJECT

public:
    explicit FaceRecognition(QObject *parent = nullptr);
    ~FaceRecognition();

    bool initializeCamera();
    void releaseCamera();
    QImage captureFrame();
    int recognizeFace();
    void trainModel();
    void addFace(int id, const QImage &faceImage);
    bool isModelTrained() const;

signals:
    void faceDetected(int id, const QString &name);

private:
    cv::VideoCapture capture;
    cv::Ptr<cv::face::LBPHFaceRecognizer> model;
    cv::CascadeClassifier faceCascade;
    bool modelTrained;
};

#endif // FACERECOGNITION_H
