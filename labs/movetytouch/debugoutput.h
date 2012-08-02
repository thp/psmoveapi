#ifndef DEBUGOUTPUT_H
#define DEBUGOUTPUT_H

#include <QObject>
#include <QTextStream>

class DebugOutput : public QObject {
    Q_OBJECT

    public:
        DebugOutput() : QObject(), out(stdout) {}

    private:
        QTextStream out;

    public slots:
        void pressed(int id, int x, int y) {
            out << "pressed: " << id << " (" << x << "," << y << ")\n";
            out.flush();
        }

        void released(int id, int x, int y) {
            out << "released: " << id << " (" << x << "," << y << ")\n";
            out.flush();
        }

        void motion(int id, int x, int y) {
            out << "motion: " << id << " (" << x << "," << y << ")\n";
            out.flush();
        }

        void hover(int idx, int x, int y) {
            out << "hover: " << idx << " (" << x << "," << y << ")\n";
            out.flush();
        }
};

#endif
