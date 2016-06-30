#include "klatschui.h"
#include "serialportlistener.h"
#include <QApplication>
//#include <Q>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    klatschui w;
    w.show();

    return a.exec();
}
