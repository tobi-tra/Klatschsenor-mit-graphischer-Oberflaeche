#include "klatschui.h"
#include "serialportlistener.h"
#include <QApplication>
//#include <Q>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    klatschui w;
    w.setFixedSize(900,610);
    w.show();

    return a.exec();
}
