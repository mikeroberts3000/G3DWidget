#include <QtWidgets/QApplication>

#include "MainWindow.hpp"

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    mojo::MainWindow mainWindow;
    mainWindow.show();
    return application.exec();
}
