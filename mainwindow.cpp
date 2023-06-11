
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <sstream>
#include <bitset>
#include <vector>
#include <QMessageBox>
#include <QFileDialog>
#include <QDataStream>
#include <QDesktopServices>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_Cypher_clicked()
{
    bool isRS4RadiobuttonPressed = ui->RS4radioButton->isChecked();
    bool isAESRadiobuttonPressed = ui->AESradioButton->isChecked();
    bool isTextRadiobuttonPressed = ui->TextRadioButton->isChecked();
    bool isImageRadiobuttonPressed = ui->ImageRadioButton->isChecked();
    if(isRS4RadiobuttonPressed == true && isTextRadiobuttonPressed == true){
        QString p_text = ui->InputText->toPlainText();
        QString key = ui->Key->text();
        QVector<int>s(256);
        for (int i = 0; i < 256; ++i)
            s[i] = i;

        int j = 0;
        for (int i = 0; i < 256; ++i)
        {
            j = (j + s[i] + (key[i % key.length()].unicode())) % 256;
            std::swap(s[i], s[j]);
        }

        int m = 0; int n = 0;
        QVector<int>text;
        for (int c = 0; c < p_text.length(); ++c)
        {
            m = (m + 1) % 256;
            n = (n + s[m]) % 256;
            std::swap(s[m], s[n]);
            int t = (s[m] + s[n]) % 256;

            ushort unicode = p_text[c].unicode();
            int value;
            if (unicode<127){
                value = static_cast<int>(unicode);
            }
            else { //для кириллицы
                uchar cp1251 = unicode - 0x0350;
                if (cp1251 == 1) cp1251=184; //для правильного отображения символа "ё"
                if (cp1251 == 177) cp1251=168; // //для правильного отображения символа "Ё"
                value = static_cast<int>(cp1251);
            }
            int r = value^s[t];
            text.push_back(r);
        }
        QString c_text = text_vector(text);
        ui->OutputText->setPlainText(c_text);
    }
    if(isAESRadiobuttonPressed == true && isTextRadiobuttonPressed == true){
        QString p_text = ui->InputText->toPlainText();
        p_text = text_padding(p_text);
        QVector<int>text = vector_text(p_text);

        QString enter_key = ui->Key->text();
        if (enter_key.length() > 16)
        {
            QMessageBox::warning(nullptr, "Warning!", "Too long Key!");
            return;
        }
        QString bin_key = bit_mask(enter_key);
        bin_key = bin_padding(bin_key);
        QVector<int>key = to_key(bin_key);

        int k = text.size() / 16;

        for (int p = 0; p < k; ++p){
            addRondKey(text, key, p);
        }
        for (int i = 0; i < 9; ++i){
            roundKey(key, i);
            for (int p = 0; p < k; ++p){
                subBytes(text, p);
                shiftRows(text, p);
                mixColumns(text, p);
                addRondKey(text, key, p);
            }
        }
        roundKey(key, 9);
        for (int p = 0; p < k; ++p)
        {
            subBytes(text, p);
            shiftRows(text, p);
            addRondKey(text, key, p);
        }
        QString c_text = text_vector(text);

        ui->OutputText->setPlainText(c_text);
    }
    if(isRS4RadiobuttonPressed == true && isImageRadiobuttonPressed == true){
        QFileDialog fileDialog;
        fileDialog.setNameFilter("BMP files (*.bmp)");
        if (fileDialog.exec()){
            QString fileName = fileDialog.selectedFiles()[0];
            QFileInfo fileInfo(fileName);
            QString newFileName = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "_copy." + fileInfo.completeSuffix();

            QFile file(fileName);
            file.open(QIODeviceBase::ReadOnly);
            qint64 fileSize = file.size();
            QByteArray buffer = file.read(fileSize);
            char* charBuffer = buffer.data();

            QString key = ui->Key->text();
            QVector<int>s(256);
            for (int i = 0; i < 256; ++i)
                s[i] = i;

            int j = 0;
            for (int i = 0; i < 256; ++i)
            {
                j = (j + s[i] + (key[i % key.length()].unicode())) % 256;
                std::swap(s[i], s[j]);
            }

            int m = 0; int n = 0;
            for (int c = 150; c < fileSize; ++c)
            {
                m = (m + 1) % 256;
                n = (n + s[m]) % 256;
                std::swap(s[m], s[n]);
                short t = (s[m] + s[n]) % 256;
                short k = s[t];
                charBuffer[c] = charBuffer[c] ^ s[t];
            }
            file.close();

            ui->OutputText->setText("Done!");

            QFile ofile(newFileName);
            ofile.open(QIODevice::WriteOnly);
            QDataStream out(&ofile);
            out.writeRawData(charBuffer, fileSize);
            ofile.close();

            QDesktopServices::openUrl(QUrl::fromLocalFile(newFileName));
        }
        else return;
    }
    if(isAESRadiobuttonPressed == true && isImageRadiobuttonPressed == true){
        QString enter_key = ui->Key->text();
        if (enter_key.length() > 16){
            QMessageBox::warning(nullptr, "Warning!", "Too long Key!");
            return;
        }
        QString bin_key = bit_mask(enter_key);
        bin_key = bin_padding(bin_key);
        QVector<int>key = to_key(bin_key);

        QFileDialog fileDialog;
        fileDialog.setNameFilter("BMP files (*.bmp)");
        if (fileDialog.exec()){
            QString fileName = fileDialog.selectedFiles()[0];
            QFileInfo fileInfo(fileName);
            QString newFileName = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "_copy." + fileInfo.completeSuffix();

            QFile file(fileName);
            file.open(QIODeviceBase::ReadOnly);
            qint64 fileSize = file.size();
            QByteArray buffer = file.read(fileSize);
            char* b = buffer.data();

            int beg = 150;// searchBGRs(b, l);
            int k = (fileSize - beg) / 16;
            file.close();

            for (int p = 0; p < k; ++p){
                i_addRoundkey(b, key, beg, p);
            }

            for (int i = 0; i < 9; ++i){
                i_roundKey(key, i);
                for (int p = 0; p < k; ++p){
                    i_subBytes(b, beg, p);
                    i_shiftRows(b, beg, p);
                    i_mixColumns(b, beg, p);
                    i_addRoundkey(b, key, beg, p);
                }
            }

            i_roundKey(key, 9);

            for (int p = 0; p < k; ++p){
                i_subBytes(b, beg, p);
                i_shiftRows(b, beg, p);
                i_addRoundkey(b, key, beg, p);
            }

            QFile ofile(newFileName);
            ofile.open(QIODevice::WriteOnly);
            QDataStream out(&ofile);
            out.writeRawData(b, fileSize);
            ofile.close();

            QDesktopServices::openUrl(QUrl::fromLocalFile(newFileName));
        }
        else return;
    }
}
void MainWindow::on_Decypher_clicked()
{
    bool isRS4RadiobuttonPressed = ui->RS4radioButton->isChecked();
    bool isAESRadiobuttonPressed = ui->AESradioButton->isChecked();
    bool isTextRadiobuttonPressed = ui->TextRadioButton->isChecked();
    bool isImageRadiobuttonPressed = ui->ImageRadioButton->isChecked();
    if(isRS4RadiobuttonPressed == true && isTextRadiobuttonPressed == true){
        QString cypher_text = ui->InputText->toPlainText();
        QVector<int>c_text = forDecypher_vector_text(cypher_text);

        QString key = ui->Key->text();
        QVector<int>s(256);
        for (int i = 0; i < 256; ++i)
            s[i] = i;

        int j = 0;
        for (int i = 0; i < 256; ++i)
        {
            j = (j + s[i] + (key[i % key.length()].unicode())) % 256;
            std::swap(s[i], s[j]);
        }

        int m = 0; int n = 0;
        QVector<int>text;
        for (int c = 0; c < c_text.size(); ++c)
        {
            m = (m + 1) % 256;
            n = (n + s[m]) % 256;
            std::swap(s[m], s[n]);
            int t = (s[m] + s[n]) % 256;
            int k = s[t];
            int r = c_text[c] ^ s[t];
            text.push_back(r);
        }
        QString d_text = decyphered_text(text);
        ui->OutputText->setPlainText(d_text);
    }
    if(isAESRadiobuttonPressed == true && isTextRadiobuttonPressed == true){
        QVector<int>d_key(16);
        QString enter_key = ui->Key->text();
        if (enter_key.length() > 16)
        {
            QMessageBox::warning(nullptr, "Warning!", "Too long Key!");
            return;
        }
        QString bin_key = bit_mask(enter_key);
        bin_key = bin_padding(bin_key);
        QVector<int>key = to_key(bin_key);

        QString cypher_text = ui->InputText->toPlainText();
        QVector<int>c_text = forDecypher_vector_text(cypher_text);

        int k = c_text.size() / 16;

        for (int j = 0; j < 16; ++j) d_key[j] = key[j];
        for (int i = 0; i < 10; ++i){
            roundKey(d_key, i);
        }
        for (int p = 0; p < k; ++p)
        {
            addRondKey(c_text, d_key, p);
            inv_shiftRows(c_text, p);
            inv_subBytes(c_text, p);
        }
        for (int r = 9; r >= 1; --r){
            for (int j = 0; j < 16; ++j) d_key[j] = key[j];
            for (int i = 0; i < r; ++i){
                roundKey(d_key, i);
            }
            for (int p = 0; p < k; ++p){
                addRondKey(c_text, d_key, p);
                inv_mixColumns(c_text, p);
                inv_shiftRows(c_text, p);
                inv_subBytes(c_text, p);
            }
        }
        for (int p = 0; p < k; ++p){
            addRondKey(c_text, key, p);
        }
        QString d_text = decyphered_text(c_text);

        short counter = 0;
        for(int i = d_text.length()-15; i<d_text.length(); i++){
            if(d_text[i]=='$'){
                for(int j=i+1; j<d_text.length(); j++){
                    if(d_text[j]=='0'){
                        counter++;
                    }
                }
            }
        }
        d_text.chop(counter+1);
        ui->OutputText->setPlainText(d_text);
    }
    if(isRS4RadiobuttonPressed == true && isImageRadiobuttonPressed == true){
        QFileDialog fileDialog;
        fileDialog.setNameFilter("BMP files (*.bmp)");
        if (fileDialog.exec()){
            QString fileName = fileDialog.selectedFiles()[0];
            QFileInfo fileInfo(fileName);
            QString newFileName = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "_copy." + fileInfo.completeSuffix();

            QFile file(fileName);
            file.open(QIODeviceBase::ReadOnly);
            qint64 fileSize = file.size();
            QByteArray buffer = file.read(fileSize);
            char* charBuffer = buffer.data();

            QString key = ui->Key->text();
            QVector<int>s(256);
            for (int i = 0; i < 256; ++i)
                s[i] = i;

            int j = 0;
            for (int i = 0; i < 256; ++i)
            {
                j = (j + s[i] + (key[i % key.length()].unicode())) % 256;
                std::swap(s[i], s[j]);
            }

            int m = 0; int n = 0;
            for (int c = 150; c < fileSize; ++c)
            {
                m = (m + 1) % 256;
                n = (n + s[m]) % 256;
                std::swap(s[m], s[n]);
                short t = (s[m] + s[n]) % 256;
                short k = s[t];
                charBuffer[c] = charBuffer[c] ^ s[t];
            }
            file.close();

            ui->OutputText->setText("Done!");

            QFile ofile(newFileName);
            ofile.open(QIODevice::WriteOnly);
            QDataStream out(&ofile);
            out.writeRawData(charBuffer, fileSize);
            ofile.close();

            QDesktopServices::openUrl(QUrl::fromLocalFile(newFileName));
        }
        else return;
    }
    if(isAESRadiobuttonPressed == true && isImageRadiobuttonPressed == true){
        QString enter_key = ui->Key->text();
        if (enter_key.length() > 16){
            QMessageBox::warning(nullptr, "Warning!", "Too long Key!");
            return;
        }
        QString bin_key = bit_mask(enter_key);
        bin_key = bin_padding(bin_key);
        QVector<int>key = to_key(bin_key);

        QFileDialog fileDialog;
        fileDialog.setNameFilter("BMP files (*.bmp)");
        if (fileDialog.exec()){
            QString fileName = fileDialog.selectedFiles()[0];
            QFileInfo fileInfo(fileName);
            QString newFileName = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "_copy." + fileInfo.completeSuffix();

            QFile file(fileName);
            file.open(QIODeviceBase::ReadOnly);
            qint64 fileSize = file.size();
            QByteArray buffer = file.read(fileSize);
            char* b = buffer.data();

            int beg = 150;// searchBGRs(b, l);
            int k = (fileSize - beg) / 16;
            file.close();

            QVector<int>d_key(16);
            for (int j = 0; j < 16; ++j) d_key[j] = key[j];
            for (int i = 0; i < 10; ++i){
                i_roundKey(d_key, i);
            }

            for (int p = 0; p < k; ++p){
               i_addRoundkey(b, d_key, beg, p);
               i_inv_shiftRows(b, beg, p);
               i_inv_subBytes(b, beg, p);
            }

            for (int r = 9; r >= 1; --r){
                for (int j = 0; j < 16; ++j) {d_key[j] = key[j];}
                for (int i = 0; i < r; ++i){
                    i_roundKey(d_key, i);
                }
                for (int p = 0; p < k; ++p){
                    i_addRoundkey(b, d_key, beg, p);
                    i_inv_mixColumns(b, beg, p);
                    i_inv_shiftRows(b, beg, p);
                    i_inv_subBytes(b, beg, p);
                }
            }

            for (int p = 0; p < k; ++p){
                i_addRoundkey(b, key, beg, p);
            }

            QFile ofile(newFileName);
            ofile.open(QIODevice::WriteOnly);
            QDataStream out(&ofile);
            out.writeRawData(b, fileSize);
            ofile.close();

            QDesktopServices::openUrl(QUrl::fromLocalFile(newFileName));
        }
        else return;
    }
}

//Функции для RS4
QString MainWindow::text_vector(QVector<int>& v){
    QString text;
    for (int i = 0; i < v.size(); ++i){
        std::stringstream ss;
        QString s;
        ss << std::hex << v[i];
        s = QString::fromStdString(ss.str());
        if (s == "1" || s == "2" || s == "3" || s == "4" || s == "5" || s == "6" ||
            s == "7" || s == "8" || s == "9" || s == "a" || s == "b" || s == "c" ||
            s == "d" || s == "e" || s == "f" || s == "0")
            s.insert(0, "0");
        text.append(s);
    }
    return text;
}
QVector<int> MainWindow::forDecypher_vector_text(QString& s) {
    QVector<int> v;
    int len = s.length();
    for (int i = 0; i < len; i += 2) {
        QString s1 = s.mid(i, 2);
        if (s1.length() < 2) {
            break;
        }
        int num1 = s1.toInt(nullptr, 16);
        v.append(num1);
    }
    return v;
}
QString MainWindow::decyphered_text(QVector<int>& v){
//    QString d;
//    for (int i = 0; i < v.size(); ++i){
//        QString l = "0";
//        l[0] = static_cast<char>(v[i]);
//        d.append(l);
//    }
//    return d;

    QString d;
    QString l;
    QByteArray byteArray;
    for (int i = 0; i < v.size(); ++i){
        byteArray.append(static_cast<char>(v[i]));
        l = QString::fromLocal8Bit(byteArray);
    }
    d.append(l);
    return d;
}

//Функции для AES
QString MainWindow::text_padding(QString& text){
    long long text_length = text.length();
    int rest = text_length % 16;
    if (rest != 0)
    {
        text.append("$");
        text_length = text.length();
        rest = text_length % 16;
        if (rest != 0)
        {
            for (int i = 0; i < (16 - rest); ++i) text.append("0");
        }
    }
    return text;
}
QVector<int> MainWindow::vector_text(QString& text){
//    QVector<int>v;
//    for (int i = 0; i < text.length(); ++i)
//    {
//        int r = static_cast<int>(text[i].unicode());
//        v.push_back(r);
//    }
//    return v;
    QVector<int>v;
    for (int i = 0; i < text.length(); ++i){
        ushort unicode = text[i].unicode();
        int value;
        if (unicode<127){
            value = static_cast<int>(unicode);
        }
        else { //для кириллицы
            uchar cp1251 = unicode - 0x0350;
            if (cp1251 == 1) cp1251=184; //для правильного отображения символа "ё"
            if (cp1251 == 177) cp1251=168; //для правильного отображения символа "Ё"
            value = static_cast<int>(cp1251);
        }
        v.push_back(value);
    }
    return v;
}
QString MainWindow::bit_mask(QString& a){
    QString a1;
    for (int k = 0; k < a.length(); ++k)
    {
        for (int i = 7; i >= 0; --i)
        {
            bool d = a[k].unicode() & (1 << i);
            if (d == true) { a1.append("1"); }
            else { a1.append("0"); }
        }
    }
    return a1;
}
QString MainWindow::bin_padding(QString& a)
{
    if (a.length() < 128)
    {
        a.append("1");
        int d = 128 - a.length();
        for (int i = 0; i < d; ++i)
        {
            a.append("0");
        }
    }
    return a;
}
QVector<int> MainWindow::to_key(QString& a){
    QVector<int>v;
    for (int i = 0; i < 16; ++i)
    {
        std::bitset<4>b1;
        std::bitset<4>b2;
        for (int k = 0; k < 4; ++k)
        {
            if (a[8 * i + k] == '1') b1[3 - k] = 1;
        }
        for (int m = 4; m < 8; ++m)
        {
            if (a[8 * i + m] == '1') b2[7 - m] = 1;
        }
        int t = b1.to_ulong() * 16 + b2.to_ulong();
        v.push_back(t);
        b1.reset();
        b2.reset();
    }
    return v;
}
std::string MainWindow::to_hex_string(const unsigned int& i){
    std::stringstream ss;
    std::string s;
    ss <<std:: hex << i;
    s = ss.str();
    if (s == "1" || s == "2" || s == "3" || s == "4" || s == "5" || s == "6" ||
        s == "7" || s == "8" || s == "9" || s == "a" || s == "b" || s == "c" ||
        s == "d" || s == "e" || s == "f")
        s.insert(0, "0");
    return s;
}
int MainWindow::to_number(std::string& s, int i)
{
    std::stringstream ss;
    ss << std::hex << s[i];
    int n;
    ss >> n;
    return n;
}
std::string MainWindow::i_to_hex_string(char b[], int beg, int p, int i){
    std::string s;
    std::stringstream ss;
    std::bitset<8>b1 = b[beg + 16 * p + i];
    ss << std::hex << b1.to_ulong();
    s = ss.str();
    if (s == "1" || s == "2" || s == "3" || s == "4" || s == "5" || s == "6" ||
        s == "7" || s == "8" || s == "9" || s == "a" || s == "b" || s == "c" ||
        s == "d" || s == "e" || s == "f" || s == "0")
        s.insert(0, "0");
    return s;
}


void MainWindow::firstAuxColumn(QVector<int>& text, std::vector<int>& av, int k, int i){
    std::vector<int>av1(5);
    av1[0] = (0x02 * text[16 * k + 4 * i + 0]);
    if (av1[0] > 0xff) av1[0] = av1[0] ^ 0x11b;
    av1[1] = (0x02 * text[16 * k + 4 * i + 1]) ^ (0x01 * text[16 * k + 4 * i + 1]);
    if (av1[1] > 0xff) av1[1] = av1[1] ^ 0x11b;
    av1[2] = text[16 * k + 4 * i + 2];
    av1[3] = text[16 * k + 4 * i + 3];
    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i] = av1[4];
}
void MainWindow::secondAuxColumn(QVector<int>& text, std::vector<int>& av, int k, int i){
    std::vector<int>av1(5);
    av1[0] = text[16 * k + 4 * i + 0];
    av1[1] = (0x02 * text[16 * k + 4 * i + 1]);
    if (av1[1] > 0xff) av1[1] = av1[1] ^ 0x11b;
    av1[2] = (0x02 * text[16 * k + 4 * i + 2]) ^ (0x01 * text[16 * k + 4 * i + 2]);
    if (av1[2] > 0xff) av1[2] = av1[2] ^ 0x11b;
    av1[3] = text[16 * k + 4 * i + 3];
    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i + 1] = av1[4];
}
void MainWindow::thirdAuxColumn(QVector<int>& text, std::vector<int>& av, int k, int i){
    std::vector<int>av1(5);
    av1[0] = text[16 * k + 4 * i + 0];
    av1[1] = text[16 * k + 4 * i + 1];
    av1[2] = (0x02 * text[16 * k + 4 * i + 2]);
    if (av1[2] > 0xff) av1[2] = av1[2] ^ 0x11b;
    av1[3] = (0x02 * text[16 * k + 4 * i + 3]) ^ (0x01 * text[16 * k + 4 * i + 3]);
    if (av1[3] > 0xff) av1[3] = av1[3] ^ 0x11b;
    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i + 2] = av1[4];
}
void MainWindow::fourthAuxColumn(QVector<int>& text, std::vector<int>& av, int k, int i){
    std::vector<int>av1(5);
    av1[0] = (0x02 * text[16 * k + 4 * i + 0]) ^ (0x01 * text[16 * k + 4 * i + 0]);
    if (av1[0] > 0xff) av1[0] = av1[0] ^ 0x11b;
    av1[1] = text[16 * k + 4 * i + 1];
    av1[2] = text[16 * k + 4 * i + 2];
    av1[3] = (0x02 * text[16 * k + 4 * i + 3]);
    if (av1[3] > 0xff) av1[3] = av1[3] ^ 0x11b;
    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i + 3] = av1[4];
}
void MainWindow::d_firstAuxColumn(QVector<int>& text, std::vector<int>& av, int k, int i){
    std::vector<int>av1(5);
    av1[0] = (0x08 * text[16 * k + 4 * i + 0]) ^
             (0x04 * text[16 * k + 4 * i + 0]) ^
             (0x02 * text[16 * k + 4 * i + 0]);
    while (av1[0] > 0xff)
    {
        if (av1[0] >= 0x400) av1[0] = av1[0] ^ (0x11b * 0x04);
        if (av1[0] >= 0x200 && av1[0] < 0x400) av1[0] = av1[0] ^ (0x11b * 0x02);
        if (av1[0] >= 0x100 && av1[0] < 0x200) av1[0] = av1[0] ^ 0x11b;
    }

    av1[1] = (0x08 * text[16 * k + 4 * i + 1]) ^
             (0x02 * text[16 * k + 4 * i + 1]) ^
             (0x01 * text[16 * k + 4 * i + 1]);
    while (av1[1] > 0xff)
    {
        if (av1[1] >= 0x400) av1[1] = av1[1] ^ (0x11b * 0x04);
        if (av1[1] >= 0x200 && av1[1] < 0x400) av1[1] = av1[1] ^ (0x11b * 0x02);
        if (av1[1] >= 0x100 && av1[1] < 0x200) av1[1] = av1[1] ^ 0x11b;
    }


    av1[2] = (0x08 * text[16 * k + 4 * i + 2]) ^
             (0x04 * text[16 * k + 4 * i + 2]) ^
             (0x01 * text[16 * k + 4 * i + 2]);
    while (av1[2] > 0xff)
    {
        if (av1[2] >= 0x400) av1[2] = av1[2] ^ (0x11b * 0x04);
        if (av1[2] >= 0x200 && av1[2] < 0x400) av1[2] = av1[2] ^ (0x11b * 0x02);
        if (av1[2] >= 0x100 && av1[2] < 0x200) av1[2] = av1[2] ^ 0x11b;
    }

    av1[3] = (0x08 * text[16 * k + 4 * i + 3]) ^
             (0x01 * text[16 * k + 4 * i + 3]);
    while (av1[3] > 0xff)
    {
        if (av1[3] >= 0x400) av1[3] = av1[3] ^ (0x11b * 0x04);
        if (av1[3] >= 0x200 && av1[3] < 0x400) av1[3] = av1[3] ^ (0x11b * 0x02);
        if (av1[3] >= 0x100 && av1[3] < 0x200) av1[3] = av1[3] ^ 0x11b;
    }

    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i] = av1[4];
}
void MainWindow::d_secondAuxColumn(QVector<int>& text, std::vector<int>& av, int k, int i){
    std::vector<int>av1(5);
    av1[0] = (0x08 * text[16 * k + 4 * i + 0]) ^
             (0x01 * text[16 * k + 4 * i + 0]);
    while (av1[0] > 0xff)
    {
        if (av1[0] >= 0x400) av1[0] = av1[0] ^ (0x11b * 0x04);
        if (av1[0] >= 0x200 && av1[0] < 0x400) av1[0] = av1[0] ^ (0x11b * 0x02);
        if (av1[0] >= 0x100 && av1[0] < 0x200) av1[0] = av1[0] ^ 0x11b;
    }

    av1[1] = (0x08 * text[16 * k + 4 * i + 1]) ^
             (0x04 * text[16 * k + 4 * i + 1]) ^
             (0x02 * text[16 * k + 4 * i + 1]);
    while (av1[1] > 0xff)
    {
        if (av1[1] >= 0x400) av1[1] = av1[1] ^ (0x11b * 0x04);
        if (av1[1] >= 0x200 && av1[1] < 0x400) av1[1] = av1[1] ^ (0x11b * 0x02);
        if (av1[1] >= 0x100 && av1[1] < 0x200) av1[1] = av1[1] ^ 0x11b;
    }


    av1[2] = (0x08 * text[16 * k + 4 * i + 2]) ^
             (0x02 * text[16 * k + 4 * i + 2]) ^
             (0x01 * text[16 * k + 4 * i + 2]);
    while (av1[2] > 0xff)
    {
        if (av1[2] >= 0x400) av1[2] = av1[2] ^ (0x11b * 0x04);
        if (av1[2] >= 0x200 && av1[2] < 0x400) av1[2] = av1[2] ^ (0x11b * 0x02);
        if (av1[2] >= 0x100 && av1[2] < 0x200) av1[2] = av1[2] ^ 0x11b;
    }

    av1[3] = (0x08 * text[16 * k + 4 * i + 3]) ^
             (0x04 * text[16 * k + 4 * i + 3]) ^
             (0x01 * text[16 * k + 4 * i + 3]);
    while (av1[3] > 0xff)
    {
        if (av1[3] >= 0x400) av1[3] = av1[3] ^ (0x11b * 0x04);
        if (av1[3] >= 0x200 && av1[3] < 0x400) av1[3] = av1[3] ^ (0x11b * 0x02);
        if (av1[3] >= 0x100 && av1[3] < 0x200) av1[3] = av1[3] ^ 0x11b;
    }

    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i + 1] = av1[4];
}
void MainWindow::d_thirdAuxColumn(QVector<int>& text, std::vector<int>& av, int k, int i){
    std::vector<int>av1(5);
    av1[0] = (0x08 * text[16 * k + 4 * i + 0]) ^
             (0x04 * text[16 * k + 4 * i + 0]) ^
             (0x01 * text[16 * k + 4 * i + 0]);
    while (av1[0] > 0xff)
    {
        if (av1[0] >= 0x400) av1[0] = av1[0] ^ (0x11b * 0x04);
        if (av1[0] >= 0x200 && av1[0] < 0x400) av1[0] = av1[0] ^ (0x11b * 0x02);
        if (av1[0] >= 0x100 && av1[0] < 0x200) av1[0] = av1[0] ^ 0x11b;
    }

    av1[1] = (0x08 * text[16 * k + 4 * i + 1]) ^
             (0x01 * text[16 * k + 4 * i + 1]);
    while (av1[1] > 0xff)
    {
        if (av1[1] >= 0x400) av1[1] = av1[1] ^ (0x11b * 0x04);
        if (av1[1] >= 0x200 && av1[1] < 0x400) av1[1] = av1[1] ^ (0x11b * 0x02);
        if (av1[1] >= 0x100 && av1[1] < 0x200) av1[1] = av1[1] ^ 0x11b;
    }

    av1[2] = (0x08 * text[16 * k + 4 * i + 2]) ^
             (0x04 * text[16 * k + 4 * i + 2]) ^
             (0x02 * text[16 * k + 4 * i + 2]);
    while (av1[2] > 0xff)
    {
        if (av1[2] >= 0x400) av1[2] = av1[2] ^ (0x11b * 0x04);
        if (av1[2] >= 0x200 && av1[2] < 0x400) av1[2] = av1[2] ^ (0x11b * 0x02);
        if (av1[2] >= 0x100 && av1[2] < 0x200) av1[2] = av1[2] ^ 0x11b;
    }

    av1[3] = (0x08 * text[16 * k + 4 * i + 3]) ^
             (0x02 * text[16 * k + 4 * i + 3]) ^
             (0x01 * text[16 * k + 4 * i + 3]);
    while (av1[3] > 0xff)
    {
        if (av1[3] >= 0x400) av1[3] = av1[3] ^ (0x11b * 0x04);
        if (av1[3] >= 0x200 && av1[3] < 0x400) av1[3] = av1[3] ^ (0x11b * 0x02);
        if (av1[3] >= 0x100 && av1[3] < 0x200) av1[3] = av1[3] ^ 0x11b;
    }

    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i + 2] = av1[4];
}
void MainWindow::d_fourthAuxColumn(QVector<int>& text, std::vector<int>& av, int k, int i){
    std::vector<int>av1(5);
    av1[0] = (0x08 * text[16 * k + 4 * i + 0]) ^
             (0x02 * text[16 * k + 4 * i + 0]) ^
             (0x01 * text[16 * k + 4 * i + 0]);
    while (av1[0] > 0xff)
    {
        if (av1[0] >= 0x400) av1[0] = av1[0] ^ (0x11b * 0x04);
        if (av1[0] >= 0x200 && av1[0] < 0x400) av1[0] = av1[0] ^ (0x11b * 0x02);
        if (av1[0] >= 0x100 && av1[0] < 0x200) av1[0] = av1[0] ^ 0x11b;
    }

    av1[1] = (0x08 * text[16 * k + 4 * i + 1]) ^
             (0x04 * text[16 * k + 4 * i + 1]) ^
             (0x01 * text[16 * k + 4 * i + 1]);
    while (av1[1] > 0xff)
    {
        if (av1[1] >= 0x400) av1[1] = av1[1] ^ (0x11b * 0x04);
        if (av1[1] >= 0x200 && av1[1] < 0x400) av1[1] = av1[1] ^ (0x11b * 0x02);
        if (av1[1] >= 0x100 && av1[1] < 0x200) av1[1] = av1[1] ^ 0x11b;
    }

    av1[2] = (0x08 * text[16 * k + 4 * i + 2]) ^
             (0x01 * text[16 * k + 4 * i + 2]);
    while (av1[2] > 0xff)
    {
        if (av1[2] >= 0x400) av1[2] = av1[2] ^ (0x11b * 0x04);
        if (av1[2] >= 0x200 && av1[2] < 0x400) av1[2] = av1[2] ^ (0x11b * 0x02);
        if (av1[2] >= 0x100 && av1[2] < 0x200) av1[2] = av1[2] ^ 0x11b;
    }

    av1[3] = (0x08 * text[16 * k + 4 * i + 3]) ^
             (0x04 * text[16 * k + 4 * i + 3]) ^
             (0x02 * text[16 * k + 4 * i + 3]);
    while (av1[3] > 0xff)
    {
        if (av1[3] >= 0x400) av1[3] = av1[3] ^ (0x11b * 0x04);
        if (av1[3] >= 0x200 && av1[3] < 0x400) av1[3] = av1[3] ^ (0x11b * 0x02);
        if (av1[3] >= 0x100 && av1[3] < 0x200) av1[3] = av1[3] ^ 0x11b;
    }

    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i + 3] = av1[4];
}
void MainWindow::i_firstAuxColumn(char* b, char av[], int beg, int p, int i){
    std::bitset<8>b1;
    std::vector<short>av1(5);

    b1 = b[beg + 16 * p + 4 * i + 0];
    av1[0] = (0x02 * b1.to_ulong());
    if (av1[0] > 0xff) av1[0] = av1[0] ^ 0x11b;
    b1 = b[beg + 16 * p + 4 * i + 1];
    av1[1] = (0x02 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    if (av1[1] > 0xff) av1[1] = av1[1] ^ 0x11b;
    av1[2] = b[beg + 16 * p + 4 * i + 2];
    av1[3] = b[beg + 16 * p + 4 * i + 3];
    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i] = av1[4];
}
void MainWindow::i_secondAuxColumn(char* b, char av[], int beg, int p, int i){
    std::bitset<8>b1;
    std::vector<short>av1(5);

    av1[0] = b[beg + 16 * p + 4 * i + 0];
    b1 = b[beg + 16 * p + 4 * i + 1];
    av1[1] = (0x02 * b1.to_ulong());
    if (av1[1] > 0xff) av1[1] = av1[1] ^ 0x11b;
    b1 = b[beg + 16 * p + 4 * i + 2];
    av1[2] = (0x02 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    if (av1[2] > 0xff) av1[2] = av1[2] ^ 0x11b;
    av1[3] = b[beg + 16 * p + 4 * i + 3];
    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i + 1] = av1[4];
}
void MainWindow::i_thirdAuxColumn(char* b, char av[], int beg, int p, int i){
    std::bitset<8>b1;
    std::vector<short>av1(5);

    av1[0] = b[beg + 16 * p + 4 * i + 0];
    av1[1] = b[beg + 16 * p + 4 * i + 1];
    b1 = b[beg + 16 * p + 4 * i + 2];
    av1[2] = (0x02 * b1.to_ulong());
    if (av1[2] > 0xff) av1[2] = av1[2] ^ 0x11b;
    b1 = b[beg + 16 * p + 4 * i + 3];
    av1[3] = (0x02 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    if (av1[3] > 0xff) av1[3] = av1[3] ^ 0x11b;
    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i + 2] = av1[4];
}
void MainWindow::i_fourthAuxColumn(char* b, char av[], int beg, int p, int i){
    std::bitset<8>b1;
    std::vector<short>av1(5);

    b1 = b[beg + 16 * p + 4 * i + 0];
    av1[0] = (0x02 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    if (av1[0] > 0xff) av1[0] = av1[0] ^ 0x11b;
    av1[1] = b[beg + 16 * p + 4 * i + 1];
    av1[2] = b[beg + 16 * p + 4 * i + 2];
    b1 = b[beg + 16 * p + 4 * i + 3];
    av1[3] = (0x02 * b1.to_ulong());
    if (av1[3] > 0xff) av1[3] = av1[3] ^ 0x11b;
    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i + 3] = av1[4];
}
void MainWindow::i_d_firstAuxColumn(char* b, char av[], int beg, int p, int i){
    std::bitset<8>b1;
    std::vector<short>av1(5);

    b1 = b[beg + 16 * p + 4 * i + 0];
    av1[0] = (0x08 * b1.to_ulong()) ^ (0x04 * b1.to_ulong()) ^ (0x02 * b1.to_ulong());
    while (av1[0] > 0xff)
    {
        if (av1[0] >= 0x400) av1[0] = av1[0] ^ (0x11b * 0x04);
        if (av1[0] >= 0x200 && av1[0] < 0x400) av1[0] = av1[0] ^ (0x11b * 0x02);
        if (av1[0] >= 0x100 && av1[0] < 0x200) av1[0] = av1[0] ^ 0x11b;
    }

    b1 = b[beg + 16 * p + 4 * i + 1];
    av1[1] = (0x08 * b1.to_ulong()) ^ (0x02 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    while (av1[1] > 0xff)
    {
        if (av1[1] >= 0x400) av1[1] = av1[1] ^ (0x11b * 0x04);
        if (av1[1] >= 0x200 && av1[1] < 0x400) av1[1] = av1[1] ^ (0x11b * 0x02);
        if (av1[1] >= 0x100 && av1[1] < 0x200) av1[1] = av1[1] ^ 0x11b;
    }

    b1 = b[beg + 16 * p + 4 * i + 2];
    av1[2] = (0x08 * b1.to_ulong()) ^ (0x04 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    while (av1[2] > 0xff)
    {
        if (av1[2] >= 0x400) av1[2] = av1[2] ^ (0x11b * 0x04);
        if (av1[2] >= 0x200 && av1[2] < 0x400) av1[2] = av1[2] ^ (0x11b * 0x02);
        if (av1[2] >= 0x100 && av1[2] < 0x200) av1[2] = av1[2] ^ 0x11b;
    }

    b1 = b[beg + 16 * p + 4 * i + 3];
    av1[3] = (0x08 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    while (av1[3] > 0xff)
    {
        if (av1[3] >= 0x400) av1[3] = av1[3] ^ (0x11b * 0x04);
        if (av1[3] >= 0x200 && av1[3] < 0x400) av1[3] = av1[3] ^ (0x11b * 0x02);
        if (av1[3] >= 0x100 && av1[3] < 0x200) av1[3] = av1[3] ^ 0x11b;
    }

    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i] = av1[4];
}
void MainWindow::i_d_secondAuxColumn(char* b, char av[], int beg, int p, int i){
    std::bitset<8>b1;
    std::vector<short>av1(5);

    b1 = b[beg + 16 * p + 4 * i + 0];
    av1[0] = (0x08 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    while (av1[0] > 0xff)
    {
        if (av1[0] >= 0x400) av1[0] = av1[0] ^ (0x11b * 0x04);
        if (av1[0] >= 0x200 && av1[0] < 0x400) av1[0] = av1[0] ^ (0x11b * 0x02);
        if (av1[0] >= 0x100 && av1[0] < 0x200) av1[0] = av1[0] ^ 0x11b;
    }

    b1 = b[beg + 16 * p + 4 * i + 1];
    av1[1] = (0x08 * b1.to_ulong()) ^ (0x04 * b1.to_ulong()) ^ (0x02 * b1.to_ulong());
    while (av1[1] > 0xff)
    {
        if (av1[1] >= 0x400) av1[1] = av1[1] ^ (0x11b * 0x04);
        if (av1[1] >= 0x200 && av1[1] < 0x400) av1[1] = av1[1] ^ (0x11b * 0x02);
        if (av1[1] >= 0x100 && av1[1] < 0x200) av1[1] = av1[1] ^ 0x11b;
    }

    b1 = b[beg + 16 * p + 4 * i + 2];
    av1[2] = (0x08 * b1.to_ulong()) ^ (0x02 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    while (av1[2] > 0xff)
    {
        if (av1[2] >= 0x400) av1[2] = av1[2] ^ (0x11b * 0x04);
        if (av1[2] >= 0x200 && av1[2] < 0x400) av1[2] = av1[2] ^ (0x11b * 0x02);
        if (av1[2] >= 0x100 && av1[2] < 0x200) av1[2] = av1[2] ^ 0x11b;
    }

    b1 = b[beg + 16 * p + 4 * i + 3];
    av1[3] = (0x08 * b1.to_ulong()) ^ (0x04 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    while (av1[3] > 0xff)
    {
        if (av1[3] >= 0x400) av1[3] = av1[3] ^ (0x11b * 0x04);
        if (av1[3] >= 0x200 && av1[3] < 0x400) av1[3] = av1[3] ^ (0x11b * 0x02);
        if (av1[3] >= 0x100 && av1[3] < 0x200) av1[3] = av1[3] ^ 0x11b;
    }

    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i + 1] = av1[4];
}
void MainWindow::i_d_thirdAuxColumn(char* b, char av[], int beg, int p, int i){
    std::bitset<8>b1;
    std::vector<short>av1(5);

    b1 = b[beg + 16 * p + 4 * i + 0];
    av1[0] = (0x08 * b1.to_ulong()) ^ (0x04 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    while (av1[0] > 0xff)
    {
        if (av1[0] >= 0x400) av1[0] = av1[0] ^ (0x11b * 0x04);
        if (av1[0] >= 0x200 && av1[0] < 0x400) av1[0] = av1[0] ^ (0x11b * 0x02);
        if (av1[0] >= 0x100 && av1[0] < 0x200) av1[0] = av1[0] ^ 0x11b;
    }

    b1 = b[beg + 16 * p + 4 * i + 1];
    av1[1] = (0x08 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    while (av1[1] > 0xff)
    {
        if (av1[1] >= 0x400) av1[1] = av1[1] ^ (0x11b * 0x04);
        if (av1[1] >= 0x200 && av1[1] < 0x400) av1[1] = av1[1] ^ (0x11b * 0x02);
        if (av1[1] >= 0x100 && av1[1] < 0x200) av1[1] = av1[1] ^ 0x11b;
    }

    b1 = b[beg + 16 * p + 4 * i + 2];
    av1[2] = (0x08 * b1.to_ulong()) ^ (0x04 * b1.to_ulong()) ^ (0x02 * b1.to_ulong());
    while (av1[2] > 0xff)
    {
        if (av1[2] >= 0x400) av1[2] = av1[2] ^ (0x11b * 0x04);
        if (av1[2] >= 0x200 && av1[2] < 0x400) av1[2] = av1[2] ^ (0x11b * 0x02);
        if (av1[2] >= 0x100 && av1[2] < 0x200) av1[2] = av1[2] ^ 0x11b;
    }

    b1 = b[beg + 16 * p + 4 * i + 3];
    av1[3] = (0x08 * b1.to_ulong()) ^ (0x02 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    while (av1[3] > 0xff)
    {
        if (av1[3] >= 0x400) av1[3] = av1[3] ^ (0x11b * 0x04);
        if (av1[3] >= 0x200 && av1[3] < 0x400) av1[3] = av1[3] ^ (0x11b * 0x02);
        if (av1[3] >= 0x100 && av1[3] < 0x200) av1[3] = av1[3] ^ 0x11b;
    }

    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i + 2] = av1[4];
}
void MainWindow::i_d_fourthAuxColumn(char* b, char av[], int beg, int p, int i){
    std::bitset<8>b1;
    std::vector<short>av1(5);

    b1 = b[beg + 16 * p + 4 * i + 0];
    av1[0] = (0x08 * b1.to_ulong()) ^ (0x02 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    while (av1[0] > 0xff)
    {
        if (av1[0] >= 0x400) av1[0] = av1[0] ^ (0x11b * 0x04);
        if (av1[0] >= 0x200 && av1[0] < 0x400) av1[0] = av1[0] ^ (0x11b * 0x02);
        if (av1[0] >= 0x100 && av1[0] < 0x200) av1[0] = av1[0] ^ 0x11b;
    }

    b1 = b[beg + 16 * p + 4 * i + 1];
    av1[1] = (0x08 * b1.to_ulong()) ^ (0x04 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    while (av1[1] > 0xff)
    {
        if (av1[1] >= 0x400) av1[1] = av1[1] ^ (0x11b * 0x04);
        if (av1[1] >= 0x200 && av1[1] < 0x400) av1[1] = av1[1] ^ (0x11b * 0x02);
        if (av1[1] >= 0x100 && av1[1] < 0x200) av1[1] = av1[1] ^ 0x11b;
    }

    b1 = b[beg + 16 * p + 4 * i + 2];
    av1[2] = (0x08 * b1.to_ulong()) ^ (0x01 * b1.to_ulong());
    while (av1[2] > 0xff)
    {
        if (av1[2] >= 0x400) av1[2] = av1[2] ^ (0x11b * 0x04);
        if (av1[2] >= 0x200 && av1[2] < 0x400) av1[2] = av1[2] ^ (0x11b * 0x02);
        if (av1[2] >= 0x100 && av1[2] < 0x200) av1[2] = av1[2] ^ 0x11b;
    }

    b1 = b[beg + 16 * p + 4 * i + 3];
    av1[3] = (0x08 * b1.to_ulong()) ^ (0x04 * b1.to_ulong()) ^ (0x02 * b1.to_ulong());
    while (av1[3] > 0xff)
    {
        if (av1[3] >= 0x400) av1[3] = av1[3] ^ (0x11b * 0x04);
        if (av1[3] >= 0x200 && av1[3] < 0x400) av1[3] = av1[3] ^ (0x11b * 0x02);
        if (av1[3] >= 0x100 && av1[3] < 0x200) av1[3] = av1[3] ^ 0x11b;
    }

    av1[4] = av1[0] ^ av1[1] ^ av1[2] ^ av1[3];
    av[4 * i + 3] = av1[4];
}

void MainWindow::addRondKey(QVector<int>& text, QVector<int>& key, int k){
    text[16 * k + 0] = text[16 * k + 0] ^ key[0];
    text[16 * k + 1] = text[16 * k + 1] ^ key[1];
    text[16 * k + 2] = text[16 * k + 2] ^ key[2];
    text[16 * k + 3] = text[16 * k + 3] ^ key[3];
    text[16 * k + 4] = text[16 * k + 4] ^ key[4];
    text[16 * k + 5] = text[16 * k + 5] ^ key[5];
    text[16 * k + 6] = text[16 * k + 6] ^ key[6];
    text[16 * k + 7] = text[16 * k + 7] ^ key[7];
    text[16 * k + 8] = text[16 * k + 8] ^ key[8];
    text[16 * k + 9] = text[16 * k + 9] ^ key[9];
    text[16 * k + 10] = text[16 * k + 10] ^ key[10];
    text[16 * k + 11] = text[16 * k + 11] ^ key[11];
    text[16 * k + 12] = text[16 * k + 12] ^ key[12];
    text[16 * k + 13] = text[16 * k + 13] ^ key[13];
    text[16 * k + 14] = text[16 * k + 14] ^ key[14];
    text[16 * k + 15] = text[16 * k + 15] ^ key[15];
}
void MainWindow::roundKey(QVector<int>& key, int i){
    QVector<int>g_key(16);
    g_key[12] = key[12];
    g_key[13] = key[13];
    g_key[14] = key[14];
    g_key[15] = key[15];
    std::rotate(g_key.begin() + 12, g_key.begin() + 13, g_key.end());

    for (int i = 12; i < 16; ++i)
    {
        std::string s = to_hex_string(g_key[i]);
        int m = to_number(s, 0);
        int n = to_number(s, 1);
        g_key[i] = SB[m][n];
    }
    g_key[12] = g_key[12] ^ Rcon[i];

    key[0] = key[0] ^ g_key[12];
    key[1] = key[1] ^ g_key[13];
    key[2] = key[2] ^ g_key[14];
    key[3] = key[3] ^ g_key[15];

    key[4] = key[4] ^ key[0];
    key[5] = key[5] ^ key[1];
    key[6] = key[6] ^ key[2];
    key[7] = key[7] ^ key[3];

    key[8] = key[8] ^ key[4];
    key[9] = key[9] ^ key[5];
    key[10] = key[10] ^ key[6];
    key[11] = key[11] ^ key[7];

    key[12] = key[12] ^ key[8];
    key[13] = key[13] ^ key[9];
    key[14] = key[14] ^ key[10];
    key[15] = key[15] ^ key[11];
}
void MainWindow::subBytes(QVector<int>& text, int k){
    for (int i = 0; i < 16; ++i)
    {
        std::string s = to_hex_string(text[16 * k + i]);
        int m = to_number(s, 0);
        int n = to_number(s, 1);
        text[16 * k + i] = SB[m][n];
    }
}
void MainWindow::shiftRows(QVector<int>& text, int k){
    QVector<int>temp(1);
    temp[0] = text[16 * k + 1];
    text[16 * k + 1] = text[16 * k + 5];
    text[16 * k + 5] = text[16 * k + 9];
    text[16 * k + 9] = text[16 * k + 13];
    text[16 * k + 13] = temp[0];

    std::swap(text[16 * k + 2], text[16 * k + 10]);
    std::swap(text[16 * k + 6], text[16 * k + 14]);

    QVector<int>temp1(1);
    temp1[0] = text[16 * k + 15];
    text[16 * k + 15] = text[16 * k + 11];
    text[16 * k + 11] = text[16 * k + 7];
    text[16 * k + 7] = text[16 * k + 3];
    text[16 * k + 3] = temp1[0];
}
void MainWindow::mixColumns(QVector<int>& text, int k){
    std::vector<int>av(16);
    for (int i = 0; i < 4; ++i)
    {
        firstAuxColumn(text, av, k, i);
        secondAuxColumn(text, av, k, i);
        thirdAuxColumn(text, av, k, i);
        fourthAuxColumn(text, av, k, i);
    }
    for (int i = 0; i < 16; ++i)
        text[16 * k + i] = av[i];
}

void MainWindow::i_addRoundkey(char* b, QVector<int>&key, int beg, long long p){
    for (int i = 0; i < 16; ++i)
        b[beg + 16 * p + i] = b[beg + 16 * p + i] ^ key[i];
}
void MainWindow::i_roundKey(QVector<int>& key, int i){
    QVector<int>g_key(16);
    g_key[12] = key[12];
    g_key[13] = key[13];
    g_key[14] = key[14];
    g_key[15] = key[15];
    g_key[11] = g_key[12]; g_key[12] = g_key[13]; g_key[13] = g_key[14]; g_key[14] = g_key[15]; g_key[15] = g_key[11];
    for (int i = 12; i < 16; ++i){
        std::string s = to_hex_string(g_key[i]);
        int m = to_number(s, 0);
        int n = to_number(s, 1);
        g_key[i] = SB[m][n];
    }

    key[0] = key[0] ^ g_key[12];
    key[1] = key[1] ^ g_key[13];
    key[2] = key[2] ^ g_key[14];
    key[3] = key[3] ^ g_key[15];

    key[4] = key[4] ^ key[0];
    key[5] = key[5] ^ key[1];
    key[6] = key[6] ^ key[2];
    key[7] = key[7] ^ key[3];

    key[8] = key[8] ^ key[4];
    key[9] = key[9] ^ key[5];
    key[10] = key[10] ^ key[6];
    key[11] = key[11] ^ key[7];

    key[12] = key[12] ^ key[8];
    key[13] = key[13] ^ key[9];
    key[14] = key[14] ^ key[10];
    key[15] = key[15] ^ key[11];
}
void MainWindow::i_subBytes(char* b, int beg, int p){
    for (int i = 0; i < 16; ++i)
    {
        std::string s = i_to_hex_string(b, beg, p, i);
        int m = to_number(s, 0);
        int n = to_number(s, 1);
        b[beg + 16 * p + i] = SB[m][n];
    }
}
void MainWindow::i_shiftRows(char* b, int beg, int p){
    char temp;
    temp = b[beg + 16 * p + 1];
    b[beg + 16 * p + 1] = b[beg + 16 * p + 5];
    b[beg + 16 * p + 5] = b[beg + 16 * p + 9];
    b[beg + 16 * p + 9] = b[beg + 16 * p + 13];
    b[beg + 16 * p + 13] = temp;

    std::swap(b[beg + 16 * p + 2], b[beg + 16 * p + 10]);
    std::swap(b[beg + 16 * p + 6], b[beg + 16 * p + 14]);

    char temp1;
    temp1 = b[beg + 16 * p + 15];
    b[beg + 16 * p + 15] = b[beg + 16 * p + 11];
    b[beg + 16 * p + 11] = b[beg + 16 * p + 7];
    b[beg + 16 * p + 7] = b[beg + 16 * p + 3];
    b[beg + 16 * p + 3] = temp1;
}
void MainWindow::i_mixColumns(char* b, int beg, int p){
    char av[16]{};
    for (int i = 0; i < 4; ++i)
    {
        i_firstAuxColumn(b, av, beg, p, i);
        i_secondAuxColumn(b, av, beg, p, i);
        i_thirdAuxColumn(b, av, beg, p, i);
        i_fourthAuxColumn(b, av, beg, p, i);
    }
    for (int i = 0; i < 16; ++i)
        b[beg + 16 * p + i] = av[i];
}

void MainWindow::inv_shiftRows(QVector<int>& text, int k){
    QVector<int>temp(1);
    temp[0] = text[16 * k + 13];
    text[16 * k + 13] = text[16 * k + 9];
    text[16 * k + 9] = text[16 * k + 5];
    text[16 * k + 5] = text[16 * k + 1];
    text[16 * k + 1] = temp[0];

    std::swap(text[16 * k + 2], text[16 * k + 10]);
    std::swap(text[16 * k + 6], text[16 * k + 14]);

    QVector<int>temp1(1);
    temp1[0] = text[16 * k + 3];
    text[16 * k + 3] = text[16 * k + 7];
    text[16 * k + 7] = text[16 * k + 11];
    text[16 * k + 11] = text[16 * k + 15];
    text[16 * k + 15] = temp1[0];
}
void MainWindow::inv_subBytes(QVector<int>& text, int k){
    for (int i = 0; i < 16; ++i)
    {
        std::string s = to_hex_string(text[16 * k + i]);
        int m = to_number(s, 0);
        int n = to_number(s, 1);
        text[16 * k + i] = InvSB[m][n];
    }
}
void MainWindow::inv_mixColumns(QVector<int>& text, int k){
    std::vector<int>av(16);
    for (int i = 0; i < 4; ++i)
    {
        d_firstAuxColumn(text, av, k, i);
        d_secondAuxColumn(text, av, k, i);
        d_thirdAuxColumn(text, av, k, i);
        d_fourthAuxColumn(text, av, k, i);
    }
    for (int i = 0; i < 16; ++i)
        text[16 * k + i] = av[i];
}
void MainWindow::i_inv_shiftRows(char* b, int beg, int p){
    char temp;
    temp = b[beg + 16 * p + 13];
    b[beg + 16 * p + 13] = b[beg + 16 * p + 9];
    b[beg + 16 * p + 9] = b[beg + 16 * p + 5];
    b[beg + 16 * p + 5] = b[beg + 16 * p + 1];
    b[beg + 16 * p + 1] = temp;

    std::swap(b[beg + 16 * p + 2], b[beg + 16 * p + 10]);
    std::swap(b[beg + 16 * p + 6], b[beg + 16 * p + 14]);

    char temp1;
    temp1 = b[beg + 16 * p + 3];
    b[beg + 16 * p + 3] = b[beg + 16 * p + 7];
    b[beg + 16 * p + 7] = b[beg + 16 * p + 11];
    b[beg + 16 * p + 11] = b[beg + 16 * p + 15];
    b[beg + 16 * p + 15] = temp1;
}
void MainWindow::i_inv_subBytes(char* b, int beg, int p){
    for (int i = 0; i < 16; ++i)
    {
        std::string s = i_to_hex_string(b, beg, p, i);
        int m = to_number(s, 0);
        int n = to_number(s, 1);
        b[beg + 16 * p + i] = InvSB[m][n];
    }
}
void MainWindow::i_inv_mixColumns(char* b, int beg, int p){
    char av[16]{};
    for (int i = 0; i < 4; ++i)
    {
        i_d_firstAuxColumn(b, av, beg, p, i);
        i_d_secondAuxColumn(b, av, beg, p, i);
        i_d_thirdAuxColumn(b, av, beg, p, i);
        i_d_fourthAuxColumn(b, av, beg, p, i);
    }
    for (int i = 0; i < 16; ++i)
        b[beg + 16 * p + i] = av[i];
}




































