#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QByteArray>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnConnect_clicked();
    void on_btnStartTest_clicked();
    void readSerialData();
    void finishAgingTest();
    void on_comboBaudRate_currentTextChanged(const QString &arg1);

    // Cập nhật thông số hiển thị mỗi giây
    void updateDashboard();

private:
    Ui::MainWindow *ui;

    QSerialPort *serial;
    QTimer *durationTimer;
    QTimer *statsTimer;
    QFile *csvFile;

    int testStep;
    QByteArray responseBuffer;

    // --- CÁC BIẾN ĐO LƯỜNG AGING TEST ---
    long totalSeconds;
    long writeSentTotal;
    long readSentTotal;
    long successPackets;
    long lcdPackets;
    long failPackets;

    // Biến đo lường tốc độ nhận (Receive Speed)
    long rxPacketsTotal;     // Tổng số gói tin (dòng) thực tế đã nhận
    long lastRxPacketsTotal; // Biến lưu tạm để tính tốc độ tức thời mỗi giây

    void sendHexCommand(uint8_t cmd);
    void logToCSV(const QString &action, const QString &status, const QString &note);
};

#endif // MAINWINDOW_H
