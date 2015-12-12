#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QProxyStyle>
#include <QDesktopWidget>

class MyProxyStyle : public QProxyStyle
{
public:
    explicit MyProxyStyle(const QString& baseStyle) : QProxyStyle(baseStyle)
    {
    }
    int pixelMetric ( PixelMetric metric, const QStyleOption * option = 0, const QWidget * widget = 0 ) const
    {
        switch(metric) {
        case PM_SliderLength  : return 50;
        case PM_SliderThickness: return 20;
        default                         : return (QProxyStyle::pixelMetric(metric,option,widget));
        }
    }
};
int main(int argc, char *argv[])
{
    QApplication::setStyle(new MyProxyStyle("fusion"));
    QString curPath = QCoreApplication::applicationDirPath();
    QApplication::addLibraryPath(curPath+"/platforms");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    w.initialize();

    return a.exec();
}
