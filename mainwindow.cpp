#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , testStep(0)
    , csvFile(nullptr)
{
    ui->setupUi(this);
    serial = new QSerialPort(this);
    durationTimer = new QTimer(this);
    statsTimer = new QTimer(this);

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        ui->comboPort->addItem(info.portName());
    }

    ui->comboPort->setCurrentText("COM5");

    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
    connect(durationTimer, &QTimer::timeout, this, &MainWindow::finishAgingTest);
    connect(statsTimer, &QTimer::timeout, this, &MainWindow::updateDashboard);
}

MainWindow::~MainWindow()
{
    if(serial->isOpen()) serial->close();
    if(csvFile && csvFile->isOpen()) csvFile->close();
    delete ui;
}

void MainWindow::on_btnConnect_clicked()
{
    if (serial->isOpen()) {
        serial->close();
        ui->btnConnect->setText("Kết nối");
        ui->btnStartTest->setEnabled(false);
        ui->textLog->append("Đã ngắt kết nối.");
    } else {
        serial->setPortName(ui->comboPort->currentText());

        serial->setBaudRate(QSerialPort::Baud9600);
        ui->comboBaudRate->setCurrentText("9600");

        serial->setDataBits(QSerialPort::Data8);
        serial->setParity(QSerialPort::NoParity);
        serial->setStopBits(QSerialPort::OneStop);

        if (serial->open(QIODevice::ReadWrite)) {
            ui->btnConnect->setText("Ngắt kết nối");
            ui->btnStartTest->setEnabled(true);
            ui->textLog->append("Kết nối thành công cổng " + ui->comboPort->currentText());
        } else {
            QMessageBox::critical(this, "Lỗi", "Không thể mở cổng COM!");
        }
    }
}

void MainWindow::on_btnStartTest_clicked()
{
    if (testStep > 0) {
        finishAgingTest();
        return;
    }

    // Reset toàn bộ các biến đo lường
    totalSeconds = 0;
    writeSentTotal = 0;
    readSentTotal = 0;
    successPackets = 0;
    lcdPackets = 0;
    failPackets = 0;
    rxPacketsTotal = 0;
    lastRxPacketsTotal = 0;

    QString filename = "AgingTest_Log_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".csv";
    csvFile = new QFile(filename, this);
    if (csvFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(csvFile);
        // Cập nhật Header CSV với cột tốc độ nhận (RxSpeed)
        out << "Timestamp,Elapsed(s),WriteSent,ReadSent,Success,LCD_Display,Fail,ErrorRate(%),RxSpeed(pkt/min)\n";
    }

    ui->btnStartTest->setText("Dừng Test");
    ui->textLog->clear();
    ui->textLog->append("BẮT ĐẦU CHU TRÌNH KIỂM TRA (1 GIỜ)...");

    durationTimer->start(3600 * 1000);
    statsTimer->start(1000);

    responseBuffer.clear();
    testStep = 1;
    sendHexCommand(0xBB);
    ui->textLog->append("-> Đã gửi lệnh GHI (0xBB). Đang chờ vi điều khiển phản hồi OK...");
}

void MainWindow::sendHexCommand(uint8_t cmd)
{
    QByteArray frame;
    frame.append((char)0xAA);
    frame.append((char)cmd);
    frame.append((char)0x55);
    serial->write(frame);

    if (cmd == 0xBB) writeSentTotal++;
    if (cmd == 0xCC) readSentTotal++;
}

void MainWindow::readSerialData()
{
    if (testStep == 0) return;

    // Đọc phần dữ liệu MỚI NHẤT vừa tới cổng COM
    QByteArray newData = serial->readAll();
    QString newStr = QString::fromLatin1(newData);

    // ĐẾM GÓI TIN TỨC THỜI: Cộng dồn số dòng dữ liệu vừa cập bến để tính tốc độ mượt mà
    rxPacketsTotal += newStr.count("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    rxPacketsTotal += newStr.count("OK");

    if (newStr.contains("?")) {
        failPackets++;
        rxPacketsTotal += newStr.count("?");
    }

    // Đưa dữ liệu mới vào bộ đệm tổng để xử lý logic State Machine
    responseBuffer.append(newData);
    QString dataStr = QString::fromLatin1(responseBuffer);

    // Loại bỏ dấu '?' khỏi bộ đệm để không bắt lỗi lặp lại
    responseBuffer.replace("?", "");

    // TRẠNG THÁI 1: Chờ nhận chữ "OK"
    if (testStep == 1) {
        if (dataStr.contains("OK")) {
            ui->textLog->append("<- Nhận thành công: OK");
            successPackets++;

            responseBuffer.clear();
            testStep = 2;

            ui->textLog->append("-> [Chờ 0.5s] Gửi lệnh ĐỌC (0xCC)...");
            QTimer::singleShot(500, this, [=]() { sendHexCommand(0xCC); });
        }
    }
    // TRẠNG THÁI 2: Chờ nhận bảng chữ cái
    else if (testStep == 2) {
        if (dataStr.count("ABCDEFGHIJKLMNOPQRSTUVWXYZ") >= 100) {
            ui->textLog->append("<- Đã nhận thành công đủ 100 dòng dữ liệu.");
            successPackets++;
            lcdPackets += 100;

            responseBuffer.clear();
            testStep = 1;

            ui->textLog->append("--------------------------------------------------");
            ui->textLog->append("-> [Chờ 0.5s] Gửi lệnh GHI (0xBB)...");
            QTimer::singleShot(500, this, [=]() { sendHexCommand(0xBB); });
        }
    }
}

void MainWindow::updateDashboard()
{
    totalSeconds++;

    // 1. Tính thời gian còn lại (1 giờ = 3600 giây)
    long remainingSeconds = 3600 - totalSeconds;
    if (remainingSeconds < 0) remainingSeconds = 0;

    // Chuyển đổi giây thành định dạng Phút:Giây cho dễ nhìn
    int mins = remainingSeconds / 60;
    int secs = remainingSeconds % 60;
    QString timeStr = QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));

    // 2. Các phần tính toán tốc độ và tỉ lệ lỗi giữ nguyên...
    long rxSpeedSec = rxPacketsTotal - lastRxPacketsTotal;
    lastRxPacketsTotal = rxPacketsTotal;
    long rxSpeedMin = rxSpeedSec * 60;

    long totalCommands = writeSentTotal + readSentTotal;
    double errorRate = (totalCommands > 0) ? ((double)failPackets / totalCommands) * 100.0 : 0.0;

    // 3. Cập nhật dòng hiển thị mới (thay 🕒 %1s bằng ⏳ Còn lại: %1)
    QString stats = QString("⏳ Còn lại: %1 | 📥 Tốc độ nhận: %2 gói/p | ✅ OK: %3 | 🖥️ LCD: %4 | ❌ Lỗi: %5 (%6%)")
                        .arg(timeStr).arg(rxSpeedMin)
                        .arg(successPackets).arg(lcdPackets)
                        .arg(failPackets).arg(errorRate, 0, 'f', 2);

    ui->statusbar->showMessage(stats);

    // 4. Phần ghi CSV giữ nguyên (vẫn nên ghi totalSeconds để làm biểu đồ thời gian thực)
    if (csvFile && csvFile->isOpen()) {
        QTextStream out(csvFile);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        out << timestamp << "," << totalSeconds << "," << writeSentTotal << ","
            << readSentTotal << "," << successPackets << "," << lcdPackets << ","
            << failPackets << "," << QString::number(errorRate, 'f', 2) << ","
            << rxSpeedMin << "\n";
        out.flush();
    }
}

void MainWindow::logToCSV(const QString &action, const QString &status, const QString &note)
{
    // Hàm này để trống vì log đã được chuyển vào updateDashboard
}

void MainWindow::finishAgingTest()
{
    testStep = 0;
    durationTimer->stop();
    statsTimer->stop();
    if(csvFile && csvFile->isOpen()) csvFile->close();

    ui->btnStartTest->setText("Bắt đầu Aging Test");
    ui->textLog->append("==================================");
    ui->textLog->append(QString("ĐÃ HOÀN THÀNH HOẶC DỪNG BÀI KIỂM TRA SAU %1 GIÂY!").arg(totalSeconds));
    ui->textLog->append(QString("Tổng Ghi: %1 | Tổng Đọc: %2 | Thành công: %3 | Sai: %4")
                            .arg(writeSentTotal).arg(readSentTotal).arg(successPackets).arg(failPackets));

    QMessageBox::information(this, "Hoàn thành", "Quá trình kiểm tra lão hóa đã kết thúc!");
}

void MainWindow::on_comboBaudRate_currentTextChanged(const QString &arg1)
{
    if (serial->isOpen()) {
        int newBaudRate = arg1.toInt();
        if (serial->setBaudRate(newBaudRate)) {
            ui->textLog->append("=> Đã thay đổi Baudrate thành: " + arg1);
        } else {
            ui->textLog->append("=> LỖI: Không thể đổi Baudrate phần cứng!");
        }
    }
}
